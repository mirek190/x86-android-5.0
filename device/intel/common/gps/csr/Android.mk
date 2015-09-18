VENDOR_PATH := csr

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.gps.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
#LOCAL_PREBUILT_MODULE_FILE := $(PLATFORM_CONF_PATH)/gps/$(GPS_CHIP_VENDOR)/init.gps.rc
LOCAL_SRC_FILES := $(VENDOR_PATH)/init.gps.rc
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_csr_cpdaemon
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    init.gps.cpd.rc \
    cpdd
ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_REQUIRED_MODULES +=  \
    cpd
endif
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.gps.cpd.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := $(VENDOR_PATH)/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_csr_gsd4t
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps_common \
    gpscerd \
    gps.conf
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_csr_gsd4t_cpd
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps_common \
    gpscerd \
    gps.conf \
    gps_csr_cpdaemon
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_csr_gsd5t
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps_common \
    gpscerd \
    gps.conf
include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps_csr_gsd5t_cpd
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps_common \
    gpscerd \
    gps.conf \
    gps_csr_cpdaemon
include $(BUILD_PHONY_PACKAGE)

##################################################
