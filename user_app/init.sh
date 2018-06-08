#!/bin/bash
# -*- ENCODING: utf-8 -*-

rm ./test
echo "Compiling..."
gcc test.c -Wall -o test
rtn_gcc=$?
if [ $rtn_gcc -ne 0 ]
then
    echo -e "\nCompilation not successful, error ${rtn_gcc}. Exiting"
    exit
fi

./test

exit
