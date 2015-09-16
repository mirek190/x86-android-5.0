PRODUCT_NAME := byt_t_crv2_m2
REF_PRODUCT_NAME := byt_t_crv2

BOARD_USE_64BIT_KERNEL := true
BOARD_HAVE_MODEM := true

# Overwrite init.byt_t_crv2.rc with the m2 one
# containing telephony configuration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/config/init.byt_t_crv2_m2.rc:root/init.byt_t_crv2.rc

include $(LOCAL_PATH)/byt_t_crv2_path.mk

include $(LOCAL_PATH)/byt_t_crv2.mk
