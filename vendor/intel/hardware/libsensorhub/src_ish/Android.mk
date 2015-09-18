ifeq ($(ENABLE_SENSOR_HUB_ISH),true)

LOCAL_PATH := $(call my-dir)

#
# sensorhubd - sensorhub daemon
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += \
    bionic \
    bionic/libstdc++/include \
    external/stlport/stlport \
    external/gtest/include \
    system/extras/tests/include \
    frameworks/base/include

LOCAL_SRC_FILES := daemon/main.cpp \
			utils/utils.c \
			daemon/hw_ish.cpp \
			daemon/quatro_sensors.cpp 

LOCAL_STATIC_LIBRARIES := libc
LOCAL_SHARED_LIBRARIES := liblog libhardware_legacy libcutils libdl libstlport


LOCAL_MODULE := sensorhubd

include $(BUILD_EXECUTABLE)

#
# libsensorhub - sensorhub client library
#
include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES := lib/libsensorhub.c \
			utils/utils.c

LOCAL_SHARED_LIBRARIES := liblog libcutils

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE := libsensorhub

include $(BUILD_SHARED_LIBRARY)


#
# sensorhub_client - sensorhub test client
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES := tests/sensor_hub_client.c

LOCAL_SHARED_LIBRARIES += libsensorhub liblog
LOCAL_MODULE := sensorhub_client

include $(BUILD_EXECUTABLE)

#
# test.c from Alek
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES := tests/test.c

# LOCAL_SHARED_LIBRARIES += libsensorhub
LOCAL_MODULE := test_alek

#
# bist_test.c
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES := tests/bist_test.c

LOCAL_SHARED_LIBRARIES += libsensorhub

LOCAL_MODULE := bist_test

include $(BUILD_EXECUTABLE)

#
# compass calibration test.
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES := tests/calibration.c

LOCAL_SHARED_LIBRARIES += libsensorhub

LOCAL_MODULE	:= calibration

include $(BUILD_EXECUTABLE)

#
# event notification test.
#
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES := tests/event_notification.c

LOCAL_SHARED_LIBRARIES += libsensorhub liblog

LOCAL_MODULE    := event_notification

include $(BUILD_EXECUTABLE)

endif
