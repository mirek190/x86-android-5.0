#!/system/bin/sh

# This script is started from watchdog.rc to
# determine if watchdog counter should be clear if
# androidboot.notclearwdogcounter is set in cmdline
notclearcounter=`getprop ro.boot.notclearwdogcounter`

if test "$notclearcounter" != "1"
then
    /sbin/uefivar -n WdtCounter -g 4a67b082-0a4c-41cf-b6c7-440b29bb8c4f -s 0 -t int -k
fi
exit 0
