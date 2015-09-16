oem write_osip_header
flash:boot#/installer/boot.img
flash:recovery#/installer/recovery.img
flash:fastboot#/installer/droidboot.img
oem start_partitioning
oem partition /installer/partition.tbl
erase:factory
erase:cache
erase:system
erase:config
erase:logs
erase:misc
erase:data
oem stop_partitioning
flash:system#/installer/system.img.gz
continue
