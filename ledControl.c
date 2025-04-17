// Includes
#include <linux/gpio.h> 
#include <linux/interrupt.h> 
#include <linux/kernel.h>
#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/version.h> 
#include <linux/atomic.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/types.h> 
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/err.h>

// Global defines /  Preprocessor
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0) 
	#define NO_GPIO_REQUEST_ARRAY 
#endif 

#define TIMEOUT_SEC 0
#define TIMEOUT_NSEC  100000
#define SUCCESS 0 
#define DEVICE_NAME "LedController"
#define MY_DELAY HZ / 5
#define BUF_LEN 80

// Global Statics
static int irq_1 = -1;
static int irq_2 = -1;
static struct hrtimer etx_hr_timer;
static int major;

static int intensity_Led1 = 0;
static int intensity_Led2 = 0;
static int intensity_Led3 = 0;
static int clicks = 0;

static uint32_t	*ledBaseAddr = NULL;
static struct kobject *fsObject;
 
enum { 
    CDEV_NOT_USED, 
    CDEV_EXCLUSIVE_OPEN, 
};

// Function Declarations
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
static int device_open(struct inode *, struct file *); 
static int device_release(struct inode *, struct file *); 
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);
enum hrtimer_restart timer_callback(struct hrtimer *);

static struct class *cls;

// File operations mapping
static struct file_operations led_Controller_fops = { 
    .read = device_read, 
    .write = device_write, 
    .open = device_open, 
    .release = device_release, 
}; 
 
// Push button defination for ISR
static struct gpio buttons[] = { { 18, GPIOF_IN, "BUTTON" }, { 16, GPIOF_IN, "BUTTON" } };

/*
	Interrupt Service Routine
	Executed whenever button is pushed
*/
static irqreturn_t button_isr(int irq, void *data) 
{
    int value_1 = gpio_get_value(buttons[0].gpio);
    int value_2 = gpio_get_value(buttons[1].gpio);
    if ((value_1 == 1) || (value_2 == 1)) {clicks++;}
    pr_info("Value1: %d\tValue2:%d\tcount: %d", value_1, value_2, clicks);

    return IRQ_HANDLED;
} 

/*
	Timer Callback function
	Executed whenever timer is expired
*/
enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    static int kCounter = 0;

    // Setting the intensity/Duty cycle for LED1
    {   
	if (kCounter < intensity_Led1/10)
	{
		writel((1<<17), ledBaseAddr+7);
		pr_info("LED1 %d", intensity_Led1);
	}
	else
	{
		writel((1<<17), ledBaseAddr+10);
		pr_info("LED1 %d", intensity_Led1);
	}
    }
    // Setting the intensity/Duty cycle for LED2
    {
	if (kCounter < intensity_Led2/10)
	{
		writel((1<<6), ledBaseAddr+7);
		pr_info("LED2 %d", intensity_Led2);
	}
	else
	{
		writel((1<<6), ledBaseAddr+10);
		pr_info("LED2 %d", intensity_Led2);
	}
    }
    // Setting the intensity/Duty cycle for LED3
    {   
	if (kCounter < intensity_Led3/10)
	{
		writel((1<<21), ledBaseAddr+7);
		pr_info("LED1 %d", intensity_Led1);
	}
	else
	{
		writel((1<<21), ledBaseAddr+10);
		pr_info("LED3 %d", intensity_Led3);
	}
    }
//     pr_info("Timer Callback function Called PWM is  [%d%%]\n",kCounter*10);

    kCounter ++;
    if(kCounter == 10)
    {
	kCounter = 0;
    }

    hrtimer_forward_now(timer,ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC));
    return HRTIMER_RESTART;
}
/*
	File system access through sysfs
*/
/*
	Funtion to show the values of led intensity.
	It reads the value from led intensity variable and gives it the user program
*/
// LED 1 intensity
static ssize_t show_led1(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Current intensity of Led1 = %d", intensity_Led1);
}
// LED 2 Intensity
static ssize_t show_led2(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Current intensity of Led2 = %d", intensity_Led2);
}
// LED 3 intensity
static ssize_t show_led3(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Current intensity of Led3 = %d", intensity_Led3);
}
/*
	Funtion to show the number of time button is pushed
	It reads the value from count variable which is updated with interrupt
*/
static ssize_t show_clicks(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", clicks);
}
/*
	Function to store the values into led intemsity variable
	it writes the given value given by user into the variable which is used bby timer(PWM) to control the LED intensity/brightness
*/
// LED 1 intensity
static ssize_t store_led1(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{	
	sscanf(buff, "%d", &intensity_Led1);
	return -EINVAL;
}
// LED 2 intensity
static ssize_t store_led2(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{
	sscanf(buff, "%d", &intensity_Led2);
	return -EINVAL;
}
// LED 3 intensity
static ssize_t store_led3(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{	
	sscanf(buff, "%d", &intensity_Led3);
	return -EINVAL;
}
/*
	Function to store value into clicks variable
	it writes the given value into clicks. It is used to reset the number of clicks when user needs to
*/
static ssize_t store_clicks(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{	
	sscanf(buff, "%d", &clicks);
	return -EINVAL;
}
// Defining show and store attributes
static struct kobj_attribute led1_attribute = __ATTR(led1, 0660, show_led1, store_led1);
static struct kobj_attribute led2_attribute = __ATTR(led2, 0660, show_led2, store_led2);
static struct kobj_attribute led3_attribute = __ATTR(led3, 0660, show_led3, store_led3);
static struct kobj_attribute clicks_attribute = __ATTR(clicks, 0660, show_clicks, store_clicks);

// Called when a driver file is opened
static int device_open(struct inode *inode, struct file *file) 
{ 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 

    pr_info("LED Controller driver is opened\n"); 
    try_module_get(THIS_MODULE); 
 
    return SUCCESS; 
} 

// Called when driver file is closed
static int device_release(struct inode *inode, struct file *file) 
{ 
    atomic_set(&already_open, CDEV_NOT_USED); 
 
    module_put(THIS_MODULE); 
    pr_info("LED Controller Module is Closed\n");

    return SUCCESS; 
}

// Called when we read data using driver file
static ssize_t device_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset) 
{ 
	pr_info("Reading from LED Controller module.\n");
	return 0;
}

// Called when we write using this driver module
static ssize_t device_write(struct file *filp, const char __user *buff, 
                            size_t len, loff_t *off) 
{ 
    pr_info("Writing through LED Controller module.\n"); 
    return -EINVAL; 
} 

// Called when module in initialized
static int __init led_controller_init(void) 
{
/*
    Initializing/creating device and adding it to file system
*/
    	// Getting the major number
	major = register_chrdev(0, DEVICE_NAME, &led_Controller_fops); 
 
	if (major < 0) { 
	pr_alert("Registering led controller device failed with %d\n", major); 
	return major; 
	} 

	pr_info("I was assigned major number %d.\n", major); 
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
	cls = class_create(DEVICE_NAME); 
	#else 
		cls = class_create(THIS_MODULE, DEVICE_NAME); 
	#endif 
	// Creating the device
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	// Initializing required parameters to process interrupts
	#ifdef NO_GPIO_REQUEST_ARRAY 
		gpio_request(buttons[0].gpio, buttons[0].label); 
	#else 
		gpio_request_array(buttons, ARRAY_SIZE(buttons)); 
	#endif 
	// Getting/assigning Interrupt request to GPIO
	irq_1 = gpio_to_irq(buttons[0].gpio); 
	irq_2 = gpio_to_irq(buttons[1].gpio);
	if ((request_irq(irq_1, button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button", NULL))!=0 || \
	    (request_irq(irq_2, button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button", NULL))!= 0)
	{
		pr_err("Failed to set ISR!");
		return -1;
	}

	// Initializing Timer
	ktime_t ktime;
	ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC);
	hrtimer_init(&etx_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	etx_hr_timer.function = &timer_callback;
	hrtimer_start( &etx_hr_timer, ktime, HRTIMER_MODE_REL);

	// Initializing Sysfs
	uint32_t error = 0;

	fsObject = kobject_create_and_add(DEVICE_NAME, kernel_kobj);
	
	if (!fsObject)
		return -ENOMEM;
	// Creating LED 1 file
	error = sysfs_create_file(fsObject, &led1_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led1 file ""in /sys/kernel/Speed_Controlled_LEDs\n");
	}
	// Creating LED 2 file
	error = sysfs_create_file(fsObject, &led2_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led2 file ""in /sys/kernel/Speed_Controlled_LEDs\n");
	}
	// Creating LED 3 file
	error = sysfs_create_file(fsObject, &led3_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led3 file ""in /sys/kernel/Speed_Controlled_LEDs\n");
	}
	// Clearing Push Button file
	error = sysfs_create_file(fsObject, &clicks_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led3 file ""in /sys/kernel/Speed_Controlled_LEDs\n");
	}
	// Initializing LED
	ledBaseAddr = ioremap(0xfe200000, 4*16);
	
	writel((1<<21), ledBaseAddr + 1);	// Sets GPIO17 as output (LED1)
	writel((1<<18), ledBaseAddr);		// Sets GPIO6 as output (LED2)
	writel((1<<3), ledBaseAddr + 2);	// Sets GPIO21 as output (LED3)
	
	pr_info("Led Controller Device created on /dev/%s\n", DEVICE_NAME); 

	return SUCCESS;

} 

// Called when module is removed
static void __exit led_controller_exit(void) 
{ 
	free_irq(irq_1, NULL); 
	free_irq(irq_2, NULL); 
	#ifdef NO_GPIO_REQUEST_ARRAY 
		gpio_free(buttons[0].gpio); 
	#else  
		gpio_free_array(buttons, ARRAY_SIZE(buttons)); 
	#endif

	hrtimer_cancel(&etx_hr_timer); 
	iounmap(ledBaseAddr);
	kobject_put(fsObject);

	device_destroy(cls, MKDEV(major, 0)); 
	class_destroy(cls); 
     
	unregister_chrdev(major, DEVICE_NAME);
	pr_info("LED Controller Device Deleted Successfuly");
}
 
module_init(led_controller_init); 
module_exit(led_controller_exit); 
 
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("This driver controls LED intensity based on the speed of interrupets generated");
MODULE_AUTHOR("Vishwesh Kota");
