#
# file: vendor/intel/x86_only_Android.mk
# Only compile this directory tree for IA builds.
#
# This makefile is copied to several places in the tree
# by a copyfile directive in the manifest.

LOCAL_PATH := $(my-dir)

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH), x86 x86_64))
include $(call first-makefiles-under,$(LOCAL_PATH))
endif
