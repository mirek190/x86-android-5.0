
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libaes86
LOCAL_SRC_FILES := libaes86.a
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_MODULE_SUFFIX:=.a
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE    := libaesni
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES := aesni.c
LOCAL_LDFLAGS := -Wl,--no-warn-shared-textrel
LOCAL_STATIC_LIBRARIES := libaes86 liblog
include $(BUILD_SHARED_LIBRARY)

