here := $(notdir $(call my-dir))

ifeq ($(CONFIG_INTELPROV_OSIP),y)
MODULES-SOURCES += $(here)/bootimage.c
endif
