#!/bin/sh
# reserved space of 256M to handle OSIP entries
# The rest for fat partition
# usage: copy flashfile content to partition
# droidboot looks for installer.cmd file

if [ $# -eq 3 ]; then
    disk=$1
    blankphone_zip=$2
    fastboot_zip=$3
else
    echo "Usage: $0 block_device blankphone.zip fastboot.zip"
    exit
fi

if [ ! -b /dev/${disk} ]; then
    echo "$disk is not a valid block device... Aborting"
    exit
fi

#Unmount all the listed partitions under "disk" device name
mount | grep ${disk} | cut -d' ' -f1 | xargs umount

unzip ${blankphone_zip} droidboot.img -d /tmp

dd if=/tmp/droidboot.img of=/dev/${disk} bs=1M

#Create one big fat partition
sfdisk /dev/${disk} -uM << EOF
256,,83
EOF
mkfs.vfat /dev/${disk}1
mount /dev/${disk}1 /mnt
echo "Unzip contents to USB installer"
mv /tmp/droidboot.img /mnt/
unzip ${blankphone_zip} boot.* recovery.img partition.tbl installer.cmd -d /mnt
unzip ${fastboot_zip} system.img -d /mnt
sync
umount /mnt

