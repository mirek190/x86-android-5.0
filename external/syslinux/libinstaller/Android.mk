LOCAL_PATH:= $(call my-dir)

# build static library for host
include $(CLEAR_VARS)
LOCAL_MODULE := syslinux_libinstaller_host
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
intermediates := $(call intermediates-dir-for,$(LOCAL_MODULE_CLASS),$(LOCAL_MODULE),true)

include $(LOCAL_PATH)/gen_code.mk

LOCAL_SRC_FILES := \
	fat.c \
	syslxmod.c \
	syslxopt.c \
	setadv.c

include $(BUILD_HOST_STATIC_LIBRARY)

# build static library for target
include $(CLEAR_VARS)
LOCAL_MODULE := syslinux_libinstaller
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
intermediates := $(call intermediates-dir-for,$(LOCAL_MODULE_CLASS),$(LOCAL_MODULE))

include $(LOCAL_PATH)/gen_code.mk

LOCAL_SRC_FILES := \
	fat.c \
	syslxmod.c \
	syslxopt.c \
	setadv.c

include $(BUILD_STATIC_LIBRARY)
