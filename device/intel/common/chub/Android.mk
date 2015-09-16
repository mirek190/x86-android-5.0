LOCAL_PATH := $(call my-dir)

#--------------------------------------------------------------------
# Build CHUB init.rc

include $(CLEAR_VARS)

LOCAL_SRC_FILES := init.chub.rc

LOCAL_MODULE := init.chub.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)

include $(BUILD_PREBUILT)

#--------------------------------------------------------------------
# Build all CHUB components

include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES := \
    init.chub.rc \
    chub_fw.bin \
    libcosai \
    libchss \
    libchssflib \
    chssd \
    info_manager.db \
    chss_tools \

LOCAL_MODULE := chub
LOCAL_MODULE_TAGS := optional

include $(BUILD_PHONY_PACKAGE)
