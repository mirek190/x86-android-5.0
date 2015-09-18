here := $(notdir $(call my-dir))

ifeq ($(CONFIG_INTELPROV_SCU_EMMC),y)
MODULES-SOURCES += $(here)/ifwi.c
endif
