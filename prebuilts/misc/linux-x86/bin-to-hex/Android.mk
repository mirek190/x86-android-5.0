LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_EXECUTABLES := prebuilt-bin-to-hex
include $(BUILD_HOST_PREBUILT)

