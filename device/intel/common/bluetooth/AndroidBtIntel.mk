LOCAL_PATH := $(my-dir)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := bt_intel
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    bt_vendor.conf \
    bt_common
# Note: additional modules for the Bluetooth firmware are
# appended in each product-specific makefile (victoriabay.mk etc)
#
# Still, bt_intel is kept instead of pointing directly to bt_common in
# $(PRODUCT_DEVICE).mk for the following reasons:
#  1) Placeholder for additional modules required by all Intel-based platforms
#  2) Symetrical code with others vendors
#  3) [most important] bt_intel is used in ComboChipVendor.mk to know if this is a Intel chip

# disable USC i=until new delivery
#ifeq ($(TARGET_BUILD_VARIANT),eng)
#LOCAL_REQUIRED_MODULES += libbtuscplugin
#endif

include $(BUILD_PHONY_PACKAGE)

# set this flag to true to use default AOSP bluedroid stack for WCS
USE_AOSP_BLUEDROID := true
# set this flag to true to use WCS specific Bluetooth apk
USE_SPECIFIC_BT_WCS := false
BOARD_HAVE_BLUETOOTH_INTEL := true

# Add intel ld driver
include linux/modules/drivers/misc/intel_ld/AndroidLd.mk

##################################################
