ifeq ($(PRODUCT_NAME),)
PRODUCT_NAME := byt_t_ffrd8
endif

# Copy common product apns-conf
COMMON_PATH := device/intel/common
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/apns-conf.xml:system/etc/apns-conf.xml

# Include product path
include $(LOCAL_PATH)/byt_t_ffrd8_path.mk
-include $(COMMON_PLUS_PATH)/carrier-001/product.mk
-include $(COMMON_PLUS_PATH)/ims/product.mk

BLANK_FLASHFILES_CONFIG := $(DEVICE_CONF_PATH)/blankflashfiles.json

# IFWI
ifeq ($(TARGET_BIOS_TYPE),"uefi")
ifeq ($(BOARD_USE_64BIT_KERNEL),true)
PRODUCT_PACKAGES += \
    ifwi_uefi_byt_t_64_dediprog \
    ifwi_uefi_byt_t_64_capsule
else
PRODUCT_PACKAGES += \
    ifwi_uefi_byt_t_dediprog \
    ifwi_uefi_byt_t_capsule
endif
endif
# Dolby DS1
#-include vendor/intel/PRIVATE/dolby_ds1/dolbyds1.mk

# product specific overlays for Intel resources
PRODUCT_PACKAGE_OVERLAYS := $(FEATURE_PLUS_OVERLAY)
ifneq ($(BUILD_VANILLA_AOSP), true)
PRODUCT_PACKAGE_OVERLAYS += $(DEVICE_CONF_PATH)/overlays_extensions
endif
# product specific overlays for Vanilla AOSP resources
PRODUCT_PACKAGE_OVERLAYS += $(DEVICE_CONF_PATH)/overlays_aosp

#HDMISettings
PRODUCT_PACKAGE_OVERLAYS += $(COMMON_PATH)/overlays_hdmisettings

# copy permission files
FRAMEWORK_ETC_PATH := frameworks/native/data/etc
PERMISSIONS_PATH := system/etc/permissions

# Touchscreen configuration file
PRODUCT_COPY_FILES += \
    $(DEVICE_CONF_PATH)/maxtouch.cfg:system/etc/firmware/maxtouch.cfg \
    $(DEVICE_CONF_PATH)/maxtouch_1664S_8.fw:system/etc/firmware/maxtouch.fw

# Wi-Fi
 PRODUCT_COPY_FILES += \
    $(FRAMEWORK_ETC_PATH)/android.hardware.wifi.xml:$(PERMISSIONS_PATH)/android.hardware.wifi.xml
PRODUCT_COPY_FILES += \
    $(FRAMEWORK_ETC_PATH)/android.hardware.wifi.direct.xml:$(PERMISSIONS_PATH)/android.hardware.wifi.direct.xml

PRODUCT_PACKAGES += \
        wifi_bcm_4334x

# Copy sar manager resources
PRODUCT_COPY_FILES += \
    $(DEVICE_CONF_PATH)/sarconfig.xml:system/etc/sarconfig.xml

# Revert me to fg_config.bin instead of fg_config_$(TARGET_PRODUCT) once BZ119617 is resoved
#Fuel gauge related
PRODUCT_PACKAGES += \
       fg_conf fg_config.bin

#remote submix audio
PRODUCT_PACKAGES += \
       audio.r_submix.default

# bcu hal
PRODUCT_PACKAGES += \
    bcu.default

# rapid ril
PRODUCT_PACKAGES += \
    librapid-ril-core \
    librapid-ril-util

# Cell Broadcast
#PRODUCT_PACKAGES += \
    CellBroadcastReceiver

# parameter-framework files
PRODUCT_PACKAGES += \
        libimc-subsystem \
        parameter-framework.audio.byt_t_ffrd8 \
        parameter-framework.vibrator.byt_t_ffrd8 \
        parameter-framework.routeMgr.byt_t_ffrd8

# MAMGR (Modem Audio Manager)
PRODUCT_PACKAGES += \
    mamgr

# build the OMX wrapper codecs
#PRODUCT_PACKAGES += \
    libstagefright_soft_mp3dec_mdp \
    libstagefright_soft_aacdec_mdp

# NFC
#PRODUCT_PACKAGES += \
    nfc_pn544pc

PRODUCT_PROPERTY_OVERRIDES += \
    ro.nfc.se.uicc=false \
    ro.nfc.se.ese=false \
    ro.nfc.clk=xtal

#alsa conf
ALSA_CONF_PATH := external/alsa-lib/
PRODUCT_COPY_FILES += \
    $(ALSA_CONF_PATH)/src/conf/alsa.conf:system/usr/share/alsa/alsa.conf

# CMS configuration files
PRODUCT_COPY_FILES += \
    $(DEVICE_CONF_PATH)/cms_throttle_config.xml:system/etc/cms_throttle_config.xml \
    $(DEVICE_CONF_PATH)/cms_device_config.xml:system/etc/cms_device_config.xml

# Add component-testing applications
#PRODUCT_PACKAGES_ENG += mcd-test

PRODUCT_PACKAGES_DEBUG += \
    run_test_ipc.sh

# thermal config files
PRODUCT_COPY_FILES += \
         $(DEVICE_CONF_PATH)/thermal_sensor_config.xml:system/etc/thermal_sensor_config.xml \
         $(DEVICE_CONF_PATH)/thermal_throttle_config.xml:system/etc/thermal_throttle_config.xml

# crashlog conf
PRODUCT_COPY_FILES += \
         $(DEVICE_CONF_PATH)/crashlog.conf:system/etc/crashlog.conf
PRODUCT_COPY_FILES += \
         $(DEVICE_CONF_PATH)/ingredients.conf:system/etc/ingredients.conf

# Include base makefile
include $(LOCAL_PATH)/device.mk

