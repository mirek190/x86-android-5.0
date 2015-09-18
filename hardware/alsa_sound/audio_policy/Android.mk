ifeq ($(BOARD_USES_ALSA_AUDIO),true)

# uncomment this variable to execute audio policy test
#AUDIO_POLICY_TEST := true

LOCAL_PATH := $(call my-dir)

# This is the ALSA audio policy manager

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    AudioPolicyManagerALSA.cpp \
    PropertyIntent.cpp

ifeq ($(AUDIO_POLICY_TEST),true)
  LOCAL_CFLAGS += -DAUDIO_POLICY_TEST
endif

LOCAL_C_INCLUDES += \
    external/stlport/stlport/ \
    bionic/

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libstlport \
    libbinder

LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_SHARED_LIBRARIES := \
    libproperty

LOCAL_STATIC_LIBRARIES := \
    libmedia_helper

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libaudiopolicy_legacy

LOCAL_MODULE := audio_policy.$(TARGET_DEVICE)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_CFLAGS += -DWITH_A2DP
endif

include $(BUILD_SHARED_LIBRARY)

endif
