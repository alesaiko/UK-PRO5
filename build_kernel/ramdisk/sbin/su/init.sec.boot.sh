#!/system/bin/sh

echo "init.sec.boot.sh: start" > /dev/kmsg

# start deferred initcalls
cat /proc/deferred_initcalls

## strace for system_server
#str=""
#while [ "$str" = "" ]; do
#  str=`ps | grep system_server`
#  sleep 0.1
#done
#
#pid=${str:10:4}
#echo "init.sec.boot.sh: strace -tt -T -o /data/log/strace.txt -p ${pid}" > /dev/kmsg
#strace -tt -T -o /data/log/strace.txt -p ${pid}

mount -t rootfs -o remount,rw rootfs

if [ -d /system/etc/init.d ]; then
  run-parts /system/etc/init.d
fi

mount -t rootfs -o remount,ro rootfs
