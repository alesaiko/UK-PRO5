#!/system/bin/sh

mount -o remount,rw /system

if [ ! -f /system/xbin/su ]; then
	#make necessary folders
	mkdir /system/etc/init.d
	mkdir /system/app/SuperSU

	#extract SU from ramdisk to correct locations
	rm -rf /system/bin/app_process
	rm -rf /system/bin/install-recovery.sh
	cp /sbin/su/supolicy /system/xbin/
	cp /sbin/su/su /system/xbin/
	cp /sbin/su/libsupol.so /system/lib64/
	cp /sbin/su/install-recovery.sh /system/etc/
	cp /sbin/su/SuperSU.apk /system/app/SuperSU/
	cp /sbin/su/init.sec.boot.sh /system/etc/
#	cp /sbin/su/99SuperSUDaemon /system/etc/init.d/

	#begin supersu install process
	cp /system/xbin/su /system/xbin/daemonsu
	cp /system/xbin/su /system/xbin/sugote
	cp /system/bin/sh /system/xbin/sugote-mksh
	mkdir -p /system/bin/.ext
	cp /system/xbin/su /system/bin/.ext/.su

	cp /system/bin/app_process64 /system/bin/app_process_init
	mv /system/bin/app_process64 /system/bin/app_process64_original

	echo 1 > /system/etc/.installed_su_daemon

	chmod 755 /system/xbin/su
	chmod 755 /system/xbin/daemonsu
	chmod 755 /system/xbin/sugote
	chmod 755 /system/xbin/sugote-mksh
	chmod 755 /system/xbin/supolicy
	chmod 777 /system/bin/.ext
	chmod 755 /system/bin/.ext/.su
	chmod 755 /system/bin/app_process_init
	chmod 755 /system/bin/app_process64_original
	chmod 644 /system/lib64/libsupol.so
	chmod 755 /system/etc/install-recovery.sh
	chmod 644 /system/etc/.installed_su_daemon
	chmod 777 /system/etc/init.d
	chmod 777 /system/etc/init.d/*
	chmod 644 /system/etc/init.sec.boot.sh
	chmod 755 /system/app/SuperSU
	chmod 644 /system/app/SuperSU/SuperSU.apk
	
	ln -s /system/etc/install-recovery.sh /system/bin/install-recovery.sh
	ln -s /system/xbin/daemonsu /system/bin/app_process
	ln -s /system/xbin/daemonsu /system/bin/app_process64

	/system/xbin/su --install
fi

