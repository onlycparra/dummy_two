#define FOR_OPTEE 1  //for conditional compilation
#define DEVICE_NAME "dummy_two"
#define CALL_SIZE 3 //number of registers to read/write at the time

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>        //struct file_operations
#include <linux/cdev.h>      //makes char devices availables: struct cdev, cdev_add()
#include <linux/semaphore.h> //semaphores to coordinate syncronization https://tuxthink.blogspot.com/2011/05/using-semaphores-in-linux.html
#include <linux/uaccess.h>   //copy_to_user, copy_from_user
#include <linux/slab.h>      //kmalloc, kfree
#include <linux/mm.h>        //vm_area_struct, PAGE_SIZE
#include <linux/ioctl.h>     //_IO, _IOR, _IOW

#if FOR_OPTEE
#include <linux/arm-smccc.h>
#include "chardev_smc.h"        //SMC call definition
#endif


//#include "../include/ioctl_commands.h" //DUMMY_SYNC
//struct for ioctl queries
struct io_t{
  unsigned long* data;
  unsigned long size;
};

#define DUMMY_SYNC _IO('q', 1001)
#define DUMMY_WRITE _IOW('q', 1002, struct io_t*)
#define DUMMY_READ  _IOW('q', 1003, struct io_t*)


//struct for device
struct Dummy_device{
  unsigned long* data;
  unsigned long size;
  struct semaphore sem;
};


//variables (declared here to not use the small available kernel stack).
#if FOR_OPTEE
struct arm_smccc_res res;       //hold returned values from smc call
#endif
struct Dummy_device dum_dev;
struct cdev *my_char_dev; // struct that will allow us to register the char device.
int major_number, ret; // will hold the device major number. used for returns.
dev_t dev_num; // stuct that stores the device number given by the OS.

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
int device_open(struct inode *inode, struct file *filp){
  printk(KERN_WARNING DEVICE_NAME ": [opening]\n");
  ret = down_interruptible(&dum_dev.sem);
  if(ret != 0){
    printk(KERN_ALERT DEVICE_NAME ":     could not lock device during open\n");
    return ret;
  }
#if FOR_OPTEE
  arm_smccc_smc(OPTEE_SMC_OPEN_DUMMY, 0, 0, 0, 0, 0, 0, 0, &res);
  if(res.a0 != OPTEE_SMC_RETURN_OK){
    printk(KERN_INFO DEVICE_NAME ":     OPTEE_SMC_RETURN: failure\n");
    return -1;
  }
  printk(KERN_INFO DEVICE_NAME ":     OPTEE_SMC_RETURN: ok\n");
  if(res.a1 != OPTEE_SMC_OPEN_DUMMY_SUCCESS){
    printk(KERN_INFO DEVICE_NAME ":     OPTEE_SMC_OPEN_DUMMY: failure\n");
    return -1;
  }
#endif
  dum_dev.size=0;
  dum_dev.data = (unsigned long*) get_zeroed_page(GFP_KERNEL);
  if(!dum_dev.data){
    printk(KERN_ALERT DEVICE_NAME ":     could not allocate page\n");
    return -1;
  }
  dum_dev.size=PAGE_SIZE;
  printk(KERN_INFO DEVICE_NAME ":     kernel %ld bytes allocated for the device\n", dum_dev.size);
  printk(KERN_INFO DEVICE_NAME ":     kernel opened\n");
  return 0;
}


int __low_write(struct io_t *mem){
  int callsize;
  int sul = sizeof(unsigned long);
  char args[sul*CALL_SIZE];
  int i,j,k,index = 0;
  unsigned long size_l = mem->size%sul == 0 ? mem->size/sul : mem->size/sul+1;
  unsigned long calls  = size_l%CALL_SIZE==0 ? size_l/CALL_SIZE : size_l/CALL_SIZE+1;
  printk(KERN_INFO DEVICE_NAME ":     call size: %d\n",CALL_SIZE);
  printk(KERN_INFO DEVICE_NAME ":     chars    : %ld\n",mem->size);
  printk(KERN_INFO DEVICE_NAME ":     uns longs: %ld\n",size_l);
  printk(KERN_INFO DEVICE_NAME ":     calls    : %ld\n", calls);
  for(i=0; i<calls; ++i){
    for(j=0; j<CALL_SIZE; ++j){
      for(k=0; k<sul; ++k){
	index = j*sul+k;
        /*args[index] = i*CALL_SIZE*sul+index<mem->size ?
	  (char) ((char*)(mem->data)) [i*CALL_SIZE*sul+index] :
	  0;
	*/
	if(i*CALL_SIZE*sul+index<mem->size){
		args[index] = (char) ((char*)(mem->data)) [i*CALL_SIZE*sul+index];
		callsize = index;
	}else{
		args[index] = 0;
	}
	//printk(KERN_INFO DEVICE_NAME ":     args[%2d] = mem->data[%2d] = %c\n", index, i*CALL_SIZE+index, args[index]);
      }
    }

#if FOR_OPTEE
    printk(KERN_INFO DEVICE_NAME ":     arm_smccc_smc(OPTEE_SMC_WRITE_DUMMY,\n");

    arm_smccc_smc(OPTEE_SMC_WRITE_DUMMY, ((unsigned long*)args)[0], ((unsigned long*)args)[1], ((unsigned long*)args)[2], ((unsigned long*)args)[3], 0, 0, 0, &res);
    if(res.a0 != OPTEE_SMC_RETURN_OK){
        printk(KERN_INFO DEVICE_NAME ":     OPTEE_SMC_WRITE_DUMMY: SMC failure\n");
        return -1;
    }else{
	printk(KERN_INFO DEVICE_NAME ":     OPTEE_SMC_WRITE_DUMMY OK\n");
    }

    if(res.a1 != OPTEE_SMC_OPEN_DUMMY_SUCCESS){
        printk(KERN_INFO DEVICE_NAME ":     OPTEE_SMC_WRITE_DUMMY: failure - writing thread\n");
        return -1;
    } 

    /*
    arm_smccc_smc(
      OPTEE_SMC_WRITE_DUMMY,
      args[0],
      args[1],
      args[2],
      args[3],
      args[4],
      args[5],
      args[6],
      &res
    );
    if(res->a2!=OK){
      return -1;
    }
    */
#else
    printk(KERN_INFO DEVICE_NAME ":     arm_smccc_smc(OPTEE_SMC_WRITE_DUMMY,\n");
    for(j=0; j<CALL_SIZE; ++j){
      for(k=0; k<sul; ++k){
	if(args[j*8+k]!=0){
	  printk(KERN_INFO DEVICE_NAME ":       %c\n",args[j*8+k]);
	}else{
	  printk(KERN_INFO DEVICE_NAME ":       Ø\n");
	}
      }
      printk(KERN_INFO DEVICE_NAME ":        \n");
    }
    printk(KERN_INFO DEVICE_NAME ":       &res\n");
    printk(KERN_INFO DEVICE_NAME ":     );\n");
#endif
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
  if(copy_to_user(userBuffer,dum_dev.data,bufCount)){
    printk(KERN_INFO DEVICE_NAME ":     error in copy_to_user\n");
      return -EACCES;
  }
  printk(KERN_INFO DEVICE_NAME ":     read\n");
  return ret;
}

ssize_t device_write(struct file* filp, const char* userBuffer, size_t bufCount, loff_t* curOffset){
  struct io_t mem;
  printk(KERN_WARNING DEVICE_NAME ": [writing]\n");
  mem.size = bufCount;
  if(mem.size > dum_dev.size){
    mem.size = dum_dev.size;
    printk(KERN_INFO DEVICE_NAME ":     data to write truncated to %ld\n",dum_dev.size);
  }
  mem.data = kmalloc(mem.size,GFP_KERNEL);
  memset(mem.data,0,mem.size);
  // copy_from_user(to,from,size)
  if(copy_from_user(mem.data,userBuffer,mem.size)){
    printk(KERN_INFO DEVICE_NAME ":     error in copy_from_user()\n");
    return -EACCES;
  }

  if(__low_write(&mem)){
    printk(KERN_INFO DEVICE_NAME ":     error in __low_write()\n");
    return -1;
  }
  
  printk(KERN_INFO DEVICE_NAME ":     %ld chars written into device\n",mem.size);
  printk(KERN_INFO DEVICE_NAME ":     written\n");
  return 0;
}


//mmap method, maps allocated memory into userspace
int device_mmap(struct file *filp, struct vm_area_struct *vma){
  printk(KERN_WARNING DEVICE_NAME ": [mmaping]\n");
  /**
   * remap_pfn_range - remap kernel memory to userspace
   * int remap_pfn_range(struct vm_area_struct* vma, // user vma to map to 
                         unsigned long addr,         // target user address to start at
   *                     unsigned long pfn,          // physical address of kernel memory
                         unsigned long size,         // size of map area
                         pgprot_t prot);             // page protection flags for this mapping
   *  Note: this is only safe if the mm semaphore is held when called.
   */
  if(remap_pfn_range(vma,
		     vma->vm_start,
		     __pa(dum_dev.data)>>PAGE_SHIFT, //physical address of data. shifted to get physical page number
		     PAGE_SIZE,
		     vma->vm_page_prot))
    return -EAGAIN;
  vma->vm_flags |= VM_LOCKED | (VM_DONTEXPAND | VM_DONTDUMP);
  printk(KERN_INFO DEVICE_NAME ":     mmaped\n");
  return 0;
}

long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
  struct io_t mem;
  unsigned long* udata; //user space pointer to data
  printk(KERN_WARNING DEVICE_NAME ": [ioctl-ing]\n");
  switch(cmd){


  case DUMMY_SYNC:
    printk(KERN_INFO DEVICE_NAME ":     command DUMMY_SYNC\n");
    break;


  case DUMMY_WRITE:
    printk(KERN_INFO DEVICE_NAME ":     command DUMMY_WRITE\n");
    if(copy_from_user(&mem,(struct io_t*)arg, sizeof(struct io_t))){
      printk(KERN_INFO DEVICE_NAME ":     error in copy_from_user(struct)\n");
      return -EACCES;
    }

    if(mem.size > dum_dev.size){
      mem.size = dum_dev.size;
      printk(KERN_INFO DEVICE_NAME ":     data to write truncated to %ld\n",dum_dev.size);
    }

    udata = mem.data;
    mem.data = kmalloc(mem.size,GFP_KERNEL);
    memset(mem.data,0,mem.size);
    
    if(copy_from_user(mem.data,udata,mem.size)){
      printk(KERN_INFO DEVICE_NAME ":     error in copy_from_user(data)\n");
      return -EACCES;
    }

    if(__low_write(&mem)){
      printk(KERN_INFO DEVICE_NAME ":     error in __low_write()\n");
      return -1;
    }
    break;

  case DUMMY_READ:
    printk(KERN_INFO DEVICE_NAME ":     command DUMMY_READ\n");
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
