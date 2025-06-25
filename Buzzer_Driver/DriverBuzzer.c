#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h>
#include <linux/pwm.h>
#include <linux/slab.h>


// echo "2000" > /dev/buzzer
struct mydev_misc_t {
    struct miscdevice mymisc_device;
    const char* device_name;
    struct pwm_device *pwm;
    bool buzzer_active;
};

static const struct of_device_id my_table_of_matching[] = {
    { .compatible = "comp,buzzer" },
    {}
};
MODULE_DEVICE_TABLE(of, my_table_of_matching);



static ssize_t my_write(struct file *flip, const char __user *buf, size_t len, loff_t *off)
    {
        char kbuf[10] = {0};
        unsigned long freq;
        unsigned long period_ns, duty_ns;
        struct mydev_misc_t *mydev = container_of(flip->private_data, struct mydev_misc_t, mymisc_device);
    
        if (!mydev->pwm) {
            pr_err("Erreur: PWM non initalised\n");
            return -ENODEV;
        }
         // Limiter la taille de la lecture
        if (len > sizeof(kbuf) - 1)
            len = sizeof(kbuf) - 1;
    
        if (copy_from_user(kbuf, buf, len))
            return -EFAULT;
        kbuf[len] = '\0';
    
        if (kstrtoul(kbuf, 10, &freq) != 0) {
            pr_err("frequency invalide: %s\n", kbuf);
            return -EINVAL;
        }
        if (freq == 0) {
            if (mydev->buzzer_active) {
                pwm_disable(mydev->pwm);
                mydev->buzzer_active = false;
                pr_info("Buzzer stopped\n");
            }
            return len;
        }
        period_ns = 1000000000UL / freq; /// Conversion Hz -> ns
        duty_ns = period_ns / 2;           // Rapport cyclique 50%
        pwm_disable(mydev->pwm);  // disable before reconfiguration
        // Configurer le PWM
        int ret =0 ;
        ret = pwm_config(mydev->pwm, duty_ns, period_ns);
        if (ret) {
            pr_err("Erreur de configuration PWM: %d\n", ret);
            return ret;
        }
        // Activer le PWM
        ret = pwm_enable(mydev->pwm);
        if (ret) {
            pr_err("Erreur d'activation PWM: %d\n", ret);
            return ret;
        }
        mydev->buzzer_active = true;
    
        pr_info("PWM active  %lu Hz (period: %lu ns, duty: %lu ns)\n", freq, period_ns, duty_ns);
    
        return len;
}
    
static ssize_t my_read(struct file* flip, char __user *buf, size_t count, loff_t* pos) {
   
    return 0;
}

static const struct file_operations my_dev_fops = {
    .owner = THIS_MODULE,
    .read = my_read,
    .write = my_write,
};

// --- Probe ---

static int mydev_probe(struct platform_device *pdev)
{
    int ret;
    struct mydev_misc_t *mydev;

    mydev = devm_kzalloc(&pdev->dev, sizeof(struct mydev_misc_t), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;

    ret = of_property_read_string(pdev->dev.of_node, "label", &mydev->device_name);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read label property\n");
        return ret;
    }

    mydev->pwm = devm_pwm_get(&pdev->dev, "buzzer");
    if (IS_ERR(mydev->pwm)) {
        dev_err(&pdev->dev, "Failed to get PWM\n");
        return PTR_ERR(mydev->pwm);
    }

    mydev->mymisc_device.minor = MISC_DYNAMIC_MINOR;
    mydev->mymisc_device.name = mydev->device_name;
    mydev->mymisc_device.fops = &my_dev_fops;
    mydev->mymisc_device.mode = 0666;

    ret = misc_register(&mydev->mymisc_device);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register misc device\n");
        return ret;
    }

    platform_set_drvdata(pdev, mydev);

    dev_info(&pdev->dev, "PWM buzzer initialized: %s\n", mydev->device_name);
    return 0;
}

// --- Remove ---

static void mydev_remove(struct platform_device *pdev)
{
    struct mydev_misc_t *mydev = platform_get_drvdata(pdev);

    pwm_disable(mydev->pwm);
    misc_deregister(&mydev->mymisc_device);

    dev_info(&pdev->dev, "Buzzer removed: %s\n", mydev->device_name);
    
}

// --- Driver struct ---

static struct platform_driver my_driver = {
    .driver = {
        .name = "Buzzer_dev",
        .of_match_table = my_table_of_matching,
        .owner = THIS_MODULE,
    },
    .probe = mydev_probe,
    .remove = mydev_remove,
};


static int __init platform_dev_init(void)
{
    int ret = platform_driver_register(&my_driver);
    if (ret)
        pr_err("Failed to register platform driver\n");
    return ret;
}

static void __exit platform_dev_exit(void)
{
    platform_driver_unregister(&my_driver);
    pr_info("Buzzer platform driver unregistered\n");
}

module_init(platform_dev_init);
module_exit(platform_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asma REBHI");
MODULE_DESCRIPTION("PWM Buzzer Device");
