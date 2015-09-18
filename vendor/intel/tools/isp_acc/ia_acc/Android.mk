LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ia_acc_android.c

LOCAL_C_INCLUDES +=

LOCAL_C_FLAGS =+ -fno-pic -W -Wall -Werror -DSYSTEM_$(SYSTEM) -D__HOST__

LOCAL_SHARED_LIBRARIES :=

LOCAL_STATIC_LIBRARIES :=

LOCAL_CFLAGS += -Wunused-variable -Werror

LOCAL_MODULE := libia_acc

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
