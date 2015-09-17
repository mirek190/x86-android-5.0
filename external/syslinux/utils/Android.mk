LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_EXECUTABLES := isohybrid
include $(BUILD_HOST_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_EXECUTABLES := android-image.sh
include $(BUILD_HOST_PREBUILT)

