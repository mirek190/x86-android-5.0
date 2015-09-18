include $(COMMON_PATH)/ComboChipVendor.mk

BOARD_WPA_SUPPLICANT_DRIVER    := NL80211
BOARD_HOSTAPD_PRIVATE_LIB      :=
BOARD_HOSTAPD_DRIVER           := NL80211

ifeq (ti,$(findstring ti,$(COMBO_CHIP_VENDOR)))
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WLAN_DEVICE := wl12xx-compat
endif

ifeq (bcm,$(findstring bcm,$(COMBO_CHIP_VENDOR)))
BOARD_WLAN_DEVICE := bcmdhd
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_PRIVATE_LIB   += lib_driver_cmd_bcmdhd
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcmdhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA      := "/etc/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP      := "/etc/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P      := "/etc/firmware/fw_bcmdhd.bin"
endif

ifeq (rtl,$(findstring rtl,$(COMBO_CHIP_VENDOR)))
BOARD_WLAN_DEVICE := rtl
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB += lib_driver_cmd_rtl
BOARD_HOSTAPD_PRIVATE_LIB   += lib_driver_cmd_rtl
endif

ifeq (marvell,$(findstring marvell,$(COMBO_CHIP_VENDOR)))
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB += lib_driver_cmd_mrvl
BOARD_HOSTAPD_PRIVATE_LIB   += lib_driver_cmd_mrvl
endif

ifeq (intel,$(findstring intel,$(COMBO_CHIP_VENDOR)))
BOARD_WLAN_DEVICE     := iwlwifi
#WPA_SUPPLICANT_VERSION := VER_0_8_X
WPA_SUPPLICANT_VERSION := VER_2_1_DEVEL_WCS
PRIVATE_WPA_SUPPLICANT_CONF := y

# Used to compile Driver
BOARD_USING_INTEL_IWL := true
INTEL_IWL_BOARD_CONFIG := mofd_lnp
INTEL_IWL_USE_COMPAT_INSTALL := y
INTEL_IWL_USE_RM_MAC_CFG := y
endif
