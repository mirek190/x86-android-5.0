ifeq ($(USE_FEATURE_ALAC),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -O3 -msse3 -mfpmath=sse

LOCAL_SRC_FILES := \
        SoftALAC.cpp \
        ALACBitUtilities.c \
        ALACEngine.cpp \
        dp_dec.c \
        EndianPortable.c \
        matrix_dec.c \
        ag_dec.c

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax \
        $(LOCAL_PATH) 

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils liblog

ifeq ($(USE_FEATURE_ALAC),true)
    LOCAL_CPPFLAGS += -DUSE_FEATURE_ALAC
endif

LOCAL_MODULE := libstagefright_soft_alacdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif
