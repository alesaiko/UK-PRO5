#!/bin/bash +x

# This build script by mcaserg (4PDA)
# For Meizu Pro5 custom kernel

clear
echo
echo

######################################## SETUP #########################################

# set variables
FIT=pro5_defconfig
P=pro5_defconfig_P
DTS=arch/arm64/boot/dts
IMG=arch/arm64/boot
DC=arch/arm64/configs
BK=build_kernel
OUT=output
DT=exynos7420-m86-codegen.dtb

# Cleanup old files from build environment
echo -n "Cleanup build environment.........................."
cd .. #move to source directory
rm -rf $BK/ramdisk/lib/modules/*.*
rm -rf $BK/ramdisk.cpio.gz
rm -rf $BK/Image*
rm -rf $BK/boot*.img
rm -rf $BK/dt*.img
rm -rf $IMG/Image
rm -rf $DTS/.*.tmp
rm -rf $DTS/.*.cmd
rm -rf $DTS/*.dtb
rm -rf $OUT/skrn/*.img
rm -rf $OUT/*.zip
rm -rf $OUT/*.tar
rm -rf .config
echo "Done"

# Set build environment variables
echo -n "Set build variables................................"
export ARCH=arm64
export SUBARCH=arm64
export ccache=ccache
# export CROSS_COMPILE=/home/serg/aarch64-linux-android-master/bin/aarch64-linux-android-;
# export CROSS_COMPILE=/home/serg/aarch64-UBERTC-linux-android-7.0-kernel/bin/aarch64-linux-android-;
export CROSS_COMPILE=/home/serg/aarch64-UBERTC-linux-android-5.3-kernel/bin/aarch64-linux-android-;
export KCONFIG_NOTIMESTAMP=true
echo "Done"
echo

#################################### DEFCONFIG CHECKS #####################################

echo -n "Checking for FIT defconfig........................."
if [ -f "$DC/$FIT" ]; then
	echo "Found - FIT"
else
	echo "Not Found - Reset"
	cp $BK/$FIT $DC/$FIT
fi

# Ask if we want to xconfig or just compile
echo
read -p "Do you want to xconfig or just compile? (x/c)  > " xc
if [ "$xc" = "X" -o "$xc" = "x" ]; then
	echo
	# Make and xconfig our defconfig
	echo -n "Loading xconfig (FIT).............................."
	cp $DC/$FIT .config
	xterm -e make ARCH=arm64 xconfig
	echo "Done"
	mv .config $DC/$FIT
fi
echo

#################################### IMAGE COMPILATION #####################################

echo -n "Compiling Kernel (FIT)............................."
cp $DC/$FIT .config
xterm -e make -j5 ARCH=arm64
if [ -f "arch/arm64/boot/Image" ]; then
	echo "Done"
	# Copy the compiled image to the build_kernel directory
	mv $IMG/Image $BK/Image
else
	clear
	echo
	echo "Compilation failed on FIT kernel !"
	echo
	while true; do
    		read -p "Do you want to run a Make command to check the error?  (y/n) > " yn
    		case $yn in
        		[Yy]* ) make; echo ; exit;;
        		[Nn]* ) echo; exit;;
        	 	* ) echo "Please answer yes or no.";;
    		esac
	done
fi

# compile, depmod, then move all modules to the ramdisk
xterm -e make INSTALL_MOD_PATH=.. modules_install
xterm -e find -name '*.ko' -exec cp -av {} $BK/ramdisk/lib/modules/ \; 

###################################### DT.IMG GENERATION #####################################

echo -n "Build dt.img......................................."

./tools/dtbtool -o $BK/dt.img -s 4096 -p ./scripts/dtc/ $DTS/ | sleep 1
if [ -f "arch/arm64/boot/exynos7420-m86-codegen.dtb" ]; then
	echo "Done"
	# Copy the compiled image to the build_kernel directory
	mv $IMG/exynos7420-m86-codegen.dtb $BK/dt.img
fi
# get rid of the temps in dts directory
rm -rf $DTS/.*.tmp
rm -rf $DTS/.*.cmd
rm -rf $DTS/*.dtb

# Calculate DTS size for all images and display on terminal output
du -k "$BK/dt.img" | cut -f1 >sizT
sizT=$(head -n 1 sizT)
rm -rf sizT
echo "$sizT Kb"

###################################### RAMDISK GENERATION #####################################

echo -n "Make Ramdisk archive..............................."
cd $BK/ramdisk
find .| cpio -o -H newc | lzma > ../ramdisk.cpio.gz

##################################### BOOT.IMG GENERATION #####################################

echo -n "Make boot.img......................................"
cd ..
./mkbootimg --kernel Image --base 0x40078000 --kernel_offset 0x00008000 --ramdisk_offset 0x01f88000 --tags_offset 0xfff88100 --pagesize 4096 --ramdisk ramdisk.cpio.gz -o boot.img
# copy the final boot.img's to output directory ready for zipping
cp boot*.img /home/serg/pro5/$OUT/
echo "Done"

######################################## ZIP GENERATION #######################################

echo -n "Creating flashable zip............................."
cd /home/serg/pro5/$OUT #move to output directory
xterm -e zip -r custom_Kernel.zip *
echo "Done"

###################################### OPTIONAL SOURCE CLEAN ###################################

echo
cd /home/serg/pro5
read -p "Do you want to Clean the source? (y/n) > " mc
if [ "$mc" = "Y" -o "$mc" = "y" ]; then
	xterm -e make clean
	xterm -e make mrproper
fi

############################################# CLEANUP ##########################################

cd /home/serg/pro5/$BK
rm -rf ramdisk.cpio.gz
rm -rf Image*
rm -rf boot*.img
rm -rf dt*.img

echo
echo "Build completed"
echo
#build script ends



