NFC_ROOT_PATH := vendor/intel/hardware/nfc

### NXP PN544 ######

ifneq (,$(filter nfc_pn544,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
 BOARD_HAVE_NXP_PN544 := true
 NFC_CHIP_VENDOR := nxp
 NFC_CHIP := pn544

 PN544_CUSTOM_CONFIGS_PATH := $(NFC_ROOT_PATH)/$(NFC_CHIP_VENDOR)/$(NFC_CHIP)/conf

 ifneq (,$(wildcard $(PN544_CUSTOM_CONFIGS_PATH)/$(TARGET_DEVICE)/nfc_custom_config.h))
  TARGET_HAS_NFC_CUSTOM_CONFIG := true
 endif

# TARGET_VENDOR_ADAPTERADDON:= com.$(NFC_CHIP_VENDOR).nfc.adapteraddon
# PRODUCT_BOOT_JARS := $(PRODUCT_BOOT_JARS):com.intel.nfc.adapteraddon
endif

### NXP PN544PC ######

ifneq (,$(filter nfc_pn544pc,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
 BOARD_HAVE_NXP_PN544PC := true
 NFC_CHIP_VENDOR := nxp
 NFC_CHIP := pn544pc

 PN544_CUSTOM_CONFIGS_PATH := $(NFC_ROOT_PATH)/$(NFC_CHIP_VENDOR)/$(NFC_CHIP)/conf

 ifneq (,$(wildcard $(PN544_CUSTOM_CONFIGS_PATH)/$(TARGET_DEVICE)/nfc_custom_config.h))
  TARGET_HAS_NFC_CUSTOM_CONFIG := true
 endif

# TARGET_VENDOR_ADAPTERADDON:= com.$(NFC_CHIP_VENDOR).nfc.adapteraddon
# PRODUCT_BOOT_JARS := $(PRODUCT_BOOT_JARS):com.intel.nfc.adapteraddon
endif

### NXP PN547 ######

ifneq (,$(filter nfc_pn547,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
 BOARD_HAVE_NXP_PN547 := true
 NFC_CHIP_VENDOR := nxp
 NFC_CHIP := pn547

# TARGET_VENDOR_ADAPTERADDON:= com.$(NFC_CHIP_VENDOR).nfc.adapteraddon
# PRODUCT_BOOT_JARS := $(PRODUCT_BOOT_JARS):com.intel.nfc.adapteraddon
endif

### FDP ######

ifneq (,$(filter nfc_fdp,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
 BOARD_HAVE_INTEL_FDP := true
 NFC_CHIP_VENDOR := intel
 NFC_CHIP := fdp

# TARGET_VENDOR_ADAPTERADDON:= com.$(NFC_CHIP_VENDOR).nfc.adapteraddon
# PRODUCT_BOOT_JARS := $(PRODUCT_BOOT_JARS):com.intel.nfc.adapteraddon
endif
