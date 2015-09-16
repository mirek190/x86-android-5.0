#!/system/bin/sh

prop=`getprop "persist.service.apklogfs.enable"`
if [ "$prop" -eq 1 ]
then
    /sbin/logcat -b system -b events -b main -b radio -n 20 -r5000 -v threadtime -d -f /logs/aplog
fi
setprop "sys.realpowerctl" "1"
