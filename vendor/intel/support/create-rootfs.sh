#!/bin/bash

#  Initialize defaults
#
TRUE=1
FALSE=0
BOOT_TARBALL=boot.tar.gz
SYSTEM_TARBALL=system.tar.gz

# Volume Label
BOOT=boot
MEDIA=media
EXTENDED=extended
SYSTEM=system
DATA=data
CACHE=cache
RECOVERY=recovery

#Filesystem Type
# none
# ext2
# ext3
# vfat

# target device partitions: fastboot compatible
# FORMAT :
#     [Array Index]=Volume Label,Partition Number,Filesystem Type
PARTITIONS=(\
        [0]=${BOOT},1,ext3 \
        [1]=${DATA},2,ext3 \
        [2]=${MEDIA},3,vfat \
        [3]=${EXTENDED},4,none \
        [4]=${RECOVERY},5,ext3 \
        [5]=${SYSTEM},6,ext3 \
        [6]=${CACHE},7,ext3 \
        )

# Some devices have a numerical suffix such as /dev/sdbN
# Some have a alpha numerical suffix such as /dev/mmcblk0pN
# Others could even have a suffix such as /dev/disk/c0d0pN
# These are special cases... 
# $1: The device name (no suffix) such as /dev/sdb
partition_suffix()
{
    case "$1" in
    /dev/sd* | /dev/hd* )
        # Nothing to echo
        ;;
    /dev/mmcblk* )
        echo "p"
        ;;
    esac
}

mnt_manual() {
    local mnt_base=/mnt

    echo "Manually mounting..."

    for p in ${PARTITIONS[@]}; do
        set -- $(echo $p | tr , \ )
        if [ ${3} != none ]; then
            if [ ! -e ${mnt_base}/${1} ]; then
                mkdir -p ${mnt_base}/${1}
            fi
            echo "  Mounting ${mnt_base}/${1}..."
            mount -t ${3} ${blkdev}${suffix}${2} ${mnt_base}/${1}
        fi
    done
}

mnt() {
    local secs=0
    local maxsecs=15
    local mcnt=0
    local npart_num=0

    echo "Verifying that $blkdev partitions are mounted"
    for p in ${PARTITIONS[@]}; do
        set -- $(echo ${p} | tr , \ )
        if [[ ${3} != none ]]; then
            ((npart_num++))
        fi
    done

    mcnt=$(mount | grep ${blkdev} | wc -l)

    while [ ${secs} -lt ${maxsecs} ]; do
        if [ ${mcnt} -lt ${npart_num} ]; then
            ((secs++))
            sleep 1
        else
            break
        fi
    done

    if [ ${mcnt} -lt ${npart_num} ]; then
        mnt_manual
    fi
}

# We make two passes unmounting... just to be sure
umnt() {
    echo "Unmounting partitions on $blkdev..."
    for i in 1 2 3; do
        sync
        for p in $(mount | grep ${blkdev} | cut -f 3 -d ' '); do
            echo "  Unmounting ${p}..."
            umount -f ${p}
        done
    done
    if [ "`mount | grep ${blkdev}`" ]; then
        echo "ERROR: Unable to umount:"
        mount | grep ${blkdev}
        exit 1
    fi
}

fmt() {
    echo "Formatting partitions on $blkdev..."
    for p in ${PARTITIONS[@]}; do
        set -- $(echo ${p} | tr , \ )
        if [[ ${3} != none ]]; then
            echo "  Making ${3} file system on ${blkdev}${suffix}${2} with name ${1}..."
            case ${3} in
                ext2|ext3)
                    mkfs.${3} -L ${1} ${blkdev}${suffix}${2}   > /dev/null 2>&1
                    ;;
                vfat)
                    mkfs.${3} -F 32 -n ${1} ${blkdev}${suffix}${2}  > /dev/null 2>&1
                    ;;
                *)
                    set -- $(echo $p | tr , \ )
                    echo "File system ${3} currently not supported by script - exiting"
                    exit 1 ;;
            esac
        elif [[ ${1} = ${EXTENDED} ]]; then
                echo "  Passed ${3} file system on ${blkdev}${suffix}${2} with name ${1}..."
        fi
    done
}

partition() {
  # wipe out the MBR and primary partition table
    echo "  Clear the first 512 bytes of the partition(${blkdev}) to zero"
    dd if=/dev/zero of=${blkdev} count=1 bs=512 >/dev/null 2>&1

    echo "  Creating partitions on $blkdev..."

#        Device Boot      Start         End      Blocks   Id  System
#/dev/mmcblk0p1   *           1        3073       98328   83  Linux
#/dev/mmcblk0p2            6147        9219       98336   83  Linux
#/dev/mmcblk0p3            3074        6146       98336   83  Linux
#/dev/mmcblk0p4            9220      121008     3577248    5  Extended
#/dev/mmcblk0p5            9220       13316      131096   83  Linux
#/dev/mmcblk0p6           13317       21509      262168   83  Linux
#/dev/mmcblk0p7           21510      121008     3183960    c  W95 FAT32 (LBA)

    fdisk ${blkdev} >/dev/null 2>&1 <<EOF
o
n
p
1

+96M
n
p
2

+96M
n
p
3

+1024M
n
e


n
l

+128M
n
l

+512M
n
l


t
7
c
a
1
w
EOF

  umnt

  echo "  Verfying partitions on ${blkdev}..."
  # the following fails on the MoviNand so manually check if the partitions are mounted
  sfdisk -V -q ${blkdev}
  if [ $? -eq 1 ]; then
       echo "Partioning of ${blkdev} failed"
      exit 1
  fi
}

usage() {
    echo "Usage: $0 [-m host ] [ -b <boot.tar.gz> ] [ -s <system.tar.gz> ] -u DISK"
    echo " -h : show this message"
    echo " -m : DISK refers to a directory on the host rather than a device"
    echo " -u : Select DISK or Directory"
    echo " -b : boot tarball (default $BOOT_TARBALL)"
    echo " -s : system tarball (default $SYSTEM_TARBALL)"
    echo ""
    echo "DISK is something like /dev/hdc, /dev/sdd or /dev/mmcblk0."
}

partition_create() {
    local blkdev=${1}

    if [ ! -e ${blkdev} ]; then
        echo "The device ${blkdev} does not exist"
        exit 0
    fi

    MOUNTED=($(mount | grep ${blkdev}))
    if [ "${MOUNTED[2]}" == "/" ]; then
        echo "The device ${blkdev} is rootdev of your system"
        exit 1
    fi

    umnt
    partition
}

partition_format() {
    local blkdev=${1}

    umnt
    fmt
}

partition_mount() {
    local blkdev=${1}

    umnt
    mnt_manual
    mnt
}

# copy_package_to ${OPT_U_HOSTDISK} ${blkdev} <package>...
copy_package_to() {
    local opt_u_hostdisk="${1}"
    local blkdev="${2}"
    local copy_base
    local package
    local origin
    shift; shift

    if [[ ${opt_u_hostdisk} == TRUE ]]; then
        copy_base=${blkdev}
    else
        copy_base=`df | grep ${blkdev} | head -1 | awk '{print $NF}' | sed -e 's,/[^/]*$,,'`
    fi

    for package in $@; do
        echo "Copying files from ${package} to ${copy_base}..."
        tar -x -C ${copy_base} -f ${package}
    done
}

create_dir() {
    local top_dir=${1}

    for p in ${PARTITIONS[@]}; do
    set -- $(echo $p | tr , \ )
    if [ ${3} != none ]; then
        if [ ! -e ${top_dir}/${1} ]; then
        mkdir -p ${top_dir}/${1}
        fi
    fi
    done
}

check_mode()
{
    case ${1} in
    media)
        ;;
    host)
        ;;
    *)
        echo "Unknown mode (expected host or media)"
        usage
        exit 1
        ;;
    esac
}

check_path()
{
    if [ -b ${1} ]; then
        return
    elif [ -d ${1} ]; then
        return
    else
        echo "ERROR: '${1}' does not exist"
        echo ""
        usage
        exit 1;
    fi
}


main() {
    local origin
    local answer

    # block device disk
    local blkdev=

    # the DISK device for autoboot
    local autoboot_mode=sdcard

    # MOVE ANDROID IMAGES TO HOST COMPUTER'S DISK, NOT SD DISK.
    local OPT_U_HOSTDISK=FALSE

    local no_args=TRUE

    while getopts s:b:hm:u: opt
    do
        case ${opt} in
            h)
                usage
                exit 0
                ;;
            m)
                no_args=FALSE
                check_mode ${OPTARG}
                if [[ ${OPTARG} == host ]]; then
                    OPT_U_HOSTDISK=TRUE
                fi
                ;;
            u)
                no_args=FALSE
                check_path ${OPTARG}
                blkdev=${OPTARG}
                suffix=`partition_suffix $blkdev`
                ;;
            s)
                SYSTEM_TARBALL=${OPTARG}
                ;;
            b)
                BOOT_TARBALL=${OPTARG}
                ;;
            ?)
                echo "Unknown option"
                exit 1
                ;;
        esac
    done

    if [[ ${no_args} == TRUE ]]; then
        usage
        exit 1
    fi

    if [[ ! -e $BOOT_TARBALL ]]; then
        echo "ERROR: $BOOT_TARBALL does not exist"
        exit 2
    fi

    if [[ $EUID -ne 0 ]]; then
        echo "need to root permission"
        exit 3
    fi

    # Verify the tarball origin for the packages
    for package in ${BOOT_TARBALL} ${SYSTEM_TARBALL}; do
        origin="`tar tf ${package} | head -1`"
        case "`tar tf ${package} | head -1`" in
        boot/ | ./boot/ | system/ | ./system/ )
            ;;
        * )
            echo "ERROR: ${package} does not have a proper origin ($origin)"
            exit 1
            ;;
        esac
    done

    while [ 1 = 1 ]; do
        echo "This operation will destroy all data on ${blkdev}."
        echo -n "Continue (y/[n])? "
        read -n 1 answer
        [ ! -z ${answer} ] && echo -e -n "\n"
        [ "${answer:-n}" = "n" ] && exit 1
        [ "${answer}" = "y" ] && break
    done

    if [[ ${OPT_U_HOSTDISK} == FALSE ]]; then
        partition_create ${blkdev}
        partition_format ${blkdev}
        partition_mount ${blkdev}
    fi

    copy_package_to ${OPT_U_HOSTDISK} ${blkdev} ${BOOT_TARBALL} ${SYSTEM_TARBALL}

    if [[ ${OPT_U_HOSTDISK} == FALSE ]]; then
        umnt
        sync
    fi
}

main $*
