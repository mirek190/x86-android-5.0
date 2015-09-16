VENDOR_PATH := ti

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
LOCAL_MODULE := gps_ti_wl128x
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    gps_common \
    wl128x_common \
    GPSCConfigFile.cfg \
    pathconfigfile.txt \
    GpsConfigFile.txt \
    SuplConfig.spl \
    PeriodicConfFile.cfg \
    client_keystore.bks \
    patch-X.0.ce \
    libgpsservices \
    libTIgps \
    libmcphalgps \
    libsuplhelperservicejni \
    libsupllocationprovider \
    navd \
    SUPLClient

ifeq ($(TARGET_BUILD_VARIANT),eng)
LOCAL_REQUIRED_MODULES +=  \
    aGps_Client
endif

include $(BUILD_PHONY_PACKAGE)

##################################################
