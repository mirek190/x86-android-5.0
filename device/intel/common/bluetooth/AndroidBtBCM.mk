LOCAL_PATH := $(my-dir)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := bt_bcm
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    bt_common \
    bt_bcm_conf

ifeq ($(BLUEDROID_ENABLE_V4L2),true)
LOCAL_REQUIRED_MODULES += \
    fm_bcm
endif

# Note: additional modules for the Bluetooth firmware are
# appended in each product-specific makefile (victoriabay.mk etc)
#
# Still, bt_bcm is kept instead of pointing directly to bt_common in
# $(PRODUCT_DEVICE).mk for the following reasons:
#  1) Placeholder for additional modules required by all BRCM-based platforms
#  2) Symetrical code with others vendors
#  3) [most important] bt_bcm is used in ComboChipVendor.mk to know if this is a BRCM chip

USE_AOSP_BLUEDROID := true
BOARD_HAVE_BLUETOOTH_BCM := true

include $(BUILD_PHONY_PACKAGE)

##################################################

include $(CLEAR_VARS)

LOCAL_MODULE := bt_bcm_conf
LOCAL_SRC_FILES := bt_vendor.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/bluetooth
LOCAL_MODULE_STEM := $(LOCAL_SRC_FILES)

include $(BUILD_PREBUILT)
##################################################
