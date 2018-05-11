/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <asm/uaccess.h>        /* for function put_user */

// #define SUCCESS "Survive your PhD program"
#define SUCCESS 0
#define DEVICE_NAME "chardev"   /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80              /* Max length of the message from the device */

/* Prototypes - this would normally go in a .h file */
int born_module(void);
void die_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);


/* Global variables are declared as static, so are global within the file. */
static int Major;               /* Major number assigned to our device driver */
static int Device_Open = 0;     /* Is device open? Used to prevent multiple access to device */
static char msg[BUF_LEN];       /* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};


/* This function is called when the module is loaded */
int born_module(void){
  printk(KERN_INFO "\n\n");
  printk(KERN_ALERT "%s.born_module()\n",DEVICE_NAME);
  Major = register_chrdev(0, DEVICE_NAME, &fops);
  if (Major < 0) {
    printk(KERN_ALERT "ERROR registering char device with major number %d\n", Major);
    return Major;
  }
  printk(KERN_INFO "My major is %d, create a dev file with:\n", Major);
  printk(KERN_INFO "   mknod /dev/%s c %d 0\n", DEVICE_NAME, Major);
  return SUCCESS;
}



/* This function is called when the module is unloaded */
void die_module(void){
  // Unregister the device from file /proc/devices
  unregister_chrdev(Major, DEVICE_NAME);
  printk(KERN_ALERT "%s.die_module()\n",DEVICE_NAME);
}
// If the function is called init_module(), and cleanup_module,
// there is no need of these lines
module_init(born_module);
module_exit(die_module);







//////////////////////////////////
//////   Methods   ///////////////

//-------------------------------------------------------
// Called when a process tries to open the device file, like
// "cat /dev/mycharfile"

static int device_open(struct inode *inode, struct file *file){
  static int counter = 1;
  printk(KERN_INFO "%s.device_open()\n",DEVICE_NAME);
  if (Device_Open)
    return -EBUSY;
  Device_Open++;
  sprintf(msg, "Whatsup world %d times!\n", counter++);
  msg_Ptr = msg;
  try_module_get(THIS_MODULE);
  return SUCCESS;
}




//-------------------------------------------------------
// Called when a process closes the device file.

static int device_release(struct inode *inode, struct file *file){
  printk(KERN_INFO "%s.device_release()\n\n",DEVICE_NAME);
  Device_Open--; // We're now ready for our next caller
  /* Decrement the usage count, or else once you opened the file,
   * never get get rid of the module. 
   */
  module_put(THIS_MODULE);
  return 0;
}




//-------------------------------------------------------
// Called when a process, which already opened
// the dev file, attempts to read from it.

static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset){
  int bytes_read = 0;// Number of bytes actually written to the buffer
  printk(KERN_INFO "%s.device_read()\n",DEVICE_NAME);
  // If we're at the end of the message, return 0 signifying end of file
  if (*msg_Ptr == 0) return 0;
  while (length && *msg_Ptr) {// Actually put the data into the buffer

    /* The buffer is in the user data segment, not the kernel 
     * segment so "*" assignment won't work.  We have to use 
     * put_user which copies data from the kernel data segment to
     * the user data segment. */
    put_user(*(msg_Ptr++), buffer++);
    length--;
    bytes_read++;
  }
  //Most read functions return the number of bytes put into the buffer
  return bytes_read;
}




//-------------------------------------------------------
// Called when a process writes to dev file: echo "hi" > /dev/hello 

static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off){
  printk(KERN_INFO "%s.device_write()\n",DEVICE_NAME);
  printk(KERN_ALERT "You can not write here.\n");
  
  return -EINVAL;
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Peter Jay Salzman");
