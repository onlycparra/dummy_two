obj-m += chardev.o

h_dir = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(h_dir) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.mod *.symvers *.order *.mod.c
