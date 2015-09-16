LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:= fg_config_xpwr.bin
LOCAL_MODULE_CLASS:= ETC
LOCAL_MODULE_TAGS:= optional
include $(BUILD_SYSTEM)/base_rules.mk

FG_CONFIG_XML := $(LOCAL_PATH)/fg_config_xpwr.xml
FG_CONFIG_PY := $(COMMON_PATH)/fg_config/fg_config_parser.py

$(LOCAL_BUILT_MODULE): $(FG_CONFIG_XML) $(FG_CONFIG_PY)
	$(hide) mkdir -p $(dir $@)
	$(hide) python $(FG_CONFIG_PY) $< $@
