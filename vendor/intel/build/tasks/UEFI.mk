INTERNAL_ESPIMAGE_FILES := $(filter $(TARGET_ROOT_OUT)/esp/%, \
    $(ALL_PREBUILT) \
    $(ALL_COPIED_HEADERS) \
    $(ALL_GENERATED_SOURCES) \
    $(ALL_DEFAULT_INSTALLED_MODULES))

BUILD_ESPIMAGE_DIR := $(PRODUCT_OUT)/esp

$(BUILD_ESPIMAGE_DIR):
	$(hide) mkdir -p $(BUILD_ESPIMAGE_DIR)

ifeq ($(TARGET_ARCH),x86)
    EFI_BOOT_FILE_NAME := bootia32.efi
endif
ifneq (,$(filter $(TARGET_ARCH),x86-64)$(filter $(BOARD_USE_64BIT_KERNEL),true))
   EFI_BOOT_FILE_NAME := bootx64.efi
endif

EFILINUX_MODULE := efilinux-$(TARGET_BUILD_VARIANT)
INSTALLED_EFILINUX_MODULE := $(PRODUCT_OUT)/$(EFILINUX_MODULE).efi

$(INSTALLED_ESPIMAGE_TARGET): $(BUILD_ESPIMAGE_DIR) $(EFILINUX_MODULE) warmdump | $(HOST_OUT_EXECUTABLES)/mcopy $(HOST_OUT_EXECUTABLES)/mkdosfs
	$(call pretty,"Target ESP image: $@")
	$(hide) mkdir -p $(PRODUCT_OUT)/esp/EFI/BOOT $(PRODUCT_OUT)/esp/EFI/Intel $(PRODUCT_OUT)/esp/EFI/Intel/Data
	$(hide) $(ACP) $(INSTALLED_EFILINUX_MODULE) $(PRODUCT_OUT)/esp/EFI/BOOT/$(EFI_BOOT_FILE_NAME)
	$(hide) $(ACP) $(INSTALLED_EFILINUX_MODULE) $(PRODUCT_OUT)/esp/EFI/Intel/efilinux.efi
	$(hide) $(ACP) $(PRODUCT_OUT)/warmdump.efi $(PRODUCT_OUT)/esp/EFI/Intel/

	$(hide) vendor/intel/support/make_vfatfs "ESP" \
		$(INSTALLED_ESPIMAGE_TARGET) \
		$(shell python -c 'import json; print json.load(open("'$(PART_MOUNT_OVERRIDE_FILE)'"))["partitions"]["ESP"]["size"]') \
		$(PRODUCT_OUT)/esp

$(ESPUPDATE_ZIP_TARGET): $(INSTALLED_ESPIMAGE_TARGET)
	$(hide) rm -f $@
	$(hide) cd $(BUILD_ESPIMAGE_DIR) && zip -r $(abspath $@) *

espimage: $(INSTALLED_ESPIMAGE_TARGET) $(ESPUPDATE_ZIP_TARGET)
