#!/bin/bash

# Use the correct java...
# Uncomment this if you have a macro for setting the 1.6 java environment
# source ~/.bashrc
# aosp

# This script builds all variations/images cleanly.


# Default the -j factor to a bit less than the number of CPUs
if [ -e /proc/cpuinfo ] ; then
    _jobs=`grep -c processor /proc/cpuinfo`
    _jobs=$(($_jobs * 2 * 8 / 10))
elif [ -e /usr/sbin/sysctl ] ; then
    _jobs=`/usr/sbin/sysctl -n hw.ncpu`
    _jobs=$(($_jobs * 2 * 8 / 10))
else
    _jobs=1
    echo "WARNING: Unavailable to determine the number of CPUs, defaulting to ${_jobs} job."
fi

# The full build list that this script knows how to build...
BOARDS=""

# x86 builds also available in AOSP/master and in honeycomb
BOARDS="$BOARDS full_x86"		# QEMU/emulator bootable disk
BOARDS="$BOARDS vbox"			# installer_vdi for virtualbox
BOARDS="$BOARDS android_disk"		# Bootable disk for virtualbox

# ARM builds (keep us honest)
BOARDS="$BOARDS full"

# SDK builds (both arm and x86)
BOARDS="$BOARDS sdk_x86"
BOARDS="$BOARDS sdk"

# List of projects that need scrubbing before each build
DIRTY_LIST="vendor/intel/hardware/PRIVATE/pvr hardware/ti/wlan $\(KERNEL_SRC_DIR\)"

BUILD_TYPE=eng

usage() {
    echo >&1 "Usage: $0 [-c <board_list> ] [ -j <jobs> ] [ -w ] [ -h ] [ -n ] [ -N ]"
    echo >&1 "       -c <board_list>:   defaults to $BOARDS"
    echo >&1 "       -j <jobs>:         defaults to $_jobs"
    echo >&1 "       -t:                Build type. Defaults to ${BUILD_TYPE}"
    echo >&1 "       -w:                Build the Windows version SDK"
    echo >&1 "       -h:                this help message"
    echo >&1 "       -n:                Don't clean up the out/ directory first"
    echo >&1 "       -N:                Don't clean up the (selected) source projects"
    echo >&1 "                          (be careful with this)"
    echo >&1 "List of source projects to clean:"
    echo >&1 "  $DIRTY_LIST"
    echo >&1
    exit 1
}

# Rotate a log file
# If the given log file exist, add a -1 to the end of the file.
# If older log files exist, rename them to -<n+1>
# $1: log file
# $2: maximum version to retain [optional]
rotate_log ()
{
    # Default Maximum versions to retain
    local MAXVER="5"
    local LOGFILE="$1"
    shift;
    if [ ! -z "$1" ] ; then
        local tmpmax="$1"
        shift;
        tmpmax=`expr $tmpmax + 0`
        if [ $tmpmax -lt 1 ] ; then
            panic "Invalid maximum log file versions '$tmpmax' invalid; defaulting to $MAXVER"
        else
            MAXVER=$tmpmax;
        fi
    fi

    # Do Nothing if the log file does not exist
    if [ ! -f ${LOGFILE} ] ; then
        return
    fi

    # Rename existing older versions
    ver=$MAXVER
    while [ $ver -ge 1 ]
    do
        local prev=`expr $ver - 1`
        local old="-$prev"

        # Instead of old version 0; use the original filename
        if [ $ver -eq 1 ] ; then
            old=""
        fi

        if [ -f "${LOGFILE}${old}" ] ; then
            mv -f ${LOGFILE}${old} ${LOGFILE}-${ver}
        fi

        ver=$prev
    done
}


# Verify java version.
java_version=`javac -version 2>&1 | head -1`
case "$java_version" in
*1.6* )
    ;;
* )
    echo >&1 "Java 1.6 must be used, version $java_version found."
    exit 1
    ;;
esac

while getopts t:snwNhj:c: opt
do
    case "${opt}" in
    t)
        BUILD_TYPE=${OPTARG}
        ;;
    c)
        BOARDS="${OPTARG}"
        ;;
    j)
        if [ ${OPTARG} -gt 0 ]; then
            _jobs=${OPTARG}
        fi
        ;;
    w )
        chk_host_os=`uname -s`
        if [ ${chk_host_os} == "Linux" ]; then
            build_windows_sdk=1
        else
            echo >&1 "Error: Build Windows SDK option only valid under Linux"
            exit 1
        fi
        ;;
    n )
        dont_clean=1
        ;;
    N )
        dont_clean_dirty=1
        ;;
    s )
        SHOW="showcommands"
        ;;
    ? | h)
        usage
        ;;
    * )
        echo >&1 "Unhandled option ${opt}"
        usage
        ;;
    esac
done
shift $(($OPTIND-1))

if [ $# -ne 0 ]; then
    usage
fi

if [ ! -f build/envsetup.sh ]; then
    echo >&2 "Execute $0 from the top of your tree"
    exit 1
fi

# Start clean
if [ -z "$dont_clean" ]; then
    echo "Removing previous 'out' directory..."
    rm -rf out
fi

# Add the Windows SDK to the Board list
# if the option is enable
if [ -n "$build_windows_sdk" ]; then
    BOARDS=`echo $BOARDS | sed -e 's/\(sdk\|sdk_x86\)/& win_&/g'`
fi

echo "Building with j-factor of $_jobs"
echo "Building for boards: $BOARDS"
echo

TODAY=`date '+%Y%m%d'`
SDK_REL_DIR=sdk-release-$TODAY

source build/envsetup.sh
for i in $BOARDS; do
  rotate_log "$i.log"
  echo Building $i ....

  if [ -d .repo -a -z "$dont_clean_dirty" ]; then
    for _dirty in $DIRTY_LIST
    do
      z=`repo forall $_dirty -c git clean -d -f -x | wc -l`
      echo "Cleaned: $z files from the $_dirty directory"
    done
  fi

  # Get information about this Build Host
  HOST_OS=`get_build_var HOST_OS`
  HOST_ARCH=`get_build_var HOST_ARCH`

  case "$i" in
  sdk )
    target=sdk
    lunch=sdk
    ;;

  win_sdk )
    target=win_sdk
    lunch=sdk
    ;;

  sdk_x86 )
    target=sdk
    lunch=sdk_x86
    ;;

  win_sdk_x86 )
    target=win_sdk
    lunch=sdk_x86
    ;;

  full )
    target="droid"
    lunch=full
    ;;

  vbox | vbox_x86 )
    target="installer_vdi"
    lunch=vbox_x86
    ;;

  android_disk | android_disk_x86 )
    target="android_disk_vdi"
    lunch=vbox_x86
    ;;

  full_x86 )
    target="droid"
    lunch=full_x86
    ;;
  
  * )
    echo >&2 "Target unknown. Guessing with target=\"droid $i\", lunch=\"$target\""
    target="droid $i"
    lunch=$target
    ;;
  esac

  lunch $lunch-$BUILD_TYPE
  time make -j$_jobs $target $SHOW > $i.log 2>&1
  rc=$?
  if [ "$rc" -ne 0 ]; then
    echo -e >&2 "Error: $rc returned from build\n\n"
    echo -e >>$i.log "\nError: $rc returned from build"
  fi


  case "$i" in
  sdk | sdk_x86 )
    mkdir -p $SDK_REL_DIR/$i
    if [ ${HOST_OS} == "darwin" ]; then
        _host_os="mac"
    else
        _host_os=${HOST_OS}
    fi
    bash -c -x "cp out/host/${HOST_OS}-${HOST_ARCH}/sdk/android-sdk_eng.${LOGNAME}_${_host_os}-${HOST_ARCH}.zip $SDK_REL_DIR/$i/" >> $i.log 2>&1
    ;;

  win_sdk | win_sdk_x86 )
    mkdir -p $SDK_REL_DIR/$i
    bash -c -x "cp out/host/windows/sdk/android-sdk_eng.${LOGNAME}_windows.zip $SDK_REL_DIR/$i/" >> $i.log 2>&1
    ;;

  full | full_x86)
    # We also build libm.a - required for making an NDK
    mmm bionic/libm >> $i.log 2>&1
    ;;
  esac
done
