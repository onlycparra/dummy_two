#!/bin/bash
# -*- ENCODING: utf-8 -*-

device="dummy_two"
group="staff"
mode="777"

rm /dev/${device}
rmmod ${device}

make clean
make
rtn_make=$?
if [ $rtn_make -ne 0 ]
then
    echo -e "\nCompilation not successful, exiting"
    exit
fi

insmod ${device}.ko
major="$(cat /proc/devices | grep ${device} | awk -F ' ' '{print $1}')"
mknod /dev/${device} c ${major} 0

chgrp $group /dev/${device}
chmod $mode /dev/${device}

exit
