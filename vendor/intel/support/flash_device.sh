#!/bin/bash
#
# This script will use the fastboot command to:
#  o  partition your SDCARD or NAND
#  o  format the file systems on the various partitions
#  o  flash the contents of the boot and system file systems

_error=0
_boot=sd
_cmd=`basename $0`


# Exit the script on failure of a command
function exit_on_failure {
    if [ "$_debug" ]; then
        set -x
    fi
    $@
    rv=$?
    if [ $rv -ne 0 ]; then
        exit $rv
    fi
}

# Exit the script on failure of a command
function warning_on_failure {
    if [ "$_debug" ]; then
        set -x
    fi
    $@
    rv=$?
    if [ $rv -ne 0 ]; then
        echo >&2 "\"$@\" exited with error: $rv"
    fi
}


# Fastboot doesn't return a non-zero exit value when a command fails.
# We have to test stderr for the error.
function do_fastboot {
    if [ "$_debug" ]; then
        set -x
    fi
    _error=0
    _rv=`fastboot $@ 2>&1 | tee /dev/tty`

    case "$_rv" in
    *OKAY* )
        # Success
        ;;
    *FAILED* )
        _error=1
        ;;
    * )
        echo >&2 "$_cmd: Unknown response from fastboot"
        _error=1
        ;;
    esac

    return $_error
}

function usage {
    echo "Usage: $_cmd [-d nand|sd|usb] [-p] [-c <dir>] [-n] [-b]"
    echo "       -d: select boot device. (default $_boot)"
    echo "       -c: find images in <dir>"
    echo "       -p: don't remake the partition table or erase the /media partition"
    echo "       -a: auto-select the image directory"
    echo "       -n: don't continue the boot after flashing the image"
    echo "       -b: flash boot only"
}

function main {
    while getopts bnapc:xd: opt
    do
        case "${opt}" in
        p )
            _preserve_media=1
            ;;
        c )
            if [ ! -d "${OPTARG}" ]; then
                _error=1
            fi
            cd ${OPTARG}
            ;;
        d )
            _boot=${OPTARG}
            ;;
        a )
            _auto_select=1
            ;;
        n )
            _dont_reboot=1
            ;;
        b )
            _bootflash=1
            ;;
        x )
            _debug=1
            set -x
            ;;
        * )
            _error=1
        esac
    done

    case "$_boot" in
    sd )
        _device=/dev/mmcblk0
        _boot_gz=boot_sdcard.tar.gz
        _system_gz=system.tar.gz
        ;;
    nand )
        _device=/dev/nda
        _boot_gz=boot_nand.tar.gz
        _system_gz=system.tar.gz
        ;;
    usb )
        _device=/dev/sda
        _boot_gz=boot_harddisk.tar.gz
        _system_gz=system.tar.gz
        ;;
    * )
        _error=1
        ;;
    esac

    if [ $_error -ne 0 ]; then
        usage
        exit 1;
    fi

    if [ -z "$_auto_select" -a -d out -a ! -f $_system_gz ]; then
        _auto_select=1
        echo >&2 "Auto Selecting Fastboot Directory"
    fi

    if [ "$_auto_select" ]
    then
        _product=`fastboot getvar product 2>&1 | tail -1 |
            sed 's/<.*>//
                 s/product://
                 s/[[:blank:]]//g'`
        _product_dir="out/target/product/$_product"
        if [ ! -d "$_product_dir" ]; then
            echo >&2 $_product_dir not found.
            exit 1
        fi
        echo >&2 Using $_product_dir
        cd $_product_dir
    fi
    if [ ! -f "$_boot_gz" ]; then
        echo -n >&2 "$_cmd: Can not find $_boot_gz. "
        _boot_gz=boot.img
        echo >&2 "Trying $_boot_gz. "
    fi

    if [ ! -r "$_boot_gz" ]; then
        echo >&2 "$_cmd: Can not find $_boot_gz."
        _error=1
    fi
    if [ ! -r "$_system_gz" ]; then
        echo >&2 "$_cmd: Can not find $_system_gz."
        _error=1
    fi

    if [ "$_error" -ne 0 ]; then
        exit 1
    fi

    echo -n "Setting bootdev to $_boot. "
    exit_on_failure do_fastboot oem bootdev $_boot

    echo -n "Setting tarball_origin to root. "
    exit_on_failure do_fastboot oem tarball_origin root

    if [ -z "$_preserve_media" ]; then
        echo -n /sbin/PartitionDisk.sh $_device
        exit_on_failure do_fastboot oem system /sbin/PartitionDisk.sh $_device

        if [ -z "$_bootflash" ]; then
            exit_on_failure do_fastboot erase sdcard
        fi
    fi

    if [ -z "$_bootflash" ]; then
        exit_on_failure do_fastboot erase recovery
        exit_on_failure do_fastboot erase cache
        exit_on_failure do_fastboot erase data
        exit_on_failure do_fastboot erase factory
        warning_on_failure do_fastboot erase config
        exit_on_failure do_fastboot erase system
        # We need to kill the syslogd and umount ilog partition first.
        # Otherwise we will get a warning here.
        warning_on_failure do_fastboot erase ilog

        echo "Flashing system image: $_system_gz"
        exit_on_failure do_fastboot flash system $_system_gz
    fi

    echo "Flashing boot image: $_boot_gz"
    exit_on_failure do_fastboot flash boot $_boot_gz

    echo -n "Syncing storage devices"
    exit_on_failure do_fastboot oem system sync

    if [ -z "$_dont_reboot" ]; then
        exit_on_failure do_fastboot continue
    fi
}

main $@
