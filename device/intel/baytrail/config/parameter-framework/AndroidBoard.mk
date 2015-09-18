include $(COMMON_PATH)/parameter-framework/AndroidBoard.mk

LOCAL_PATH := $(PLATFORM_CONF_PATH)/parameter-framework

##################################################
# PACKAGE : parameter-framework.audio.baytrail

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.baytrail
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.audio.common \
    Realtek_RT5640Subsystem.xml \
    LPESubsystem.xml \
    LPEMixerSubsystem.xml
include $(BUILD_PHONY_PACKAGE)

##################################################
# PACKAGE : parameter-framework.audio.pmdown_time.subsystem

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.pmdown_time.subsystem
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    SysfsPmdownTimeSubsystem.xml
include $(BUILD_PHONY_PACKAGE)

##################################################
# PACKAGE : parameter-framework.audio.intelSSP.subsystem

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.intelSSP.subsystem
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    IntelSSPSubsystem.xml
include $(BUILD_PHONY_PACKAGE)

##################################################
# MODULES REQUIRED by parameter-framework.audio.baytrail Package

include $(CLEAR_VARS)
LOCAL_MODULE := Realtek_RT5640Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := LPESubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := LPEMixerSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
# MODULES REQUIRED by parameter-framework.audio.pmdown_time.subsystem

include $(CLEAR_VARS)
LOCAL_MODULE := SysfsPmdownTimeSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
# MODULES REQUIRED by parameter-framework.audio.intelSSP.subsystem

include $(CLEAR_VARS)
LOCAL_MODULE := IntelSSPSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
