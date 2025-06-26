#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/gpio/consumer.h>
#include <linux/pwm.h>
#include <linux/slab.h>

#define SERVO_PWM_PERIOD  20000000 // 20ms
#define SERVO_MAX_DUTY    2500000  // 2.5ms
#define SERVO_MIN_DUTY    700000   // 0.7ms
#define SERVO_DEGREE      ((SERVO_MAX_DUTY - SERVO_MIN_DUTY) / 180)

struct myservo_misc_t {
    struct miscdevice miscdev;
    const char* device_name;
    struct pwm_device *pwm;
    unsigned int current_angle;
};

static const struct of_device_id servo_of_match[] = {
    { .compatible = "comp,servomotor" },
    { }
};
MODULE_DEVICE_TABLE(of, servo_of_match);

// this function wait a value between 0 and 180 else an error will display
static ssize_t myservo_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char kbuf[8] = {0};
    unsigned long angle;
    unsigned long duty_ns;
    int ret;
    struct myservo_misc_t *myservo = container_of(file->private_data, struct myservo_misc_t, miscdev);
    if (!myservo->pwm) {
        pr_err("Servo: PWM not initialized\n");
        return -ENODEV;
    }
    if (len >= sizeof(kbuf))
        len = sizeof(kbuf) - 1;
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;
    kbuf[len] = '\0';
    if (kstrtoul(kbuf, 10, &angle) != 0 || angle > 180) {
        pr_err("Servo: Invalid angle '%s'\n", kbuf);
        return -EINVAL;
    }
    duty_ns = SERVO_MIN_DUTY + (angle * SERVO_DEGREE);
    pwm_disable(myservo->pwm);
    ret = pwm_config(myservo->pwm, duty_ns, SERVO_PWM_PERIOD);
    if (ret) {
        pr_err("Servo: pwm_config failed (%d)\n", ret);
        return ret;
    }
    ret = pwm_enable(myservo->pwm);
    if (ret) {
        pr_err("Servo: pwm_enable failed (%d)\n", ret);
        return ret;
    }
    myservo->current_angle = angle;
    pr_info("Servo: set to %lu degrees (duty: %lu ns)\n", angle, duty_ns);
    return len;
}

static const struct file_operations myservo_fops = {
    .owner = THIS_MODULE,
    .write = myservo_write,
};

// probe function to save the device and make a file device under /dev
static int myservo_probe(struct platform_device *pdev)
{
    struct myservo_misc_t *myservo;
    int ret;

    myservo = devm_kzalloc(&pdev->dev, sizeof(*myservo), GFP_KERNEL);
    if (!myservo)
        return -ENOMEM;

    ret = of_property_read_string(pdev->dev.of_node, "label", &myservo->device_name);
    myservo->pwm = devm_pwm_get(&pdev->dev, "servomotor");
    if (IS_ERR(myservo->pwm)) {
        dev_err(&pdev->dev, "Servo: failed to get PWM\n");
        return PTR_ERR(myservo->pwm);
    }
    myservo->miscdev.minor = MISC_DYNAMIC_MINOR;
    myservo->miscdev.name = myservo->device_name;
    myservo->miscdev.fops = &myservo_fops;
    myservo->miscdev.mode = 0666;
    ret = misc_register(&myservo->miscdev);
    if (ret) {
        dev_err(&pdev->dev, "Servo: misc_register failed\n");
        return ret;
    }
    platform_set_drvdata(pdev, myservo);
    dev_info(&pdev->dev, " device initialized : %s\n", myservo->device_name);
    return 0;
}

//this function to remove from device 
static void myservo_remove(struct platform_device *pdev)
{
    struct myservo_misc_t *myservo = platform_get_drvdata(pdev);
    pwm_disable(myservo->pwm);
    misc_deregister(&myservo->miscdev);
    dev_info(&pdev->dev, "Servo device removed\n");
    
}

static struct platform_driver servo_driver = {
    .driver = {
        .name = "servo_dev",
        .of_match_table = servo_of_match,
        .owner = THIS_MODULE,
    },
    .probe = myservo_probe,
    .remove = myservo_remove,
};

module_platform_driver(servo_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asma REBHI");
MODULE_DESCRIPTION("PWM ServoMotor Driver");
