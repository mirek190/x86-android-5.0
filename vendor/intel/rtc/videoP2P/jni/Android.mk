# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(call all-makefiles-under, $(LOCAL_PATH))

# Specify BUILDTYPE=Release on the command line for a release build.
BUILDTYPE ?= Debug

#MY_LIBS_PATH := ../../../../../out/$(BUILDTYPE)/obj.target
MY_LIBS_PATH := ../../../../../../out/$(BUILDTYPE)/obj.target/third_party

MY_LIBJINGLE_PATH := ../../../../../../out/$(BUILDTYPE)


include $(CLEAR_VARS)
LOCAL_MODULE := libvoice_engine_core
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/voice_engine/libvoice_engine_core.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libvideo_engine_core
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/video_engine/libvideo_engine_core.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libvideo_processing
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libvideo_processing.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libvideo_processing_sse2
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libvideo_processing_sse2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwebrtc_video_coding
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libwebrtc_video_coding.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libvideo_render_module
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libvideo_render_module.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libvideo_capture_module
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libvideo_capture_module.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_coding_module
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libaudio_coding_module.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_processing
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libaudio_processing.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_processing_sse2
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libaudio_processing_sse2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libPCM16B
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libPCM16B.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libCNG
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libCNG.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libNetEq
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libNetEq.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libG722
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libG722.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libiSAC
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libiSAC.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libG711
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libG711.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libiLBC
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libiLBC.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libiSACFix
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libiSACFix.a
include $(PREBUILT_STATIC_LIBRARY)

# Remove the following file existense check when opus is always enabled.
#ifneq ($(wildcard $(MY_LIBS_PATH)/opus/libopus.a),)
include $(CLEAR_VARS)
LOCAL_MODULE := libopus
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/opus/libopus.a
include $(PREBUILT_STATIC_LIBRARY)
#endif

#ifneq ($(wildcard $(MY_LIBS_PATH)/webrtc/modules/libwebrtc_opus.a),)
include $(CLEAR_VARS)
LOCAL_MODULE := libwebrtc_opus
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libwebrtc_opus.a
include $(PREBUILT_STATIC_LIBRARY)
#endif

include $(CLEAR_VARS)
LOCAL_MODULE := libvad
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/common_audio/libvad.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libbitrate_controller
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libbitrate_controller.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libresampler
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/common_audio/libresampler.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsignal_processing
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/common_audio/libsignal_processing.a
include $(PREBUILT_STATIC_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := libsignal_processing_neon
#LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/common_audio/libsignal_processing_neon.a
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcommon_video
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/common_video/libcommon_video.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsystem_wrappers
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/system_wrappers/source/libsystem_wrappers.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcpu_features_android
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/system_wrappers/source/libcpu_features_android.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_device
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libaudio_device.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libremote_bitrate_estimator
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libremote_bitrate_estimator.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := librtp_rtcp
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/librtp_rtcp.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmedia_file
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libmedia_file.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libudp_transport
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libudp_transport.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwebrtc_utility
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libwebrtc_utility.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_conference_mixer
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libaudio_conference_mixer.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libyuv
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/libyuv/libyuv.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwebrtc_i420
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libwebrtc_i420.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwebrtc_vp8
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/video_coding/codecs/vp8/libwebrtc_vp8.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwebrtc_avc
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/video_coding/codecs/avc/libwebrtc_avc.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjpeg_turbo
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/libjpeg_turbo/libjpeg_turbo.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudioproc_debug_proto
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libaudioproc_debug_proto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libprotobuf_lite
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/protobuf/libprotobuf_lite.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libvpx
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/libvpx/libvpx.a
include $(PREBUILT_STATIC_LIBRARY)

# start add of libjingle libs
include $(CLEAR_VARS)
LOCAL_MODULE := libjingle
LOCAL_SRC_FILES := \
    $(MY_LIBJINGLE_PATH)/libjingle.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libexpat
LOCAL_SRC_FILES := \
    $(MY_LIBJINGLE_PATH)/libexpat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjsoncpp
LOCAL_SRC_FILES := \
    $(MY_LIBJINGLE_PATH)/libjsoncpp.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libopenssl
LOCAL_SRC_FILES := \
    $(MY_LIBJINGLE_PATH)/libopenssl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjingle_p2p
LOCAL_SRC_FILES := \
	$(MY_LIBJINGLE_PATH)/obj.target/talk/libjingle_p2p.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libjingle_media
LOCAL_SRC_FILES := \
	$(MY_LIBJINGLE_PATH)/obj.target/talk/libjingle_media.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsrtp 
LOCAL_SRC_FILES := \
	$(MY_LIBJINGLE_PATH)/obj.target/third_party/libsrtp/libsrtp.a
include $(PREBUILT_STATIC_LIBRARY)

# end add of libjingle libs

include $(CLEAR_VARS)
#LOCAL_MODULE := libvpx_sse2
LOCAL_MODULE := libvpx_intrinsics
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/libvpx/libvpx_intrinsics.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libpaced_sender
LOCAL_SRC_FILES := \
    $(MY_LIBS_PATH)/webrtc/modules/libpaced_sender.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE := libwebrtc-video-p2p-jni
#LOCAL_MODULE := libwebrtc-video-demo-jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    videoclient.cc \
	kxmppthread.cc \
    callclient.cc \
    friendinvitesendtask.cc \

#    presencepushtask.cc 
#    vie_android_java_api.cc \
#    android_media_codec_decoder.cc \
#    $(LOCAL_PATH)/../../../../../../talk/xmpp/xmppthread.cc \
#    $(LOCAL_PATH)/../../../../../../talk/xmpp/xmpppump.cc \
#    $(LOCAL_PATH)/../../../../../../talk/xmpp/xmppsocket.cc \
#    $(LOCAL_PATH)/../../../../../../talk/xmpp/xmpptask.cc

LOCAL_CFLAGS := \
	$(JINGLE_CONFIG) \
    -DANDROID \
    -DINTEL_ANDROID \
    -fno-rtti \
    -DDISABLE_DYNAMIC_CAST \
    -D_REENTRANT \
    -DPOSIX \
    -DEXPAT_RELATIVE_PATH \
    -DXML_STATIC \
    -DFEATURE_ENABLE_SSL \
    -DFEATURE_ENABLE_PSTN \
    -DFEATURE_ENABLE_VOICEMAIL \
    -DHAVE_OPENSSL_SSL_H=1 \
    -DHAVE_SRTP \
    -DHAVE_WEBRTC_VIDEO \
    -DHAVE_WEBRTC_VOICE \
    -DWEBRTC_ANDROID \
    -DWEBRTC_TARGET_PC \
    -DSTUN_PORT=19302 \
	-DANDROID_LOG \
    '-DSTUN_ADDRESS="stun.l.google.com"' \
    '-DPMUC_DOMAIN="groupchat.google.com"' \
	-DICE_UDP


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
    $(call include-path-for, gtest) \
    $(LOCAL_PATH)/../../../.. \
    $(LOCAL_PATH)/../../../../.. \
    $(LOCAL_PATH)/../../../../../.. \
    $(LOCAL_PATH)/../../../include \
    $(LOCAL_PATH)/../../../../voice_engine/include \
    $(LOCAL_PATH)/../../../../system_wrappers/interface \
    $(LOCAL_PATH)/../../../../modules/video_capture/include \
    $(LOCAL_PATH)/../../../../modules/video_capture/android \
    $(LOCAL_PATH)/../../../../modules/video_render/include \
    $(TARGET_OUT_HEADERS) \
    $(TARGET_OUT_HEADERS)/webrtc \
    $(TARGET_OUT_HEADERS)/webrtc/talk \
    $(TARGET_OUT_HEADERS)/webrtc/talk/app \
    $(TARGET_OUT_HEADERS)/webrtc/talk/app/webrtc \
    $(TARGET_OUT_HEADERS)/webrtc/talk/base \
    $(TARGET_OUT_HEADERS)/webrtc/talk/examples \
    $(TARGET_OUT_HEADERS)/webrtc/talk/examples/call \
    $(TARGET_OUT_HEADERS)/webrtc/talk/examples/peerconnection \
    $(TARGET_OUT_HEADERS)/webrtc/talk/examples/peerconnection/client \
    $(TARGET_OUT_HEADERS)/webrtc/talk/media/base \
    $(TARGET_OUT_HEADERS)/webrtc/talk/media/devices \
    $(TARGET_OUT_HEADERS)/webrtc/talk/media/webrtc \
    $(TARGET_OUT_HEADERS)/webrtc/talk/p2p/base \
    $(TARGET_OUT_HEADERS)/webrtc/talk/p2p/client \
    $(TARGET_OUT_HEADERS)/webrtc/talk/session/media \
    $(TARGET_OUT_HEADERS)/webrtc/talk/xmllite \
    $(TARGET_OUT_HEADERS)/webrtc/talk/xmpp \
    $(TARGET_OUT_HEADERS)/webrtc/third_party \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/common_video \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/common_video/interface \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/common_video/libyuv/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/interface \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_capture \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_capture/android \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_capture/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/modules/video_render/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/system_wrappers/interface \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/video_engine/include \
    $(TARGET_OUT_HEADERS)/webrtc/third_party/webrtc/voice_engine/include

LOCAL_LDLIBS := \
    -llog \
    -lgcc \
    -lGLESv2 \
    -lOpenSLES \
    -lva_videoencoder \
    -lva_videodecoder \
	-L../../../../android_tools/ndk/platforms/android-9/arch-x86/usr/lib \
	-L$(ANDROID_BUILD_ROOT)/out/target/product/$(TARGET_PRODUCT)/system/lib \
	-L$(MY_LIBS_PATH)

LOCAL_STATIC_LIBRARIES :=  \
    libstlport \
    libvideo_engine_core \
    libvoice_engine_core \
    libvideo_processing \
    libvideo_processing_sse2 \
    libwebrtc_video_coding \
    libvideo_render_module \
    libvideo_capture_module \
    libaudio_coding_module \
    libaudio_processing \
    libaudio_processing_sse2 \
	libwebrtc_utility \
    libPCM16B \
    libCNG \
    libNetEq \
    libG722 \
    libiSAC \
    libG711 \
    libiLBC \
    libiSACFix \
    libwebrtc_opus \
    libopus \
    libvad \
    libbitrate_controller \
    libresampler \
    libsignal_processing \
    libcommon_video \
    libsystem_wrappers \
    libcpu_features_android \
    libaudio_device \
    libremote_bitrate_estimator \
    librtp_rtcp \
    libmedia_file \
    libudp_transport \
    libaudio_conference_mixer \
    libwebrtc_i420 \
    libwebrtc_vp8 \
    libwebrtc_avc \
    libaudioproc_debug_proto \
    libprotobuf_lite \
    libvpx \
    libvpx_intrinsics \
    libpaced_sender \
	libjingle_p2p \
	libsrtp \
	libjingle_media \
    libjingle \
    libexpat  \
    libjsoncpp  \
    libopenssl \
    libyuv  \
    libjpeg_turbo \
    $(MY_SUPPLEMENTAL_LIBS)


#    libwebrtc_yuv \

#LOCAL_WHOLE_STATIC_LIBRARIES := \
    libwebrtc_media_file \
    libwebrtc_resampler \
    libwebrtc_system_wrappers \
    libwebrtc_vie_core \
    libwebrtc_voe_core \
    libwebrtc_audio_conference_mixer \
    libwebrtc_audio_coding \
    libwebrtc_apm \
    libwebrtc_utility \
    libwebrtc_isacfix \
    libwebrtc_g722 \
    libwebrtc_neteq \
    libwebrtc_cng \
    libwebrtc_ns \
    libwebrtc_vad \
    libwebrtc_webm \
    libwebrtc_jpeg \
    libwebrtc_remote_bitrate_estimator \
    libwebrtc_pcm16b  \
    libwebrtc_video_processing 

#    libwebrtc_spl 
#    libwebrtc_yuv
#    libvpx_sse2
include $(BUILD_SHARED_LIBRARY)
