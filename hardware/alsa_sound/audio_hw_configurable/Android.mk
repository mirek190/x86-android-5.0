#
# INTEL CONFIDENTIAL
# Copyright © 2013 Intel
# Corporation All Rights Reserved.
#
# The source code contained or described herein and all documents related to
# the source code ("Material") are owned by Intel Corporation or its suppliers
# or licensors. Title to the Material remains with Intel Corporation or its
# suppliers and licensors. The Material contains trade secrets and proprietary
# and confidential information of Intel or its suppliers and licensors. The
# Material is protected by worldwide copyright and trade secret laws and
# treaty provisions. No part of the Material may be used, copied, reproduced,
# modified, published, uploaded, posted, transmitted, distributed, or
# disclosed in any way without Intel’s prior express written permission.
#
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery
# of the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be
# express and approved by Intel in writing.
#

ifeq ($(BOARD_USES_ALSA_AUDIO),true)
ifeq ($(BOARD_USES_AUDIO_HAL_CONFIGURABLE),true)

#ENABLE_AUDIO_DUMP := true
LOCAL_PATH := $(call my-dir)

#######################################################################
# Common variables

AUDIO_PLATHW := audio_route_manager/AudioPlatformHardware_$(REF_PRODUCT_NAME).cpp

ifeq ($(wildcard $(LOCAL_PATH)/$(AUDIO_PLATHW)),)
    AUDIO_PLATHW := audio_route_manager/AudioPlatformHardware_default.cpp
endif

audio_hw_configurable_src_files :=  \
    ALSAStreamOps.cpp \
    audio_hw_hal.cpp \
    AudioHardwareALSA.cpp \
    AudioHardwareInterface.cpp \
    AudioStreamInALSA.cpp \
    AudioStreamOutALSA.cpp

audio_hw_configurable_src_files +=  \
    audio_route_manager/AudioCompressedStreamRoute.cpp \
    audio_route_manager/AudioExternalRoute.cpp \
    audio_route_manager/AudioParameterHandler.cpp \
    $(AUDIO_PLATHW) \
    audio_route_manager/AudioPlatformState.cpp \
    audio_route_manager/AudioPort.cpp \
    audio_route_manager/AudioPortGroup.cpp \
    audio_route_manager/AudioRoute.cpp \
    audio_route_manager/AudioRouteManager.cpp \
    audio_route_manager/AudioStreamRoute.cpp \
    audio_route_manager/VolumeKeys.cpp \
    audio_route_manager/AudioStreamRouteIaSspWorkaround.cpp

audio_hw_configurable_includes_dir := \
    $(LOCAL_PATH)/audio_route_manager \
    $(TARGET_OUT_HEADERS)/libaudioresample \
    $(TARGET_OUT_HEADERS)/mamgr-interface \
    $(TARGET_OUT_HEADERS)/mamgr-core \
    $(TARGET_OUT_HEADERS)/audiocomms-include \
    $(TARGET_OUT_HEADERS)/audio_hal_utils \
    $(TARGET_OUT_HEADERS)/hw \
    $(TARGET_OUT_HEADERS)/parameter \
    frameworks/av/include \
    external/tinyalsa/include \
    system/media/audio_utils/include \
    system/media/audio_effects/include

audio_hw_configurable_includes_dir_host := \
    $(audio_hw_configurable_includes_dir)

audio_hw_configurable_includes_dir_target := \
    $(audio_hw_configurable_includes_dir) \
    external/stlport/stlport \
    bionic

audio_hw_configurable_header_files :=  \
    ALSAStreamOps.h \
    AudioDumpInterface.h \
    AudioHardwareALSA.h \
    audio_route_manager/AudioCompressedStreamRoute.h \
    audio_route_manager/AudioExternalRoute.h \
    audio_route_manager/AudioParameterHandler.h \
    audio_route_manager/AudioPlatformHardware.h \
    audio_route_manager/AudioPlatformState.h \
    audio_route_manager/AudioPortGroup.h \
    audio_route_manager/AudioPort.h \
    audio_route_manager/AudioRoute.h \
    audio_route_manager/AudioRouteManager.h \
    audio_route_manager/AudioStreamRoute.h \
    audio_route_manager/VolumeKeys.h \
    audio_route_manager/AudioStreamRouteIaSspWorkaround.h \
    AudioStreamInALSA.h \
    AudioStreamOutALSA.h

audio_hw_configurable_header_copy_folder_unit_test := \
    audio_hw_configurable_unit_test

audio_hw_configurable_static_lib += \
    libsamplespec_static \
    libaudioconversion_static \
    libhalaudiodump \
    libaudio_comms_utilities

audio_hw_configurable_static_lib_host += \
    $(foreach lib, $(audio_hw_configurable_static_lib), $(lib)_host)

audio_hw_configurable_static_lib_target += \
    $(audio_hw_configurable_static_lib) \
    libmedia_helper

audio_hw_configurable_shared_lib_target += \
    libtinyalsa \
    libcutils \
    libutils \
    libmedia \
    libhardware \
    libhardware_legacy \
    libparameter \
    libstlport \
    libicuuc \
    libevent-listener \
    libaudioresample \
    libaudioutils \
    libproperty \
    libaudiohalutils \
    libinterface-provider-lib

audio_hw_configurable_include_dirs_from_static_libraries := \
    libevent-listener_static \
    libinterface-provider-lib_static \
    libproperty_static

audio_hw_configurable_include_dirs_from_static_libraries_target := \
    $(audio_hw_configurable_include_dirs_from_static_libraries)

audio_hw_configurable_include_dirs_from_static_libraries_host := \
    $(foreach lib, $(audio_hw_configurable_include_dirs_from_static_libraries), $(lib)_host)

audio_hw_configurable_cflags := -Wall -Werror

#######################################################################
# Phony package definition

include $(CLEAR_VARS)
LOCAL_MODULE := audio_hal_configurable
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := \
    audio.primary.$(TARGET_DEVICE) \
    audio_policy.$(TARGET_DEVICE) \
    libaudiohalutils \
    libhalaudiodump

include $(BUILD_PHONY_PACKAGE)

#######################################################################
# Build for target audio.primary

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(audio_hw_configurable_includes_dir_target)

# for testing with dummy-stmd daemon, comment previous include
# path and uncomment the following one
#LOCAL_C_INCLUDES += \
#        hardware/alsa_sound/test-app/
#

LOCAL_SRC_FILES := $(audio_hw_configurable_src_files)
LOCAL_CFLAGS := $(audio_hw_configurable_cflags)

ifeq ($(ENABLE_AUDIO_DUMP),true)
  LOCAL_CFLAGS += -DENABLE_AUDIO_DUMP
  LOCAL_SRC_FILES += AudioDumpInterface.cpp
endif

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_CFLAGS += -DWITH_A2DP
endif

LOCAL_MODULE := audio.primary.$(TARGET_DEVICE)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

LOCAL_STATIC_LIBRARIES := \
    $(audio_hw_configurable_static_lib_target)

LOCAL_SHARED_LIBRARIES := \
    $(audio_hw_configurable_shared_lib_target)

# gcov build
ifeq ($($(LOCAL_MODULE).gcov),true)
  LOCAL_CFLAGS += -O0 --coverage -include GcovFlushWithProp.h
  LOCAL_LDFLAGS += --coverage
  LOCAL_STATIC_LIBRARIES += gcov_flush_with_prop
endif

include $(BUILD_SHARED_LIBRARY)

#######################################################################
# Build for test with and without gcov for host and target

# Compile macro
define make_audio_hw_configurable_test_lib
$( \
    $(eval LOCAL_COPY_HEADERS_TO := $(audio_hw_configurable_header_copy_folder_unit_test)) \
    $(eval LOCAL_COPY_HEADERS := $(audio_hw_configurable_header_files)) \
    $(eval LOCAL_C_INCLUDES := $(audio_hw_configurable_includes_dir_$(1))) \
    $(eval LOCAL_STATIC_LIBRARIES := $(audio_hw_configurable_static_lib_$(1))) \
    $(eval LOCAL_SRC_FILES := $(audio_hw_configurable_src_files)) \
    $(eval LOCAL_CFLAGS := $(audio_hw_configurable_cflags)) \
    $(eval LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := $(audio_hw_configurable_include_dirs_from_static_libraries_$(1)))
    $(eval LOCAL_MODULE_TAGS := optional) \
)
endef

define add_gcov
$( \
    $(eval LOCAL_CFLAGS += -O0 --coverage) \
    $(eval LOCAL_LDFLAGS += --coverage) \
)
endef

# Build for host test with gcov
ifeq ($(audiocomms_test_gcov_host),true)

include $(CLEAR_VARS)
$(call make_audio_hw_configurable_test_lib,host)
$(call add_gcov)
LOCAL_MODULE := libaudio_hw_configurable_static_gcov_host
include $(BUILD_HOST_STATIC_LIBRARY)

endif #ifeq ($(audiocomms_test_gcov_host),true)

# Build for target test with gcov
ifeq ($(audiocomms_test_gcov_target),true)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_hw_configurable_static_gcov
$(call make_audio_hw_configurable_test_lib,target)
$(call add_gcov)
include $(BUILD_STATIC_LIBRARY)

endif #ifeq ($(audiocomms_test_gcov_target),true)

# Build for host test
ifeq ($(audiocomms_test_host),true)

include $(CLEAR_VARS)
$(call make_audio_hw_configurable_test_lib,host)
LOCAL_MODULE := libaudio_hw_configurable_static_host
include $(BUILD_HOST_STATIC_LIBRARY)

endif #ifeq ($(audiocomms_test_host),true)

# Build for target test
ifeq ($(audiocomms_test_target),true)

include $(CLEAR_VARS)
LOCAL_MODULE := libaudio_hw_configurable_static
$(call make_audio_hw_configurable_test_lib,target)
include $(BUILD_STATIC_LIBRARY)

endif #ifeq ($(audiocomms_test_target),true)


endif #ifeq ($(BOARD_USES_AUDIO_HAL_CONFIGURABLE),true)
endif #ifeq ($(BOARD_USES_ALSA_AUDIO),true)
