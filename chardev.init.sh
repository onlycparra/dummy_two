#!/bin/bash
# -*- ENCODING: utf-8 -*-

module="chardev"
device="chardev"
mode="664"
group="staff"

# run log monitor
gnome-terminal -- dmesg -wH / &

#compile driver
make clean
make



# remove previous module, (visible in /proc/devices)
sudo rmmod ${module}

# insert module
sudo insmod ./${module}.ko || exit 1

# remove possible old nodes
sudo rm -f /dev/${device}



# get from the /proc/devices the major number assigned by the kernel at the module init time
major="$(cat /proc/devices | grep ${device} | awk -F ' ' '{print $1}')"

# create new node in /dev/
sudo mknod /dev/${device} c $major 0



# Not all distributions have group staff, some have "wheel" instead.
grep -q '^staff:' /etc/group || group="wheel"

# give appropriate group/permissions, and change the group.
sudo chgrp $group /dev/${device}
sudo chmod $mode /dev/${device}



# compiling user space program
gcc -Wall program.c -o program

# accessing the driver
sudo ./program
