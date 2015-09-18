LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := bt_rtl8723b
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
        bt_rtl \
        rtl8723b_config \
        rtl8723b_fw

include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := rtl8723b_config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware/rtlbt
LOCAL_MODULE_STEM := rtl8723b_config
LOCAL_SRC_FILES := rtl8723b_gpiowake_config
include $(BUILD_PREBUILT)

#################################################

include $(CLEAR_VARS)
LOCAL_MODULE := rtl8723b_fw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware/rtlbt
LOCAL_MODULE_STEM := rtl8723b_fw
LOCAL_SRC_FILES := rtl8723b_fw
include $(BUILD_PREBUILT)

##################################################
