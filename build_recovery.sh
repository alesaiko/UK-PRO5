#!/bin/bash
export KERNELDIR=`readlink -f .`
export RAMFS_SOURCE=`readlink -f $KERNELDIR/recovery`

echo "kerneldir = $KERNELDIR"
echo "ramfs_source = $RAMFS_SOURCE"

RAMFS_TMP="/tmp/arter97-exynos7420-recovery"

echo "ramfs_tmp = $RAMFS_TMP"
cd $KERNELDIR

echo "Building new ramdisk"
#remove previous ramfs files
rm -rf '$RAMFS_TMP'*
rm -rf $RAMFS_TMP
rm -rf $RAMFS_TMP.cpio
#copy ramfs files to tmp directory
cp -ax $RAMFS_SOURCE $RAMFS_TMP
cd $RAMFS_TMP

find . -name '*.sh' -exec chmod 755 {} \;

$KERNELDIR/ramdisk_fix_permissions.sh 2>/dev/null

#clear git repositories in ramfs
find . -name .git -exec rm -rf {} \;
find . -name EMPTY_DIRECTORY -exec rm -rf {} \;
cd $KERNELDIR
rm -rf $RAMFS_TMP/tmp/*

cd $RAMFS_TMP
find .| cpio -o -H newc | lzma > $RAMFS_TMP.cpio.gz
ls -lh $RAMFS_TMP.cpio.gz
cd $KERNELDIR

echo "Making new boot image"
gcc -w -s -pipe -O2 -Itools/libmincrypt -o tools/mkbootimg/mkbootimg tools/libmincrypt/*.c tools/mkbootimg/mkbootimg.c
tools/mkbootimg/mkbootimg --kernel $KERNELDIR/arch/arm64/boot/Image --ramdisk $RAMFS_TMP.cpio.gz --base 0x40078000 --pagesize 4096 --ramdisk_offset 0x01f88000 --tags_offset 0xfff88100 --kernel_offset 0x00008000 -o $KERNELDIR/recovery.img
if echo "$@" | grep -q "CC=\$(CROSS_COMPILE)gcc" ; then
	dd if=/dev/zero bs=$((29360128-$(stat -c %s recovery.img))) count=1 >> recovery.img
fi

echo "done"
ls -al recovery.img
echo ""
