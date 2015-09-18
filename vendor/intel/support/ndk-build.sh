#!/bin/bash

#
# This script is used internally to re-build the toolchains and NDK.
# This is an engineer's tool... definitely not a production tool. Don't expect it to be
# flawelss... at least at this revision.

# Don't use it blindly and then commit the result... look at the files that have been updated
# and consider carefully what changes really need to be made to the ndk/ and prebuilt/ projects.
#
# You'll want to edit this scripot to your needs. Change the VERSIONs and the PRODUCT as required.
# You may want to remote the 'repo init' for the toolchain... at least after you've pulled the
# toolchain the first time.

export SDK=$PWD
export TOP=$SDK
export TODAY=`date '+%Y%m%d'`
export BUILD_OUT=/tmp/Toolchain.$LOGNAME.$TODAY
export PLATFORM=android-9
export PACKAGE_DIR=$SDK/ndk-release-$TODAY
export ANDROID_NDK_ROOT=$SDK/ndk
export PATH=$PATH:$ANDROID_NDK_ROOT/build/tools
# export VERBOSE=--verbose

# If BUILD_NUM_CPUS is not already defined in your environment,
# define it as the double of HOST_NUM_CPUS. This is used to
# run make commands in parallel, as in 'make -j$BUILD_NUM_CPUS'
#
if [ -z "$BUILD_NUM_CPUS" ] ; then
    HOST_NUM_CPUS=`cat /proc/cpuinfo | grep -c processor`
    export BUILD_NUM_CPUS=`expr $HOST_NUM_CPUS`
fi

# CLEAN
rm -rf $BUILD_OUT
rm -rf /tmp/ndk-release
rm -rf /tmp/ndk-release-*
rm -rf /tmp/ndk-toolchain/*
rm -rf /tmp/android-toolchain*
rm -rf /tmp/android-ndk-*

for ARCH in arm x86
do
    case "$ARCH" in
    arm )
        PRODUCT=generic
        SYSROOT=$SDK/ndk/platforms/$PLATFORM/arch-$ARCH
        BUILD_TOOLCHAIN=prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi
        TOOLCHAIN_VERSION=4.4.3
        TOOLCHAIN=arm-linux-androideabi-4.4.3

        unset MPFR_VERSION
        unset GDB_VERSION
        unset BINUTILS_VERSION
        unset TOOLCHAIN_NAME
        ;;
    x86 )
        PRODUCT=generic_$ARCH
        SYSROOT=$SDK/ndk/platforms/$PLATFORM/arch-$ARCH
        BUILD_TOOLCHAIN=prebuilt/linux-x86/toolchain/i686-android-linux-4.4.3/bin/i686-android-linux
        # BUILD_TOOLCHAIN=prebuilt/linux-x86/toolchain/i686-unknown-linux-gnu-4.2.1/bin/i686-unknown-linux-gnu

        TOOLCHAIN_VERSION=4.4.3
        TOOLCHAIN=$ARCH-$TOOLCHAIN_VERSION

        MPFR_VERSION="--mpfr-version=2.4.1"
        GDB_VERSION="--gdb-version=6.6"
        # BINUTILS_VERSION="--binutils-version=2.21.51"
        BINUTILS_VERSION="--binutils-version=2.20.1"
        TOOLCHAIN_NAME=i686-android-linux-$TOOLCHAIN_VERSION
        ;;
    esac

    export ANDROID_PRODUCT_OUT=$SDK/out/target/product/$PRODUCT
    if [ ! -d $ANDROID_PRODUCT_OUT ]; then
        echo >&2 Rebuild for $PRODUCT first... or change PRODUCT in $0.
        exit 1
    fi

    echo
    echo "$ARCH: $PLATFORM build/tools/build-ndk-sysroot.sh (ensures that libthread_db.so and other libraries are up to date)"
    cd $SDK/ndk
    build/tools/build-ndk-sysroot.sh \
          $VERBOSE \
          --build-out=$SDK/out/target/product/$PRODUCT \
          --platform=$PLATFORM \
          --package \
          --abi=$ARCH > $SDK/$ARCH.build-ndk-sysroot.log 2>&1
    if [ $? -ne 0 ]; then
        echo >&2 "build/tools/build-ndk-sysroot.sh failed. Logfile in $SDK/$ARCH.build-ndk-sysroot.log"
        exit 1
    fi

    echo
    echo "$ARCH: Copy files from ndk/build/platforms to development/ndk/platforms."
    cd $SDK
    ndk/build/tools/dev-system-import.sh \
        --toolchain-prefix=$BUILD_TOOLCHAIN \
        --arch=$ARCH 3 4 5 6 7 8 9 > $SDK/$ARCH.dev-system-import.log 2>&1
    if [ $? -ne 0 ]; then
        echo >&2 "ndk/build/tools/dev-system-import.sh failed. Logfile in $SDK/$ARCH.dev-system-import.log"
        exit 1
    fi

    echo
    echo "$ARCH: Build the ndk/platforms directory"
    ndk/build/tools/build-platforms.sh \
        $VERBOSE \
        --abi=$ARCH --no-symlinks --no-samples > $SDK/$ARCH.build-platforms.log 2>&1
    if [ $? -ne 0 ]; then
        echo >&2 "ndk/build/tools/build-platforms.sh failed. Logfile in $SDK/$ARCH.build-platforms.log"
        exit 1
    fi

    echo
    echo "$ARCH: Rebuilding the toolchain and prebuilt tarballs"
    cd $SDK
    ndk/build/tools/rebuild-all-prebuilt.sh \
        --sysroot=$SDK/ndk/platforms/$PLATFORM/arch-$ARCH \
        --arch=$ARCH \
        --package-dir=$PACKAGE_DIR \
        $MPFR_VERSION $GDB_VERSION $BINUTILS_VERSION \
        $VERBOSE > $SDK/$ARCH.rebuild-all.log 2>&1
    if [ $? -ne 0 ]; then
        echo >&2 "ndk/build/tools/rebuild-all-prebuilt.sh failed. Logfile in $SDK/$ARCH.rebuild-all.log"
        echo >&2 "Ignoring the error and continuing"
        # exit 1
    fi

    echo
    echo "$ARCH: Building the NDK release"
    cd $SDK/ndk
    build/tools/package-release.sh \
        --toolchains=$TOOLCHAIN \
        --prebuilt-dir=/tmp/ndk-prebuilt/prebuilt-$TODAY/ \
        --prebuilt-dir=$PACKAGE_DIR \
        --systems="linux-x86" \
        --out-dir=$PACKAGE_DIR \
        --abi=$ARCH \
        --prefix=android-ndk-$ARCH \
        --no-git > $SDK/$ARCH.release-package.log 2>&1
    if [ $? -ne 0 ]; then
        echo >&2 "ndk/build/tools/package-release.sh failed. Logfile in $SDK/$ARCH.release-package.log"
        exit 1
    fi
done


exit 0
##
## Note, you can install the new toolchain by something like:
    TODAY=`date '+%Y%m%d'`
    NDK_RELEASE="ndk-release-$TODAY"
    mkdir -p ~/AOSP/prebuilt/linux-x86/toolchain/i686-android-linux-4.4.3
    cd ~/AOSP/prebuilt/linux-x86/toolchain/i686-android-linux-4.4.3
    tar -x --strip-components=4 -f ~/AOSP/$NDK_RELEASE/x86-4.4.3-linux-x86.tar.bz2

    mkdir -p ~/AOSP/prebuilt/android-x86/gdbserver
    cd ~/AOSP/prebuilt/android-x86/gdbserver
    tar -x --strip-components=3 -f ~/AOSP/$NDK_RELEASE/x86-4.4.3-gdbserver.tar.bz2
