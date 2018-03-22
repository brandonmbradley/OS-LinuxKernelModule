/* Programming Assignment 2: Kernel Module
 * COP4600 - University of Central Florida
 * Group 35 - Alexander Alvarez, Brandon Bradley
 * 
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "chardevice"
#define CLASS_NAME  "char"
#define BUFFERMAX 10 

/* References: http://www.tldp.org/LDP/lkmpg/2.6/html/
	          http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
*/


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexander Alvarez, Brandon Bradley");
MODULE_DESCRIPTION("Assignment 2 COP4600");
MODULE_VERSION("1.0");


static int    major, counter = 0;
static char   response[BUFFERMAX];
static int size = 0;   
static struct class* chardev_class = NULL;
static struct device* chardev_device = NULL;


static int     dev_open(struct inode *, struct file *); // Open device
static int     dev_release(struct inode *, struct file *); //Close the device
static ssize_t dev_read(struct file *, char *, size_t, loff_t *); //Read from
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *); // Write to

//Contained in fs.h
static struct file_operations fops =
{
	  .open = dev_open,
	  .read = dev_read,
	  .write = dev_write,
	  .release = dev_release,
};

static int __init chardevice_init(void)
{
	printk(KERN_INFO "chardevice: Initializing chardevice\n");
 
	// register_chrdev first agrument 0 for dynamic allocation
	major = register_chrdev(0, DEVICE_NAME, &fops);
 
	if(major < 0)
	{
		printk(KERN_ALERT "Failed to create register a major number\n");
		return major;
	}
	
	
	printk(KERN_INFO "chardevice: successfully registered with major number #%d\n",major);
 
	//Populate Class
	chardev_class = class_create(THIS_MODULE, CLASS_NAME);
 

	// Upon failure to register de-register device
	if(IS_ERR(chardev_class))
	{
		unregister_chrdev(major, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register class\n");
		return PTR_ERR(chardev_class);
	}
	 printk(KERN_INFO "chardevice: class registered successfully\n");
	 
	
	 chardev_device = device_create(chardev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	
	// Upon failure to create failed de-register device
	 if(IS_ERR(chardev_device))
	 {
		 class_destroy(chardev_class);
		 unregister_chrdev(major, DEVICE_NAME);
		 printk(KERN_ALERT "Failed to create device driver\n");
		 return PTR_ERR(chardev_device);
	 }
	 printk(KERN_INFO "chardevice: device class created successfully\n");
	 return 0;
}

// Called to remove driver
static void __exit chardevice_exit(void)
{
	 device_destroy(chardev_class, MKDEV(major, 0));
	 class_unregister(chardev_class);
	 class_destroy(chardev_class);
	 unregister_chrdev(major, DEVICE_NAME);
	 printk(KERN_INFO "chardevice: succsfully removed\n");
}


static int dev_open(struct inode *inodep, struct file *filep)
{
	 counter++;
	 printk(KERN_INFO "chardevice: Opened, access count: %d\n", counter);
	 return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	 int errorFlag = 0, i;
	 
	 if(size>len)
	 {
		 int new_size = (size - len);
		 errorFlag = copy_to_user(buffer, response, len);
		 //Success Message
		 if(errorFlag == 0)
		 {
			 printk(KERN_INFO "User has been sent %d chars from system\n", len);
			 size = new_size;
			 for(i = 0; i< BUFFERMAX; i++)
			 {
				if(i<(BUFFERMAX-len))
				{
					response[i]=response[i+len];
				}
				else
				{
				response[i]= '\0';
				}
			 }
			 return 0;
		 }
		 
		 else
		 {
			 printk(KERN_INFO "User was not able to be sent %d chars from system\n", errorFlag);
			 return -EFAULT;
		 }
	 }
	 
	 else
	 {
		 errorFlag = copy_to_user(buffer, response, size);
		
		 if(errorFlag == 0)
		 {
			printk(KERN_INFO "User has obtained %d chars from system\n", size);
			for(i = 0; i < BUFFERMAX; i++)
			{
				response[i]= '\0';
			}
			
			return (size = 0);
		 }
		 
		 else
		 {
			 printk(KERN_INFO "User was not able to obtain %d chars from system\n", errorFlag);
			 return -EFAULT;
		 }
	 } 
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{


// Handle overflow conditions
	int i = 0;
	if((size+len)>BUFFERMAX)
	{
		for(i = 0; i<(BUFFERMAX - size); i++)
		{
			response[i+(size)] = buffer[i];
		}
		
		printk(KERN_INFO "Maximum buffer size reached only %d were stored.\n", i);
		size = strlen(response);
	}
	
	else
	{
		if(strlen(response) == 0)
		{
			sprintf(response, "%s", buffer);
		}
		
		else
		{
			strcat(response, buffer);
		}
		
		size = strlen(response);
		printk(KERN_INFO "chardevice: %d characters received from user [%s].\n", len,buffer);
		i = len;
	}
	
	return i;
 	
	
}


/*
	
*/


static int dev_release(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "chardevice: Device closed.\n");
	return 0;
}

module_init(chardevice_init);
module_exit(chardevice_exit);
