LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mbr.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/usr/lib/syslinux
LOCAL_MODULE_STEM := mbr.bin
LOCAL_SRC_FILES := mbr.bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := gptmbr.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/usr/lib/syslinux
LOCAL_MODULE_STEM := gptmbr.bin
LOCAL_SRC_FILES := gptmbr.bin
include $(BUILD_PREBUILT)

