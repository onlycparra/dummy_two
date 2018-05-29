#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>        //file operations structure (open/close, read/write)
#include <linux/cdev.h>      //makes char devices availables.
#include <linux/semaphore.h> //access to semaphores to coordinate syncronization https://tuxthink.blogspot.com/2011/05/using-semaphores-in-linux.html
#include <linux/uaccess.h>   //copy to and from user
#define DEV_SIZE 64

//struct for device
struct Dummy_device{
  char data[DEV_SIZE];
  struct semaphore sem;
} dum_dev;


//variables (declared here to not use the small available kernel stack).
struct cdev *my_char_dev;  // struct that will allow us to register the char device.
int major_number;               // will hold the device major number.
int ret;                        // used for returns.
dev_t dev_num;                  // stuct that stores the device number given by the OS.
#define DEVICE_NAME "dummy_two"


static int driver_entry(void);
static void driver_exit(void);
int device_open(struct inode *inode, struct file *filp);
ssize_t device_read(struct file* filp, char* userBuffer, size_t bufCount, loff_t* curOffset);
ssize_t device_write(struct file* filp, const char* userBuffer, size_t bufCount, loff_t* curOffset);
int device_close(struct inode *inode, struct file *filp);


// tell the kernel what functions to call when the user operates in our device file.
struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .write = device_write,
  .read = device_read
};


// define entry and exit functions of the driver
module_init(driver_entry);
module_exit(driver_exit);











//////////////////////////////////////////////////////////////
////////////////////// DEFINITIONS ///////////////////////////
//////////////////////////////////////////////////////////////




static int driver_entry(void){
  // register the device with the system:
  // (step 1) dynamic allocation of device major number
  // alloc_chardev_region(dev_t*, uint fminor, uint count, char* name)
  ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
  if(ret < 0){
    printk(KERN_ALERT DEVICE_NAME ": failed to allocate major number\n");
    return ret;   //propagate error
  }
  major_number = MAJOR(dev_num); //extract major number assigned by OS.
  printk(KERN_INFO DEVICE_NAME ": major number allocated\n");
  printk(KERN_INFO DEVICE_NAME ": \tuse \"mknod /dev/" DEVICE_NAME " c %d 0\" to create device file\n",major_number);

  // (step 2) create the cdev structure
  my_char_dev = cdev_alloc();  // initialize char dev.
  my_char_dev->ops = &fops;    // file operations offered by the driver.
  my_char_dev->owner = THIS_MODULE;

  // (step 3) add the character device to the system
  // int cdev_add(struct cdev* dev, dev_t num, unsigned int count)
  ret = cdev_add(my_char_dev, dev_num, 1);
  if(ret < 0){
    printk(KERN_ALERT DEVICE_NAME ": failed to add cdev to kernel\n");
    return ret;   //propagate error
  }

  //(step 4) initialize semaphore
  sema_init(&dum_dev.sem,1);   // initial value 1, it can access only one thread at the time.
  return 0;
}

static void driver_exit(void){
  // remove char device from system
  cdev_del(my_char_dev);

  // deallocate device major number
  unregister_chrdev_region(dev_num,1);
  printk(KERN_INFO DEVICE_NAME ": cdev removed from kernel\n");
}

int device_open(struct inode *inode, struct file *filp){
  ret = down_interruptible(&dum_dev.sem);
  if(ret != 0){
    printk(KERN_ALERT DEVICE_NAME ": could not lock device during open\n");
    return ret;
  }
  printk(KERN_INFO DEVICE_NAME ": device opened\n");
  return 0;
}

ssize_t device_read(struct file* filp, char* userBuffer, size_t bufCount, loff_t* curOffset){
  // parameters: abstract file, buffer where to copy the data,
  // how much to copy, current possition in the opened file.

  if(bufCount > DEV_SIZE){
    bufCount = DEV_SIZE;
    printk(KERN_INFO DEVICE_NAME ": data read truncated to %d\n",DEV_SIZE);
  }
  
  // move data from kernel space (device) to user space (process).
  // copy_to_user(to,from,size)
  printk(KERN_INFO DEVICE_NAME ": reading %ld chars from device\n",bufCount);
  ret = copy_to_user(userBuffer,dum_dev.data,bufCount);
  return ret;
}

ssize_t device_write(struct file* filp, const char* userBuffer, size_t bufCount, loff_t* curOffset){
  // parameters: abstract file, buffer where to get data from,
  // how much to copy, current possition in the opened file.
  
  if(bufCount > DEV_SIZE){
    bufCount = DEV_SIZE;
    printk(KERN_INFO DEVICE_NAME ": data to write truncated to %d\n",DEV_SIZE);
  }
  
  // move data from the user space (process) to the kernel space (device).
  // copy_from_user(to,from,size)
  printk(KERN_INFO DEVICE_NAME ": writing %ld chars to device\n",bufCount);
  ret = copy_from_user(dum_dev.data,userBuffer,bufCount);
  return ret;
}

int device_close(struct inode *inode, struct file *filp){
  up(&dum_dev.sem);
  printk(KERN_INFO DEVICE_NAME ": device closed\n");
  return 0;
}





