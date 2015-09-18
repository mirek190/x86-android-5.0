LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := com.intel.internal.telephony.MmgrClient

# Only compile source java files in this apk.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Amtl

LOCAL_AAPT_FLAGS := --product amtl

LOCAL_CERTIFICATE := platform

LOCAL_JNI_SHARED_LIBRARIES := libamtl_jni

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_REQUIRED_MODULES := amtl_cfg

include $(BUILD_PACKAGE)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := com.intel.internal.telephony.MmgrClient
# Only compile source java files in this apk.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Amtl2

LOCAL_CERTIFICATE := platform

LOCAL_AAPT_FLAGS := \
    --rename-manifest-package com.intel.amtl2 \
    --product amtl2

LOCAL_JNI_SHARED_LIBRARIES := libamtl_jni

LOCAL_PROGUARD_ENABLED := disabled

LOCAL_REQUIRED_MODULES := amtl_cfg

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call first-makefiles-under,$(LOCAL_PATH))
