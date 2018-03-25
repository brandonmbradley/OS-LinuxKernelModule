/* Programming Assignment 2: Kernel Module
 * COP4600 - University of Central Florida
 * Group 35 - Alexander Alvarez, Brandon Bradley
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
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
	 printk(KERN_INFO "chardevice: device was successfully removed\n");
}


static int dev_open(struct inode *inodep, struct file *filep)
{
	 counter++;
	 printk(KERN_INFO "chardevice: Opened, access count: %d\n", counter);
	 return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{

	 int bytesnotread = 0;
	 int x;
	
	 
	 if(size > len)
	 {
		 int new_size = (size - len);

		 // Will return the amount of bytes that were not successfully copied, returns 0 on sucess
		 bytesnotread = copy_to_user(buffer, response, len);

		 // copy_to_user was succesful

		 if(bytesnotread == 0)
		 {
		 printk(KERN_INFO "chardevice: User received %d chars from system\n", len);
		 size = new_size;
		 	for(x = 0; x<BUFFERMAX; x++)
		 	{
		 		if(x < (BUFFERMAX-len))
		 		{
		 			response[x]=response[x+len];
		 		}
	 			else
				{
	 				response[x]= '\0';
	 			}
	 		}
	 		return 0;
	  }
	 //Error Message
		 else
		 {
		 		printk(KERN_INFO "chardevice: user has failed to receive %d chars from system\n", bytesnotread);
		 		return -EFAULT;
		 }
	 }
	 else
	 {
			 bytesnotread = copy_to_user(buffer, response, size);
			 //Success Message
			 if(bytesnotread == 0)
			 {
			 		printk(KERN_INFO "chardevice: user has received %d chars from system\n", size);
				 for(x = 0; x<BUFFERMAX; x++)
				 {
				 		response[x]= '\0';
				 }
				 return (size = 0);
			 }
			 //Error Message
			 else
			 {
				 printk(KERN_INFO "chardevice: user has failed to obtain %d chars from system\n", bytesnotread);
				 return -EFAULT;
	 		  }
	}

}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{


		int i = 0;
		int overflow = 0;
		int messageLength = strlen(buffer);
		int messageLessThan = 0;



		//If the len is less than the actual message length, we are storing a substring
	   if (len < messageLength)
		 {

			//Change the message length to the requested length
			messageLength = len ;

			//Set the flag
			messageLessThan = 1;
		

		}



		//If the message will overflow the buffer
	 	if((size+messageLength) > BUFFERMAX)
		{

			//Calculate the overflow
			overflow = BUFFERMAX - (size+messageLength);

			//Store what we can in the response buffer
			for(i = 0; i < (BUFFERMAX - size); i++)
			{
				response[i+(size)] = buffer[i];

			}

			//-1 for the null character
			printk(KERN_INFO "chardevice: Maximum buffer size reached only %i chars were stored of %s.\n",i, buffer);


			//The new size is the size of the response buffer
			size = strlen(response);


			//Return the actual message length - buffer - 1 for null terminator
			return i-1;
		}



		//If our buffer is currently empty
		if(strlen(response) == 0)
		{
			//The flag was set for substring message
			if (messageLessThan == 1) {


				//Store the substring message
				int j = 0;

				for(j = 0; j < messageLength; j++)
				{
					response[j] = buffer[j];

				}

			}

			//If we are normally storing to the empty buffer
			else {

				sprintf(response, "%s", buffer);

			}


		}

		//The buffer is not empty
		else
		{

			//We are storing a substring of the message
			if (messageLessThan == 1) {

				//The available index to store is the size of the buffer, +k on each iteration
				int k = 0;

				for(k = 0; k < messageLength; k++)
				{
					response[size+k] = buffer[k];

				}

			}

			//We are normally concatenating the message
			else {

				strcat(response, buffer);

			}
		}

		//The size is the length
		size = strlen(response);
		printk(KERN_INFO "chardevice: %i characters received from user %s.\n", messageLength-1, buffer);

		//Return the actual message length
		return messageLength-1;

}




static int dev_release(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "chardevice: Device closed.\n");
	return 0;
}

module_init(chardevice_init);
module_exit(chardevice_exit);
