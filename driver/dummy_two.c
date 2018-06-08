#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>        //struct file_operations
#include <linux/cdev.h>      //makes char devices availables: struct cdev, cdev_add()
#include <linux/semaphore.h> //semaphores to coordinate syncronization https://tuxthink.blogspot.com/2011/05/using-semaphores-in-linux.html
#include <linux/uaccess.h>   //copy_to_user, copy_from_user
#include <linux/slab.h>      //kmalloc, kfree
#include <linux/mm.h>        //vm_area_struct, PAGE_SIZE
#include "../include/ioctl_commands.h" //DUMMY_SYNC

//struct for device
struct Dummy_device{
  unsigned long* data;
  unsigned long size;
  struct semaphore sem;
} dum_dev;


//variables (declared here to not use the small available kernel stack).
struct cdev *my_char_dev; // struct that will allow us to register the char device.
int major_number; // will hold the device major number.
int ret; // used for returns.
dev_t dev_num; // stuct that stores the device number given by the OS.
#define DEVICE_NAME "dummy_two"


int device_open(struct inode *inode, struct file *filp);
ssize_t device_read(struct file* filp, char* userBuffer, size_t bufCount, loff_t* curOffset);
ssize_t device_write(struct file* filp, const char* userBuffer, size_t bufCount, loff_t* curOffset);
int device_close(struct inode *inode, struct file *filp);
int device_mmap(struct file *filp, struct vm_area_struct *vma);
long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

// tell the kernel what functions to call when the user operates in our device file.
struct file_operations fops = {
  .owner           = THIS_MODULE,
  .open            = device_open,
  .release         = device_close,
  .write           = device_write,
  .read            = device_read,
  .mmap            = device_mmap,
  .unlocked_ioctl  = device_ioctl
};



//////////////////////////////////////////////////////////////
////////////////////// DEFINITIONS ///////////////////////////
//////////////////////////////////////////////////////////////

/*
int device_open_2(struct inode *inode, struct file *filp){
  printk(KERN_WARNING DEVICE_NAME ": [opening]\n");
  ret = down_interruptible(&dum_dev.sem);
  if(ret != 0){
    printk(KERN_ALERT DEVICE_NAME ":     could not lock device during open\n");
    return ret;
  }

  arm_smccc_smc(OPTEE_SMC_OPEN_DUMMY, 0, 0, 0, 0, 0, 0, 0, &res);
  if(res.a0 == OPTEE_SMC_RETURN_OK){
    printk(KERN_INFO "DUMMY DRV: device_open ... SUCCESS\n");
    if(res.a1 == OPTEE_SMC_OPEN_DUMMY_SUCCESS) {
      printk(KERN_INFO "DUMMY DRV: device_open ... called smc call :");
      dum_dev.data = (unsigned long*) get_zeroed_page(GFP_KERNEL);
      if(dum_dev.data == NULL){
	printk(KERN_ALERT DEVICE_NAME ":     could not allocate memory\n");
	return -1;
      }
      dum_dev.size=PAGE_SIZE;

    }
    else
      printk(KERN_INFO "DUMMY DRV: device_open ... called smc call :");
  }

  printk(KERN_INFO DEVICE_NAME ":     opened\n");
  return 0;
}
*/






int device_open(struct inode *inode, struct file *filp){
  printk(KERN_WARNING DEVICE_NAME ": [opening]\n");
  ret = down_interruptible(&dum_dev.sem);
  if(ret != 0){
    printk(KERN_ALERT DEVICE_NAME ":     could not lock device during open\n");
    return ret;
  }
  arm_smccc_smc(OPTEE_SMC_OPEN_DUMMY, 0, 0, 0, 0, 0, 0, 0, &res);
  if(res.a0 == OPTEE_SMC_RETURN_OK){
    printk(KERN_INFO DEVICE_NAME ":     smc call... SUCCESS\n");
    if(res.a1 == OPTEE_SMC_OPEN_DUMMY_SUCCESS) {
      printk(KERN_INFO DEVICE_NAME ":     secure_device_open... SUCCESS\n");
      dum_dev.data = (unsigned long*) get_zeroed_page(GFP_KERNEL);
      dum_dev.size = 0;
      if(!dum_dev.data){
	printk(KERN_ALERT DEVICE_NAME ":     kernel could not allocate memory\n");
	return -1;
      }
      dum_dev.size=PAGE_SIZE;
      printk(KERN_INFO DEVICE_NAME ":     kernel %ld bytes allocated for the device\n", dum_dev.size);
    }else{
      printk(KERN_INFO DEVICE_NAME ":     secure_device_open... FAILURE\n");
    }
    printk(KERN_INFO DEVICE_NAME ":     kernel opened\n");
  }
  return 0;
}

ssize_t device_read(struct file* filp, char* userBuffer, size_t bufCount, loff_t* curOffset){
  printk(KERN_WARNING DEVICE_NAME ": [reading]\n");
  if(bufCount > dum_dev.size){
    bufCount = dum_dev.size;
    printk(KERN_INFO DEVICE_NAME ":     data read truncated to %ld\n",dum_dev.size);
  }
  
  // move data from kernel space (device) to user space (process).
  // copy_to_user(to,from,size)
  printk(KERN_INFO DEVICE_NAME ":     reading %ld chars from device\n",bufCount);
  ret = copy_to_user(userBuffer,dum_dev.data,bufCount);
  printk(KERN_INFO DEVICE_NAME ":     read\n");
  return ret;
}

ssize_t device_write(struct file* filp, const char* userBuffer, size_t bufCount, loff_t* curOffset){
  printk(KERN_WARNING DEVICE_NAME ": [writing]\n");
  if(bufCount > dum_dev.size){
    bufCount = dum_dev.size;
    printk(KERN_INFO DEVICE_NAME ":     data to write truncated to %ld\n",dum_dev.size);
  }
  
  // move data from the user space (process) to the kernel space (device).
  // copy_from_user(to,from,size)
  ret = copy_from_user(dum_dev.data,userBuffer,bufCount);
  //arm_smccc_smc(write_opcode, 0, 0, 0, 0, 0, 0, 0, &res);
  int i, blk_size=6,regs;
  int num_calls=(bufCount-1)/blk_size+1;
  int last_trim=(bufCount-1)%blk_size+1;
  for(i=0, i<num_calls; ++i){
    regs = i==num_calls-1? last_trim : blk_size;
    printk(KERN_INFO DEVICE_NAME ":     call:\n");
    printk(KERN_INFO DEVICE_NAME ":     arm_smccc_smc(OPTEE_SMC_OPEN_DUMMY,\n");
    printk(KERN_INFO DEVICE_NAME ":       %ld,\n",dum_dev.data[blk_size*i+0]);
    printk(KERN_INFO DEVICE_NAME ":       %ld,\n",dum_dev.data[blk_size*i+1]);
    printk(KERN_INFO DEVICE_NAME ":       %ld,\n",dum_dev.data[blk_size*i+2]);
    printk(KERN_INFO DEVICE_NAME ":       %ld,\n",dum_dev.data[blk_size*i+3]);
    printk(KERN_INFO DEVICE_NAME ":       %ld,\n",dum_dev.data[blk_size*i+4]);
    printk(KERN_INFO DEVICE_NAME ":       %ld,\n",dum_dev.data[blk_size*i+5]);
    printk(KERN_INFO DEVICE_NAME ":     );\n");
    /*
    arm_smccc_smc(OPTEE_SMC_OPEN_DUMMY,
		  regs,
		  dum_dev.data[blk_size*i+0],
		  dum_dev.data[blk_size*i+1],
		  dum_dev.data[blk_size*i+2],
		  dum_dev.data[blk_size*i+3],
		  dum_dev.data[blk_size*i+4],
		  dum_dev.data[blk_size*i+5],
		  &res
		  );
    */
  }


    
  printk(KERN_INFO DEVICE_NAME ":     %ld chars written into device\n",bufCount);
  printk(KERN_INFO DEVICE_NAME ":     written\n");
  return ret;
}

//mmap method, maps allocated memory into userspace

int device_mmap(struct file *filp, struct vm_area_struct *vma){
  printk(KERN_WARNING DEVICE_NAME ": [mmaping]\n");
  /**
   * remap_pfn_range - remap kernel memory to userspace
   * int remap_pfn_range(struct vm_area_struct* vma, // user vma to map to 
                         unsigned long addr,         \ target user address to start at
   *                     unsigned long pfn,          \ physical address of kernel memory
                         unsigned long size,         \ size of map area
                         pgprot_t prot);             \ page protection flags for this mapping
   *  Note: this is only safe if the mm semaphore is held when called.
   */
  if(remap_pfn_range(vma,
		     vma->vm_start,
		     __pa(dum_dev.data)>>PAGE_SHIFT, //physical address of data, shifted. to get physical page number
		     PAGE_SIZE,
		     vma->vm_page_prot))
    return -EAGAIN;
  vma->vm_flags |= VM_LOCKED | (VM_DONTEXPAND | VM_DONTDUMP);
  printk(KERN_INFO DEVICE_NAME ":     mmaped\n");
  return 0;
}

long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
  printk(KERN_WARNING DEVICE_NAME ": [ioctl-ing]\n");
  switch(cmd){
  case DUMMY_SYNC:
    printk(KERN_INFO DEVICE_NAME ":     command 1001\n");
    break;
  default:
    printk(KERN_INFO DEVICE_NAME ":     command %d not valid\n",cmd);
  }
  printk(KERN_INFO DEVICE_NAME ":     ioctled\n");
  return 0;
}

int device_close(struct inode *inode, struct file *filp){
  printk(KERN_WARNING DEVICE_NAME ": [closing]\n");
  if(dum_dev.size>0){
    free_page((unsigned long)dum_dev.data);
    printk(KERN_INFO DEVICE_NAME ":     %ld bytes released from the device\n", dum_dev.size);
    dum_dev.size=0;
  }
  up(&dum_dev.sem);
  printk(KERN_INFO DEVICE_NAME ":     closed\n");
  return 0;
}


static int driver_entry(void){
  printk(KERN_WARNING "\n" DEVICE_NAME ": [[registring]]\n");
  // register the device with the system:
  // (step 1) dynamic allocation of device major number
  // alloc_chardev_region(dev_t*, uint fminor, uint count, char* name)
  ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
  if(ret < 0){
    printk(KERN_ALERT DEVICE_NAME ":     failed to allocate major number\n");
    return ret;   //propagate error
  }
  major_number = MAJOR(dev_num); //extract major number assigned by OS.
  printk(KERN_INFO DEVICE_NAME ":     use \"mknod /dev/" \
	 DEVICE_NAME " c %d 0\" to create device file\n",major_number);

  // (step 2) create the cdev structure
  my_char_dev = cdev_alloc();  // initialize char dev.
  my_char_dev->ops = &fops;    // file operations offered by the driver.
  my_char_dev->owner = THIS_MODULE;

  // (step 3) add the character device to the system
  // int cdev_add(struct cdev* dev, dev_t num, unsigned int count)
  ret = cdev_add(my_char_dev, dev_num, 1);
  if(ret < 0){
    printk(KERN_ALERT DEVICE_NAME ":     failed to add cdev to kernel\n");
    return ret;   //propagate error
  }

  //(step 4) initialize semaphore
  sema_init(&dum_dev.sem,1);   // initial value 1, it can access only one thread at the time.
  printk(KERN_INFO DEVICE_NAME ":     registred\n");
  return 0;
}

static void driver_exit(void){
  printk(KERN_WARNING DEVICE_NAME ": [[removing]]\n");
  // remove char device from system
  cdev_del(my_char_dev);

  // deallocate device major number
  unregister_chrdev_region(dev_num,1);
  printk(KERN_INFO DEVICE_NAME ":     device removed from kernel\n");
}


// macros to define entry and exit functions of the driver
module_init(driver_entry)
module_exit(driver_exit)
MODULE_LICENSE("GPL");
