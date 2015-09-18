here := $(notdir $(call my-dir))

ifeq ($(CONFIG_INTELPROV_GPT),y)
MODULES-SOURCES += $(here)/bootimage.c
endif
