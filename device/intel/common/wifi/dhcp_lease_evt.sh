#!system/bin/sh

# Arguments.
# $1 is action (add, del, old)
# $2 is MAC
# $3 is address
# $4 is hostname

#if [ "${1}" = "del" ] ; then
# Currently it is not used
#fi

if [ "${1}" = "add" ] ; then
          am broadcast -a  android.net.wifi.WIFI_AP_STA_TETHER_CONNECT --es "device_address" "${2}" --es "host_name" "${4}" --es "ip_address" "${3}"
fi

if [ "${1}" = "old" ] ; then
          am broadcast -a  android.net.wifi.WIFI_AP_STA_TETHER_CONNECT --es "device_address" "${2}" --es "host_name" "${4}" --es "ip_address" "${3}"
fi
