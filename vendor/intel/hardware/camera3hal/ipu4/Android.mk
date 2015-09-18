
LOCAL_SRC_FILES += \
    ipu4/CSS2600CameraHw.cpp \
    ipu4/ProcessingUnit.cpp \
    ipu4/MediaController.cpp \
    ipu4/MediaEntity.cpp \
    ipu4/InputSystem.cpp \
    ipu4/SensorHw.cpp \
    ipu4/LensHw.cpp \
    ipu4/FlashHw.cpp \
    ipu4/CaptureUnit.cpp \
    ipu4/ControlUnit.cpp \
    ipu4/Intel3aPlus.cpp \
    ipu4/PSysPipeline.cpp \
    ipu4/IPU4ConfParser.cpp \
    ipu4/ProcUnitTask.cpp \
    ipu4/RawtoYUVTask.cpp \
    ipu4/StreamOutputTask.cpp \
    ipu4/JpegEncodeTask.cpp \
    ipu4/RawBypassTask.cpp \
    ipu4/HwStreamBase.cpp \
    common/PollerThread.cpp

LOCAL_SHARED_LIBRARIES += \
    libia_camera \
    libia_cipf

LOCAL_CFLAGS += -DBXTPOC

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
