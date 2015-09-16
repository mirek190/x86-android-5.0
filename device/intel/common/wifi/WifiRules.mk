####################################
# Build configuration
####################################

USE_MOST_RESTRICTIVE_REGDOM=true

####################################
# Paths declaration
####################################

COMMON_WIFI_DIR := $(my-dir)

####################################
# Configuration filenames
####################################

STA_CONF_FILE_NAME      = wpa_supplicant.conf
P2P_CONF_FILE_NAME      = p2p_supplicant.conf
HOSTAPD_CONF_FILE_NAME  = hostapd.conf

####################################
# Files path on target
####################################

CONF_PATH_ON_TARGET = system/etc/wifi
STA_CONF_PATH_ON_TARGET       = $(CONF_PATH_ON_TARGET)/$(STA_CONF_FILE_NAME)
P2P_CONF_PATH_ON_TARGET       = $(CONF_PATH_ON_TARGET)/$(P2P_CONF_FILE_NAME)
HOSTAPD_CONF_PATH_ON_TARGET   = $(CONF_PATH_ON_TARGET)/$(HOSTAPD_CONF_FILE_NAME)

####################################
# Manufacturer
####################################

BCM43xx_BASEDIR:=linux/modules/wlan/broadcom/bcm43xx
define include-bcm-wifi-src-or-prebuilt
  BCM43xx_CHIP:=$(1)
  ifneq ($(wildcard $(BCM43xx_BASEDIR)),$(empty))
    include $(BCM43xx_BASEDIR)/AndroidBcm.mk
  else
    include $(call intel-prebuilts-path,$(BCM43xx_BASEDIR))/AndroidBcm.mk
  endif
endef

ifneq (,$(filter wifi_ti,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  include hardware/ti/wlan/wl12xx-compat/AndroidWl12xxCompat.mk
endif

ifneq (,$(filter wifi_bcm_43241,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,43241))
endif

ifneq (,$(filter wifi_bcm_4334,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,4334))
endif

ifneq (,$(filter wifi_bcm_4335 wifi_bcm_4339,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,4335))
endif

ifneq (,$(filter wifi_bcm_4334x,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,4334x))
endif

ifneq (,$(filter wifi_bcm_4354,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,4354))
endif

ifneq (,$(filter wifi_bcm_43430,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,43430))
endif

ifneq (,$(filter wifi_bcm_4330f,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,4330f))
endif

ifneq (,$(filter wifi_bcm_43362,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  $(eval $(call include-bcm-wifi-src-or-prebuilt,43362))
endif

ifneq (,$(filter wifi_intel_%,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  -include linux/modules/wlan/PRIVATE/iwlwifi/iwl-stack-dev/Android.mk
endif

ifneq (,$(filter wifi_rtl_8723,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  -include linux/modules/wlan/realtek/AndroidRtl8723bs.mk
endif

ifneq (,$(filter wifi_rtl_8189,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  -include linux/modules/wlan/realtek/rtl8189/AndroidRtl8189es.mk
endif

ifneq (,$(filter marvell88w8777,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
  -include $(TOP)/linux/modules/wlan/marvell/88W8777/AndroidMrvl8777.mk
  -include $(TOP)/linux/modules/bt/marvell/88W8777/AndroidMrvl8777.mk
endif

####################################
# Configuration files
####################################
ifneq (,$(COMBO_CHIP_VENDOR))

# common configuration files
STA_CONF_FILES      += $(COMMON_WIFI_DIR)/$(STA_CONF_FILE_NAME)
P2P_CONF_FILES      += $(COMMON_WIFI_DIR)/$(P2P_CONF_FILE_NAME)
HOSTAPD_CONF_FILES  += $(COMMON_WIFI_DIR)/$(HOSTAPD_CONF_FILE_NAME)

# manufacturer specific files
STA_CONF_FILES      += $(COMMON_WIFI_DIR)/$(COMBO_CHIP_VENDOR)_specific/$(STA_CONF_FILE_NAME)
P2P_CONF_FILES      += $(COMMON_WIFI_DIR)/$(COMBO_CHIP_VENDOR)_specific/$(P2P_CONF_FILE_NAME)
HOSTAPD_CONF_FILES  += $(COMMON_WIFI_DIR)/$(COMBO_CHIP_VENDOR)_specific/$(HOSTAPD_CONF_FILE_NAME)

####################################
# Locale variables
####################################

ifeq ($(USE_MOST_RESTRICTIVE_REGDOM),true)
REGDOM=00
else
REGDOM=$(word 2,$(subst _, ,$(word 1,$(PRODUCT_LOCALES))))
endif
MV=mv
CAT=cat
SED=sed
MKDIR=mkdir
GREP=grep

####################################
# Functions
####################################

define set_def_regdom
	$(SED) 's/country=XY/country=$(REGDOM)/g' $1 > $1.tmp
	$(MV) $1.tmp $1
endef

define clean_conf_file
	$(CAT) $1 | $(GREP) -v "(todel)" > $1.tmp
	$(MV) $1.tmp $1
endef

endif

##################################################

include $(CLEAR_VARS)
ifneq (,$(filter wifi_bcm_%,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
LOCAL_MODULE := wpa_supplicant_overlay.conf
else
LOCAL_MODULE := wpa_supplicant.conf
endif
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/wifi
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(STA_CONF_FILES)
	@echo "Building $@"
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $(STA_CONF_FILES) > $@
	$(hide) $(call set_def_regdom,$@)
	$(hide) $(call clean_conf_file,$@)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := p2p_supplicant.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/wifi
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(P2P_CONF_FILES)
	@echo "Building $@"
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $(P2P_CONF_FILES) > $@
	$(hide) $(call set_def_regdom,$@)
	$(hide) $(call clean_conf_file,$@)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := hostapd.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/wifi
include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(HOSTAPD_CONF_FILES)
	@echo "Building $@"
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $(HOSTAPD_CONF_FILES) > $@

##################################################
