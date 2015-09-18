LOCAL_PATH := $(call my-dir)
PLATFORM_PATH := ../../$(TARGET_BOARD_PLATFORM)

include $(LOCAL_PATH)/ChipVendor.mk

ifeq (,$(filter none,$(GPS_CHIP_VENDOR) $(GPS_CHIP)))

##################################################

include $(LOCAL_PATH)/$(GPS_CHIP_VENDOR)/Android.mk

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_common
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps.$(TARGET_DEVICE) \
    init.gps.rc
include $(BUILD_PHONY_PACKAGE)

##################################################

endif
