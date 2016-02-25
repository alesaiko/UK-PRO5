#!/bin/bash

cp -pv .config .config.bkp;
make ARCH=arm CROSS_COMPILE=/home/serg/arm-eabi-4.9-master/bin/arm-eabi- mrproper;
cp -pv .config.bkp .config;
make clean;
rm -rf tmp/*

# clean ccache
read -t 5 -p "clean ccache, 5sec timeout (y/n)?";
if [ "$REPLY" == "y" ]; then
ccache -C;
fi;
