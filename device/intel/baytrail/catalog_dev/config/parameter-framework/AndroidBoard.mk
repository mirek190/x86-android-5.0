include $(PLATFORM_CONF_PATH)/parameter-framework/AndroidBoard.mk

LOCAL_PATH := $(DEVICE_CONF_PATH)/parameter-framework

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.catalog_dev
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.audio.catalog_dev.nodomains \
    AudioConfigurableDomains.xml
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.catalog_dev.nodomains
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.audio.baytrail \
    SysfsPmdownTimeBytcrSubsystem.xml \
    AudioClass.xml

ifeq ($(BOARD_USES_CODEC), TI31XX)
LOCAL_REQUIRED_MODULES += TI_TLV320AIC3100Subsystem.xml
endif
ifeq ($(BOARD_USES_CODEC), RT5645)
LOCAL_REQUIRED_MODULES += Realtek_RT5645Subsystem.xml
endif
ifeq ($(BOARD_USES_CODEC), RT5651)
LOCAL_REQUIRED_MODULES += Realtek_RT5651Subsystem.xml
endif
ifeq ($(BOARD_USES_CODEC), ES8396)
LOCAL_REQUIRED_MODULES += Everst_ES8396Subsystem.xml
endif
ifeq ($(BOARD_USES_CODEC), ES8316)
LOCAL_REQUIRED_MODULES += Everst_ES8316Subsystem.xml
endif

ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_REQUIRED_MODULES += ParameterFrameworkConfiguration.xml
else
LOCAL_REQUIRED_MODULES += ParameterFrameworkConfigurationNoTuning.xml
endif

include $(BUILD_PHONY_PACKAGE)

##################################################


include $(CLEAR_VARS)
LOCAL_MODULE := AudioClass.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
ifeq ($(BOARD_USES_CODEC), TI31XX)
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
endif
ifeq ($(BOARD_USES_CODEC), RT5645)
LOCAL_SRC_FILES := XML/Structure/Audio/AudioClass_RT5645.xml
endif
ifeq ($(BOARD_USES_CODEC), RT5651)
LOCAL_SRC_FILES := XML/Structure/Audio/AudioClass_RT5651.xml
endif
ifeq ($(BOARD_USES_CODEC), ES8396)
LOCAL_SRC_FILES := XML/Structure/Audio/AudioClass_ES8396.xml
endif
ifeq ($(BOARD_USES_CODEC), ES8316)
LOCAL_SRC_FILES := XML/Structure/Audio/AudioClass_ES8316.xml
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := TI_TLV320AIC3100Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := Realtek_RT5645Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := Realtek_RT5651Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := Everst_ES8396Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := Everst_ES8316Subsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := SysfsPmdownTimeBytcrSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Audio
LOCAL_SRC_FILES := XML/Structure/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(BOARD_USES_AUDIO_HAL_XML),true)

## Audio Tuning + Routing

include $(CLEAR_VARS)
LOCAL_MODULE := AudioConfigurableDomains.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Settings/Audio
LOCAL_REQUIRED_MODULES := \
        hostDomainGenerator.sh \
        parameter-framework.audio.catalog_dev.nodomains
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): MY_TOOL := $(ANDROID_HOST_OUT)/bin/hostDomainGenerator.sh
ifeq ($(BOARD_USES_CODEC), TI31XX)
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
        $(TARGET_OUT_ETC)/parameter-framework/ParameterFrameworkConfiguration.xml \
        $(LOCAL_PATH)/criteria.txt \
        $(LOCAL_PATH)/XML/Settings/Audio/AudioConfigurableDomains.xml \
        $(LOCAL_PATH)/XML/Settings/Audio/catalog_dev_routing_tlv320aic3100.pfw
endif

ifeq ($(BOARD_USES_CODEC), RT5645)
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
        $(TARGET_OUT_ETC)/parameter-framework/ParameterFrameworkConfiguration.xml \
        $(LOCAL_PATH)/criteria.txt \
        $(LOCAL_PATH)/XML/Settings/Audio/AudioConfigurableDomains_RT5645.xml \
        $(LOCAL_PATH)/XML/Settings/Audio/catalog_dev_routing_rt5645.pfw
endif

ifeq ($(BOARD_USES_CODEC), RT5651)
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
        $(TARGET_OUT_ETC)/parameter-framework/ParameterFrameworkConfiguration.xml \
        $(LOCAL_PATH)/criteria.txt \
        $(LOCAL_PATH)/XML/Settings/Audio/AudioConfigurableDomains_RT5651.xml \
        $(LOCAL_PATH)/XML/Settings/Audio/catalog_dev_routing_rt5651.pfw
endif

ifeq ($(BOARD_USES_CODEC), ES8396)
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
        $(TARGET_OUT_ETC)/parameter-framework/ParameterFrameworkConfiguration.xml \
        $(LOCAL_PATH)/criteria.txt \
        $(LOCAL_PATH)/XML/Settings/Audio/AudioConfigurableDomains_ES8396.xml \
        $(LOCAL_PATH)/XML/Settings/Audio/catalog_dev_routing_es8396.pfw
endif

ifeq ($(BOARD_USES_CODEC), ES8316)
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
        $(TARGET_OUT_ETC)/parameter-framework/ParameterFrameworkConfiguration.xml \
        $(LOCAL_PATH)/criteria.txt \
        $(LOCAL_PATH)/XML/Settings/Audio/AudioConfigurableDomains_ES8316.xml \
        $(LOCAL_PATH)/XML/Settings/Audio/catalog_dev_routing_es8316.pfw
endif

$(LOCAL_BUILT_MODULE): $(LOCAL_REQUIRED_MODULES)
	$(hide) mkdir -p $(dir $@)
	bash $(MY_TOOL) --validate $(MY_SRC_FILES) > $@

else

## Audio Tuning only

include $(CLEAR_VARS)
LOCAL_MODULE := AudioConfigurableDomains.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Settings/Audio
LOCAL_SRC_FILES := XML/Settings/Audio/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

endif

##################################################

ifeq ($(TARGET_BUILD_VARIANT),eng)

### Engineering build: Tuning allowed

# baylake
include $(CLEAR_VARS)
LOCAL_MODULE := ParameterFrameworkConfiguration.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework
LOCAL_SRC_FILES := XML/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

else

### Userdebug build: NoTuning allowed


# baytrail
include $(CLEAR_VARS)
LOCAL_MODULE := ParameterFrameworkConfigurationNoTuning.xml
LOCAL_MODULE_STEM := ParameterFrameworkConfiguration.xml
LOCAL_SRC_FILES := XML/ParameterFrameworkConfigurationNoTuning.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework
include $(BUILD_PREBUILT)

endif


##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.routeMgr.catalog_dev
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.routeMgr.catalog_dev.nodomains \
    RouteConfigurableDomains.xml
#ifeq ($(BOARD_USES_AUDIO_HAL_XML),true)
include $(BUILD_PHONY_PACKAGE)
#endif

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.routeMgr.catalog_dev.nodomains
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    RouteClass.xml \
    RouteSubsystem.xml \
    DebugFsSubsystem.xml \

ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_REQUIRED_MODULES += \
    ParameterFrameworkConfigurationRoute.xml
else
LOCAL_REQUIRED_MODULES += \
    ParameterFrameworkConfigurationRouteNoTuning.xml
endif

include $(BUILD_PHONY_PACKAGE)

##################################################
# Generate routing domains file
include $(CLEAR_VARS)
LOCAL_MODULE := RouteConfigurableDomains.xml
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    hostDomainGenerator.sh \
    parameter-framework.routeMgr.catalog_dev.nodomains

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Settings/Route
LOCAL_MODULE_STEM := RouteConfigurableDomains.xml
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): MY_TOOL := $(ANDROID_HOST_OUT)/bin/hostDomainGenerator.sh
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
    $(OUT)/system/etc/parameter-framework/ParameterFrameworkConfigurationRoute.xml \
    $(COMMON_PATH)/parameter-framework/RouteCriteria.txt \
    /dev/null \
    $(COMMON_PATH)/parameter-framework/XML/Settings/Route/parameters.pfw \
    $(LOCAL_PATH)/XML/Settings/Route/routes.pfw

$(LOCAL_BUILT_MODULE): $(LOCAL_REQUIRED_MODULES)
	$(hide) mkdir -p $(dir $@)
	bash $(MY_TOOL) --validate $(MY_SRC_FILES) > $@

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := RouteSubsystem.xml
LOCAL_MODULE_STEM := RouteSubsystem.xml
LOCAL_REQUIRED_MODULES := \
    RouteSubsystem-CommonCriteria.xml \
    RouteSubsystem-RoutesTypes.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Route
LOCAL_SRC_FILES := XML/Structure/Route/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := RouteSubsystem-common.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Route
LOCAL_SRC_FILES := XML/Structure/Route/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
