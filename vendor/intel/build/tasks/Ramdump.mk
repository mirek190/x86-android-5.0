# ramdump image
ifeq ($(TARGET_USE_RAMDUMP),true)
TARGET_RAMDUMP_OUT := $(PRODUCT_OUT)/ramdump
TARGET_RAMDUMP_ROOT_OUT := $(TARGET_RAMDUMP_OUT)/root
RAMDUMP_PATH := $(TOP)/bootable/ramdump

INSTALLED_RAMDUMPIMAGE_TARGET := $(PRODUCT_OUT)/ramdump.img
INSTALLED_KERNEL_RAMDUMP_TARGET := $(PRODUCT_OUT)/kdumpbzImage

ramdump_initrc := $(call get-specific-config-file ,ramdump.init.rc)
ramdump_kernel := $(INSTALLED_KERNEL_RAMDUMP_TARGET) # same as kexec
ramdump_ramdisk := $(PRODUCT_OUT)/ramdisk-ramdump.img
ramdump_build_prop := $(INSTALLED_BUILD_PROP_TARGET)
droidboot_binary := $(call intermediates-dir-for,EXECUTABLES,droidboot)/droidboot
ramdump_logcat := $(call intermediates-dir-for,EXECUTABLES,logcat)/logcat
ramdump_netcat := $(call intermediates-dir-for,EXECUTABLES,nc)/nc
ramdump_thermald := $(call intermediates-dir-for,EXECUTABLES,thermald)/thermald
droidboot_resources_common := $(DROIDBOOT_PATH)/res
ramdump_bootstub := $(PRODUCT_OUT)/bootstub_ramdump

ramdump_modules := \
	libc \
	libcutils \
	libdl \
	liblog \
	libcrypto \
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
	gzip \
	kexec \
	ramdump

ifneq ($(call intel-target-need-intel-libraries),)
ramdump_modules += libimf libintlc libsvml
endif

ramdump_system_files := $(call module-installed-files,$(ramdump_modules))

# $(1): source base dir
# $(2): target base dir
define ramdump-copy-files
$(hide) $(foreach srcfile,$(ramdump_system_files), \
	destfile=$(patsubst $(1)/%,$(2)/%,$(srcfile)); \
	mkdir -p `dirname $$destfile`; \
	cp -fR $(srcfile) $$destfile; \
)
endef

# Base address hardcoded in system/core/mkbootimg/mkbootimg.c is 0x1000_0000
# second bootloader offset is relative to this one: 0x1000_0000 + 0x6800_0000
# => bootstub is expected to be located at 0x7800_0000
INTERNAL_RAMDUMPIMAGE_ARGS := \
	$(addprefix --second ,$(ramdump_bootstub)) \
	--kernel $(ramdump_kernel) \
	--ramdisk $(ramdump_ramdisk) \
	--base 0x78000000

ifeq ($(TARGET_MAKE_NO_DEFAULT_BOOTIMAGE),true)
	# Allow board to overwrite the type used for droid boot image
	ifdef RAMDUMP_OS_TYPE
		INTERNAL_RAMDUMPIMAGE_ARGS += --type $(RAMDUMP_OS_TYPE)
	else
		INTERNAL_RAMDUMPIMAGE_ARGS += --type ramdump
	endif
endif


# Assumes this has already been stripped
ifdef BOARD_KERNEL_CMDLINE
  INTERNAL_RAMDUMPIMAGE_ARGS += --cmdline "$(BOARD_KERNEL_CMDLINE) $(BOARD_KERNEL_RAMDUMP_EXTRA_CMDLINE)"
endif
ifdef BOARD_KERNEL_BASE
  INTERNAL_RAMDUMPIMAGE_ARGS += --base $(BOARD_KERNEL_BASE)
endif
BOARD_KERNEL_PAGESIZE := $(strip $(BOARD_KERNEL_PAGESIZE))
ifdef BOARD_KERNEL_PAGESIZE
  INTERNAL_RAMDUMPIMAGE_ARGS += --pagesize $(BOARD_KERNEL_PAGESIZE)
endif

$(INSTALLED_KERNEL_RAMDUMP_TARGET): build_bzImage_kdump

$(INSTALLED_RAMDUMPIMAGE_TARGET): $(MKBOOTFS) $(MKBOOTIMG) $(MINIGZIP)\
		$(INSTALLED_RAMDISK_TARGET) \
		$(INSTALLED_BOOTIMAGE_TARGET) \
		$(ramdump_system_files) \
		$(ramdump_binary) \
		$(ramdump_bootstub) \
		$(ramdump_initrc) \
		$(ramdump_kernel) \
		$(ramdump_logcat) \
		$(ramdump_netcat) \
		$(ramdump_thermald) \
		$(ramdump_build_prop) \
		$(PRODUCT_OUT)/partition.tbl \
		isu \
		isu_wrapper
	@echo ----- Making ramdump image ------
	rm -rf $(TARGET_RAMDUMP_OUT)
	mkdir -p $(TARGET_RAMDUMP_OUT)
	mkdir -p $(TARGET_RAMDUMP_ROOT_OUT)
	mkdir -p $(TARGET_RAMDUMP_ROOT_OUT)/tmp
	mkdir -p $(TARGET_RAMDUMP_ROOT_OUT)/system/etc
	mkdir -p $(TARGET_RAMDUMP_ROOT_OUT)/system/bin
	mkdir -p $(TARGET_RAMDUMP_ROOT_OUT)/mnt/sdcard
	echo Copying baseline ramdisk...
	cp -R $(TARGET_ROOT_OUT) $(TARGET_RAMDUMP_OUT)
	rm $(TARGET_RAMDUMP_ROOT_OUT)/init*.rc
	cp $(TARGET_ROOT_OUT)/init.watchdog.rc $(TARGET_RAMDUMP_OUT)/root/
	-cp $(TARGET_ROOT_OUT)/init.firmware.rc $(TARGET_RAMDUMP_OUT)/root/
	echo Modifying ramdisk contents...
	PART_MOUNT_OUT_FILE=$(TARGET_RAMDUMP_OUT)/root/fstab.$(TARGET_DEVICE) $(MKPARTITIONFILE)
	PART_MOUNT_OUT_FILE=$(TARGET_RAMDUMP_OUT)/root/system/etc/recovery.fstab $(MKPARTITIONFILE)
	cp -f $(PRODUCT_OUT)/partition.tbl $(TARGET_RAMDUMP_ROOT_OUT)/system/etc/
	cp -f $(ramdump_initrc) $(TARGET_RAMDUMP_ROOT_OUT)/init.rc
	if [ -f $(DEVICE_CONF_PATH)/ramdump.init.$(TARGET_DEVICE).rc ]; then \
	cp -f $(DEVICE_CONF_PATH)/ramdump.init.$(TARGET_DEVICE).rc $(TARGET_RAMDUMP_ROOT_OUT); \
	fi
	cp -f $(droidboot_binary) $(TARGET_RAMDUMP_ROOT_OUT)/system/bin/
	cp -f $(ramdump_logcat) $(TARGET_RAMDUMP_ROOT_OUT)/system/bin/logcat
	cp -f $(ramdump_netcat) $(TARGET_RAMDUMP_ROOT_OUT)/system/bin/nc
	cp -f $(ramdump_thermald) $(TARGET_RAMDUMP_ROOT_OUT)/sbin/
	cp -rf $(droidboot_resources_common) $(TARGET_RAMDUMP_ROOT_OUT)/
	cat $(INSTALLED_DEFAULT_PROP_TARGET) $(ramdump_build_prop) \
	        > $(TARGET_RAMDUMP_ROOT_OUT)/default.prop
	$(hide) $(call ramdump-copy-files,$(TARGET_OUT),$(TARGET_RAMDUMP_ROOT_OUT)/system/)
	$(MKBOOTFS) $(TARGET_RAMDUMP_ROOT_OUT) | $(MINIGZIP) > $(ramdump_ramdisk)
ifeq ($(TARGET_MAKE_NO_DEFAULT_BOOTIMAGE), true)
	LOCAL_SIGN=$(LOCAL_SIGN) $(MKBOOTIMG) $(RAMDUMP_BOOTIMAGE_ARGS) $(INTERNAL_RAMDUMPIMAGE_ARGS) --output $@ $(ADDITIONAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS)
else
	LOCAL_SIGN=$(LOCAL_SIGN) $(MKBOOTIMG) $(RAMDUMP_BOOTIMAGE_ARGS) $(INTERNAL_RAMDUMPIMAGE_ARGS) --output $@ $(BOARD_MKBOOTIMG_ARGS)
endif
	@echo ----- Made ramdump image -------- $@

else
INSTALLED_RAMDUMPIMAGE_TARGET :=
endif

.PHONY: ramdumpimage
ramdumpimage: $(INSTALLED_DROIDBOOTIMAGE_TARGET) $(INSTALLED_RAMDUMPIMAGE_TARGET)
