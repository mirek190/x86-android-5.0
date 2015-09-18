LOCAL_PATH:= $(call my-dir)
#
## Install the prebuilt touch filter library
#
#
include $(CLEAR_VARS)
LOCAL_MODULE := libeventprocessing
LOCAL_MODULE_SUFFIX := .so
LOCAL_SRC_FILES := libeventprocessing.so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
include $(BUILD_PREBUILT)
