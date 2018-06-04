obj-m := dummy_two.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.mod.* *.symvers *.order .*.ko.cmd .*.o.cmd *~
#	rm *.ko
