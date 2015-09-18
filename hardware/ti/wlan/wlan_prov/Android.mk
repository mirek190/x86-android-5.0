ifeq ($(strip $(BOARD_WLAN_DEVICE)),wl12xx-compat)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BUILD_WITH_CHAABI_SUPPORT),true)
LOCAL_C_INCLUDES = \
	$(TARGET_OUT_HEADERS)/chaabi
endif

LOCAL_SRC_FILES:= \
	wlan_provisioning.c

LOCAL_CFLAGS:=

ifeq ($(BUILD_WITH_CHAABI_SUPPORT),true)
LOCAL_CFLAGS += -DBUILD_WITH_CHAABI_SUPPORT
LOCAL_STATIC_LIBRARIES := \
	CC6_UMIP_ACCESS CC6_ALL_BASIC_LIB
endif

ifeq ($(REF_DEVICE_NAME),mfld_gi)
LOCAL_CFLAGS += -DSINGLE_BAND
endif

LOCAL_SHARED_LIBRARIES := \
	libc libcutils libhardware_legacy libcrypto

LOCAL_MODULE:= wlan_prov.ti
LOCAL_MODULE_STEM := wlan_prov
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

NVS_FILE = wl1271-nvs.bin

SYMLINKS := $(addprefix $(PRODUCT_OUT)/system/etc/firmware/ti-connectivity/,$(NVS_FILE))
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
	@echo "Symlink: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(addprefix /config/wifi/,$(NVS_FILE)) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
	$(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)

endif
