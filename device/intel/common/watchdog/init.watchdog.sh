#!/system/bin/sh

# This script is started from watchdog.rc to
# determine if watchdog counter should be clear if
# androidboot.notclearwdogcounter is set in cmdline
notclearcounter=`getprop ro.boot.notclearwdogcounter`

if test "$notclearcounter" != "1"
then
    echo "1" > /sys/firmware/osnib/clean_osnib
fi
exit 0
