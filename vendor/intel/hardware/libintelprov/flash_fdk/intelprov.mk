here := $(notdir $(call my-dir))

ifeq ($(CONFIG_INTELPROV_FDK),y)
MODULES-SOURCES += $(here)/capsule.c
endif
