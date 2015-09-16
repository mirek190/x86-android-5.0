#!/system/bin/sh -e
# Copy ${file}(in internal SD card), so sensor HAL can access it
# Create an empty file if ${file} is not existed

# MUST BE the same as DISABLE_SENSOR_LIST_FILE in scalability/PlatformConfig.cpp
dest_file="/data/system/disable_sensors.txt"

file="/data/media/0/disable_sensors.txt" # file in internal SD card
log_file="/logs/disable_sensors.log"

echo "Try to copy ${file} to ${dest_file}" > ${log_file}

if [ -f "${file}" ]
then
	echo "Found ${file}" >> ${log_file} 
	cp ${file} ${dest_file}
else
	echo "No found ${file}" >> ${log_file}
	rm -f ${dest_file}
	touch ${dest_file}

	## disable proximity sensor by default
	# echo "JSA1212 Digital Proximity Sensor" > ${dest_file}
fi
chmod 0664 ${dest_file}
