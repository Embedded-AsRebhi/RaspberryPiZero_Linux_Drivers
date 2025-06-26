#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

static const struct of_device_id ultrasonic_of_match[] = {
    { .compatible = "comp,ultrasonic" },
    {},
};

MODULE_DEVICE_TABLE(of, ultrasonic_of_match);

struct mydev_misc_t {
    struct miscdevice miscdev;
    const char* device_name;
    struct gpio_desc *gpio_trig;
    struct gpio_desc *gpio_echo;
};

static int irq_echo;
static volatile ktime_t start, end;
static volatile int measuring_in_progress = 0;
static volatile unsigned int measured_data = 0;
static DECLARE_WAIT_QUEUE_HEAD(measure_wq);

// Ultrasonic Sensor Measurement
//use the ECHO pin as an interrupt in an ultrasonic sensor 
static irqreturn_t echo_irq_handler(int irq, void *dev_id) {
    int gpio_val = gpiod_get_value(((struct mydev_misc_t *)dev_id)->gpio_echo);
    if (!measuring_in_progress) {
        pr_info(" pas de mesure en cours\n");
        return IRQ_HANDLED;
    }
    if (gpio_val) {
        // Front montant : begin of measure 
        start = ktime_get();
    } else {
        end = ktime_get();
        uint64_t diff_time_ns = ktime_to_ns(ktime_sub(end, start));
        u32 diff_us = (u32)div_u64(diff_time_ns, 1000);  // ns -> us
        u32 distance_cm = (diff_us * 343) / 20000;       // calcul distance en cm
        measured_data = distance_cm;
        measuring_in_progress = 0;
        wake_up_interruptible(&measure_wq);
    }
    return IRQ_HANDLED;
}

// Write : envoie au trigger
// on utilise container_of pour récupérer un pointeur vers notre structure "device"
static ssize_t start_measure(struct mydev_misc_t *dev) {
    if (measuring_in_progress) {
        pr_info("Mesure déjà en cours\n");
        return -EBUSY;
    }
    measured_data = 0;
    measuring_in_progress = 1;
    // Déclenchement du trigger
    gpiod_set_value(dev->gpio_trig, 0);
    udelay(2);
    gpiod_set_value(dev->gpio_trig, 1);
    udelay(10);
    gpiod_set_value(dev->gpio_trig, 0);
    pr_info("Trigger envoyé, attente de l'echo...\n");
    return 0;
}

// read function to to start the measure and write the value under the file device /dev/ultrasonic
static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos) {
    struct mydev_misc_t *dev = container_of(filp->private_data, struct mydev_misc_t, miscdev);
    char kbuf[32];
    size_t len;
    if (*ppos != 0)
        return 0;
    int ret = start_measure(dev);
    if (ret < 0)
        return ret;
    ret = wait_event_interruptible_timeout(measure_wq, !measuring_in_progress, msecs_to_jiffies(100));
    if (ret == 0) {
        pr_info("Timeout lors de la lecture de la mesure\n");
        return -ETIMEDOUT;
    }
    len = snprintf(kbuf, sizeof(kbuf), "%u\n", measured_data);
    if (count < len)
        return -EINVAL;
    if (copy_to_user(buf, kbuf, len))
        return -EFAULT;
    *ppos += len;
    return len;
}

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .read = my_read,
    
};

// probe function to save the device 
static int ultrasonic_probe(struct platform_device *pdev) {
    int ret;
    struct mydev_misc_t *mydev;
    // Allocation mémoire
    mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;
    // Stocker dans les données privées du périphérique
    platform_set_drvdata(pdev, mydev);
    // Lire l’étiquette (label)
    ret = of_property_read_string(pdev->dev.of_node, "label", &mydev->device_name);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read label property\n");
        return ret;
    }
    // Enregistrer le périphérique character
    mydev->miscdev.minor = MISC_DYNAMIC_MINOR;
    mydev->miscdev.name = mydev->device_name;
    mydev->miscdev.fops = &my_fops;
    mydev->miscdev.mode = 0666;
    // Récupérer les GPIOs
    mydev->gpio_trig = gpiod_get(&pdev->dev, "trig", GPIOD_OUT_LOW);
    if (IS_ERR(mydev->gpio_trig)) {
        dev_err(&pdev->dev, "Failed to get trig GPIO\n");
        return PTR_ERR(mydev->gpio_trig);
    }
    mydev->gpio_echo = gpiod_get(&pdev->dev, "echo", GPIOD_IN);
    if (IS_ERR(mydev->gpio_echo)) {
        dev_err(&pdev->dev, "Failed to get echo GPIO\n");
        gpiod_put(mydev->gpio_trig);
        return PTR_ERR(mydev->gpio_echo);
    }
    // Configurer les directions
    gpiod_direction_output(mydev->gpio_trig, 0);
    gpiod_direction_input(mydev->gpio_echo);
    // Obtenir l’IRQ
    irq_echo = platform_get_irq(pdev, 0);
    if (irq_echo <= 0) {
        dev_err(&pdev->dev, "platform_get_irq failed\n");
        gpiod_put(mydev->gpio_echo);
        gpiod_put(mydev->gpio_trig);
        return irq_echo;
    }
    pr_info("IRQ echo is: %d\n", irq_echo);
    // Enregistrer le handler IRQ
    ret = devm_request_irq(&pdev->dev, irq_echo, echo_irq_handler, IRQF_SHARED, "ultrasonic_irq", mydev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
        gpiod_put(mydev->gpio_echo);
        gpiod_put(mydev->gpio_trig);
        return ret;
    }
    // Enregistrement du périphérique
    ret = misc_register(&mydev->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register misc device\n");
        gpiod_put(mydev->gpio_echo);
        gpiod_put(mydev->gpio_trig);
        return ret;
    }
    pr_info("Driver %s initialise avec IRQ\n", mydev->device_name);
    return 0;
}


// remove function to remove the device driver
static void ultrasonic_remove(struct platform_device *pdev) {
    struct mydev_misc_t *misc_dev = platform_get_drvdata(pdev);
    // Désenregistrer le périphérique character
    misc_deregister(&misc_dev->miscdev);
    pr_info("Driver %s retired\n", misc_dev->device_name);
    //  Libérer les GPIOs
    if (misc_dev->gpio_trig)
        gpiod_put(misc_dev->gpio_trig);
    if (misc_dev->gpio_echo)
        gpiod_put(misc_dev->gpio_echo);

    pr_info("Ultrasonic GPIO Libered.\n");
}


static struct platform_driver ultrasonic_driver = {
    .driver = {
        .name = "ultrasonic_driver",
        .of_match_table = ultrasonic_of_match,
        .owner = THIS_MODULE,
    },
    .probe = ultrasonic_probe,
    .remove = ultrasonic_remove,
};

module_platform_driver(ultrasonic_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asma REBHI");
MODULE_DESCRIPTION("Driver HC-SR04 avec interruption");
