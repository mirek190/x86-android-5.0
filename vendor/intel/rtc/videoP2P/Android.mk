#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This makefile shows how to build a shared library and an activity that
# bundles the shared library and calls it using JNI.

# Modified from development/samples/SimpleJNI/Android.mk

ifeq ($(ENABLE_WEBRTC),true)
TOP_LOCAL_PATH:= $(call my-dir)

# Build activity

LOCAL_PATH:= $(TOP_LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/webrtc

LOCAL_STATIC_JAVA_LIBRARIES := webrtc_video_render webrtc_video_capture

LOCAL_SRC_FILES := \
    $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := videoP2P

LOCAL_REQUIRED_MODULES := libwebrtc-video-p2p-jni

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_SDK_VERSION := current

LOCAL_CFLAGS += -DINTEL_ANDROID

include $(BUILD_PACKAGE)

# ============================================================

LOCAL_CFLAGS += -DINTEL_ANDROID
# Also build all of the sub-targets under this one: the shared library.
#include $(call all-makefiles-under,$(LOCAL_PATH))
include $(TOP_LOCAL_PATH)/jni/ITBAndroid.mk
endif # ENABLE_WEBRTC
