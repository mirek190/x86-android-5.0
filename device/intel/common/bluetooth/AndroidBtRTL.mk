LOCAL_PATH := $(my-dir)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := bt_rtl
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    bt_vendor.conf \
    bt_common
# Note: additional modules for the Bluetooth firmware are
# appended in each product-specific makefile (victoriabay.mk etc)
#
# Still, bt_rtl is kept instead of pointing directly to bt_common in
# $(PRODUCT_DEVICE).mk for the following reasons:
#  1) Placeholder for additional modules required by all RTL-based platforms
#  2) Symetrical code with others vendors
#  3) [most important] bt_rtl is used in ComboChipVendor.mk to know if this is a RTL chip

USE_AOSP_BLUEDROID := true
BOARD_HAVE_BLUETOOTH_RTK := true
RTK_BLUETOOTH_INTERFACE := uart
BLUETOOTH_BLUEDROID_RTK := true
BLUETOOTH_HCI_USE_RTK_H5 := true

include $(BUILD_PHONY_PACKAGE)

##################################################
