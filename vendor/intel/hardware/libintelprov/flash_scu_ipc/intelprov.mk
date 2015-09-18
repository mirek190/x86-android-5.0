here := $(notdir $(call my-dir))

ifeq ($(CONFIG_INTELPROV_SCU_IPC),y)
MODULES-SOURCES += $(here)/ifwi.c
endif
