LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/av/media/libstagefright/codecs/common/Config.mk

LOCAL_SRC_FILES:=                         \
        ACodec.cpp                        \
        AACExtractor.cpp                  \
        AACWriter.cpp                     \
        AMRExtractor.cpp                  \
        AMRWriter.cpp                     \
        AudioPlayer.cpp                   \
        AudioSource.cpp                   \
        AwesomePlayer.cpp                 \
        CameraSource.cpp                  \
        CameraSourceTimeLapse.cpp         \
        ClockEstimator.cpp                \
        CodecBase.cpp                     \
        DataSource.cpp                    \
        DataURISource.cpp                 \
        DRMExtractor.cpp                  \
        ESDS.cpp                          \
        FileSource.cpp                    \
        FLACExtractor.cpp                 \
        HTTPBase.cpp                      \
        JPEGSource.cpp                    \
        MP3Extractor.cpp                  \
        MPEG2TSWriter.cpp                 \
        MPEG4Extractor.cpp                \
        AVIExtractor.cpp                  \
        MPEG4Writer.cpp                   \
        MediaAdapter.cpp                  \
        MediaBuffer.cpp                   \
        MediaBufferGroup.cpp              \
        MediaCodec.cpp                    \
        MediaCodecList.cpp                \
        MediaCodecSource.cpp              \
        MediaDefs.cpp                     \
        MediaExtractor.cpp                \
        http/MediaHTTP.cpp                \
        MediaMuxer.cpp                    \
        MediaSource.cpp                   \
        MetaData.cpp                      \
        NuCachedSource2.cpp               \
        NuMediaExtractor.cpp              \
        OMXClient.cpp                     \
        OMXCodec.cpp                      \
        OggExtractor.cpp                  \
        SampleIterator.cpp                \
        SampleTable.cpp                   \
        SkipCutBuffer.cpp                 \
        StagefrightMediaScanner.cpp       \
        StagefrightMetadataRetriever.cpp  \
        SurfaceMediaSource.cpp            \
        ThrottledSource.cpp               \
        TimeSource.cpp                    \
        TimedEventQueue.cpp               \
        Utils.cpp                         \
        VBRISeeker.cpp                    \
        WAVExtractor.cpp                  \
        WVMExtractor.cpp                  \
        XINGSeeker.cpp                    \
        avc_utils.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/include/media/ \
        $(TOP)/frameworks/av/include/media/stagefright/timedtext \
        $(TOP)/frameworks/native/include/media/hardware \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/external/flac/include \
        $(TOP)/external/tremolo \
        $(TOP)/external/openssl/include \
        $(TOP)/external/libvpx/libwebm \
        $(TOP)/system/netd/include \
        $(TOP)/external/icu/icu4c/source/common \
        $(TOP)/external/icu/icu4c/source/i18n \
        $(TOP)/external/alac \

LOCAL_SHARED_LIBRARIES := \
        libbinder \
        libcamera_client \
        libcutils \
        libdl \
        libdrmframework \
        libexpat \
        libgui \
        libicui18n \
        libicuuc \
        liblog \
        libmedia \
        libnetd_client \
        libopus \
        libsonivox \
        libssl \
        libstagefright_omx \
        libstagefright_yuv \
        libsync \
        libui \
        libutils \
        libvorbisidec \
        libz \
        libpowermanager

LOCAL_STATIC_LIBRARIES := \
        libstagefright_color_conversion \
        libstagefright_aacenc \
        libstagefright_matroska \
        libstagefright_webm \
        libstagefright_timedtext \
        libvpx \
        libwebm \
        libstagefright_mpeg2ts \
        libstagefright_id3 \
        libFLAC \
        libmedia_helper

ifeq ($(ENABLE_BACKGROUND_MUSIC),true)
  LOCAL_CFLAGS += -DBGM_ENABLED
endif

LOCAL_SHARED_LIBRARIES += \
        libstagefright_enc_common \
        libstagefright_avc_common \
        libstagefright_foundation \
        libdl

ifeq ($(USE_INTEL_ASF_EXTRACTOR),true)
LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/libmix_asf_extractor
LOCAL_STATIC_LIBRARIES += libasfextractor
LOCAL_SHARED_LIBRARIES += libasfparser
LOCAL_CPPFLAGS += -DUSE_INTEL_ASF_EXTRACTOR
endif

ifeq ($(USE_INTEL_MULT_THREAD),true)
LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/libthreadedsource
LOCAL_STATIC_LIBRARIES += libthreadedsource
LOCAL_CPPFLAGS += -DUSE_INTEL_MULT_THREAD
endif

ifeq ($(USE_INTEL_MDP),true)
LOCAL_C_INCLUDES += \
          $(TARGET_OUT_HEADERS)/media_codecs

LOCAL_STATIC_LIBRARIES += \
          lib_stagefright_mdp_wmadec \
          libmc_wma_dec \
          libmc_umc_core_merged \
          libmc_codec_common

LOCAL_CPPFLAGS += -DUSE_INTEL_MDP

ifeq ($(TARGET_BOARD_PLATFORM), clovertrail)
    IPP_LIBS_32_EXT := ia32
    IPP_LIBS_32_ARCH := s_s8
else
    IPP_LIBS_32_EXT := ia32
    IPP_LIBS_32_ARCH := s_p8
endif
IPP_LIBS_64_EXT := intel64
IPP_LIBS_64_ARCH := s_e9

LOCAL_STATIC_LIBRARIES_32 += \
    libmediaipp_libippcore \
    libmediaipp_libippac_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libippvc_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libippdc_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libippcc_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libipps_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libippsc_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libippi_$(IPP_LIBS_32_ARCH) \
    libmediaipp_libippvm \
    libmediaipp_libsvml \
    libmediaipp_libimf \
    libmediaipp_libirc

LOCAL_STATIC_LIBRARIES_64 += \
    libmediaipp_libippcore \
    libmediaipp_libippac_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libippvc_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libippdc_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libippcc_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libipps_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libippsc_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libippi_$(IPP_LIBS_64_ARCH) \
    libmediaipp_libippvm \
    libmediaipp_libsvml \
    libmediaipp_libimf \
    libmediaipp_libirc

# group static libraries to resolve cyclic dependencies for ipp
LOCAL_GROUP_STATIC_LIBRARIES := true

LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel

endif

LOCAL_CFLAGS += -Wno-multichar

ifeq ($(TARGET_HAS_ISV), true)
LOCAL_CFLAGS +=-DTARGET_HAS_ISV
endif

ifeq ($(TARGET_HAS_MULTIPLE_DISPLAY),true)
    LOCAL_CFLAGS += -DTARGET_HAS_MULTIPLE_DISPLAY
    LOCAL_STATIC_LIBRARIES += libmultidisplayvideoclient
    LOCAL_SHARED_LIBRARIES += libmultidisplay
endif

ifeq ($(USE_FEATURE_ALAC),true)
LOCAL_CPPFLAGS += -DUSE_FEATURE_ALAC
LOCAL_C_INCLUDES += \
        $(TOP)/frameworks/av/media/libstagefright/omx
endif

ifeq ($(INTEL_VIDEOENCODER_MULTIPLE_NALUS),true)
LOCAL_CPPFLAGS += -DINTEL_VIDEOENCODER_MULTIPLE_NALUS
endif

LOCAL_MODULE:= libstagefright

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
