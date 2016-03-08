#!/sbin/busybox sh

BB=/sbin/busybox;

mount -o remount,rw /
mount -o remount,rw /system /system

#
# Setup for Cron Task
#
if [ ! -d /data/.SkyHigh ]; then
	$BB mkdir -p /data/.SkyHigh
	$BB chmod -R 0777 /.SkyHigh/
fi;
# Copy Cron files
$BB cp -a /res/crontab/ /data/
if [ ! -e /data/crontab/custom_jobs ]; then
	$BB touch /data/crontab/custom_jobs;
	$BB chmod 777 /data/crontab/custom_jobs;
fi;


#
# Stop Google Service and restart it on boot (dorimanx)
# This removes high CPU load and ram leak!
#
if [ "$($BB pidof com.google.android.gms | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms);
fi;
if [ "$($BB pidof com.google.android.gms.unstable | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms.unstable);
fi;
if [ "$($BB pidof com.google.android.gms.persistent | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms.persistent);
fi;
if [ "$($BB pidof com.google.android.gms.wearable | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms.wearable);
fi;

# Tweak interactive
# A53 Cluster
sleep 30
echo "19000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay
echo "800" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/multi_enter_load
echo "80000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/multi_enter_time
echo "0" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/boost
echo "" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/boostpulse
echo "40000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/boostpulse_duration
echo "85" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load
echo "1000000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq
echo "0" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy
echo "360" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/multi_exit_load
echo "320000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/multi_exit_time
echo "400000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/multi_cluster0_min_freq
echo "200" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/single_enter_load
echo "160000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/single_enter_time
echo "40000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time
echo "75" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads
echo "20000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate
echo "20000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_slack
echo "90" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/single_exit_load
echo "80000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/single_exit_time
echo "400000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/single_cluster0_min_freq

# A57 Cluster
echo "59000 1300000:39000 1700000:19000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay
echo "360" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/multi_enter_load
echo "99000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/multi_enter_time
echo "0" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/boost
echo "" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/boostpulse
echo "40000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/boostpulse_duration
echo "89" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load
echo "1200000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq
echo "0" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy
echo "240" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/multi_exit_load
echo "299000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/multi_exit_time
echo "800000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/multi_cluster0_min_freq
echo "95" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/single_enter_load
echo "199000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/single_enter_time
echo "40000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time
echo "65 1500000:75" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads
echo "20000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate
echo "20000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_slack
echo "60" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/single_exit_load
echo "99000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/single_exit_time
echo "800000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/single_cluster0_min_freq

#
# Fast e/Random Generator (frandom) support on boot
#
chmod 444 /dev/erandom
chmod 444 /dev/frandom

#
# Allow untrusted apps to read from debugfs (mitigate SELinux denials)
#
/system/xbin/supolicy --live \
	"allow untrusted_app debugfs file { open read getattr }" \
	"allow untrusted_app sysfs_lowmemorykiller file { open read getattr }" \
	"allow untrusted_app sysfs_devices_system_iosched file { open read getattr }" \
	"allow untrusted_app persist_file dir { open read getattr }" \
	"allow debuggerd gpu_device chr_file { open read getattr }" \
	"allow netd netd capability fsetid" \
	"allow netd { hostapd dnsmasq } process fork" \
	"allow { system_app shell } dalvikcache_data_file file write" \
	"allow { zygote mediaserver bootanim appdomain }  theme_data_file dir { search r_file_perms r_dir_perms }" \
	"allow { zygote mediaserver bootanim appdomain }  theme_data_file file { r_file_perms r_dir_perms }" \
	"allow system_server { rootfs resourcecache_data_file } dir { open read write getattr add_name setattr create remove_name rmdir unlink link }" \
	"allow system_server resourcecache_data_file file { open read write getattr add_name setattr create remove_name unlink link }" \
	"allow system_server dex2oat_exec file rx_file_perms" \
	"allow mediaserver mediaserver_tmpfs file execute" \
	"allow drmserver theme_data_file file r_file_perms" \
	"allow zygote system_file file write" \
	"allow atfwd property_socket sock_file write" \
	"allow untrusted_app sysfs_display file { open read write getattr add_name setattr remove_name }" \
	"allow debuggerd app_data_file dir search" \
	"allow sensors diag_device chr_file { read write open ioctl }" \
	"allow sensors sensors capability net_raw" \
	"allow init kernel security setenforce" \
	"allow netmgrd netmgrd netlink_xfrm_socket nlmsg_write" \
	"allow netmgrd netmgrd socket { read write open ioctl }"

#
# Synapse
#
if [ "$($BB mount | grep rootfs | cut -c 26-27 | grep -c ro)" -eq "1" ]; then
	$BB mount -o remount,rw /;
fi;
if [ "$($BB mount | grep system | grep -c ro)" -eq "1" ]; then
	$BB mount -o remount,rw /system;
fi;

$BB chmod -R 755 /res/*
ln -s /res/synapse/uci /sbin/uci
/sbin/uci

if [ "$($BB mount | grep rootfs | cut -c 26-27 | grep -c ro)" -eq "1" ]; then
	$BB mount -o remount,rw /;
fi;
if [ "$($BB mount | grep system | grep -c ro)" -eq "1" ]; then
	$BB mount -o remount,rw /system;
fi;

#
# Init.d
#
if [ ! -d /system/etc/init.d ]; then
	mkdir -p /system/etc/init.d/;
	chown -R root.root /system/etc/init.d;
	chmod 777 /system/etc/init.d/;
	chmod 777 /system/etc/init.d/*;
fi;

$BB run-parts /system/etc/init.d


if [ "$($BB mount | grep rootfs | cut -c 26-27 | grep -c ro)" -eq "1" ]; then
	$BB mount -o remount,rw /;
fi;
if [ "$($BB mount | grep system | grep -c ro)" -eq "1" ]; then
	$BB mount -o remount,rw /system;
fi;
mkdir /system/su.d
chmod 0700 /system/su.d


$BB mount -t rootfs -o remount,rw rootfs
$BB mount -o remount,rw /system /system
$BB mount -o remount,rw /system
$BB mount -o remount,rw /data
