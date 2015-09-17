ifneq ($(strip $(TARGET_NO_BOOTLOADER)),true)
ifeq ($(TARGET_USE_SYSLINUX),true)

include $(all-subdir-makefiles)

SYSLINUX_BIN := $(HOST_OUT_EXECUTABLES)/syslinux
SYSLINUX_BASE := $(HOST_OUT)/usr/lib/syslinux

SYSLINUX_MK_IMG := external/syslinux/utils/android-image.sh

# TARGET_SYSLINUX_FILES - any splash screens, com32 modules
# TARGET_SYSLINUX_CONFIG - file to use as syslinux.cfg
# These should be defined in BoardConfig.mk per-product

ifdef TARGET_SYSLINUX_IMAGE_EXTRA_SPACE
	SYSLINUX_EXTRA_SPACE_PARAM := --extra-size $(TARGET_SYSLINUX_IMAGE_EXTRA_SPACE)
endif

$(PRODUCT_OUT)/bootloader: \
		$(TARGET_SYSLINUX_FILES) \
		$(TARGET_SYSLINUX_CONFIG) \
		$(SYSLINUX_BIN) \
		$(SYSLINUX_MK_IMG)
	$(call pretty, "Target SYSLINUX image: $@")
	$(SYSLINUX_MK_IMG) $(SYSLINUX_EXTRA_SPACE_PARAM) \
		--syslinux $(SYSLINUX_BIN) \
		--tmpdir $(call intermediates-dir-for,EXECUTABLES,syslinux-img)/syslinux-img \
		--config $(TARGET_SYSLINUX_CONFIG) \
		--output $@ \
		$(TARGET_SYSLINUX_FILES)

.PHONY: syslinux-image
syslinux-image: $(INSTALLED_BOOTLOADER_MODULE)

endif
endif
