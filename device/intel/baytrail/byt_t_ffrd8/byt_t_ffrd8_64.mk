PRODUCT_NAME := byt_t_ffrd8_64
REF_PRODUCT_NAME := byt_t_ffrd8

BOARD_USE_64BIT_KERNEL := true

include $(LOCAL_PATH)/has_uefi.mk

include $(LOCAL_PATH)/byt_t_ffrd8_path.mk

include $(LOCAL_PATH)/byt_t_ffrd8.mk
