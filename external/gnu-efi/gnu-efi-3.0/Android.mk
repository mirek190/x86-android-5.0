LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),x86)
	EFI_ARCH := ia32
	SPECIFIC_GNU_EFI_SRC := ""
endif

ifneq (,$(filter $(TARGET_ARCH),x86-64)$(filter $(BOARD_USE_64BIT_KERNEL),true))
	EFI_ARCH := x86_64
	SPECIFIC_GNU_EFI_SRC := \
		lib/$(EFI_ARCH)/callwrap.c \
		lib/$(EFI_ARCH)/efi_stub.S
	LOCAL_CFLAGS := -m64 -ffreestanding -D$(EFI_ARCH) \
		-I prebuilts/gcc/$(HOST_PREBUILT_TAG)/host/$(EFI_ARCH)-$(HOST_OS)-glibc2.7-4.6/sysroot/usr/include/
	LOCAL_ASFLAGS := -m64
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/lib \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/inc/protocol \
	$(LOCAL_PATH)/inc/$(EFI_ARCH)

LOCAL_EXPORT_C_INCLUDE_DIRS := \
	$(LOCAL_PATH)/lib \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/inc/protocol \
	$(LOCAL_PATH)/inc/$(EFI_ARCH)

COMMON_GNU_EFI_SRC := \
	lib/boxdraw.c \
	lib/console.c \
	lib/crc.c \
	lib/data.c \
	lib/debug.c \
	lib/dpath.c \
	lib/error.c \
	lib/event.c \
	lib/guid.c \
	lib/hand.c \
	lib/hw.c \
	lib/init.c \
	lib/lock.c \
	lib/misc.c \
	lib/print.c \
	lib/smbios.c \
	lib/sread.c \
	lib/str.c \
	lib/runtime/efirtlib.c \
	lib/runtime/rtdata.c \
	lib/runtime/rtlock.c \
	lib/runtime/rtstr.c \
	lib/runtime/vm.c  \
	lib/$(EFI_ARCH)/initplat.c \
	lib/$(EFI_ARCH)/math.c \
	gnuefi/reloc_$(EFI_ARCH).c \
	gnuefi/crt0-efi-$(EFI_ARCH).S

LOCAL_CFLAGS += -Wall -O2 -D_FORTIFY_SOURCE=2 -Wl,-z,noexecstack -fstack-protector \
		-fPIE -fPIC -fno-stack-protector -fshort-wchar

LOCAL_SRC_FILES := \
	$(COMMON_GNU_EFI_SRC) \
	$(SPECIFIC_GNU_EFI_SRC)

LOCAL_MODULE := libgnuefi
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE := elf_$(EFI_ARCH)_efi.lds
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := gnuefi/elf_$(EFI_ARCH)_efi.lds
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

include $(BUILD_PREBUILT)
