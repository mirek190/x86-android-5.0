LOCAL_PATH := $(call my-dir)

LIBCHAABI := $(TOP)/vendor/intel/hardware/PRIVATE/chaabi
ifeq ($(wildcard $(LIBCHAABI)),)
	external_release := yes
else
	external_release := no
endif

include $(CLEAR_VARS)

common_pmdb_files := \
	pmdb-access-sep.c \
	pmdb.c

ifeq ($(BUILD_WITH_SECURITY_FRAMEWORK),chaabi_token)
token_implementation := \
	tee_connector.c \
	token.c
else
token_implementation := \
	token.c
endif

CONFIG_INTELPROV_EDK2 := y
CONFIG_INTELPROV_FDK := y
CONFIG_INTELPROV_GPT := y
CONFIG_INTELPROV_OSIP := y
CONFIG_INTELPROV_SCU_EMMC := y
CONFIG_INTELPROV_SCU_IPC := y
CONFIG_INTELPROV_ULPMC := y

MODULES-SOURCES :=
INTELPROV_CONFIGS := $(filter CONFIG_INTELPROV_%,$(.VARIABLES))
INTELPROV_DEFINES := $(foreach v,$(INTELPROV_CONFIGS),$(filter %y,-D$(v)=$($(v))))
libintelprov-modules := $(shell find $(LOCAL_PATH) -name "intelprov.mk")
LOCAL_CFLAGS += $(INTELPROV_DEFINES)
include $(libintelprov-modules)

common_libintelprov_files := \
	update_osip.c \
	fw_version_check.c \
	util.c \
	flash_ops.c \
	flash.c \
	$(MODULES-SOURCES)

common_libintelprov_includes := \
	$(call include-path-for, libc-private) \
	$(call include-path-for, mkbootimg) \

chaabi_dir := $(TOP)/vendor/intel/hardware/PRIVATE/chaabi
sep_lib_includes := $(chaabi_dir)/SepMW/VOS6/External/Linux/inc/

# Provisionning CC54 tool
ifeq ($(BUILD_WITH_SECURITY_FRAMEWORK),chaabi_token)
include $(CLEAR_VARS)
LOCAL_MODULE := teeprov
LOCAL_SRC_FILES := teeprov.c tee_connector.c util.c
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(common_libintelprov_includes) bootable/recovery
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter
LOCAL_STATIC_LIBRARIES := libdx_cc7_static
LOCAL_SHARED_LIBRARIES := libc liblog libifp libkeymaster
include $(BUILD_EXECUTABLE)
endif

# Partitionning library
include $(CLEAR_VARS)
LOCAL_MODULE := liboempartitioning_static
LOCAL_SRC_FILES := oem_partition.c
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := bootable/droidboot/volumeutils $(LOCAL_PATH)/gpt/lib/include
LOCAL_WHOLE_STATIC_LIBRARIES := libcgpt_static
include $(BUILD_STATIC_LIBRARY)

# Plug-in library for AOSP updater
include $(CLEAR_VARS)
LOCAL_MODULE := libintel_updater
LOCAL_SRC_FILES := updater.c $(common_libintelprov_files)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_C_INCLUDES := $(call include-path-for, recovery) $(common_libintelprov_includes) $(LOCAL_PATH)/gpt/lib/include
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter
LOCAL_WHOLE_STATIC_LIBRARIES := liboempartitioning_static
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
  LOCAL_CFLAGS += -DCLVT
endif
ifeq ($(BUILD_WITH_SECURITY_FRAMEWORK),chaabi_token)
LOCAL_SRC_FILES += tee_connector.c
LOCAL_WHOLE_STATIC_LIBRARIES += libdx_cc7_static
LOCAL_SHARED_LIBRARIES += libifp libkeymaster
LOCAL_CFLAGS += -DTEE_FRAMEWORK
endif
LOCAL_CFLAGS += $(INTELPROV_DEFINES)
include $(BUILD_STATIC_LIBRARY)

# plugin for recovery_ui
include $(CLEAR_VARS)
LOCAL_SRC_FILES := recovery_ui.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(call include-path-for, recovery) $(call include-path-for, libc-private)
LOCAL_MODULE := libintel_recovery_ui
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter
ifeq ($(REF_DEVICE_NAME), mfld_pr2)
LOCAL_CFLAGS += -DMFLD_PRX_KEY_LAYOUT
endif
include $(BUILD_STATIC_LIBRARY)

# if DROIDBOOT is not used, we dont want this...
# allow to transition smoothly
ifeq ($(TARGET_USE_DROIDBOOT),true)

# Plug-in libary for Droidboot
include $(CLEAR_VARS)
LOCAL_MODULE := libintel_droidboot

LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter -Wno-unused-but-set-variable
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_C_INCLUDES := bootable/droidboot $(call include-path-for, recovery) $(common_libintelprov_includes)

LOCAL_SRC_FILES := droidboot.c $(common_libintelprov_files)

LOCAL_WHOLE_STATIC_LIBRARIES := liboempartitioning_static

ifeq ($(external_release),no)
LOCAL_SRC_FILES += $(common_pmdb_files) $(token_implementation)
LOCAL_C_INCLUDES += $(sep_lib_includes)
LOCAL_WHOLE_STATIC_LIBRARIES += libsecurity_sectoken libcrypto_static CC6_UMIP_ACCESS CC6_ALL_BASIC_LIB
else
LOCAL_CFLAGS += -DEXTERNAL
ifeq ($(BUILD_WITH_SECURITY_FRAMEWORK),chaabi_token)
LOCAL_SRC_FILES += tee_connector.c
endif
endif
ifeq ($(BUILD_WITH_SECURITY_FRAMEWORK),chaabi_token)
LOCAL_CFLAGS += -DTEE_FRAMEWORK
LOCAL_WHOLE_STATIC_LIBRARIES += libdx_cc7_static
LOCAL_SHARED_LIBRARIES += libifp libkeymaster
endif

ifneq ($(DROIDBOOT_NO_GUI),true)
LOCAL_CFLAGS += -DUSE_GUI
endif
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DCLVT
endif
ifeq ($(TARGET_PARTITIONING_SCHEME),"full-gpt")
  LOCAL_CFLAGS += -DFULL_GPT
endif
LOCAL_CFLAGS += $(INTELPROV_DEFINES)
include $(BUILD_STATIC_LIBRARY)

# a test flashtool for testing the intelprov library
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := flashtool
LOCAL_SHARED_LIBRARIES := liblog libcutils

LOCAL_C_INCLUDES := $(common_libintelprov_includes) $(call include-path-for, recovery)
LOCAL_SRC_FILES := flashtool.c $(common_libintelprov_files)
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter

ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
LOCAL_CFLAGS += -DCLVT
endif

LOCAL_CFLAGS += $(INTELPROV_DEFINES)
include $(BUILD_EXECUTABLE)

# update_recovery: this binary is updating the recovery from MOS
# because we dont want to update it from itself.
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := update_recovery
LOCAL_SRC_FILES:= update_recovery.c $(common_libintelprov_files)
LOCAL_C_INCLUDES := $(common_libintelprov_includes) $(call include-path-for, recovery)/applypatch $(call include-path-for, recovery) $(call include-path-for, mkbootimg)
LOCAL_CFLAGS := -Wall -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES := liblog libcutils libz
LOCAL_STATIC_LIBRARIES := libmincrypt libapplypatch libbz
LOCAL_CFLAGS += $(INTELPROV_DEFINES)
include $(BUILD_EXECUTABLE)
endif

include $(call all-makefiles-under,$(LOCAL_PATH))
