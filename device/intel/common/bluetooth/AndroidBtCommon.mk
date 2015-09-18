LOCAL_PATH := $(my-dir)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := bt_common
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    bd_prov \
    init.bt.rc \
    init.bt.vendor.rc \
    libbt-vendor \
    rfkill_bt.sh

include $(BUILD_PHONY_PACKAGE)

#################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.bt.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.bt.vendor.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := init.bt.$(COMBO_CHIP_VENDOR).rc
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := rfkill_bt.sh
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := rfkill_bt.sh
include $(BUILD_PREBUILT)

##################################################

#ifeq ($(COMBO_CHIP_VENDOR),bcm)
#LIBBT_CONF_PATH := hardware/broadcom/libbt/conf/intel
#include $(LIBBT_CONF_PATH)/Android.mk
#endif

