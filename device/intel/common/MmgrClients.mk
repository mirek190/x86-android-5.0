# rapid ril
# MAMGR (Modem Audio Manager)
# AT Proxy
# Modem Trace Server (MTS)
# Modem Manager
# NVM Server
# Modem Image Updater
# Modem files
PRODUCT_PACKAGES += \
    mdm_fw_pkg \
    librapid-ril-core \
    librapid-ril-util \
    mamgr \
    mmgr \
    libmodemupdate \
    nvm_server \
    proxy  \
    mts \
    init.class_main.sh

#OemTelephony for OEM HOOK API
PRODUCT_PACKAGES += \
    OemTelephonyApp

#SARManager
PRODUCT_PACKAGES += \
    SARManager

# AMTL : Android Modem Traces and Logs
# TelephonyEventsNotifier
ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
PRODUCT_PACKAGES += \
    Amtl \
    libamtl_jni \
    Amtl2   \
    libamtl2_jni \
    TelephonyEventsNotifier

PRODUCT_COPY_FILES += \
    $(DEVICE_CONF_PATH)/activate_trace_modem:system/bin/activate_trace_modem \
    $(DEVICE_CONF_PATH)/configure_trace_modem:system/bin/configure_trace_modem
endif

# Add component-testing applications
PRODUCT_PACKAGES_ENG += mcd-test

PRODUCT_PACKAGES_DEBUG += \
    run_test_ipc.sh \
    mmgr-debug \
    miu-app

# Modem init actions
PRODUCT_COPY_FILES += \
    $(DEVICE_CONF_PATH)/init.modem.rc:root/init.modem.rc

# Set default network type to LTE/GSM/WCDMA (9)
PRODUCT_PROPERTY_OVERRIDES += ro.telephony.default_network=9

# SIM Hot Swap Property
PRODUCT_PROPERTY_OVERRIDES += persist.tel.hot_swap.support=true
