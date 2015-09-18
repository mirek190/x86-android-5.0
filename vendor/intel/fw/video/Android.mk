LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
include $(CLEAR_VARS)
LOCAL_MODULE := msvdx_fw_mfld_DE2.0.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/msvdx_fw_mfld_DE2.0.bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := topazsc_fw.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/topazsc_fw.bin
include $(BUILD_PREBUILT)
endif




ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
include $(CLEAR_VARS)
LOCAL_MODULE := msvdx_fw_mfld_DE2.0.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/msvdx_fw_mfld_DE2.0.bin
include $(BUILD_PREBUILT)
endif




ifeq ($(TARGET_BOARD_PLATFORM),moorefield)
include $(CLEAR_VARS)
LOCAL_MODULE := msvdx.bin.prod
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mofd/msvdx.bin.prod
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := topaz.bin.prod
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mofd/topaz.bin.prod
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := vsp.bin.prod
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mofd/vsp.bin.prod
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := msvdx.bin.verf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mofd/msvdx.bin.verf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := topaz.bin.verf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mofd/topaz.bin.verf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := vsp.bin.verf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mofd/vsp.bin.verf
include $(BUILD_PREBUILT)
endif




ifeq ($(TARGET_BOARD_PLATFORM),merrifield)
include $(CLEAR_VARS)
LOCAL_MODULE := msvdx.bin.prod
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mrfl/msvdx.bin.prod
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := topaz.bin.prod
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mrfl/topaz.bin.prod
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := vsp.bin.prod
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mrfl/vsp.bin.prod
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := msvdx.bin.verf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mrfl/msvdx.bin.verf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := topaz.bin.verf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mrfl/topaz.bin.verf
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := vsp.bin.verf
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := lnc/video_fw/mrfl/vsp.bin.verf
include $(BUILD_PREBUILT)
endif
