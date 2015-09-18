# This makefile is included from vendor/intel/common/wifi/WifiRules.mk.
.PHONY: build_wl12xx-compat
TI_SRC_DIR := $(call my-dir)
TI_BUILD_DIR := $(PRODUCT_OUT)/wl12xx-compat

build_wl12xx-compat: build_bzImage $(PREV_MODINSTALL)
	@echo Building kernel module $@
	@rm -rf $(TI_BUILD_DIR)
	$(KERNEL_BLD_ENV) $(MAKE) -C $(TI_SRC_DIR) ARCH=$(KERNEL_ARCH) $(KERNEL_EXTRA_FLAGS) KLIB=$(ANDROID_BUILD_TOP)/$(TI_BUILD_DIR) KLIB_BUILD=$(ANDROID_BUILD_TOP)/$(KERNEL_OUT_DIR) KERNEL_MODINSTALL=$(KERNEL_MODINSTALL) KMODDIR=kernel install-modules

copy_modules_to_root: build_wl12xx-compat

PREV_MOD_INSTALL := build_wl12xx-compat

