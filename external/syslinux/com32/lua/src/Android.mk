LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/../../com32_build_prebuilt.mk

C32_MODULES := \
	lua.c32

$(foreach module,$(C32_MODULES),$(eval $(call com32_build_prebuilt,$(module))))
