#!/system/bin/sh

# This script is started from /init.rc.
# Put any "late initialization" in here.
# Sleeps are ok... but don't put anything in this file that
# could possibly go into init.<platform>.rc

# Bring up Ethernet (over USB) interface, for testing/debugging.
case `getprop ro.debuggable` in
1)
    # if property net.eth0.ip is defined, use it as ip address for eth0
    # else use dhcp
    use_static=Y
    addr=`getprop net.eth0.ip`
    netmask=`getprop net.eth0.netmask`

    if [ -z "$addr" -o -f /storage/sdcard0/use_dhcp ]; then
        use_static=N
    fi

    case $use_static in
    Y)
        ifconfig eth0 $addr netmask $netmask up
        ;;
    N)
        # Bring up eth0 using dhcpcd as DHCP client in background with
        # debug log. This allows the shell to exit and configures
        # ethernet interface whenever ethernet cable is plugged in.
        /system/bin/dhcpcd -bd eth0
        ;;
    esac # use_static
;;
esac # ro.debuggable

exit 0
