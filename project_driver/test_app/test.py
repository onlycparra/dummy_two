#!/usr/bin/python3

import os

input("Open File. press [Enter]")
dev=os.open("/dev/dummy_two",os.O_RDWR)

input("Read all from the device. press [Enter]")
data = os.read(dev,16)
print(data)

input("Write in device. press [Enter]")
str = "here we have 21 chars, and here 33"
message = str.encode("utf-8")
os.write(dev,message)
#os.lseek(dev,0,0)

input("*Read 21 chars from the device. press [Enter]")
data = os.read(dev,21)
print(message)
#os.lseek(dev,0,os.SEEK_SET)

input("Read all from the device. press [Enter]")
data = os.read(dev,999)
print(data)

input("Close device. press [Enter]")
os.close(dev)

input("All done. press [Enter]")
