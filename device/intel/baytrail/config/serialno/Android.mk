LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := serialno.c
LOCAL_MODULE := serialno
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := bionic/libc/bionic
LOCAL_STATIC_LIBRARIES := libc libcutils liblog
include $(BUILD_EXECUTABLE)

