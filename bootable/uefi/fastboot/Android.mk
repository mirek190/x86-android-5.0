LOCAL_PATH := $(call my-dir)

FASTBOOT_SRC_FILES := \
    main.c \
    flash.c \
    sparse.c \
    fastboot.c \
    fastboot_usb.c


FASTBOOT_VERSION_STRING := $(shell cd $(LOCAL_PATH) ; git describe --abbrev=12 --dirty --always)
FASTBOOT_VERSION_DATE := $(shell cd $(LOCAL_PATH) ; git log --pretty=%cD HEAD^.. HEAD)
FASTBOOT_CFLAGS +=  -DFASTBOOT_VERSION_STRING='"$(FASTBOOT_VERSION_STRING)"'
FASTBOOT_CFLAGS +=  -DFASTBOOT_VERSION_DATE='"$(FASTBOOT_VERSION_DATE)"'
FASTBOOT_CFLAGS +=  -DFASTBOOT_BUILD_STRING='"$(BUILD_NUMBER) $(PRODUCT_NAME)"'
FASTBOOT_CFLAGS +=  -DCONFIG_LOG_TAG='L"FASTBOOT"'

FASTBOOT_DEBUG_CFFLAGS := -DCONFIG_LOG_LEVEL=LEVEL_DEBUG -DCONFIG_LOG_TIMESTAMP

FASTBOOT_LIBRARIES := libuefi_log libuefi_utils libuefi_profiling_stub libuefi_stack_chk libuefi_gpt libuefi_bootimg libuefi_posix libuefi_watchdog

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := fastboot-user
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_CFLAGS += $(FASTBOOT_CFLAGS) -DCONFIG_LOG_LEVEL=LEVEL_ERROR
LOCAL_SRC_FILES := $(FASTBOOT_SRC_FILES)
LOCAL_C_INCLUDES := $(FASTBOOT_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(FASTBOOT_LIBRARIES)
include $(BUILD_UEFI_EXECUTABLE)

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := fastboot-eng
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_CFLAGS += $(FASTBOOT_CFLAGS) $(FASTBOOT_DEBUG_CFFLAGS)
LOCAL_SRC_FILES := $(FASTBOOT_SRC_FILES)
LOCAL_C_INCLUDES := $(FASTBOOT_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(FASTBOOT_LIBRARIES)
include $(BUILD_UEFI_EXECUTABLE)

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := fastboot-userdebug
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)
LOCAL_CFLAGS += $(FASTBOOT_CFLAGS) $(FASTBOOT_DEBUG_CFFLAGS)
LOCAL_SRC_FILES := $(FASTBOOT_SRC_FILES)
LOCAL_C_INCLUDES := $(FASTBOOT_C_INCLUDES)
LOCAL_STATIC_LIBRARIES := $(FASTBOOT_LIBRARIES)
include $(BUILD_UEFI_EXECUTABLE)
