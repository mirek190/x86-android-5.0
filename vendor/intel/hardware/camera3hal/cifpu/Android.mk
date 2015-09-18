LOCAL_SRC_FILES += \
    cifpu/CIFCameraHw.cpp \
    cifpu/CIFConfParser.cpp \
    cifpu/CaptureUnit.cpp \
    cifpu/Intel3aPlus.cpp \
    ipu4/HwStreamBase.cpp \
    cifpu/ControlUnit.cpp \
    cifpu/CIFVideoNode.cpp \
    common/PollerThread.cpp

LOCAL_C_INCLUDES += \
    bionic/libc/kernel/common \
    $(TARGET_OUT_HEADERS)/libgralloc

#Common Modules inside HAL
include $(LOCAL_PATH)/jpeg/Android.mk
include $(LOCAL_PATH)/common/Android.mk
include $(LOCAL_PATH)/imageProcess/Android.mk
include $(LOCAL_PATH)/platformdata/Android.mk
include $(LOCAL_PATH)/v4l2dev/Android.mk

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/jpeg \
    $(LOCAL_PATH)/platformdata \
    $(LOCAL_PATH)/common \
    $(LOCAL_PATH)/v4l2dev \
    $(LOCAL_PATH)/imageProcess

# libraries from libmfldadvci
LOCAL_SHARED_LIBRARIES += \
    libia_aiq \
    libia_cmc_parser \
    libia_log \
    libia_emd_decoder \
    libia_exc \
    libia_mkn \
    libia_nvm


LOCAL_STATIC_LIBRARIES += \
    libia_coordinate

# libraries from cameralibs
LOCAL_SHARED_LIBRARIES += \
    libtbd

LOCAL_STATIC_LIBRARIES += \
    libcameranvm \
    gdctool
