LOCAL_PATH:= $(call my-dir)

# Build
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := src/com/intel/sensor/SensorCalSettings.java \
                   src/com/intel/sensor/CompassCal.java \
                   src/com/intel/sensor/GyroscopeCal.java

ifeq ($(ENABLE_SENSOR_HUB),true)
LOCAL_SRC_FILES += src/com/intel/sensor/psh_cal/SensorCalibration.java

else
LOCAL_SRC_FILES += src/com/intel/sensor/cal/SensorCalibration.java
endif

LOCAL_PACKAGE_NAME := SensorCal

LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

# Use the folloing include to make other modules under current path.
include $(call all-makefiles-under,$(LOCAL_PATH))
