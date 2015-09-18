LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := OpenGsmtty.c
LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_MODULE := libnvmconfig_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
