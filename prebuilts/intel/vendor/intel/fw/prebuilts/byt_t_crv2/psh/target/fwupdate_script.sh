#!/system/bin/sh -e

sfile="/data/fwupdate.flag"
lfile="/data/pshfwupdate.log"
fwfile=$1

echo > $lfile

exec 1>>$lfile
exec 2>>$lfile

set -x
if [ $2 == force ]; then
	 needupdate=1

elif [ -f $sfile ]; then
	echo "file" $sfile

	read flag value < $sfile
	echo "flag" $flag
	echo "value" $value

	if [ "$flag" == "update" ]; then
		if [ "$value" == "done" ]; then
			needupdate=0
		else
			needupdate=1
		fi
	else
		needupdate=1
	fi
else
	echo "file not exist"
	needupdate=1
fi

if [ $needupdate -eq 1 ]; then

	echo 0 > /sys/class/gpio/gpio59/value
	echo 1 > /sys/class/gpio/gpio95/value
	echo 0 > /sys/class/gpio/gpio95/value
	echo 1 > /sys/class/gpio/gpio95/value

	sleep 1

	echo "update firmware start"

	/system/bin/fwupdatetool -f $fwfile

	if [ $? -eq 0 ]; then
		echo "update firmware success"
		updatesuccess=1
	else
		echo "update firmware failed"
		updatesuccess=0
	fi

	echo 1 > /sys/class/gpio/gpio59/value
	echo 1 > /sys/class/gpio/gpio95/value
	echo 0 > /sys/class/gpio/gpio95/value
	echo 1 > /sys/class/gpio/gpio95/value

	sleep 1

	if [ -f $sfile ]; then
		rm $sfile
	fi

	if [ $updatesuccess -eq 1 ]; then
		echo "update done" > $sfile
	else
		echo "update failed" > $sfile
	fi

fi

exit 0
