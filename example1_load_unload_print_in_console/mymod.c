#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#define MAX_PARAMS 64

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Claudio A. Parra");

//my array of parameters
int args[MAX_PARAMS];
int argc=0;
/*
register parameters permissions
module_param_array(array_name,type,parameters_count,permissions);
  availabe permissions:
    S_IRUSR read  user
    S_IWUSR write user
    S_IXUSR execu user
    S_IRGRP read  group
    S_IWGRP write group

    combinations: S_Ixxxx | S_Iyyyy
*/
module_param_array(args,int,&argc,S_IRUSR|S_IWUSR);




void show_params(void){
  int i;
  printk("There are %d parameters:\n",argc);
  for(i=0;i<argc; ++i){
    printk(KERN_INFO "  args[%d] = %d\n", i, args[i]);
  }
}

static int hello_init(void){
  printk(KERN_ALERT "my module is alive and running, inside \"%s\".\n", __FUNCTION__);
  show_params();
  return 0;
}

static void hello_exit(void){
  printk(KERN_ALERT "byebye module\n");
}

module_init(hello_init);
module_exit(hello_exit);
