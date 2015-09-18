ifeq ($(ENABLE_SENSOR_HUB),true)

LOCAL_PATH := $(call my-dir)

#
# sensorhubd - sensorhub daemon
#
include $(CLEAR_VARS)
LOCAL_MODULE := sensorhubd
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    src/daemon/main.c \
    src/utils/utils.c
LOCAL_SHARED_LIBRARIES := liblog libhardware_legacy

ifeq ($(strip $(INTEL_FEATURE_AWARESERVICE)),true)

LOCAL_SHARED_LIBRARIES += libcontextarbitor

LOCAL_CFLAGS := -DENABLE_CONTEXT_ARBITOR

endif

include $(BUILD_EXECUTABLE)

#
# libsensorhub - sensorhub client library
#
include $(CLEAR_VARS)
LOCAL_MODULE := libsensorhub
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    src/lib/libsensorhub.c \
    src/utils/utils.c
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/src/include
include $(BUILD_SHARED_LIBRARY)


#
# sensorhub_client - sensorhub test client
#
include $(CLEAR_VARS)
LOCAL_MODULE := sensorhub_client
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/tests/sensor_hub_client.c
LOCAL_SHARED_LIBRARIES := libsensorhub liblog
include $(BUILD_EXECUTABLE)

#
# test.c from Alek
#
include $(CLEAR_VARS)
LOCAL_MODULE := test_alek
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/tests/test.c
include $(BUILD_EXECUTABLE)

#
# bist_test.c
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/tests/bist_test.c
LOCAL_SHARED_LIBRARIES := libsensorhub
LOCAL_MODULE := bist_test
include $(BUILD_EXECUTABLE)

#
# test_get_property.c
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/tests/test_get_property.c
LOCAL_SHARED_LIBRARIES := libsensorhub
LOCAL_MODULE := test_get_property
include $(BUILD_EXECUTABLE)

#
# compass calibration test.
#
include $(CLEAR_VARS)
LOCAL_MODULE := calibration
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/tests/calibration.c
LOCAL_SHARED_LIBRARIES := libsensorhub
include $(BUILD_EXECUTABLE)

#
# event notification test.
#
include $(CLEAR_VARS)
LOCAL_MODULE := event_notification
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := src/tests/event_notification.c
LOCAL_SHARED_LIBRARIES := libsensorhub liblog
include $(BUILD_EXECUTABLE)

endif


###############################################
# For ISH build
###############################################
ifeq ($(ENABLE_SENSOR_HUB_ISH),true)
LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/src_ish/Android.mk
endif
