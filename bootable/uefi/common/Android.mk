LOCAL_PATH := $(call my-dir)

define common_defs
$(strip \
    $(eval LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := libgnuefi) \
    $(eval LOCAL_CFLAGS += $(EFI_EXTRA_CFLAGS)) \
)
endef

include $(CLEAR_VARS)
LOCAL_SRC_FILES := log.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_utils libuefi_time
LOCAL_MODULE := libuefi_log
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := uefi_utils.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
LOCAL_MODULE := libuefi_utils
LOCAL_CFLAGS := -finstrument-functions
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/time
LOCAL_SRC_FILES := time/time.c time/time_silvermont.c
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_cpu
LOCAL_MODULE := libuefi_time
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/cpu
LOCAL_SRC_FILES := cpu/cpu.c
LOCAL_MODULE := libuefi_cpu
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := profiling_stub.c
LOCAL_MODULE := libuefi_profiling_stub
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := stack_chk.c
LOCAL_MODULE := libuefi_stack_chk
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := gpt/gpt.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/gpt
LOCAL_MODULE := libuefi_gpt
LOCAL_CFLAGS := -finstrument-functions
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_log libuefi_utils
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := watchdog/watchdog.c watchdog/tco_reset.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/watchdog
LOCAL_MODULE := libuefi_watchdog
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_log libuefi_utils
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := bootimg/bootimg.c bootimg/check_signature.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/bootimg
LOCAL_MODULE := libuefi_bootimg
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_log libuefi_posix

ifneq ($(BOARD_HAVE_LIMITED_POWERON_FEATURES),true)
ifneq (,$(findstring isu,$(TARGET_OS_SIGNING_METHOD)))
LOCAL_CFLAGS := -DUSE_INTEL_OS_VERIFICATION
endif
ifeq ($(TARGET_OS_SIGNING_METHOD),uefi)
LOCAL_CFLAGS := -DUSE_SHIM
endif
endif

$(call common_defs)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := posix/stdio.c posix/stdlib.c
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/posix
LOCAL_MODULE := libuefi_posix
LOCAL_CFLAGS := -finstrument-functions
LOCAL_WHOLE_STATIC_LIBRARIES := libuefi_utils
$(call common_defs)
include $(BUILD_STATIC_LIBRARY)
