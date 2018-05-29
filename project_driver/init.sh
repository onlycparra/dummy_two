#!/bin/bash
# -*- ENCODING: utf-8 -*-

device="dummy_two"
group="staff"
mode="777"

rm /dev/${device}
rmmod ${device}

make clean
make
insmod ${device}.ko
major="$(cat /proc/devices | grep ${device} | awk -F ' ' '{print $1}')"
mknod /dev/${device} c ${major} 0

chgrp $group /dev/${device}
chmod $mode /dev/${device}
