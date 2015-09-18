LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# TODO Now that tag is optional, package selection should be added through PRODUCT_PACKAGES_DEBUG += ATProxy
#      tag should always be optional so that the module is not built for targets that do not need it (including non-Intel targets)
LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_JAVA_LIBRARIES := telephony-common

# Only compile source java files in this apk.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := ATProxy

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
#include $(call all-makefiles-under,$(LOCAL_PATH))
#endif
