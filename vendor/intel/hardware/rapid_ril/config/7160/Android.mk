LOCAL_PATH := .

#Definition of a generic copy file function. Used to copy all cfg files
define copy_file
include $(CLEAR_VARS)
LOCAL_MODULE := $(notdir $(1))
LOCAL_SRC_FILES := $(1)
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/telephony
include $(BUILD_PREBUILT)
endef

CFG_FILES := $(wildcard $(call my-dir)/*.txt)
$(foreach txt, $(CFG_FILES), $(eval $(call copy_file, $(txt))))

include $(CLEAR_VARS)
LOCAL_MODULE := rril_7160_cfg
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(notdir $(CFG_FILES))

include $(BUILD_PHONY_PACKAGE)
