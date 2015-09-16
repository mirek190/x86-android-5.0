##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := fm_intel
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=  \
    fm_common

ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
LOCAL_REQUIRED_MODULES += fmr_test
endif

# Note: additional modules for the FM firmware are
# appended in each product-specific makefile
#
# Still, fm_intel is kept instead of pointing directly to fm_common in
# $(PRODUCT_DEVICE).mk for the following reasons:
#  1) Placeholder for additional modules required by all Intel-based platforms
#  2) Symetrical code with others vendors
#  3) [most important] fm_intel is used in ComboChipVendor.mk to know if this is a Intel chip

include $(BUILD_PHONY_PACKAGE)

# Add FMR V4L2 driver
include linux/modules/fm/PRIVATE/intel/lnp/fmr-v4l2/AndroidFmr.mk

##################################################
