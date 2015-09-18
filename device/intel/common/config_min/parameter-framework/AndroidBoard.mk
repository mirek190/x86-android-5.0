LOCAL_PATH := $(call my-dir)

##################################################

ifeq ($(BOARD_USES_AUDIO_HAL_XML) ,true)

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.routeMgr.confMin
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    parameter-framework.routeMgr.common.nodomains \
    RouteConfigurableDomains.xml
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := parameter-framework.routeMgr.common.nodomains
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    RouteClass.xml \
    RouteSubsystem.xml

ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_REQUIRED_MODULES += \
    ParameterFrameworkConfigurationRoute.xml
else
LOCAL_REQUIRED_MODULES += \
    ParameterFrameworkConfigurationRouteNoTuning.xml
endif

include $(BUILD_PHONY_PACKAGE)

endif

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := RouteClass.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Route
LOCAL_SRC_FILES := XML/Structure/Route/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := RouteSubsystem.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Structure/Route
LOCAL_SRC_FILES := XML/Structure/Route/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################
# Generate routing domains file
include $(CLEAR_VARS)
LOCAL_MODULE := RouteConfigurableDomains.xml
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    hostDomainGenerator.sh \
    parameter-framework.routeMgr.common.nodomains

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework/Settings/Route
LOCAL_MODULE_STEM := RouteConfigurableDomains.xml
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): hostDomainGenerator.sh
$(LOCAL_BUILT_MODULE): MY_SRC_FILES := \
    $(OUT)/system/etc/parameter-framework/ParameterFrameworkConfigurationRoute.xml \
    $(LOCAL_PATH)/RouteCriterion.txt \
    /dev/null \
    $(LOCAL_PATH)/XML/Settings/Route/routes.pfw \
    $(LOCAL_PATH)/XML/Settings/Route/parameters.pfw

$(LOCAL_BUILT_MODULE): $(LOCAL_REQUIRED_MODULES)
	$(hide) mkdir -p $(dir $@)
	bash hostDomainGenerator.sh $(MY_SRC_FILES) > $@

##################################################

ifeq ($(TARGET_BUILD_VARIANT),eng)

### Engineering build: Tuning allowed

include $(CLEAR_VARS)
LOCAL_MODULE := ParameterFrameworkConfigurationRoute.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework
LOCAL_SRC_FILES := XML/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

else

### Userdebug build: NoTuning allowed

include $(CLEAR_VARS)
LOCAL_MODULE := ParameterFrameworkConfigurationRouteNoTuning.xml
LOCAL_MODULE_STEM := ParameterFrameworkConfigurationRoute.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/parameter-framework
LOCAL_SRC_FILES := XML/ParameterFrameworkConfigurationRouteNoTuning.xml
include $(BUILD_PREBUILT)

endif



