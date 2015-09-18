LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := apb_test
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wall -Werror -std=c99
LOCAL_SRC_FILES := apb_test.c
LOCAL_SHARED_LIBRARIES := libcutils
include $(BUILD_EXECUTABLE)
