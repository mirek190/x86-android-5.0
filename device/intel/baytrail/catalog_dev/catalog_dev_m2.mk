PRODUCT_NAME := catalog_dev_m2
REF_PRODUCT_NAME := byt_t_crv2

BOARD_USE_64BIT_KERNEL := true
BOARD_HAVE_MODEM := true

# Overwrite init.catalog_dev.rc with the m2 one
# containing telephony configuration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/config/init.catalog_dev_m2.rc:root/init.catalog_dev.rc

include $(LOCAL_PATH)/catalog_dev_path.mk

include $(LOCAL_PATH)/catalog_dev.mk
