#!/system/bin/sh

# This script is started from /init.rc.
# Put any "late initialization" in here.
# Sleeps are ok... but don't put anything in this file that
# could possibly go into init.<platform>.rc

lid=`getprop init.panel_ignore_lid`
[ "$lid" != "" ] && echo $lid > /sys/module/i915/parameters/panel_ignore_lid

for dirname in /sys/class/scsi_host/host* ; do
        if [ -e $dirname/link_power_management_policy ]
        then
                echo min_power > $dirname/link_power_management_policy
        fi
done

# Used for UFO graphics driver; harmless if some other driver is used
if test ! -f /data/ufo.prop; then
    ln -s /system/etc/ufo.prop /data/ufo.prop
    chmod 644 /data/ufo.prop
fi

exit 0
