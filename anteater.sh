#!/bin/bash
# -*- ENCODING: utf-8 -*-

module="anteater"
device="anteater"
mode="664"
major=60
group="staff"

# run log monitor
gnome-terminal -- dmesg -wH / &

#compile driver
make

# invoke insmod with all arguments we got and use a pathname,
# as newer modutils don't look in . by default
/sbin/insmod ./${module}.ko $* || exit 1

# remove stale nodes
rm -f /dev/${device}0


exit

# create file in /dev/
mknod /dev/${device}0 c $major 0

# Not all distributions have group staff, some have "wheel" instead.
grep -q '^staff:' /etc/group || group="wheel"

# give appropriate group/permissions, and change the group.
chgrp $group /dev/${device}0
chmod $mode /dev/${device}0
