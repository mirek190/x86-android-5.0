LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= intel_prop.c

LOCAL_MODULE:= intel_prop
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/sbin
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := \
	libcutils \
	libc \
	liblog

ifeq ($(TARGET_BIOS_TYPE),"uefi")
LOCAL_CFLAGS += -DENABLE_DMI
LOCAL_STATIC_LIBRARIES += libdmi
LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/libdmi
endif

include $(BUILD_EXECUTABLE)
