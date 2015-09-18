This project contains a user-space acceleration library which can be implemented for multiple platforms.
Currently Android, CSim CSS 1.5 and CSim CSS 2.0 are supported, a windows implementation is to be added.

CSS folders inclucded in this package are based on ci_master_byt_20130823_2200 tag
with additional patch from https://git-ger-3.devtools.intel.com/gerrit/#change,4395,patchset=9

====== Prerequisites =========
ISP SDK: set the following variables, example assumes SDK is installed in $HOME/opt/hive
Next 3 settings are required for CSim
export HIVEBIN=$HOME/opt/hive/bin
export HIVECORES=$HIVEBIN/../cores
export HIVESYSTEMS=$HIVEBIN/../systems

Android tree: to build the host code for Android, we use the build environment from an android
tree. This example has the android tree installed in $HOME/android:
export ANDROID_BUILD_TOP=$HOME/android
export TOP=$ANDROID_BUILD_TOP

Other environment variables.
We need HIVE_CVSWORK to point to the directory that contains acc_test/, ia_acc/
Example:
export HIVE_CVSWORK=$ANDROID_BUILD_TOP/vendor/intel/tools/isp_acc

To compile ISP binaries for execution with the CSS acceleration API, you need an acceleration package.
The package is found via the following environment variable.
It should contain build/ lib/ sched/ crun/
Example:
export HIVE_ACC_PKG=$HIVE_CVSWORK/css_acc_pkg_2400A0

====== Building this package on CSim ===========
To build the contents of this package for CSim:
cd $HIVE_CVSWORK/
cd acc_test
# default: SYSTEM=hive_isp_css_2400A0_system
# for diff system, make sched SYSTEM=hive_isp_css_2400_system
make crun SYSTEM=hive_isp_css_2400A0_system
make sched SYSTEM=hive_isp_css_2400A0_system

====== Building this package on Android ===========
go to your android source tree, run source build/envsetup.sh and lunch (with the proper lunch target).
cd $ANDROID_BUILD_TOP
source $ANDROID_BUILD_TOP/build/envsetup.sh
lunch saltbay_pr1-eng (Example only) 
cd $HIVE_CVSWORK/ia_acc; mm
cd ../acc_test
# compile the ISP binary:
make android SYSTEM=hive_isp_css_2400A0_system
# compile the host code for Android:
mm

================ Testing on phone ==============
cd $HIVE_CVSWORK/acc_test/hive_isp_css_2400A0_system_target
# make sure /sdcard/isp/ and /data/local/tmp/ folders exist on phone
adb push add.bin /sdcard/isp/add.bin
adb push add /data/local/tmp/
adb push host /data/local/tmp/
cd $ANDROID_BUILD_TOP/out/target/product/saltbay/system/lib/hw
adb push add_test /data/local/tmp/
adb shell
/data/local/tmp/add_test

