#
# Copyright © 2011-2012 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#

LOCAL_PATH := $(call my-dir)

ifeq ($(ENABLE_GEN_GRAPHICS),true)

include $(CLEAR_VARS)

LIBDRM_TOP := $(LOCAL_PATH)

# Import variable LIBDRM_SOURCES.
include $(LOCAL_PATH)/sources.mk

LOCAL_MODULE := libdrm
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(LIBDRM_SOURCES)
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/include/drm $(LOCAL_PATH)/intel $(LOCAL_PATH)

LOCAL_C_INCLUDES := \
	$(LIBDRM_TOP)/include/drm

LOCAL_CFLAGS := \
	-DHAVE_LIBDRM_ATOMIC_PRIMITIVES=1

LOCAL_COPY_HEADERS :=            \
	xf86drm.h                \
	include/drm/drm_fourcc.h \
	include/drm/drm.h        \
	include/drm/drm_mode.h   \
	include/drm/drm_sarea.h  \
	include/drm/i915_drm.h   \
	intel/intel_bufmgr.h     \
	intel/intel_psr.h	\

LOCAL_COPY_HEADERS_TO := libdrm
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/intel/Android.mk
endif

