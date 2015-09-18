LOCAL_PATH:= $(call my-dir)
ifeq ($(TARGET_HAS_ISV),true)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := VppSettings
LOCAL_CERTIFICATE := platform
#LOCAL_JAVA_LIBRARIES += com.intel.multidisplay

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))

endif
