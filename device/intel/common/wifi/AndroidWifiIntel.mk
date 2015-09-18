LOCAL_PATH := $(call my-dir)

##################################################
include $(CLEAR_VARS)
LOCAL_MODULE :=  wifi_intel
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES +=  \
     load_inteldriver

# Wifi user space components
LOCAL_REQUIRED_MODULES +=  \
    wifi_intel_usc

# WARNING: To be kept as the last required module.
LOCAL_REQUIRED_MODULES +=  \
    wifi_common

include $(BUILD_PHONY_PACKAGE)

##################################################
include $(CLEAR_VARS)
LOCAL_MODULE := load_inteldriver
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := intel_specific/load_inteldriver
include $(BUILD_PREBUILT)
