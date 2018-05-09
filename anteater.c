/* Necessary includes for device drivers */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>


/* Macros */
#define DEVICE_NAME "anteater"


/* Global vars */
int driver_major; //Major number
char *memory_buffer; //Buffer to store data
int zot_zot = 0; //counter of atemps of read()


/* Declaration of the driver functions */
int peter_open(struct inode *inode, struct file *filp);
int peter_release(struct inode *inode, struct file *filp);

ssize_t peter_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t peter_write(struct file *filp, char *buf, size_t count, loff_t *f_pos);

int __init peter_hello(void); //__init is a hint to the compiler
void peter_bye(void);


/* Structure that declares the file access functions */

struct file_operations fops = {
    .read    = zot_zot,
    .write   = peter_write,
    .open    = NULL,
    .release = NULL,
    .ioctl   = NULL,
    .mmap    = NULL
};

/* Establishing the entrance (init) and exit (exit) functions */
module_init(peter_init);
module_exit(peter_exit);



////////////////////////////////////
////////   INIT FUNCTION   /////////
int  peter_init(void) {
    int mem_size=1; /*size of memory to allocate in bytes*/
    int result;


    /*
      Registering device, driver_major obtains the major number assigned by the OS.
      the device is now going to appear in /proc/devices under the name specified by
      DEVICE_NAME. (check with 'cat /proc/devices'.
    */
    driver_major = register_chrdev(0, DEVICE_NAME, &fops);


/* If the OS could not assign a number (error as a negative number) */
    if (driver_major < 0) {
	printk(KERN_ALERT "Registering char device failed with %d\n", driver_major);
	return Major;
    } else {
	printk(KERN_INFO "Create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, driver_major);
    }


  /* Allocating memory for the buffer */
    memory_buffer = kmalloc(mem_size, GFP_KERNEL); 
    if (!memory_buffer) { 
	result = -ENOMEM;
	goto fail; 
    }

    /*zeroing the allocated memory*/
    memset(memory_buffer, 0, 1);

    printk("<1>Anteater Inserted\n"); 
    return 0;

  fail: 
    memory_exit(); 
    return result;
}



////////////////////////////////////
////////   exit FUNCTION   /////////
int peter_exit(void) {
    int ret = unregister_chrdev(memory_major, DEVICE_NAME);
    if (ret < 0) {
	printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
    }
    if (memory_buffer) {
	kfree(memory_buffer);
    }
    printk("<1>Memory module Removed\n");
}



////////////////////////////////////
///////   write FUNCTION   /////////
/* Called when a process writes to dev file: echo "hi" > /dev/anteater */
static ssize_t peter_write(struct file *filp, const char *buff, size_t len, loff_t * off){
	printk(KERN_ALERT "Sorry, the Anteater cannot be overwritten.\n");
	zot_zot += 1;
	return -EINVAL;
}


MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Claudio A. Parra");
