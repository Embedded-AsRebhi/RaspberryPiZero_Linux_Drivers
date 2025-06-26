#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/fs.h>
#include <linux/pinctrl/consumer.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/pwm.h>

#define DEVICE_NAME "mydev"
#define NB_MAX_BUTTON 10 // we can make until 10 button for diff devices , each button interrupt a device function
#define BUZZER_FREQ_DEFAULT 2000  // frequency en HZ

static int my_button_probe(struct platform_device *pdev);
static int my_buzzer_probe(struct platform_device *pdev);
static ssize_t button_read(struct file *file, char __user *buff, size_t count, loff_t *ppos);
static int control_buzzer(void *data);
static void my_buzzer_remove(struct platform_device *pdev);
static void my_button_remove(struct platform_device *pdev);

static char *INTERRUPT_NAME = "BUTTON_INT";
static unsigned int button_count = 0;
static int irq_array[NB_MAX_BUTTON] = {};

// Structure pour le buzzer
struct buzzer_data {
    struct pwm_device *pwm;
    bool active;
    unsigned long freq;
    struct task_struct *thread;
};

static struct buzzer_data buzzer;

// Structure pour le bouton
struct blink_t {
    bool is_pressed;
};

static struct blink_t tab_blink[NB_MAX_BUTTON];

//struct of button 
static const struct of_device_id my_of_ids[] = {
    { .compatible = "comp,intbutton" },
    {},
};
MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver my_button_platform_driver = {
    .probe = my_button_probe,
    .remove = my_button_remove,
    .driver = {
        .name = "int_button",
        .of_match_table = my_of_ids,
        .owner = THIS_MODULE,
    }
};

//struct of buzzer 
static const struct of_device_id my_buzzer_of_ids[] = {
    { .compatible = "comp,buzzer" },
    {},
};
MODULE_DEVICE_TABLE(of, my_buzzer_of_ids);

static struct platform_driver my_buzzer_platform_driver = {
    .probe = my_buzzer_probe,
    .remove = my_buzzer_remove,
    .driver = {
        .name = "my_Buzzer",
        .of_match_table = my_buzzer_of_ids,
        .owner = THIS_MODULE,
    }
};
// file_operation of button
static const struct file_operations button_fops = {
    .owner = THIS_MODULE,
    .read = button_read,
};

static struct miscdevice my_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .mode = 0666,
    .fops = &button_fops,
};

//control and config the the buzzer 
static int control_buzzer(void *data) {
    struct buzzer_data *buzz = (struct buzzer_data *)data;
    unsigned long period_ns, duty_ns;
    
    while (!kthread_should_stop()) {
        if (buzz->active) {
            period_ns = 1000000000UL / buzz->freq;
            duty_ns = period_ns / 2;
            pwm_disable(buzz->pwm);
            pwm_config(buzz->pwm, duty_ns, period_ns);
            pwm_enable(buzz->pwm);
            pr_info("Buzzer active at %lu Hz\n", buzz->freq);
        } else {
            pwm_disable(buzz->pwm);
        }
        msleep_interruptible(100);
    }
    return 0;
}

//interrupt function 
static irqreturn_t keys_isr(int irq, void *data) {    
    for (size_t i = 0; i < button_count; i++) {
        if (irq == irq_array[i]) {
            tab_blink[i].is_pressed = !tab_blink[i].is_pressed;
            
            // Contrôle du buzzer
            if (tab_blink[i].is_pressed) {
                buzzer.active = true;
                buzzer.freq = BUZZER_FREQ_DEFAULT + (i * 500); 
            } else {
                buzzer.active = false;
            }
        }
    }
    return IRQ_HANDLED;
}

// read the button state 
static ssize_t button_read(struct file *file, char __user *buff, size_t count, loff_t *ppos) {
    char kbuff[1000];
    size_t len = 0;
    int ret;
    if (*ppos != 0) {
        return 0;
    }
    for (size_t i = 0; i < button_count; i++) {
        len += snprintf(kbuff + len, sizeof(kbuff) - len, 
                      "button %zu : %s\n", 
                      i+1, 
                      tab_blink[i].is_pressed ? "ON" : "OFF");
    }
    ret = copy_to_user(buff, kbuff, len);
    if (ret < 0) {
        pr_err("copy_to_user\n");
        return ret;
    }
    *ppos += 1;
    return len;
}

//probe function of the button : button define as an interruption 
static int my_button_probe(struct platform_device *pdev) {
    int ret_val, irq;
    struct device *dev = &pdev->dev;
    dev_info(dev, "my_button_probe function is called.\n");
    irq = platform_get_irq(pdev, 0);
    if (irq <= 0) {
        dev_err(dev, "platform_get_irq\n");
        return irq;
    }
    // will wait the interrupt function : to verify the bush button is pressed or not yet 
    ret_val = devm_request_irq(dev, irq, keys_isr, IRQF_SHARED, INTERRUPT_NAME, dev);
    if (ret_val < 0) {
        dev_err(dev, "request_irq\n");
        return ret_val;
    }
    irq_array[button_count] = irq;
    button_count++;
    
    return 0;
}

//the probe function of the buzzer 
static int my_buzzer_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    //int ret;
    
    dev_info(dev, "my_buzzer_probe() function is called.\n");

    buzzer.pwm = devm_pwm_get(dev, NULL);
    if (IS_ERR(buzzer.pwm)) {
        dev_err(dev, "Failed to get PWM\n");
        return PTR_ERR(buzzer.pwm);
    }
    buzzer.active = false;
    buzzer.freq = BUZZER_FREQ_DEFAULT;
    // Création du thread to execute the buzzer function (config and send a command)
    buzzer.thread = kthread_run(control_buzzer, &buzzer, "buzzer_thread");
    if (IS_ERR(buzzer.thread)) {
        dev_err(dev, "Failed to create buzzer thread\n");
        return PTR_ERR(buzzer.thread);
    }
    return 0;
}

//Remove the buzzer function 
static void my_buzzer_remove(struct platform_device *pdev) {
    dev_info(&pdev->dev, "my_buzzer_remove() function is called.\n");
    if (buzzer.thread) {
        kthread_stop(buzzer.thread);
    }
    pwm_disable(buzzer.pwm);
}

//the button remove 
static void my_button_remove(struct platform_device *pdev) {
    dev_info(&pdev->dev, "my_button_remove() function is called.\n");    
}

static int __init my_init(void) {
    int ret_val;
    pr_info("Register buzzer platform device\n");
    ret_val = platform_driver_register(&my_buzzer_platform_driver);
    if (ret_val != 0) {
        pr_err("platform buzzer value returned %d\n", ret_val);
        return ret_val;
    }
    pr_info("Register button platform device\n");
    ret_val = platform_driver_register(&my_button_platform_driver);
    if (ret_val != 0) {
        pr_err("platform value returned %d\n", ret_val);
        platform_driver_unregister(&my_buzzer_platform_driver);
        return ret_val;
    }
    pr_info("Misc registering...\n");
    ret_val = misc_register(&my_miscdevice);
    if (ret_val != 0) {
        pr_err("could not register the misc device mydev\n");
        platform_driver_unregister(&my_buzzer_platform_driver);
        platform_driver_unregister(&my_button_platform_driver);
        return ret_val;
    }
    return 0;
}

static void __exit my_exit(void) {
    pr_info("Push button driver exit\n");
    pr_info("Deregister misc device...\n");
    misc_deregister(&my_miscdevice);
    platform_driver_unregister(&my_button_platform_driver);
    platform_driver_unregister(&my_buzzer_platform_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asma REBHI");
MODULE_DESCRIPTION("Using interrupts with push buttons and PWM buzzer");