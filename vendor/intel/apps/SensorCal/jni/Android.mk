LOCAL_PATH:= $(call my-dir)

# Build jni lib if necessary
ifeq ($(ENABLE_SENSOR_HUB),true)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := liblog libsensorhub

LOCAL_SRC_FILES := psh_sensor_calibration_jni.cpp

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libPSHSensorCal_JNI

include $(BUILD_SHARED_LIBRARY)

endif
