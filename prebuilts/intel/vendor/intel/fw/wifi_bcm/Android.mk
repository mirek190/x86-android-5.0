LOCAL_PATH := $(call my-dir)
ifeq ($(strip $(BOARD_HAVE_WIFI)),true)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := wifi_bcm
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    wlan_prov.bcm          \
    lib_driver_cmd_bcmdhd 

ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
LOCAL_REQUIRED_MODULES +=   \
     wlx
endif

# WARNING: To be kept as the last required module.
LOCAL_REQUIRED_MODULES +=  \
    wifi_common
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := wlx
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SYSTEM
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################

#include $(call first-makefiles-under,$(LOCAL_PATH))

endif

