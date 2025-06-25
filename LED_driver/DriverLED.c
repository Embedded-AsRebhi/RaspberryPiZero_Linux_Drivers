#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/gpio/consumer.h>
#include <linux/uaccess.h>  
#include <linux/string.h>   


//cette structure la pour décrire un peripherique
struct mydev_misc_t {
    struct miscdevice mymisc_device;
    const char *device_name;
    struct gpio_desc *gpio;
    bool led_state;
};


//mydevwrite(file , buffer, taille, position)
//container_of :: Sert à remonter à la structure complète
static ssize_t mydev_write(struct file *flip, const char __user *buf, size_t len, loff_t *off)
{
    char kbuf[10] = {0};
   
    struct mydev_misc_t *mydev = container_of(flip->private_data, struct mydev_misc_t, mymisc_device);

    if (len > 9)
        len = 9;
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;
    
    //c important de mettre \0 a chaque fois dans le kbuf puisque c une chaine de caractére sinon on aura des errors
    kbuf[len] = '\0';
    //pr_info("mydev_write() received: %s\n", kbuf);
    //strncasecmp pour faire la comparaison
    if (!strncasecmp(kbuf, "1", 1)) {
        gpiod_set_value(mydev->gpio, 1);
        mydev->led_state = true;
        pr_info("LED allumé\n");
    } else if (!strncasecmp(kbuf, "0", 1)) {
        gpiod_set_value(mydev->gpio, 0);
        mydev->led_state = false;
        pr_info("LED éteinte \n");
    } else {
        pr_info("Commande inconnue: %s\n", kbuf);
    }
    return len;
}
// Fonction read , pour voir le status de LED 
static ssize_t mydev_read(struct file *file, char __user *buff, size_t count, loff_t *ppos)
{   
    struct mydev_misc_t *mydev = container_of(file->private_data, struct mydev_misc_t, mymisc_device);
    char kbuf[10];
    int len;

    if (*ppos > 0)
        return 0; 
    if (mydev->led_state)
        len = snprintf(kbuf, sizeof(kbuf), "ON\n");
    else
        len = snprintf(kbuf, sizeof(kbuf), "OFF\n");
    if (count < len)
        return -EINVAL;
    if (copy_to_user(buff, kbuf, len))
        return -EFAULT;
    *ppos += len;
    return len;
}

//define the  fonctions that will execute on the developped drivers 
static const struct file_operations mydev_fops = {
    .owner = THIS_MODULE,
    .read = mydev_read,
    .write = mydev_write,
};


//mydev_probe will execute each time we add the drivers to the kernel using insmod 
static int mydev_probe(struct platform_device *pdev)
{
    int ret_val;
    struct mydev_misc_t *mydev;
    pr_info("LED_probe function is called.\n");
    mydev = devm_kzalloc(&pdev->dev, sizeof(struct mydev_misc_t), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;
    of_property_read_string(pdev->dev.of_node, "label", &mydev->device_name);
    mydev->mymisc_device.minor = MISC_DYNAMIC_MINOR;
    mydev->mymisc_device.name = mydev->device_name;
    mydev->mymisc_device.fops = &mydev_fops;
    mydev->mymisc_device.mode = 0666;
    mydev->led_state = true;
    mydev->gpio = gpiod_get(&pdev->dev, "led", GPIOD_OUT_HIGH);
    if (IS_ERR(mydev->gpio))
        return PTR_ERR(mydev->gpio);
    gpiod_direction_output(mydev->gpio,0);
    ret_val = misc_register(&mydev->mymisc_device);
    if (ret_val) {
        pr_err("Failed to register misc LED device\n");
        gpiod_put(mydev->gpio);
        return ret_val;
    }
    platform_set_drvdata(pdev, mydev);
    pr_info("Probe device : %s\n", mydev->device_name); 
    return 0;
}
//mydev_remove called each time we delete the drivers from kernel using rmmod 
static void mydev_remove(struct platform_device *pdev)
{
    struct mydev_misc_t *mydev = platform_get_drvdata(pdev);

    pr_info("LED_remove function is called.\n");
    misc_deregister(&mydev->mymisc_device);
    gpiod_put(mydev->gpio);
}

// comp,mygpio : is the reference in the device tree , it should be compatible with the description in Device tree
static const struct of_device_id my_table_of_matching[] = {
    { .compatible = "comp,mygpio" },
    {},
};

MODULE_DEVICE_TABLE(of, my_table_of_matching);

// This structure defines the callbacks that will be executed 
// when the driver is successfully registered or removed  with/from the kernel.
//LED : driver names /dev/LED
static struct platform_driver my_platform_driver = {
    .probe = mydev_probe,
    .remove = mydev_remove,
    .driver = {
        .name = "LED", 
        .of_match_table = my_table_of_matching,
        .owner = THIS_MODULE,
    }
};

static __init int mydev_init(void)
{
    pr_info("LED driver initializing\n");
    return platform_driver_register(&my_platform_driver);
}

static __exit void mydev_exit(void)
{
    pr_info("LED Driver Exit \n");
    platform_driver_unregister(&my_platform_driver);
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asma REBHI");
MODULE_DESCRIPTION("LED driver contrôle LED ");
