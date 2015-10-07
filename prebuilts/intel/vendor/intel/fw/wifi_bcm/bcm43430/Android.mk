LOCAL_PATH:= $(call my-dir)

FW_TARGET_PATH := $(TARGET_OUT_ETC)/firmware/

##################################################
################## WiFi BCM 43430 ################
include $(CLEAR_VARS)
LOCAL_MODULE := wifi_bcm_43430
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    fw_bcmdhd.bin_43430_a0  \
    bcmdhd_aob.cal_43430_a0

LOCAL_REQUIRED_MODULES +=  \
    wifi_bcm
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := fw_bcmdhd.bin_43430_a0
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(FW_TARGET_PATH)
LOCAL_SRC_FILES := firmware_bcm43430a0.bin
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := bcmdhd_aob.cal_43430_a0
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(FW_TARGET_PATH)
LOCAL_SRC_FILES := nvram.txt_43430_a0
include $(BUILD_PREBUILT)

##################################################

