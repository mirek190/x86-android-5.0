LOCAL_PATH:= $(call my-dir)

# install the com.intel.config.xml file into /system/etc/permissions/
include $(CLEAR_VARS)
LOCAL_MODULE := com.intel.config.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(strip $(INTEL_FEATURE_ARKHAM)),true)
include $(CLEAR_VARS)
LOCAL_MODULE := init.intel.features.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
endif
