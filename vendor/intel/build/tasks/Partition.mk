ifeq ($(TARGET_ARCH),x86)
ifneq ($(TARGET_PRODUCT),sdk)
ifeq ($(filter generic%,$(TARGET_DEVICE)),)
DEFAULT_PARTITION := $(TOP)/device/intel/common/storage/default_partition.json
DEFAULT_MOUNT := $(TOP)/device/intel/common/storage/default_mount.json

ifneq ($(OVERRIDE_PARTITION_FILE),)
PART_MOUNT_OVERRIDE_FILE := $(OVERRIDE_PARTITION_FILE)
else
PART_MOUNT_OVERRIDE_FILE := $(call get-specific-config-file ,storage/part_mount_override.json)
endif

ifeq ($(BIGCORE_USB_INSTALLER),true)
PART_MOUNT_OVERRIDE_FILE := $(call get-specific-config-file ,storage/part_mount_override_usbboot.json)
endif
PART_MOUNT_OVERRIDE_FILES := $(call get-all-config-files ,storage/part_mount_override.json)
PART_MOUNT_OUT := $(PRODUCT_OUT)

TARGET_PARTITION_TABLE := $(PRODUCT_OUT)/partition.tbl

MKPARTITIONFILE:= \
	$(TOP)/vendor/intel/support/partition.py \
	$(DEFAULT_PARTITION) \
	$(DEFAULT_MOUNT) \
	$(PART_MOUNT_OVERRIDE_FILE) \
	"$(PART_MOUNT_OUT)" \
	"$(TARGET_DEVICE)"

# partition table for fastboot os
$(TARGET_PARTITION_TABLE): $(DEFAULT_PARTITION) $(DEFAULT_MOUNT) $(PART_MOUNT_OVERRIDE_FILES)
	$(hide)mkdir -p $(dir $@)
	PART_MOUNT_OUT_FILE=$@	$(MKPARTITIONFILE)

# android main fstab
$(PRODUCT_OUT)/root/fstab.$(TARGET_DEVICE): $(DEFAULT_PARTITION) $(DEFAULT_MOUNT) $(PART_MOUNT_OVERRIDE_FILES)
	$(hide)mkdir -p $(dir $@)
	PART_MOUNT_OUT_FILE=$@	$(MKPARTITIONFILE)

$(PRODUCT_OUT)/root/fstab: $(PRODUCT_OUT)/root/fstab.$(TARGET_DEVICE)
	$(hide)cp $< $@

# android charger fstab
$(PRODUCT_OUT)/root/fstab.charger.$(TARGET_DEVICE): $(DEFAULT_PARTITION) $(DEFAULT_MOUNT) $(PART_MOUNT_OVERRIDE_FILES)
	$(hide)mkdir -p $(dir $@)
	PART_MOUNT_OUT_FILE=$@	$(MKPARTITIONFILE)

# android ramconsole fstab
$(PRODUCT_OUT)/root/fstab.ramconsole.$(TARGET_DEVICE): $(DEFAULT_PARTITION) $(DEFAULT_MOUNT) $(PART_MOUNT_OVERRIDE_FILES)
	$(hide)mkdir -p $(dir $@)
	PART_MOUNT_OUT_FILE=$@	$(MKPARTITIONFILE)

$(BUILT_RAMDISK_TARGET): \
	$(PRODUCT_OUT)/root/fstab.$(TARGET_DEVICE) \
	$(PRODUCT_OUT)/root/fstab \
	$(PRODUCT_OUT)/root/fstab.charger.$(TARGET_DEVICE) \
	$(PRODUCT_OUT)/root/fstab.ramconsole.$(TARGET_DEVICE)

blank_flashfiles: $(TARGET_PARTITION_TABLE)
endif    # TARGET_DEVICE != generic*
endif    # TARGET_PRODUCT != sdk
endif    # TARGET_ARCH := x86
