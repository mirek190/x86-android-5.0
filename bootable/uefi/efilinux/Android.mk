LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_HAVE_LIMITED_POWERON_FEATURES),true)
	OSLOADER_EM_POLICY ?= fake
endif

OSLOADER_EM_POLICY ?= uefi
EFILINUX_CFLAGS += -DOSLOADER_EM_POLICY_OPS=$(OSLOADER_EM_POLICY)_em_ops

EFILINUX_C_INCLUDES = \
	$(LOCAL_PATH)/android \
	$(LOCAL_PATH)/loaders \
	$(LOCAL_PATH)/security \
	$(LOCAL_PATH)/platform \
	$(LOCAL_PATH)/bzimage \
	$(LOCAL_PATH)/fs
EFILINUX_C_INCLUDES += $(call include-path-for, recovery)

EFILINUX_SRC_FILES := \
	esrt.c \
	config.c \
	entry.c \
	acpi.c \
	bootlogic.c \
	intel_partitions.c \
	uefi_osnib.c \
	platform/platform.c \
	platform/x86.c \
	platform/pmic.c \
	uefi_keys.c \
	uefi_boot.c \
	commands.c \
	em.c \
	fake_em.c \
	uefi_em.c \
	fs/fs.c \
	splash_intel.bmp

EFILINUX_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=12 --dirty --always)
EFILINUX_VERSION_DATE := $(shell cd $(LOCAL_PATH) ; git log --pretty=%cD HEAD^.. HEAD)
EFILINUX_CFLAGS +=  -DEFILINUX_VERSION_STRING='"$(EFILINUX_VERSION_STRING)"'
EFILINUX_CFLAGS +=  -DEFILINUX_VERSION_DATE='"$(EFILINUX_VERSION_DATE)"'
EFILINUX_CFLAGS +=  -DEFILINUX_BUILD_STRING='"$(BUILD_NUMBER) $(PRODUCT_NAME)"'

OSLOADER_FILE_PATH := EFI/BOOT/boot$(EFI_ARCH).efi
EFILINUX_CFLAGS +=  -DOSLOADER_FILE_PATH='L"$(OSLOADER_FILE_PATH)"'

WARMDUMP_FILE_PATH := EFI/Intel/warmdump.efi
EFILINUX_CFLAGS +=  -DWARMDUMP_FILE_PATH='L"$(WARMDUMP_FILE_PATH)"'

EFILINUX_CFLAGS +=  -DCONFIG_LOG_TAG='L"EFILINUX"'
EFILINUX_DEBUG_CFFLAGS := -DRUNTIME_SETTINGS -DCONFIG_LOG_LEVEL=LEVEL_DEBUG \
        -DCONFIG_LOG_FLUSH_TO_VARIABLE -DCONFIG_LOG_BUF_SIZE=51200 \
        -DCONFIG_LOG_TIMESTAMP -DCONFIG_ENABLE_FACTORY_MODES

ifeq ($(BOARD_DO_COLD_RESET_AFTER_KERNEL_WD_WARM_RESET),true)
	EFILINUX_CFLAGS += -DCONFIG_DO_COLD_RESET_AFTER_KERNEL_WD_WARM_RESET
ifeq ($(BOARD_USE_WARMDUMP),true)
	EFILINUX_DEBUG_CFFLAGS += -DCONFIG_HAS_WARMDUMP
endif
endif

EFILINUX_PROFILING_CFLAGS := -finstrument-functions -finstrument-functions-exclude-file-list=stack_chk.c,profiling.c,efilinux.h,stdlib.h,loaders/ -finstrument-functions-exclude-function-list=handover_kernel,checkpoint,exit_boot_services,setup_efi_memory_map,Print,SPrint,VSPrint,memory_map,stub_get_current_time_us,rdtsc,rdmsr
EFILINUX_PROFILING_SRC_FILES := profiling.c

EFILINUX_LIBRARIES := libuefi_log libuefi_utils libuefi_posix libuefi_cpu libuefi_time libuefi_stack_chk libuefi_bootimg libuefi_watchdog
################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := efilinux-user
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_CFLAGS := $(EFILINUX_CFLAGS) -DCONFIG_LOG_LEVEL=LEVEL_ERROR
LOCAL_SRC_FILES := $(EFILINUX_SRC_FILES)
LOCAL_C_INCLUDES := $(EFILINUX_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(EFILINUX_LIBRARIES) libuefi_profiling_stub
include $(BUILD_UEFI_EXECUTABLE)

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := efilinux-eng
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_CFLAGS += $(EFILINUX_CFLAGS) $(EFILINUX_DEBUG_CFFLAGS) $(EFILINUX_PROFILING_CFLAGS)
LOCAL_SRC_FILES := $(EFILINUX_SRC_FILES) $(EFILINUX_PROFILING_SRC_FILES)
LOCAL_C_INCLUDES := $(EFILINUX_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(EFILINUX_LIBRARIES)
include $(BUILD_UEFI_EXECUTABLE)

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := efilinux-userdebug
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_CFLAGS := $(EFILINUX_CFLAGS) $(EFILINUX_DEBUG_CFFLAGS) $(EFILINUX_PROFILING_CFLAGS)
LOCAL_SRC_FILES := $(EFILINUX_SRC_FILES) $(EFILINUX_PROFILING_SRC_FILES)
LOCAL_C_INCLUDES := $(EFILINUX_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(EFILINUX_LIBRARIES)
include $(BUILD_UEFI_EXECUTABLE)

################################################################################

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := bootlogic.h
LOCAL_COPY_HEADERS_TO := efilinux
include $(BUILD_COPY_HEADERS)

################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := warmdump
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/platform \
	$(LOCAL_PATH)/fs

LOCAL_SRC_FILES := \
	acpi.c \
	fs/fs.c \
	config.c \
	warmdump.c

LOCAL_STATIC_LIBRARIES := libuefi_log libuefi_utils libuefi_profiling_stub libuefi_stack_chk libuefi_posix
WARMDUMP_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=12 --dirty --always)
WARMDUMP_VERSION_DATE := $(shell cd $(LOCAL_PATH) ; git log --pretty=%cD HEAD^..HEAD)
LOCAL_CFLAGS := -DWARMDUMP_VERSION_STRING='L"$(WARMDUMP_VERSION_STRING)"'
LOCAL_CFLAGS += -DWARMDUMP_VERSION_DATE='L"$(WARMDUMP_VERSION_DATE)"'
LOCAL_CFLAGS += -DWARMDUMP_BUILD_STRING='L"$(BUILD_NUMBER) $(PRODUCT_NAME)"'
LOCAL_CFLAGS += -DCONFIG_LOG_TAG='L"WARMDUMP"' -DCONFIG_LOG_LEVEL=LEVEL_DEBUG

include $(BUILD_UEFI_EXECUTABLE)
