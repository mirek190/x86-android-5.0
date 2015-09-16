# make file for Baytrail
#
INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_OUT)/droidboot.img
INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_OUT)/esp.zip

include device/intel/common/AndroidBoard.mk

# Add socwatchdk driver
-include linux/modules/debug_tools/socwatchdk/src/AndroidSOCWatchDK.mk
-include vendor/intel/tools/PRIVATE/debug_internal_tools/socwatchdk/src/AndroidSOCWatchDK.mk

# Add SocPerf driver
-include vendor/intel/tools/PRIVATE/debug_internal_tools/socperfdk/driver/src/AndroidSOCPERF.mk

# Add LM driver
-include vendor/intel/tools/PRIVATE/debug_internal_tools/lmdk/AndroidLMDK.mk

# Add SAT module
-include vendor/intel/tools/PRIVATE/debug_internal_tools/sat/kernel-module/AndroidSAT.mk

# Add ioaccess driver for PETS
ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
include linux/modules/ioaccess/AndroidIOA.mk
endif

# wifi
ifeq ($(strip $(BOARD_HAVE_WIFI)),true)
include $(DEVICE_CONF_PATH)/wifi/WifiRules.mk
endif

.PHONY: images firmware $(TARGET_PRODUCT)

$(TARGET_SYSTEM): droid
# Legacy target - same as 'make images'
$(TARGET_PRODUCT): images

images: firmware bootimage $(TARGET_SYSTEM) recoveryimage
ifeq ($(TARGET_USE_DROIDBOOT),true)
images: droidbootimage
endif

blank_flashfiles: recoveryimage
