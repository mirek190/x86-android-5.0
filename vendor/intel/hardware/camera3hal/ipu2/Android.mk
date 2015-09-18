

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/ipu2/3a

# Only required by IPU2
LOCAL_SHARED_LIBRARIES += \
    libglasscameracal \
    libia_dvs_2\
    libia_isp_2_2 \

LOCAL_SRC_FILES += \
    ipu2/HwStream.cpp \
    ipu2/PreviewStream.cpp \
    ipu2/PostProcessStream.cpp \
    ipu2/IPU2HwIsp.cpp \
    ipu2/IPU2HwSensor.cpp \
    ipu2/CaptureStream.cpp \
    ipu2/IPU2CameraHw.cpp \
    ipu2/SensorEmbeddedMetaData.cpp \
    ipu2/IaDvs2.cpp \
    ipu2/FakeStreamManager.cpp \
    ipu2/Aiq3ARawSensor.cpp \
    ipu2/Aiq3AThread.cpp \
    ipu2/Aiq3ARequestManager.cpp \
    ipu2/3a/Aiq3A.cpp \
    ipu2/3a/Soc3A.cpp \
    ipu2/IPU2ConfParser.cpp \
    common/BatteryStatus.cpp

ifeq ($(USE_SHARED_IA_FACE), true)
LOCAL_SRC_FILES += \
    ipu2/FaceDetectionThread.cpp \
    ipu2/FaceDetector.cpp

LOCAL_SHARED_LIBRARIES += \
    libia_face
endif

LOCAL_STATIC_LIBRARIES += \
    libscheduling_policy

# External libraries which is used by 3A when is build with ICC
# TODO: this should be moved out from here to the 3A library mk file
LOCAL_SHARED_LIBRARIES += libintlc libsvml libimf libirng

ifeq ($(USE_CSS_2_0), true)
LOCAL_CFLAGS += -DATOMISP_CSS2
endif

ifeq ($(USE_CSS_2_1), true)
LOCAL_CFLAGS += -DATOMISP_CSS2 -DATOMISP_CSS21
endif


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
    $(LOCAL_PATH)/imageProcess \

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
