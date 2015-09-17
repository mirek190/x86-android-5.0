#!/bin/bash

set -e

# Some distributions put tools like mkdosfs in sbin, which isn't
# always in the default PATH
PATH="${PATH}:/sbin:/usr/sbin"

# Process command line arguments
while [ $# -gt 0 ]; do
    case $1 in
        --syslinux)
            SYSLINUX="$2"
            shift
            ;;

        --output)
            OUTPUT="$2"
            shift
            ;;

        --tmpdir)
            TMPDIR="$2"
            shift
            ;;

        --config)
            CONFIG="$2"
            shift
            ;;

        --extra-size)
            EXTRA_SIZE="$2"
            shift
            ;;

        --help)
            echo "Usage: $0 OPTIONS <files to copy>"
            echo "Create a FAT32 image that contains everything necessary"
            echo "to be the boot file system."
            echo "The following options exist:"
	        echo "  --syslinux <path>               Path to SYSLINUX installer"
            echo "  --tmpdir <directory>            Location for assembling image files"
            echo "                                  does not need to currently exist and will be deleted"
            echo "  --config <file>                 File to use as syslinux.cfg"
            echo "  --output <output>               The filename of the output image"
            echo "  --extra-size <size>             Custom extra size of the of the output image"
            echo
            exit 0
            ;;
	    *)
            if [ ${1:0:2} = "--" ]; then
                echo "Unknown option $1"
                exit 1
            else
                COPYFILE="$COPYFILE $1"
            fi
            ;;
    esac
    shift
done

# test for presence of required arguments
for i in SYSLINUX CONFIG OUTPUT TMPDIR; do
    if [ -z "${!i}" ]; then
        echo "Missing $i parameter!"
        exit 1
    fi
done

# test validity of files supplied
for i in SYSLINUX CONFIG COPYFILE; do
    for j in ${!i}; do
        if ! [ -f "$j" ]; then
            echo "Missing or invalid $i file: $j"
            exit 1
        fi
    done
done

# check for mkdosfs,mcopy in PATH
for i in mkdosfs mcopy; do
    if ! which "$i" &> /dev/null; then
        echo "Missing required program $i"
        exit 1
    fi
done

rm -rf $TMPDIR
mkdir -p $TMPDIR

cp -f $COPYFILE $TMPDIR
cp -f $CONFIG $TMPDIR/syslinux.cfg

boot_size=`du -sk $TMPDIR | tail -n1 | awk '{print $1;}'`

# add 1% extra space, minimum 512K
extra=$(($boot_size / 100))
reserve=512
[ $extra -lt $reserve ] && extra=$reserve

# Add on top of slack space 32K for ldloader.sys
boot_size=$(($boot_size + $extra + 32))

# Add a custom slack space for the platforms that need it.
boot_size=$(($boot_size + ${EXTRA_SIZE:-0}))

# Round the size of the disk up to 16K so that total sectors is
# a multiple of sectors per track (mtools complains otherwise)
# XXX at what size threshold does sectors/track stop being 32??
mod=$((boot_size % 16))
if [ $mod != 0 ]; then
    boot_size=$(($boot_size + 16 - $mod))
fi

rm -f $OUTPUT
echo "Create FAT filesystem of size $boot_size"
mkdosfs -n BOOT -v -C $OUTPUT $boot_size

for f in $TMPDIR/*; do
    echo "Copying $f"
    mcopy -i $OUTPUT $f ::$(basename $f)
done

echo "Installing syslinux"
$SYSLINUX $OUTPUT

