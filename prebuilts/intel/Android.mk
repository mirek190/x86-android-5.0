LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_ARCH),x86)
include $(shell find $(LOCAL_PATH) -mindepth 2 -name "Android.mk")
endif
