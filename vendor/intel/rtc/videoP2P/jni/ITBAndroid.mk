# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

# This is for building in the Android build tree, it won't work with ndk

LOCAL_PATH := $(call my-dir)

include $(call all-makefiles-under, $(LOCAL_PATH))

# Specify BUILDTYPE=Release on the command line for a release build.
BUILDTYPE ?= Debug

# JINGLE_SRC_PATH defines the directory that 'talk' is located under
# WEBRTC_SRC_PATH defines the directory that contains the webrtc engines

# Relative paths, if videoP2P is located at the same hierarchy level as
# the webrtc/jingle codebase
#JINGLE_SRC_PATH:=$(LOCAL_PATH)/../../webrtc
#WEBRTC_SRC_PATH:=$(LOCAL_PATH)/../../webrtc/third_party/webrtc

# Absolute paths
JINGLE_SRC_PATH:=vendor/intel/PRIVATE/rtc/webrtc
WEBRTC_SRC_PATH:=vendor/intel/PRIVATE/rtc/webrtc/third_party/webrtc

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/videoP2P
LOCAL_MODULE := libwebrtc-video-p2p-jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    videoclient.cc \
    kxmppthread.cc \
    callclient.cc \
    friendinvitesendtask.cc \
    presencepushtask.cc \
    gsession.cc \
    gsessionmanager.cc \
    gcontentparser.cc \
    conductor.cc \
    defaults.cc

LOCAL_CFLAGS := \
    -DVIDEOP2P_TESTMODE \
    '-D_DEBUG=1' \
    '-DENABLE_DEBUG=1' \
    '-DLOGGING=1' \
    '-DWEBRTC_ANDROID_DEBUG=1' \
    '-DANDROID' \
    '-DINTEL_ANDROID' \
    '-DDISABLE_DYNAMIC_CAST' \
    '-D_REENTRANT' \
    '-DPOSIX' \
    '-DEXPAT_RELATIVE_PATH' \
    '-DXML_STATIC' \
    '-DFEATURE_ENABLE_SSL' \
    '-DFEATURE_ENABLE_PSTN' \
    '-DFEATURE_ENABLE_VOICEMAIL' \
    '-DHAVE_OPENSSL_SSL_H=1' \
    '-DHAVE_SRTP' \
    '-DHAVE_WEBRTC_VIDEO' \
    '-DHAVE_WEBRTC_VOICE' \
    '-DWEBRTC_TARGET_PC' \
    '-DWEBRTC_ANDROID' \
    '-DPOSIX' \
    '-DSTUN_PORT=19302' \
    '-DANDROID_LOG' \
    '-DSTUN_ADDRESS="stun.l.google.com"' \
    '-DPMUC_DOMAIN="groupchat.google.com"' \
    -fno-rtti \
    -Wno-non-virtual-dtor \
    -Wno-return-type

# webrtc-unstable/third_party/webrtc/video_engine/test/videoP2P/jni
LOCAL_C_INCLUDES := \
    $(call include-path-for, expat-lib) \
    $(call include-path-for, gtest) \
    $(call include-path-for, stlport) \
    bionic \
    $(WEBRTC_SRC_PATH) \
    $(WEBRTC_SRC_PATH)/voice_engine/include \
    $(WEBRTC_SRC_PATH)/video_engine/include \
    $(WEBRTC_SRC_PATH)/system_wrappers/interface \
    $(WEBRTC_SRC_PATH)/modules/video_capture/include \
    $(WEBRTC_SRC_PATH)/modules/video_capture/android \
    $(WEBRTC_SRC_PATH)/modules/video_render/include \
    $(JINGLE_SRC_PATH)/third_party \
    $(JINGLE_SRC_PATH) \
    $(TARGET_OUT_HEADERS) \
    $(TARGET_OUT_HEADERS)/webrtc \
    $(TARGET_OUT_HEADERS)/webrtc/third_party \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/voice_engine/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/video_engine/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/system_wrappers/interface \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_capture/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_capture/android \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_render/include


LOCAL_SHARED_LIBRARIES := \
    libGLESv2 \
    libOpenSLES \
    libva_videodecoder \
    libva_videoencoder \
    liblog \
    libexpat \
    libicuuc \
    libdl \
    libcutils \
    libutils \
    libui \
    libgui \
    libjpeg \
    libjingle_full \
    libwebrtc_full \
    libstlport

LOCAL_STATIC_LIBRARIES := \
    $(MY_SUPPLEMENTAL_LIBS)

include $(BUILD_SHARED_LIBRARY)
