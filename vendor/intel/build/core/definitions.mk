ifeq ($(TARGET_KERNEL_SOURCE_IS_PRESENT),true)
# Rules to build the kernel
include $(COMMON_PATH)/AndroidKernel.mk
endif

# Additional rules for ifwi
# $(1) is the prefix of the module name, $(2) is source file
# $(3) is type ( dediprog, capsule, stage2 )

define ifwi_prebuild

include $(CLEAR_VARS)
LOCAL_MODULE := $(1)_$(3)
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := $(2)
LOCAL_MODULE_STEM := $(2)
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/ifwi/$(1)/$(3)
include $(BUILD_PREBUILT)

endef

# Additional rules for uefi apps
ifeq ($(TARGET_ARCH),x86)
    EFI_ARCH := ia32
    EFI_TARGET_LIBGCC := $(dir $(TARGET_LIBGCC))
endif

ifneq (,$(filter $(TARGET_ARCH),x86-64)$(filter $(BOARD_USE_64BIT_KERNEL),true))
    EFI_ARCH := x86_64
    EFI_TARGET_LIBGCC := $(patsubst %/32/,%,$(dir $(TARGET_LIBGCC)))
endif

EFI_ARCH_CFLAGS_ia32 := -m32 -DCONFIG_X86
EFI_ARCH_CFLAGS_x86_64 := -m64 -DCONFIG_X86_64
EFI_EXTRA_CFLAGS := $(EFI_ARCH_CFLAGS_$(EFI_ARCH)) -fPIC -fPIE \
    -fshort-wchar -ffreestanding -Wall -fstack-protector \
    -Wl,-z,noexecstack -O2 -D_FORTIFY_SOURCE=2 \
    -g -fno-merge-constants -DGNU_EFI_USE_MS_ABI

BUILD_UEFI_EXECUTABLE := bootable/uefi/uefi_executable.mk
