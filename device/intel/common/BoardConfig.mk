COMMON_PATH := device/intel/common
SUPPORT_PATH := vendor/intel/support

TARGET_ARCH := x86
TARGET_ARCH_VARIANT := x86-atom
TARGET_CPU_VARIANT := x86
TARGET_CPU_ABI := x86
TARGET_CPU_SMP := true
TARGET_NO_BOOTLOADER := false
TARGET_NO_RADIOIMAGE := true
TARGET_NO_RECOVERY := false
TARGET_PRELINK_MODULE := false
TARGET_PROVIDES_INIT_RC := true
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_RIL_DISABLE_STATUS_POLLING := true
TARGET_BOARD_KERNEL_HEADERS := $(COMMON_PATH)/kernel-headers
KERNEL_SRC_DIR ?= linux/kernel

# enable ARM codegen for x86 with Houdini
BUILD_ARM_FOR_X86 := true

BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648

# PRODUCT_OUT and HOST_OUT are now defined after BoardConfig is included.
# Add early definition here
PRODUCT_OUT ?= out/target/product/$(TARGET_DEVICE)
HOST_OUT ?= out/host/$(HOST_OS)-$(HOST_PREBUILT_ARCH)

# Customization of BOOTCLASSPATH and init.environ.rc
PRODUCT_BOOT_JARS += com.intel.config com.intel.multidisplay
ifeq ($(strip $(INTEL_FEATURE_AWARESERVICE)),true)
PRODUCT_BOOT_JARS += com.intel.aware.awareservice
endif
ifeq ($(strip $(DOLBY_DAP)),true)
PRODUCT_BOOT_JARS += dolby_ds
endif

# Appends path to ARM libs for Houdini
PRODUCT_LIBRARY_PATH := $(PRODUCT_LIBRARY_PATH):/system/lib/arm

# By default, signing is performed using ISU (Intel Signing Utility).  Can be
# overridden on specific target BoardConfig.mk.  Currently supported values for
# the signing method are 'xfstk', 'xfstk_no_xml', 'isu', 'isu_plat2'.
TARGET_OS_SIGNING_METHOD ?= isu

FLASHFILE_NO_OTA := true
INTEL_INGREDIENTS_VERSIONS := true
INTEL_CAMERA := false
INTEL_TEST_CAMERA := true
INTEL_USB_SUPERSPEED_ADB := true

BOARD_GPFLAG := 0x80000045

#TARGET_USE_PRIVATE_LIBM := true

ifneq ($(wildcard vendor/intel/PRIVATE/cert/testkey*),)
PRODUCT_DEFAULT_DEV_CERTIFICATE := vendor/intel/PRIVATE/cert/testkey
endif

### See upstream patch:
### https://android-review.googlesource.com/#/c/64639/
### Define in bsp until merged
# Macro to add include directories of modules in pathmap_INCL
# relative to root of source tree. Usage:
# $(call add-path-map, project1:path1)
# OR
# $(call add-path-map, \
#        project1:path1 \
#        project2:path1)
#
define add-path-map
$(eval pathmap_INCL += \
    $(foreach path, $(1), \
        $(if $(filter $(firstword $(subst :, ,$(path))):%, $(pathmap_INCL)), \
            $(error Duplicate AOSP path map $(path)), \
            $(path) \
         ) \
     ) \
 )
endef

#Extend the AOSP path includes
$(call add-path-map, stlport:external/stlport/stlport \
        alsa-lib:external/alsa-lib/include \
        libxml2:external/libxml2/include \
        webcore-icu:external/webkit/Source/WebCore/icu \
        tinyalsa:external/tinyalsa/include \
        core-jni:frameworks/base/core/jni \
        vss:frameworks/av/libvideoeditor/vss/inc \
        vss-common:frameworks/av/libvideoeditor/vss/common/inc \
        vss-mcs:frameworks/av/libvideoeditor/vss/mcs/inc \
        vss-stagefrightshells:frameworks/av/libvideoeditor/vss/stagefrightshells/inc \
        lvpp:frameworks/av/libvideoeditor/lvpp \
        osal:frameworks/av/libvideoeditor/osal/inc \
        frameworks-base-core:frameworks/base/core/jni \
        frameworks-av:frameworks/av/include \
        frameworks-openmax:frameworks/native/include/media/openmax \
        jpeg:external/jpeg \
        skia:external/skia/include \
        sqlite:external/sqlite/dist \
        opencv-cv:external/opencv/cv/include \
        opencv-cxcore:external/opencv/cxcore/include \
        opencv-ml:external/opencv/ml/include \
        libstagefright:frameworks/av/media/libstagefright/include \
        libstagefright-rtsp:frameworks/av/media/libstagefright/rtsp \
        libmediaplayerservice:frameworks/av/media/libmediaplayerservice \
        gtest:external/gtest/include \
        frameworks-base-libs:frameworks/base/libs \
        frameworks-av-services:frameworks/av/services \
        tinycompress:external/tinycompress/include \
        libnfc-nci:external/libnfc-nci/src/include \
        libnfc-nci_hal:external/libnfc-nci/src/hal/include \
        libnfc-nci_nfc:external/libnfc-nci/src/nfc/include \
        libnfc-nci_nfa:external/libnfc-nci/src/nfa/include \
        libnfc-nci_gki:external/libnfc-nci/src/gki \
        libc-private:bionic/libc/private \
        icu4c-common:external/icu/icu4c/source/common \
        expat-lib:external/expat/lib \
        libvpx:external/libvpx \
        protobuf:external/protobuf/src \
        zlib:external/zlib \
        openssl:external/openssl/include \
        libnl-headers:external/libnl-headers \
        system-security:system/security/keystore/include/keystore \
        libpcap:external/libpcap \
        libsensorhub:vendor/intel/hardware/libsensorhub/src/include \
        libsensorhub_ish:vendor/intel/hardware/libsensorhub/src_ish/include \
        icu4c-i18n:external/icu/icu4c/source/i18n \
        bt-bluez:system/bluetooth/bluez-clean-headers \
        astl:external/astl/include \
        libusb:external/libusb/libusb \
        libc-kernel:bionic/libc/kernel \
        libc-x86:bionic/libc/arch-x86/include \
        strace:external/strace \
        bionic:bionic \
        opengl:frameworks/native/opengl/include \
        libstagefright-wifi-display:frameworks/av/media/libstagefright/wifi-display \
        libffi:external/libffi/include \
        libffi-x86:external/libffi/linux-x86)

#Platform
#Enable display driver debug interface for eng and userdebug builds
ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
export DISPLAY_DRIVER_DEBUG_INTERFACE=true
endif

# Enable build-time pre-optimization for userdebug
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
WITH_DEXPREOPT := true
endif

# Enabling logs into file system for eng and user debug builds
ifeq ($(PRODUCT_MANUFACTURER),intel)
ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
ADDITIONAL_DEFAULT_PROPERTIES += persist.service.apklogfs.enable=1
endif

ifeq ($(TARGET_BUILD_VARIANT),eng)
ADDITIONAL_DEFAULT_PROPERTIES += persist.service.profile.enable=1
endif

# Logcat use android kernel logger
TARGET_USES_LOGD := false

ifeq ($(BOARD_HAVE_SMALL_RAM),true)
ADDITIONAL_DEFAULT_PROPERTIES += ro.config.low_ram=true
endif

endif

ifeq ($(TARGET_BUILD_VARIANT),eng)
ADDITIONAL_DEFAULT_PROPERTIES += persist.core.enabled=1
endif

# Enabling collecting of additional information into tombstone file for eng and user debug builds
ifneq ($(TARGET_BUILD_VARIANT),user)
ADDITIONAL_BUILD_PROPERTIES += system.debug.plugins=libcrash.so
endif

ifeq ($(TARGET_RIL_DISABLE_STATUS_POLLING),true)
ADDITIONAL_BUILD_PROPERTIES += ro.ril.status.polling.enable=0
endif

ifeq ($(TARGET_BUILD_VARIANT),eng)
BUILD_INIT_EXEC := true
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
cmdline_extra += watchdog.watchdog_thresh=60
endif

ifeq ($(BOARD_BOOTMEDIA),)
BOARD_BOOTMEDIA := sdcard
endif

BOOT_TARBALL_FORMAT := gz
SYSTEM_TARBALL_FORMAT := gz

# Required for the size calculations in definitions.mk. Since
# definitions.mk assume a nand... a bit of space will be wasted
BOARD_FLASH_BLOCK_SIZE := 2048
BUILD_WITH_FULL_STAGEFRIGHT := true


# Set property of maximal runtime heap size to 64MB for intel's mfld board.
# If not set, one app can only use at most 16MB for runtime heap, which is
# too small and sometimes would cause out-of-memory error.
ADDITIONAL_BUILD_PROPERTIES += dalvik.vm.heapsize=64m

# Enabling Houdini by default
ADDITIONAL_BUILD_PROPERTIES += ro.product.cpu.abi2=armeabi-v7a

# kernel Mmap memory bottom-up
ADDITIONAL_BUILD_PROPERTIES += ro.config.personality=compat_layout

# Security
BUILD_WITH_CHAABI_SUPPORT := true
BUILD_WITH_WATCHDOG_DAEMON_SUPPORT := true

# Use Intel camera extras (HDR, face detection, panorama, etc.) by default
USE_INTEL_CAMERA_EXTRAS := true

# select libcamera2 as the camera HAL
USE_CAMERA_HAL2 := true

# disable the new V3 HAL by default so it can be added to the tree without conflicts
# it will be enabled in selected platforms
USE_CAMERA_HAL_3 := false

# Set USE_VIDEO_EFFECT to 'false' to unsupport live face effect. And Set OMX Component Input Buffer Count to 2.
USE_VIDEO_EFFECT := true

# Do not use shared object of ia_face by default
USE_SHARED_IA_FACE := false

# Use multi-thread for acceleration
USE_INTEL_MULT_THREAD := true

# Use Async OMX for http streaming
USE_ASYNC_OMX_CLIENT := true

# Use panorama v1.0 by default
IA_PANORAMA_VERSION := 1.0

# Turn on GR_STATIC_RECT_VB flag in skia to boost performance
TARGET_USE_GR_STATIC_RECT_VB := true

# customize the malloced address to be 16-byte aligned
BOARD_MALLOC_ALIGNMENT := 16


# Enabled Bluetooth GAP test build in bluez
BUILD_BT_GAP_TEST := true

# force user build variant to display build number for internal dev builds. External release builds should not set this flag
DISPLAY_BUILD_NUMBER := true

# Wi-Fi
include $(COMMON_PATH)/wifi/WifiBoardConfig.mk

# Gps
include $(COMMON_PATH)/gps/GpsBoardConfig.mk

# Bluetooth
include $(COMMON_PATH)/bluetooth/BtBoardConfig.mk

# SPID
#
# Can be customized for each board simply defining "SPID=" in local BoardConfig.mk
# Without customization, will be auto-set by kernel
#
# SPID format :
#        vend:cust:manu:plat:prod:hard
USE_SPID ?= true
ifeq ($(USE_SPID), true)
	SPID ?= "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx"
	cmdline_extra += androidboot.spid=$(SPID)
endif

USE_BL_SERIALNO ?= false
ifeq ($(USE_BL_SERIALNO), false)
	cmdline_extra += androidboot.serialno=01234567890123456789
endif

STORAGE_CFLAGS ?= -DSTORAGE_BASE_PATH=\"/dev/block/mmcblk0\" -DSTORAGE_PARTITION_FORMAT=\"%sp%d\"
COMMON_GLOBAL_CFLAGS += $(STORAGE_CFLAGS)

# OS images signing
TARGET_BOOT_IMAGE_KEYS_PATH ?=  vendor/intel/PRIVATE/cert
TARGET_BOOT_IMAGE_PRIV_KEY ?= $(TARGET_BOOT_IMAGE_KEYS_PATH)/OS_priv.pem
TARGET_BOOT_IMAGE_PUB_KEY ?= $(TARGET_BOOT_IMAGE_KEYS_PATH)/OS_pub.pub
TARGET_BOOT_LOADER_PRIV_KEY ?= $(TARGET_BOOT_IMAGE_KEYS_PATH)/OSBL_priv.pem
TARGET_BOOT_LOADER_PUB_KEY ?= $(TARGET_BOOT_IMAGE_KEYS_PATH)/OSBL_pub.pub

# partitioning scheme
# osip-gpt:
# 	- osip used by iafw
# 	- gpt used by kernel
# full-gpt:
# 	- gpt used by iafw
# 	- gpt used by kernel
TARGET_PARTITIONING_SCHEME ?= "osip-gpt"

ifeq ($(TARGET_PARTITIONING_SCHEME),"full-gpt")
	TARGET_MAKE_NO_DEFAULT_BOOTIMAGE := false
	TARGET_MAKE_INTEL_BOOTIMAGE := true
	TARGET_BOOTIMAGE_USE_EXT2 ?= false

	ifneq (, $(findstring isu,$(TARGET_OS_SIGNING_METHOD)))
		BOARD_MKBOOTIMG_ARGS += --signsize 1024 --signexec "isu_wrapper.sh isu $(TARGET_BOOT_IMAGE_PRIV_KEY) $(TARGET_BOOT_IMAGE_PUB_KEY)"
	endif

	ifeq ($(TARGET_OS_SIGNING_METHOD),uefi)
		BOARD_MKBOOTIMG_ARGS += --signsize 256 --signexec "openssl dgst -sha256 -sign $(TARGET_BOOT_IMAGE_PRIV_KEY)"
	endif
endif

ifeq ($(TARGET_PARTITIONING_SCHEME), "osip-gpt")
INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
BOARD_CUSTOM_MKBOOTIMG := $(SUPPORT_PATH)/mkbootimg
BOARD_CUSTOM_MAKE_RECOVERY_PATCH := vendor/intel/hardware/libintelprov/make_recovery_patch
MKBOOTIMG := $(SUPPORT_PATH)/mkbootimg
BOARD_MKBOOTIMG_ARGS += --sign-with $(TARGET_OS_SIGNING_METHOD) \
	--bootstub $(PRODUCT_OUT)/bootstub

$(INSTALLED_BOOTIMAGE_TARGET): $(PRODUCT_OUT)/bootstub
endif

# If LOCAL_SIGN is not set, sign the OS locally (don't use signing server)
# this can be overriden with an environment variable
LOCAL_SIGN ?= true

# BIOS TYPE
# - iafw
# - uefi
TARGET_BIOS_TYPE ?= "iafw"

ifeq ($(TARGET_BIOS_TYPE),"uefi")
INSTALLED_ESPIMAGE_TARGET := $(PRODUCT_OUT)/esp.img
ESPUPDATE_ZIP_TARGET := $(PRODUCT_OUT)/esp.zip
endif

# If the kernel source is present, AndroidBoard.mk will perform a kernel build.
# otherwise, AndroidBoard.mk will find the kernel binaries in a tarball.
ifneq ($(wildcard $(KERNEL_SRC_DIR)/Makefile),)
TARGET_KERNEL_SOURCE_IS_PRESENT ?= true
endif

.PHONY: build_kernel build_kernel-nodeps
ifeq ($(TARGET_KERNEL_SOURCE_IS_PRESENT),true)
build_kernel: get_kernel_from_source
build_kernel-nodeps: get_kernel_from_source
else
build_kernel: get_kernel_from_tarball
build_kernel-nodeps: get_kernel_from_tarball
endif

.PHONY: get_kernel_from_tarball
get_kernel_from_tarball:
	tar -xv -C $(PRODUCT_OUT) -f $(TARGET_KERNEL_TARBALL)

bootimage: build_kernel

$(INSTALLED_KERNEL_TARGET): build_kernel
$(INSTALLED_RAMDISK_TARGET): build_kernel

# external release
include $(COMMON_PATH)/external/external.mk

BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_CHARGER_DISABLE_INIT_BLANK := true

# Define platform battery healthd library
BOARD_HAL_STATIC_LIBRARIES += libhealthd.intel

# crashlogd configuration
CRASHLOGD_FACTORY_CHECKSUM ?= true
CRASHLOGD_FULL_REPORT ?= true
CRASHLOGD_APLOG ?= true
CRASHLOGD_COREDUMP ?= true
ifeq ($(TARGET_BIOS_TYPE),"uefi")
CRASHLOGD_EFILINUX ?= true
endif
ifeq ($(TARGET_BIOS_TYPE),"iafw")
CRASHLOGD_FDK ?= true
endif
CRASHLOGD_MODULE_IPTRAK ?= true
CRASHLOGD_MODULE_SPID ?= true
CRASHLOGD_MODULE_BACKTRACE ?= false
CRASHLOGD_MODULE_KCT ?= true
CRASHLOGD_MODULE_MODEM ?= true
CRASHLOGD_MODULE_FABRIC ?= true
CRASHLOGD_MODULE_FW_UPDATE ?= true
CRASHLOGD_MODULE_RAMDUMP ?= true

# SELinux droidboot set as permissive for all targets
BOARD_KERNEL_DROIDBOOT_EXTRA_CMDLINE += androidboot.selinux=permissive

# SELinux
ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
cmdline_extra += androidboot.selinux=permissive
endif

BOARD_SEPOLICY_DIRS +=\
        device/intel/common/sepolicy
BOARD_SEPOLICY_UNION +=\
        asf.te \
        droidboot.te \
        file_contexts \
        genfs_contexts \
        intel_prop.te \
        platform_app.te \
        recovery.te \
        service_contexts \
        service.te \
        system_app.te
