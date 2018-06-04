// definition of the opcode for sync
// more info about ioctl: https://opensourceforu.com/2011/08/io-control-in-linux/

#ifndef DUM_DRIV_IOCTL_H
#define DUM_DRIV_IOCTL_H

#include <linux/ioctl.h>
//DUMMY_SYNC 1001
#define DUMMY_SYNC _IO('q', 1001)

#endif
