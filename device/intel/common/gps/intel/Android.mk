VENDOR_PATH := intel

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.gps.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := $(VENDOR_PATH)/init.gps.rc
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_intel_$(GPS_CHIP)
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps_common \
    gnssCG2000 \
    gnssStelp \
    libgnssconf \
    libgnsspatch

include $(BUILD_PHONY_PACKAGE)

##################################################
