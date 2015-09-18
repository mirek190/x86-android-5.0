ifeq ($(BOARD_HAVE_MODEM),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := com.intel.internal.telephony.MmgrClient

# Only compile source java files in this apk.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := NVM_Config

LOCAL_CERTIFICATE := platform

LOCAL_JNI_SHARED_LIBRARIES := libnvmconfig_jni

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
