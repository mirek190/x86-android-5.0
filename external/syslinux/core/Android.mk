
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := isolinux.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/usr/lib/syslinux
LOCAL_MODULE_STEM := isolinux.bin
LOCAL_SRC_FILES := isolinux.bin
include $(BUILD_PREBUILT)
