LOCAL_PATH := $(call my-dir)

# Common Configuration
################################################

include $(CLEAR_VARS)
LOCAL_MODULE := route_criteria.common.conf
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := route_criteria.vendor.conf
include $(BUILD_PHONY_PACKAGE)

# Vendor Configuration
#################################################

include $(CLEAR_VARS)
LOCAL_MODULE := route_criteria.vendor.conf
LOCAL_MODULE_STEM := route_criteria.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


