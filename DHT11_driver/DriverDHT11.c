#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/ktime.h>


#define TIMEOUT 1000 // wainting time of data


struct mydev_misc_t {
    struct miscdevice mymisc_device;
    const char *device_name;
    struct gpio_desc *gpio;
};

struct my_data_t {
    int temperature;
    int humidity;
};



static u8 CalculateParity(u8 HumidityHigh, u8 HumidityLow,u8 TemperatureHigh, u8 TemperatureLow);
static s64 wait_for_low(struct gpio_desc *gpio);
static s64 wait_for_high(struct gpio_desc *gpio);
static void send_start_signal(struct gpio_desc *gpio);

//Extracte the temperature and Humidity  
static struct my_data_t ProcessData(u64 Data) {
    u8 humidity_h=Data>>32;
    u8 humidity_l=Data>>24;
    u8 temp_h=Data>>16;
    u8 temp_l=Data>>8;
    u8 checksum=Data;
    if (CalculateParity(humidity_h,humidity_l,temp_h,temp_l)==checksum) {
        struct my_data_t res={.temperature=temp_h,.humidity=humidity_h};
        pr_info("Temperature=%d and Humidity=%d\n",res.temperature,res.humidity);
        return res;
    }
    pr_err("Data is incorrect!\n");
    struct my_data_t res={.temperature=999,.humidity=999};
    return res;
}




//calcule de checksum
static u8 CalculateParity(u8 HumidityHigh, u8 HumidityLow,u8 TemperatureHigh, u8 TemperatureLow) {
    return (HumidityHigh + HumidityLow + TemperatureHigh+ TemperatureLow);
}

//Function for the wait for low signal 
static s64 wait_for_low(struct gpio_desc *gpio) {
    ktime_t start_time, end_time;
    s64 microseconds;
    start_time = ktime_get();
    while (gpiod_get_value(gpio)) {
        end_time = ktime_get();
        microseconds = ktime_to_us(ktime_sub(end_time, start_time));
        //ici il attend juste le timeout sinon il break sinon 
       
        if (microseconds > TIMEOUT) {
            pr_err("Time out WaitForLow!\n");
            break;
        }
    }
    end_time = ktime_get();
    return ktime_to_us(ktime_sub(end_time, start_time));
}

//Function for the wait for High signal 
static s64 wait_for_high(struct gpio_desc *gpio) {
    ktime_t start_time, end_time;
    s64 microseconds;
    start_time = ktime_get();
    while (!gpiod_get_value(gpio)) {
        end_time = ktime_get();
        microseconds = ktime_to_us(ktime_sub(end_time, start_time));
        if (microseconds > TIMEOUT) {
            pr_err("Time out WaitForHigh!\n");
            break;
        }
    }
    end_time = ktime_get();
    return ktime_to_us(ktime_sub(end_time, start_time));
}

// send start signal 
static void send_start_signal(struct gpio_desc *gpio) {

    //set 0 : initialement le capteur va envoyer 
    gpiod_set_value(gpio,0);
    msleep(20);
    //apres 20 ms va etre en mode repos 
    gpiod_set_value(gpio,1);
    //mettre le gpio es input 
    gpiod_direction_input(gpio);
}

//main Function to get the Temp et Hum 
static struct my_data_t measure(struct gpio_desc *gpio) {
    send_start_signal(gpio); // Réveiller le capteur
    wait_for_low(gpio);
    wait_for_high(gpio);
    wait_for_low(gpio);
    u64 data=0;
    for (int i=0;i<40;++i) {
        s64 d1=wait_for_high(gpio); // Durée de signal à 0
        s64 d2=wait_for_low(gpio); // Durée de signal à 1
        if (d1<d2) {
            data|=1;
        }
        data<<=1;
    }
    data>>=1;
    gpiod_direction_output(gpio,1);
    struct my_data_t res=ProcessData(data);
    return res;
}
// To get the Temp et Hum 
static ssize_t mydev_read(struct file *file, char __user *buff, size_t count, loff_t *ppos)
{
    pr_info("mydev_read() is called.\n");
    struct mydev_misc_t  *my_misc_dev = container_of(file->private_data, struct mydev_misc_t, mymisc_device);
    struct my_data_t data=measure(my_misc_dev->gpio);
    char kbuf[100];
    if (*ppos != 0) return 0;
    *ppos = 1;
    snprintf(kbuf, sizeof(kbuf), " %d %d\n", data.temperature, data.humidity);
    if (copy_to_user(buff, kbuf, strlen(kbuf)))
        return -EFAULT;
    return strlen(kbuf);
}



static const struct file_operations mydev_fops = {
    .owner = THIS_MODULE,
    .read = mydev_read,
};

static int mydev_probe(struct platform_device *pdev)
{
    int ret_val;
    struct mydev_misc_t *mydev;
    pr_info("mydev_probe DHT11 function is called.\n");
    mydev = devm_kzalloc(&pdev->dev, sizeof(struct mydev_misc_t), GFP_KERNEL);
    of_property_read_string(pdev->dev.of_node, "label", &mydev->device_name); 

    mydev->mymisc_device.minor = MISC_DYNAMIC_MINOR;
    mydev->mymisc_device.name = mydev->device_name;
    mydev->mymisc_device.fops = &mydev_fops;
    mydev->mymisc_device.mode=0444;
    
    mydev->gpio=gpiod_get(&pdev->dev,"data",GPIOD_OUT_HIGH);
    //configure en sortie avec valeur initiale 1
    gpiod_direction_output(mydev->gpio,1);
    ret_val = misc_register(&mydev->mymisc_device);
    if (ret_val) return ret_val; /* misc_register returns 0 if success */
    platform_set_drvdata(pdev, mydev);
    return 0;
}

static void mydev_remove(struct platform_device *pdev)
{
    struct mydev_misc_t  *misc_device = platform_get_drvdata(pdev);
    pr_info("DHt11 removefunction is called.\n");
    gpiod_put(misc_device->gpio);
    misc_deregister(&misc_device->mymisc_device);
}

/* Declare a list of devices supported by the driver */
static const struct of_device_id my_table_of_matching[] = {
{ .compatible = "comp,dht11"},
{},
};

MODULE_DEVICE_TABLE(of, my_table_of_matching);

static struct platform_driver my_platform_driver = {
    .probe = mydev_probe,
    .remove = mydev_remove,
    .driver = {
        .name = "Dht11",
        .of_match_table = my_table_of_matching,
        .owner = THIS_MODULE,
    }
};

// starting point 
static __init int mydev_init(void) {
    int ret;
    pr_info("dht driver initializing\n");
    ret = platform_driver_register(&my_platform_driver);
    if (ret) {
        pr_err("Failed to register platform DHt11 driver: %d\n", ret);
        return ret;
    }
    return 0;
}

// the leaving point
static __exit void mydev_exit(void)
{
    pr_info("dht Exit... driver exit\n");
    platform_driver_unregister(&my_platform_driver);
}

module_init(mydev_init);
module_exit(mydev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asma REBHI");
MODULE_DESCRIPTION(" platform driver DHT11 misc devices");
