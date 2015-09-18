LOCAL_PATH := $(call my-dir)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := wl128x_common
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    bd_prov \
    uim \
    init.wl128x.rc
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.wl128x.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
