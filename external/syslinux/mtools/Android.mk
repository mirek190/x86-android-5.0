LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := syslinux
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	syslinux.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libfat
LOCAL_STATIC_LIBRARIES := syslinux_libfat_host

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libinstaller
LOCAL_STATIC_LIBRARIES += syslinux_libinstaller_host

LOCAL_CFLAGS := -g -Os -W -Wall -Wstrict-prototypes -D_FILE_OFFSET_BITS=64 -m32

include $(BUILD_HOST_EXECUTABLE)
