
ifeq ($(USE_GENERAL_SENSOR_DRIVER),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := sensor_helper.c
LOCAL_C_INCLUDES +=  $(COMMON_INCLUDES) \
                $(call include-path-for, icu4c-common) \
                $(call include-path-for, libxml2)

LOCAL_CFLAGS += -g -Wall
LOCAL_CFLAGS += -DBASEDIR=\"$(DEVICE_CONF_PATH)\"
LOCAL_STATIC_LIBRARIES := libxml2
LOCAL_SHARED_LIBRARIES := libicuuc-host
LOCAL_MODULE := sensor_helper
include $(BUILD_HOST_EXECUTABLE)

#XML Driver&HAL config for each product
SENSOR_XMLS_CONFIG := $(DEVICE_CONF_PATH)/sensors/config
SENSOR_XMLS := $(patsubst %, $(dir $(SENSOR_XMLS_CONFIG))/xmls/%, $(shell cat $(SENSOR_XMLS_CONFIG)))

SENSOR_DRIVER_CONFIG_XML := sensor_driver_config.xml
SENSOR_HAL_CONFIG_XML := sensor_hal_config_default.xml
SENSOR_INITRC := init.sensor.rc

#plugin driver module
SENSOR_GENERAL_PLUGIN_SOURCE :=\
	$(PRODUCT_OUT)/obj/ETC/sensor_config.bin_intermediates/sensor_general_plugin.c
SENSOR_GENERAL_PLUGIN_MAKEFILE := $(LOCAL_PATH)/Makefile
$(eval $(call build_kernel_module,$(dir $(SENSOR_GENERAL_PLUGIN_SOURCE)),sensor_general_plugin))
sensor_general_plugin: sensor_config.bin

include $(CLEAR_VARS)
LOCAL_MODULE := sensor_config.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware/
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): sensor_helper $(ACP)
	@echo "Generating Sensor Driver Firmware image..."
	$(hide)mkdir -p $(dir $@)
	$(hide)mkdir -p $(TARGET_OUT_ETC)
	$(hide)mkdir -p $(PRODUCT_OUT)/root
	$(hide)sensor_helper -b -x $(dir $@)/$(SENSOR_DRIVER_CONFIG_XML) \
				-h $(dir $@)/$(SENSOR_HAL_CONFIG_XML) \
				-i $(dir $@)/$(SENSOR_INITRC) \
				   $(SENSOR_XMLS)
	$(hide)sensor_helper -p -x $(dir $@)/sensor_driver_config.xml \
				-m $(SENSOR_GENERAL_PLUGIN_SOURCE) \
				-f $@
	$(hide)$(ACP) $(SENSOR_GENERAL_PLUGIN_MAKEFILE) $(dir $@)
	$(hide)$(ACP) -fp $(dir $@)/$(SENSOR_HAL_CONFIG_XML) $(TARGET_OUT_ETC)
	$(hide)$(ACP) -fp $(dir $@)/$(SENSOR_INITRC) $(PRODUCT_OUT)/root
#	$(hide)sensor_helper -d -f $@ > $(dir $@)/sensor_config.dump

endif # USE_GENERAL_SENSOR_DRIVER
