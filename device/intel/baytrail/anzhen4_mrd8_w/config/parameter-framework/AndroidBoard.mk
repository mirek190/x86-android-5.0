include $(PLATFORM_CONF_PATH)/parameter-framework/AndroidBoard.mk

LOCAL_PATH := $(DEVICE_CONF_PATH)/parameter-framework

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.anzhen4_mrd8_w
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.audio.anzhen4_mrd8_w.nodomains \
    AudioConfigurableDomains.xml
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.audio.anzhen4_mrd8_w.nodomains
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.audio.baytrail \
    parameter-framework.audio.pmdown_time.subsystem \
    parameter-framework.audio.intelSSP.subsystem \
    parameter-framework.audio.imc.subsystem \
    AudioClass.xml

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
        parameter-framework.audio.anzhen4_mrd8_w.nodomains
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): MY_TOOL := $(ANDROID_HOST_OUT)/bin/hostDomainGenerator.sh
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
        $(TARGET_OUT_ETC)/parameter-framework/ParameterFrameworkConfiguration.xml \
        $(LOCAL_PATH)/criteria.txt \
        $(LOCAL_PATH)/XML/Settings/Audio/AudioConfigurableDomains.xml \
        $(LOCAL_PATH)/XML/Settings/Audio/anzhen4_mrd8_w_routing_realtek5640.pfw \
        $(LOCAL_PATH)/XML/Settings/Audio/anzhen4_mrd8_w_routing_xmm.pfw

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
LOCAL_MODULE := parameter-framework.routeMgr.anzhen4_mrd8_w
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.routeMgr.anzhen4_mrd8_w.nodomains \
    RouteConfigurableDomains.xml
#ifeq ($(BOARD_USES_AUDIO_HAL_XML),true)
include $(BUILD_PHONY_PACKAGE)
#endif

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.routeMgr.anzhen4_mrd8_w.nodomains
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
    parameter-framework.routeMgr.anzhen4_mrd8_w.nodomains

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


##################################################
# PACKAGE : parameter-framework.vibrator.baytrail

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.vibrator.anzhen4_mrd8_w
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.vibrator.common \
    MiscConfigurationSubsystem.xml \
    SysfsVibratorClass.xml \
    SysfsVibratorSubsystem.xml \
    VibratorConfigurableDomains.xml

ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_REQUIRED_MODULES += ParameterFrameworkConfigurationVibrator.xml
else
LOCAL_REQUIRED_MODULES += ParameterFrameworkConfigurationVibratorNoTuning.xml
endif

include $(BUILD_PHONY_PACKAGE)

##################################################
# MODULES REQUIRED by parameter-framework.vibrator.baytrail Package

include $(CLEAR_VARS)
LOCAL_MODULE := SysfsVibratorClass.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Vibrator
LOCAL_SRC_FILES := XML/Structure/Vibrator/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := SysfsVibratorSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Vibrator
LOCAL_SRC_FILES := XML/Structure/Vibrator/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := VibratorConfigurableDomains.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Settings/Vibrator
LOCAL_SRC_FILES := XML/Settings/Vibrator/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
