
#check for compression level, to speed up build
FLASHFILE_COMPRESSION_LEVEL ?= 1
ZIP_COMP := -$(FLASHFILE_COMPRESSION_LEVEL)

# If neither TARGET_NO_KERNEL nor TARGET_NO_RECOVERY are true
ifeq ($(TARGET_MAKE_NO_DEFAULT_RECOVERY),true)
ifeq (,$(filter true, $(TARGET_NO_KERNEL) $(TARGET_NO_RECOVERY) $(BUILD_TINY_ANDROID)))

INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img

recovery_initrc := $(call get-specific-config-file ,recovery.init.rc)
recovery_aplogs_script := $(call get-specific-config-file ,retrieve_aplogs.sh)
recovery_kernel := $(INSTALLED_KERNEL_TARGET) # same as a non-recovery system
recovery_ramdisk := $(PRODUCT_OUT)/ramdisk-recovery.img
recovery_build_prop := $(INSTALLED_BUILD_PROP_TARGET)
recovery_binary := $(call intermediates-dir-for,EXECUTABLES,recovery)/recovery
recovery_thermal_rosd := $(call intermediates-dir-for,EXECUTABLES,thermald)/thermald
recovery_resources_common := $(call include-path-for, recovery)/res-xhdpi
recovery_resources_private := $(strip $(wildcard $(TARGET_DEVICE_DIR)/recovery/res))
recovery_resource_deps := $(shell find $(recovery_resources_common) \
  $(recovery_resources_private) -type f)
recovery_modules := \
	toolbox \
	libselinux \
	sh \
	strace \
	linker \
	libc \
	libcutils \
	liblog \
	libcrypto \
	libm \
	libstdc++ \
	libusbhost \
	teeprov

recovery_system_files := $(call module-installed-files,$(recovery_modules))
define recovery-copy-files
$(hide) $(foreach srcfile,$(recovery_system_files), \
	destfile=$(patsubst $(1)/%,$(2)/%,$(srcfile)); \
	mkdir -p `dirname $$destfile`; \
	cp -fR $(srcfile) $$destfile; \
)
endef

ifeq ($(recovery_resources_private),)
  $(info Intel Recovery: No private recovery resources for TARGET_DEVICE $(TARGET_DEVICE))
endif

INTERNAL_RECOVERYIMAGE_ARGS := \
	$(addprefix --second ,$(INSTALLED_2NDBOOTLOADER_TARGET)) \
	--kernel $(recovery_kernel) \
	--ramdisk $(recovery_ramdisk)

ifeq ($(TARGET_MAKE_NO_DEFAULT_BOOTIMAGE),true)
  INTERNAL_RECOVERYIMAGE_ARGS += --type recovery
endif

# Assumes this has already been stripped
ifdef BOARD_KERNEL_CMDLINE
  INTERNAL_RECOVERYIMAGE_ARGS += --cmdline "$(BOARD_KERNEL_CMDLINE)"
endif
ifdef BOARD_KERNEL_BASE
  INTERNAL_RECOVERYIMAGE_ARGS += --base $(BOARD_KERNEL_BASE)
endif
BOARD_KERNEL_PAGESIZE := $(strip $(BOARD_KERNEL_PAGESIZE))
ifdef BOARD_KERNEL_PAGESIZE
  INTERNAL_RECOVERYIMAGE_ARGS += --pagesize $(BOARD_KERNEL_PAGESIZE)
endif

# Keys authorized to sign OTA packages this build will accept.  The
# build always uses dev-keys for this; release packaging tools will
# substitute other keys for this one.
OTA_PUBLIC_KEYS := $(DEFAULT_SYSTEM_DEV_CERTIFICATE).x509.pem

# Generate a file containing the keys that will be read by the
# recovery binary.
RECOVERY_INSTALL_OTA_KEYS := \
	$(call intermediates-dir-for,PACKAGING,ota_keys)/keys
DUMPKEY_JAR := $(HOST_OUT_JAVA_LIBRARIES)/dumpkey.jar
$(RECOVERY_INSTALL_OTA_KEYS): PRIVATE_OTA_PUBLIC_KEYS := $(OTA_PUBLIC_KEYS)
$(RECOVERY_INSTALL_OTA_KEYS): extra_keys := $(patsubst %,%.x509.pem,$(PRODUCT_EXTRA_RECOVERY_KEYS))
$(RECOVERY_INSTALL_OTA_KEYS): $(OTA_PUBLIC_KEYS) $(DUMPKEY_JAR) $(extra_keys)
	@echo "DumpPublicKey: $@ <= $(PRIVATE_OTA_PUBLIC_KEYS) $(extra_keys)"
	@rm -rf $@
	@mkdir -p $(dir $@)
	java -jar $(DUMPKEY_JAR) $(PRIVATE_OTA_PUBLIC_KEYS) $(extra_keys) > $@

$(INSTALLED_RECOVERYIMAGE_TARGET): $(MKBOOTFS) $(MKBOOTIMG) $(MINIGZIP) \
		$(INSTALLED_RAMDISK_TARGET) \
		$(INSTALLED_BOOTIMAGE_TARGET) \
		$(recovery_binary) \
		$(recovery_thermal_rosd) \
		$(recovery_initrc) \
		$(recovery_kernel) \
		$(INSTALLED_2NDBOOTLOADER_TARGET) \
		$(recovery_build_prop) $(recovery_resource_deps) \
		$(recovery_system_files) \
		$(RECOVERY_INSTALL_OTA_KEYS) \
		isu \
		isu_wrapper
	@echo ----- Making recovery image ------
	rm -rf $(TARGET_RECOVERY_OUT)
	mkdir -p $(TARGET_RECOVERY_OUT)
	mkdir -p $(TARGET_RECOVERY_ROOT_OUT)
	mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/etc
	mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/tmp
	echo Copying baseline ramdisk...
	cp -R $(TARGET_ROOT_OUT) $(TARGET_RECOVERY_OUT)
	rm $(TARGET_RECOVERY_ROOT_OUT)/init*.rc
	cp $(TARGET_ROOT_OUT)/init.watchdog.rc $(TARGET_RECOVERY_OUT)/root/
	-cp $(TARGET_ROOT_OUT)/init.firmware.rc $(TARGET_RECOVERY_OUT)/root/
	echo Modifying ramdisk contents...
	PART_MOUNT_OUT_FILE=$(TARGET_RECOVERY_OUT)/root/fstab.$(TARGET_DEVICE) $(MKPARTITIONFILE)
	PART_MOUNT_OUT_FILE=$(TARGET_RECOVERY_OUT)/root/etc/recovery.fstab $(MKPARTITIONFILE)
	cp -f $(recovery_initrc) $(TARGET_RECOVERY_ROOT_OUT)/init.rc
	cp -f $(recovery_aplogs_script) $(TARGET_RECOVERY_ROOT_OUT)/etc/retrieve_aplogs.sh
	if [ -f $(RECOVERY_DEBUG_PATH)/init.recovery.debug.rc ]; then \
	cp -f $(RECOVERY_DEBUG_PATH)/init.recovery.debug.rc $(TARGET_RECOVERY_ROOT_OUT); \
	fi
	if [ -f $(DEVICE_CONF_PATH)/recovery.init.$(TARGET_DEVICE).rc ]; then \
	cp -f $(DEVICE_CONF_PATH)/recovery.init.$(TARGET_DEVICE).rc $(TARGET_RECOVERY_ROOT_OUT); \
	fi
	cp -f $(recovery_binary) $(TARGET_RECOVERY_ROOT_OUT)/sbin/
	cp -f $(recovery_thermal_rosd) $(TARGET_RECOVERY_ROOT_OUT)/sbin/
	$(hide) $(call recovery-copy-files,$(TARGET_OUT),$(TARGET_RECOVERY_ROOT_OUT)/system/)
	cp -rf $(recovery_resources_common)/* $(TARGET_RECOVERY_ROOT_OUT)/res/
	$(foreach item,$(recovery_resources_private), \
	  cp -rf $(item) $(TARGET_RECOVERY_ROOT_OUT)/)
	cp $(RECOVERY_INSTALL_OTA_KEYS) $(TARGET_RECOVERY_ROOT_OUT)/res/keys
	cat $(INSTALLED_DEFAULT_PROP_TARGET) $(recovery_build_prop) \
	        > $(TARGET_RECOVERY_ROOT_OUT)/default.prop
	$(MKBOOTFS) $(TARGET_RECOVERY_ROOT_OUT) | $(MINIGZIP) > $(recovery_ramdisk)
ifeq ($(TARGET_MAKE_NO_DEFAULT_BOOTIMAGE), true)
	LOCAL_SIGN=$(LOCAL_SIGN) $(MKBOOTIMG) $(COMMON_BOOTIMAGE_ARGS) $(INTERNAL_RECOVERYIMAGE_ARGS) --output $@ $(ADDITIONAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS)
else
	LOCAL_SIGN=$(LOCAL_SIGN) $(MKBOOTIMG) $(COMMON_BOOTIMAGE_ARGS) $(INTERNAL_RECOVERYIMAGE_ARGS) --output $@ $(BOARD_MKBOOTIMG_ARGS)
endif
	@echo ----- Made recovery image -------- $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
else
INSTALLED_RECOVERYIMAGE_TARGET :=
endif

PHONY: recoveryimage
recoveryimage: $(INSTALLED_RECOVERYIMAGE_TARGET)

endif # TARGET_MAKE_NO_DEFAULT_RECOVERY

ifeq ($(TARGET_MAKE_NO_DEFAULT_OTA_PACKAGE),true)

name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  name := $(name)_debug
endif
name := $(name)-target_files-$(FILE_NAME_TAG)

intermediates := $(call intermediates-dir-for,PACKAGING,target_files)
BUILT_TARGET_FILES_PACKAGE := $(intermediates)/$(name).zip
$(BUILT_TARGET_FILES_PACKAGE): intermediates := $(intermediates)
$(BUILT_TARGET_FILES_PACKAGE): \
		zip_root := $(intermediates)/$(name)

# $(1): Directory to copy
# $(2): Location to copy it to
# The "ls -A" is to prevent "acp s/* d" from failing if s is empty.
define package_files-copy-root
  if [ -d "$(strip $(1))" -a "$$(ls -A $(1))" ]; then \
    mkdir -p $(2) && \
    $(ACP) -rd $(strip $(1))/* $(2); \
  fi
endef

built_ota_tools := \
	$(call intermediates-dir-for,EXECUTABLES,applypatch)/applypatch \
	$(call intermediates-dir-for,EXECUTABLES,applypatch_static)/applypatch_static \
	$(call intermediates-dir-for,EXECUTABLES,check_prereq)/check_prereq \
	$(call intermediates-dir-for,EXECUTABLES,sqlite3)/sqlite3 \
	$(call intermediates-dir-for,EXECUTABLES,updater)/updater
$(BUILT_TARGET_FILES_PACKAGE): PRIVATE_OTA_TOOLS := $(built_ota_tools)

$(BUILT_TARGET_FILES_PACKAGE): PRIVATE_RECOVERY_API_VERSION := $(RECOVERY_API_VERSION)

ifeq ($(TARGET_RELEASETOOLS_EXTENSIONS),)
# default to common dir for device vendor
$(BUILT_TARGET_FILES_PACKAGE): tool_extensions := $(TARGET_DEVICE_DIR)/../common
else
$(BUILT_TARGET_FILES_PACKAGE): tool_extensions := $(TARGET_RELEASETOOLS_EXTENSIONS)
endif

$(BUILT_TARGET_FILES_PACKAGE): \
		$(INSTALLED_BOOTIMAGE_TARGET) \
		$(INSTALLED_RADIOIMAGE_TARGET) \
		$(INSTALLED_RECOVERYIMAGE_TARGET) \
		$(INSTALLED_DROIDBOOTIMAGE_TARGET) \
		$(INSTALLED_SYSTEMIMAGE) \
		$(INSTALLED_USERDATAIMAGE_TARGET) \
		$(INSTALLED_CACHEIMAGE_TARGET) \
		$(INSTALLED_ANDROID_INFO_TXT_TARGET) \
		$(INSTALLED_CAPSULE_TARGET) \
		$(built_ota_tools) \
		$(APKCERTS_FILE) \
		$(HOST_OUT_EXECUTABLES)/fs_config \
		firmware \
		$(INSTALLED_ESPIMAGE_TARGET) \
		| $(ACP)
	@echo "Package target files: $@"
	$(hide) rm -rf $@ $(zip_root)
	$(hide) mkdir -p $(dir $@) $(zip_root)
	@# Components of the recovery image
	$(hide) $(call package_files-copy-root, \
		$(TARGET_RECOVERY_ROOT_OUT),$(zip_root)/RECOVERY/RAMDISK)
ifdef INSTALLED_KERNEL_TARGET
	$(hide) $(ACP) $(INSTALLED_KERNEL_TARGET) $(zip_root)/RECOVERY/kernel
endif
ifdef INSTALLED_2NDBOOTLOADER_TARGET
	$(hide) $(ACP) \
		$(INSTALLED_2NDBOOTLOADER_TARGET) $(zip_root)/RECOVERY/second
endif
ifdef BOARD_KERNEL_CMDLINE
	$(hide) echo "$(BOARD_KERNEL_CMDLINE)" > $(zip_root)/RECOVERY/cmdline
endif
ifdef BOARD_KERNEL_BASE
	$(hide) echo "$(BOARD_KERNEL_BASE)" > $(zip_root)/RECOVERY/base
endif
ifdef BOARD_KERNEL_PAGESIZE
	$(hide) echo "$(BOARD_KERNEL_PAGESIZE)" > $(zip_root)/RECOVERY/pagesize
endif
ifeq ($(RECOVERY_DO_PARTITIONING),true)
	$(hide) $(ACP) $(PRODUCT_OUT)/partition.tbl $(zip_root)/RECOVERY/
endif
	$(hide) mkdir -p $(zip_root)/RECOVERY/RAMDISK/etc
	$(hide) $(ACP) $(PRODUCT_OUT)/recovery.img $(zip_root)/RECOVERY/
	$(hide) $(ACP) $(TARGET_RECOVERY_ROOT_OUT)/etc/recovery.fstab $(zip_root)/RECOVERY/RAMDISK/etc
	$(hide) $(ACP) -p $(TARGET_RECOVERY_ROOT_OUT)/etc/retrieve_aplogs.sh $(zip_root)/RECOVERY/RAMDISK/etc/retrieve_aplogs.sh
ifeq ($(TARGET_USE_DROIDBOOT),true)
	$(hide) $(ACP) $(PRODUCT_OUT)/droidboot.img $(zip_root)/RECOVERY/
endif
	@# Components of the boot image
	$(hide) $(call package_files-copy-root, \
		$(TARGET_ROOT_OUT),$(zip_root)/BOOT/RAMDISK)
	$(hide) $(ACP) $(INSTALLED_BOOTIMAGE_TARGET) $(zip_root)/BOOT/
ifdef INSTALLED_KERNEL_TARGET
	$(hide) $(ACP) $(INSTALLED_KERNEL_TARGET) $(zip_root)/BOOT/kernel
endif
ifdef INSTALLED_2NDBOOTLOADER_TARGET
	$(hide) $(ACP) \
		$(INSTALLED_2NDBOOTLOADER_TARGET) $(zip_root)/BOOT/second
endif
ifdef BOARD_KERNEL_CMDLINE
	$(hide) echo "$(BOARD_KERNEL_CMDLINE)" > $(zip_root)/BOOT/cmdline
endif
ifdef BOARD_KERNEL_BASE
	$(hide) echo "$(BOARD_KERNEL_BASE)" > $(zip_root)/BOOT/base
endif
ifdef BOARD_KERNEL_PAGESIZE
	$(hide) echo "$(BOARD_KERNEL_PAGESIZE)" > $(zip_root)/BOOT/pagesize
endif
	$(hide) $(foreach t,$(INSTALLED_RADIOIMAGE_TARGET),\
		mkdir -p $(zip_root)/RADIO; \
		$(ACP) $(t) $(zip_root)/RADIO/$(notdir $(t));)
	@# Contents of the system image
	$(hide) $(call package_files-copy-root, \
		$(SYSTEMIMAGE_SOURCE_DIR),$(zip_root)/SYSTEM)
	@# Contents of the data image
	$(hide) $(call package_files-copy-root, \
		$(TARGET_OUT_DATA),$(zip_root)/DATA)
	@# Extra contents of the OTA package
	$(hide) mkdir -p $(zip_root)/OTA/bin
	$(hide) $(ACP) $(INSTALLED_ANDROID_INFO_TXT_TARGET) $(zip_root)/OTA/
	$(hide) $(ACP) $(PRIVATE_OTA_TOOLS) $(zip_root)/OTA/bin/
	@# Components of the firmware
	$(hide) mkdir -p $(zip_root)/FIRMWARE
	$(hide) -find $(PRODUCT_OUT)/ifwi -type f -exec \
		zip -qj $(zip_root)/FIRMWARE/ifwi.zip {} \;
ifeq ($(BOARD_HAS_CAPSULE),true)
	@#Test IFWI_PREBUILT_PATHS to avoid issue with external build
ifneq (,$(wildcard $(IFWI_PREBUILT_PATHS)))
	$(hide) $(ACP) $(IFWI_PREBUILT_PATHS)/capsule.bin \
		$(zip_root)/FIRMWARE/capsule.bin
else
ifneq (,$(INSTALLED_CAPSULE_TARGET))
	$(hide) $(ACP) $(INSTALLED_CAPSULE_TARGET) \
		$(zip_root)/FIRMWARE/capsule.bin
endif
endif
endif

ifeq ($(BOARD_HAS_ULPMC),true)
	$(ACP) $(ULPMC_BINARY) $(zip_root)/FIRMWARE/ulpmc.bin
endif
ifneq ($(INSTALLED_ESPIMAGE_TARGET),)
	$(hide) $(ACP) $(ESPUPDATE_ZIP_TARGET) $(zip_root)/FIRMWARE
endif
ifeq ($(INTEL_FEATURE_SILENTLAKE),true)
	$(hide) $(ACP) $(PRODUCT_OUT)/sl_vmm.bin $(zip_root)/FIRMWARE/silentlake.img
endif
	@# Files that do not end up in any images, but are necessary to
	@# build them.
	$(hide) mkdir -p $(zip_root)/META
	$(hide) $(ACP) $(APKCERTS_FILE) $(zip_root)/META/apkcerts.txt
	$(hide)	echo "$(PRODUCT_OTA_PUBLIC_KEYS)" > $(zip_root)/META/otakeys.txt
	$(hide) echo "recovery_api_version=$(PRIVATE_RECOVERY_API_VERSION)" > $(zip_root)/META/misc_info.txt
	$(hide) echo "fstab_version=2" >> $(zip_root)/META/misc_info.txt
ifdef BOARD_FLASH_BLOCK_SIZE
	$(hide) echo "blocksize=$(BOARD_FLASH_BLOCK_SIZE)" >> $(zip_root)/META/misc_info.txt
endif
ifdef BOARD_BOOTIMAGE_PARTITION_SIZE
	$(hide) echo "boot_size=$(BOARD_BOOTIMAGE_PARTITION_SIZE)" >> $(zip_root)/META/misc_info.txt
endif
ifdef BOARD_RECOVERYIMAGE_PARTITION_SIZE
	$(hide) echo "recovery_size=$(BOARD_RECOVERYIMAGE_PARTITION_SIZE)" >> $(zip_root)/META/misc_info.txt
endif
	$(hide) echo "tool_extensions=$(tool_extensions)" >> $(zip_root)/META/misc_info.txt
	$(hide) echo "default_system_dev_certificate=$(DEFAULT_SYSTEM_DEV_CERTIFICATE)" >> $(zip_root)/META/misc_info.txt
ifdef PRODUCT_EXTRA_RECOVERY_KEYS
	$(hide) echo "extra_recovery_keys=$(PRODUCT_EXTRA_RECOVERY_KEYS)" >> $(zip_root)/META/misc_info.txt
endif
	$(hide) echo "intel_capsule=$(BOARD_HAS_CAPSULE)" >> $(zip_root)/META/misc_info.txt
	$(hide) echo "intel_ulpmc=$(BOARD_HAS_ULPMC)" >> $(zip_root)/META/misc_info.txt
ifeq ($(BUILD_WITH_SECURITY_FRAMEWORK),chaabi_token)
	$(hide) echo "intel_chaabi_token=true" >> $(zip_root)/META/misc_info.txt
else
	$(hide) echo "intel_chaabi_token=false" >> $(zip_root)/META/misc_info.txt
endif
	$(hide) echo "bios_type=$(TARGET_BIOS_TYPE)" >> $(zip_root)/META/misc_info.txt
	$(hide) echo "do_partitioning=$(RECOVERY_DO_PARTITIONING)" >> $(zip_root)/META/misc_info.txt
	$(hide) echo "has_silentlake=$(INTEL_FEATURE_SILENTLAKE)" >> $(zip_root)/META/misc_info.txt
	$(call generate-userimage-prop-dictionary, $(zip_root)/META/misc_info.txt)
	@# Zip everything up, preserving symlinks
	$(hide) (cd $(zip_root) && zip $(ZIP_COMP) -qry ../$(notdir $@) .)
	@# Run fs_config on all the system, boot ramdisk, and recovery ramdisk files in the zip, and save the output
	$(hide) zipinfo -1 $@ | awk 'BEGIN { FS="SYSTEM/" } /^SYSTEM\// {print "system/" $$2}' | $(HOST_OUT_EXECUTABLES)/fs_config > $(zip_root)/META/filesystem_config.txt
	$(hide) zipinfo -1 $@ | awk 'BEGIN { FS="BOOT/RAMDISK/" } /^BOOT\/RAMDISK\// {print $$2}' | $(HOST_OUT_EXECUTABLES)/fs_config > $(zip_root)/META/boot_filesystem_config.txt
	$(hide) zipinfo -1 $@ | awk 'BEGIN { FS="RECOVERY/RAMDISK/" } /^RECOVERY\/RAMDISK\// {print $$2}' | $(HOST_OUT_EXECUTABLES)/fs_config > $(zip_root)/META/recovery_filesystem_config.txt
	$(hide) (cd $(zip_root) && zip $(ZIP_COMP) -q ../$(notdir $@) META/*filesystem_config.txt)

target-files-package: $(BUILT_TARGET_FILES_PACKAGE)



ifneq ($(TARGET_PRODUCT),sdk)
ifeq ($(filter generic%,$(TARGET_DEVICE)),)
ifneq ($(TARGET_NO_KERNEL),true)

# -----------------------------------------------------------------
# OTA update package

name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  name := $(name)_debug
endif
name := $(name)-ota-$(FILE_NAME_TAG)

INTERNAL_OTA_PACKAGE_TARGET := $(PRODUCT_OUT)/$(name).zip
OTA_FROM_TARGET_FILES ?= ./build/tools/releasetools/ota_from_target_files

$(INTERNAL_OTA_PACKAGE_TARGET): KEY_CERT_PAIR := $(DEFAULT_KEY_CERT_PAIR)

$(INTERNAL_OTA_PACKAGE_TARGET): $(BUILT_TARGET_FILES_PACKAGE) $(DISTTOOLS) $(SELINUX_DEPENDS)
	@echo "Package OTA: $@"
	$(hide) $(OTA_FROM_TARGET_FILES) -v \
	   -p $(HOST_OUT) \
	   -k $(KEY_CERT_PAIR) \
	   --no_prereq \
	   $(EXTRA_OTA_GEN_OPTIONS) \
	   $(BUILT_TARGET_FILES_PACKAGE) $@

.PHONY: otapackage
otapackage: $(INTERNAL_OTA_PACKAGE_TARGET)

# -----------------------------------------------------------------
# The update package

name := $(TARGET_PRODUCT)
ifeq ($(TARGET_BUILD_TYPE),debug)
  name := $(name)_debug
endif
name := $(name)-img-$(FILE_NAME_TAG)

INTERNAL_UPDATE_PACKAGE_TARGET := $(PRODUCT_OUT)/$(name).zip

ifeq ($(TARGET_RELEASETOOLS_EXTENSIONS),)
# default to common dir for device vendor
$(INTERNAL_UPDATE_PACKAGE_TARGET): extensions := $(TARGET_DEVICE_DIR)/../common
else
$(INTERNAL_UPDATE_PACKAGE_TARGET): extensions := $(TARGET_RELEASETOOLS_EXTENSIONS)
endif

$(INTERNAL_UPDATE_PACKAGE_TARGET): $(BUILT_TARGET_FILES_PACKAGE) $(DISTTOOLS) $(SELINUX_DEPENDS)
	@echo "Package: $@"
	$(hide) ./build/tools/releasetools/img_from_target_files -v \
	   -s $(extensions) \
	   -p $(HOST_OUT) \
	   $(BUILT_TARGET_FILES_PACKAGE) $@

.PHONY: updatepackage
updatepackage: $(INTERNAL_UPDATE_PACKAGE_TARGET)

endif    # TARGET_NO_KERNEL != true
endif    # TARGET_DEVICE != generic*
endif    # TARGET_PRODUCT != sdk
endif # TARGET_MAKE_NO_DEFAULT_OTA_PACKAGE
