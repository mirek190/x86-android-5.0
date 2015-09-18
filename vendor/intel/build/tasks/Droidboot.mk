# droidboot image
ifeq ($(TARGET_USE_DROIDBOOT),true)
TARGET_DROIDBOOT_OUT := $(PRODUCT_OUT)/droidboot
TARGET_DROIDBOOT_ROOT_OUT := $(TARGET_DROIDBOOT_OUT)/root
DROIDBOOT_PATH := $(TOP)/bootable/droidboot

INSTALLED_DROIDBOOTIMAGE_TARGET := $(PRODUCT_OUT)/droidboot.img
INSTALLED_DROIDBOOTIMAGE_DNX_TARGET := $(PRODUCT_OUT)/droidboot_dnx.img

DNX_MKBOOTIMG := vendor/intel/support/

droidboot_initrc := $(call get-specific-config-file ,droidboot.init.rc)
droidboot_kernel := $(INSTALLED_KERNEL_TARGET) # same as a non-recovery system
droidboot_ramdisk := $(PRODUCT_OUT)/ramdisk-droidboot.img
droidboot_build_prop := $(INSTALLED_BUILD_PROP_TARGET)
droidboot_binary := $(call intermediates-dir-for,EXECUTABLES,droidboot)/droidboot
droidboot_logcat := $(call intermediates-dir-for,EXECUTABLES,logcat)/logcat
droidboot_thermald := $(call intermediates-dir-for,EXECUTABLES,thermald)/thermald
droidboot_resources_common := $(DROIDBOOT_PATH)/res
droidboot_bootstub := $(PRODUCT_OUT)/bootstub

droidboot_modules := \
	libc \
	libcutils \
	libdl \
	liblog \
	libcrypto \
	libext2_quota \
	libm \
	libstdc++ \
	linker \
	sh \
	systembinsh \
	toolbox \
	libdiskconfig \
	libext2fs \
	libext2_com_err \
	libext2_e2p \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext4_utils \
	libselinux \
	libsparse \
	libusbhost \
	libz \
	resize2fs \
	tune2fs \
	e2fsck \
	gzip \
	kexec \
	droidboot

droidboot_modules += \
	FPT \
	fparts.txt \
	TXEManuf \
	TXEManuf.cfg \
	vsccommn.bin \
	FWUpdLcl \
    TXEInfo \
    TXEI_SEC_TOOLS


ifeq ($(TARGET_BOARD_PLATFORM), clovertrail)
droidboot_pvrsrvctl := $(PRODUCT_OUT)/system/vendor/bin/pvrsrvctl

droidboot_modules += \
	libdrm \
	libhardware \
	libsrv_init \
	libsrv_um \
	pvrsrvctl \
	shisp_css15.bin
endif

ifneq ($(call intel-target-need-intel-libraries),)
droidboot_modules += libimf libintlc libsvml
endif

droidboot_system_files := $(call module-installed-files,$(droidboot_modules))

# $(1): source base dir
# $(2): target base dir
define droidboot-copy-files
$(hide) $(foreach srcfile,$(droidboot_system_files), \
	destfile=$(patsubst $(1)/%,$(2)/%,$(srcfile)); \
	mkdir -p `dirname $$destfile`; \
	cp -fR $(srcfile) $$destfile; \
)
endef


INTERNAL_DROIDBOOTIMAGE_ARGS := \
	$(addprefix --second ,$(INSTALLED_2NDBOOTLOADER_TARGET)) \
	--kernel $(droidboot_kernel) \
	--ramdisk $(droidboot_ramdisk)

ifeq ($(TARGET_MAKE_NO_DEFAULT_BOOTIMAGE),true)
	# Allow board to overwrite the type used for droid boot image
	ifdef DROIDBOOT_OS_TYPE
		INTERNAL_DROIDBOOTIMAGE_ARGS += --type $(DROIDBOOT_OS_TYPE)
	else
		INTERNAL_DROIDBOOTIMAGE_ARGS += --type droidboot
	endif
endif

# Assumes this has already been stripped
ifdef BOARD_KERNEL_CMDLINE
  INTERNAL_DROIDBOOTIMAGE_ARGS += --cmdline "$(BOARD_KERNEL_CMDLINE) $(BOARD_KERNEL_DROIDBOOT_EXTRA_CMDLINE)"
endif
ifdef BOARD_KERNEL_BASE
  INTERNAL_DROIDBOOTIMAGE_ARGS += --base $(BOARD_KERNEL_BASE)
endif
BOARD_KERNEL_PAGESIZE := $(strip $(BOARD_KERNEL_PAGESIZE))
ifdef BOARD_KERNEL_PAGESIZE
  INTERNAL_DROIDBOOTIMAGE_ARGS += --pagesize $(BOARD_KERNEL_PAGESIZE)
endif

$(INSTALLED_DROIDBOOTIMAGE_TARGET): $(MKBOOTFS) $(MKBOOTIMG) $(MINIGZIP)\
		$(INSTALLED_RAMDISK_TARGET) \
		$(INSTALLED_BOOTIMAGE_TARGET) \
		$(droidboot_system_files) \
		$(droidboot_binary) \
		$(droidboot_bootstub) \
		$(droidboot_initrc) \
		$(droidboot_kernel) \
		$(droidboot_logcat) \
		$(droidboot_thermald) \
		$(droidboot_build_prop) \
		$(PRODUCT_OUT)/partition.tbl \
		isu \
		isu_wrapper
	@echo ----- Making droidboot image ------
	rm -rf $(TARGET_DROIDBOOT_OUT)
	mkdir -p $(TARGET_DROIDBOOT_OUT)
	mkdir -p $(TARGET_DROIDBOOT_ROOT_OUT)
	mkdir -p $(TARGET_DROIDBOOT_ROOT_OUT)/tmp
	mkdir -p $(TARGET_DROIDBOOT_ROOT_OUT)/system/etc
	mkdir -p $(TARGET_DROIDBOOT_ROOT_OUT)/system/bin
	mkdir -p $(TARGET_DROIDBOOT_ROOT_OUT)/mnt/sdcard
	echo Copying baseline ramdisk...
	cp -R $(TARGET_ROOT_OUT) $(TARGET_DROIDBOOT_OUT)
	rm $(TARGET_DROIDBOOT_ROOT_OUT)/init*.rc
	cp $(TARGET_ROOT_OUT)/init.watchdog.rc $(TARGET_DROIDBOOT_OUT)/root/
	-cp $(TARGET_ROOT_OUT)/init.firmware.rc $(TARGET_DROIDBOOT_OUT)/root/
	echo Modifying ramdisk contents...
	PART_MOUNT_OUT_FILE=$(TARGET_DROIDBOOT_OUT)/root/fstab.$(TARGET_DEVICE) $(MKPARTITIONFILE)
	PART_MOUNT_OUT_FILE=$(TARGET_DROIDBOOT_OUT)/root/system/etc/recovery.fstab $(MKPARTITIONFILE)
	cp -f $(PRODUCT_OUT)/partition.tbl $(TARGET_DROIDBOOT_ROOT_OUT)/system/etc/
	cp -f $(droidboot_initrc) $(TARGET_DROIDBOOT_ROOT_OUT)/init.rc
	if [ -f $(DROIDBOOT_DEBUG_PATH)/init.droidboot.debug.rc ]; then \
	cp -f $(DROIDBOOT_DEBUG_PATH)/init.droidboot.debug.rc $(TARGET_DROIDBOOT_ROOT_OUT); \
	fi
	if [ -f $(DEVICE_CONF_PATH)/droidboot.init.$(TARGET_DEVICE).rc ]; then \
	cp -f $(DEVICE_CONF_PATH)/droidboot.init.$(TARGET_DEVICE).rc $(TARGET_DROIDBOOT_ROOT_OUT); \
	fi
	cp -f $(droidboot_binary) $(TARGET_DROIDBOOT_ROOT_OUT)/system/bin/
	cp -f $(droidboot_logcat) $(TARGET_DROIDBOOT_ROOT_OUT)/system/bin/logcat
	cp -f $(droidboot_thermald) $(TARGET_DROIDBOOT_ROOT_OUT)/sbin/
ifeq ($(TARGET_BOARD_PLATFORM), clovertrail)
	cp -f $(droidboot_pvrsrvctl) $(TARGET_DROIDBOOT_ROOT_OUT)/system/bin/
endif
	cp -rf $(droidboot_resources_common) $(TARGET_DROIDBOOT_ROOT_OUT)/
	cat $(INSTALLED_DEFAULT_PROP_TARGET) $(droidboot_build_prop) \
	        > $(TARGET_DROIDBOOT_ROOT_OUT)/default.prop
	cp -rf $(TARGET_OUT)/bin/fpt-tools-package/ $(TARGET_DROIDBOOT_ROOT_OUT)/system/bin/fpt/
	$(hide) $(call droidboot-copy-files,$(TARGET_OUT),$(TARGET_DROIDBOOT_ROOT_OUT)/system/)
	$(MKBOOTFS) $(TARGET_DROIDBOOT_ROOT_OUT) | $(MINIGZIP) > $(droidboot_ramdisk)
ifeq ($(TARGET_MAKE_NO_DEFAULT_BOOTIMAGE), true)
	LOCAL_SIGN=$(LOCAL_SIGN) $(MKBOOTIMG) $(COMMON_BOOTIMAGE_ARGS) $(INTERNAL_DROIDBOOTIMAGE_ARGS) --output $@ $(ADDITIONAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS)
else
	LOCAL_SIGN=$(LOCAL_SIGN) $(MKBOOTIMG) $(COMMON_BOOTIMAGE_ARGS) $(INTERNAL_DROIDBOOTIMAGE_ARGS) --output $@ $(BOARD_MKBOOTIMG_ARGS)
endif
	@echo ----- Made droidboot image -------- $@

ifeq ($(TARGET_MAKE_OSIP_DNX_IMAGE), true)
$(INSTALLED_DROIDBOOTIMAGE_DNX_TARGET): $(INSTALLED_DROIDBOOTIMAGE_TARGET)
	# Generate OSIP stitched droidboot img version
	LOCAL_SIGN=$(LOCAL_SIGN) $(DNX_MKBOOTIMG)/mkbootimg $(COMMON_BOOTIMAGE_ARGS) $(INTERNAL_DROIDBOOTIMAGE_ARGS) --output $@ $(ADDITIONAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --bootstub $(droidboot_bootstub) --type droidboot
	@echo ----- Made droidboot_dnx image --------  $@
else
INSTALLED_DROIDBOOTIMAGE_DNX_TARGET :=
endif
else
INSTALLED_DROIDBOOTIMAGE_TARGET :=
INSTALLED_DROIDBOOTIMAGE_DNX_TARGET :=
endif

.PHONY: droidbootimage
droidbootimage: $(INSTALLED_DROIDBOOTIMAGE_TARGET) $(INSTALLED_DROIDBOOTIMAGE_DNX_TARGET)
