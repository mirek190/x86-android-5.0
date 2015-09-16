## Small ram device definition

ifeq ($(BOARD_HAVE_SMALL_RAM),true)
  NO_LIVEWALLPAPER := true
  SMALL_CODE_SIZE := true
  CAMERA_NO_REPOOL := true
  NO_MPM := true
endif

## Small and mid ram device definition

ifneq (,$(filter true,$(BOARD_HAVE_MID_RAM) $(BOARD_HAVE_SMALL_RAM)))
  BOARD_HAVE_COMPACTION := true
  BOARD_HAVE_KSM := true
  BOARD_HAVE_ZRAM := true
  LIMIT_READAHEAD := true
  ENABLE_PIC_PIE := true
  MALLOC_IMPL := dlmalloc
endif

## Mid ram device definition
ifeq ($(BOARD_HAVE_MID_RAM),true)
  CAMERA_REPOOL_8MP := true
endif

# KSM activation
ifeq ($(BOARD_HAVE_KSM),true)
  PRODUCT_COPY_FILES += $(COMMON_PATH)/memory/init.ksm.rc:root/init.ksm.rc
endif

# Zram introduction
ifeq ($(BOARD_HAVE_ZRAM),true)
  KERNEL_DIFFCONFIG += $(COMMON_PATH)/memory/zram_diffconfig
  OVERRIDE_PARTITION_FILE := $(LOCAL_PATH)/storage/part_mount_override_zram.json
  PRODUCT_COPY_FILES += \
                        $(COMMON_PATH)/memory/init.zram.rc:root/init.zram.rc \
                        $(COMMON_PATH)/memory/fstab.zram:root/fstab.zram
endif

# Kernel Compaction
ifeq ($(BOARD_HAVE_COMPACTION),true)
    KERNEL_DIFFCONFIG += $(COMMON_PATH)/memory/compaction_diffconfig
endif
