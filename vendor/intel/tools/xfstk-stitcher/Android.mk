LOCAL_PATH := $(call my-dir)
STITCHER_EXT_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_EXECUTABLES := bin/xfstk-stitcher
include $(BUILD_HOST_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_EXECUTABLES := bin/ifcl/ifcl
include $(BUILD_HOST_PREBUILT)

include $(CLEAR_VARS)
LOCAL_PREBUILT_EXECUTABLES := bin/capsule_builder/capsulebuilder
include $(BUILD_HOST_PREBUILT)
