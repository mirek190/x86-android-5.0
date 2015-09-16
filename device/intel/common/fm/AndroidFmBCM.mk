##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := fm_bcm
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    fm_common \
    brcm-uim-sysfs

# Note: additional modules for the FM firmware are
# appended in each product-specific makefile
#
# Still, fm_bcm is kept instead of pointing directly to fm_common in
# $(PRODUCT_DEVICE).mk for the following reasons:
#  1) Placeholder for additional modules required by all BCM-based platforms
#  2) Symetrical code with others vendors
#  3) [most important] fm_bcm is used in ComboChipVendor.mk to know if this is a BCM chip

include $(BUILD_PHONY_PACKAGE)

# Add drivers
include linux/modules/PRIVATE/fm/broadcom/line_discipline_driver/AndroidLd.mk
include linux/modules/PRIVATE/fm/broadcom/v4l2_fm_driver/AndroidV4l2.mk
include linux/modules/PRIVATE/fm/broadcom/bt_protocol_driver/AndroidBt.mk

##################################################
