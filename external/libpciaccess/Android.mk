#
# Copyright © 2011 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

ifndef BOARD_LIBPCIACCESS_HWDATA
    BOARD_LIBPCIACCESS_HWDATA := external/hwids
endif

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libpciaccess
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := \
    -DHAVE_INTTYPES_H \
    -DHAVE_STDINT_H \
    -DHAVE_STRINGS_H \
    -DHAVE_STRING_H \
    -Dlinux \
    -DPCIIDS_PATH=\"$(BOARD_LIBPCIACCESS_HWDATA)\"

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/common_bridge.c \
	src/common_capability.c \
	src/common_device_name.c \
	src/common_init.c \
	src/common_interface.c \
	src/common_io.c \
	src/common_iterator.c \
	src/common_map.c \
	src/common_vgaarb.c \
	src/linux_devmem.c \
	src/linux_sysfs.c

include $(BUILD_SHARED_LIBRARY)

