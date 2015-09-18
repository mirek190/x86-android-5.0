#
# Copyright (C) Intel 2014
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/test_config.mk

include $(CLEAR_VARS)

LOCAL_MODULE := crashlogd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := intel

LOCAL_SRC_FILES := \
    main.c \
    config.c \
    inotify_handler.c \
    startupreason.c \
    crashutils.c \
    usercrash.c \
    anruiwdt.c \
    recovery.c \
    history.c \
    trigger.c \
    dropbox.c \
    fsutils.c \
    panic.c \
    config_handler.c \
    uefivar.c \
    checksum.c \
    ingredients.c

LOCAL_SHARED_LIBRARIES := libcutils libcrypto

LOCAL_C_INCLUDES += \
    external/openssl/include

# Options

ifeq ($(CRASHLOGD_FULL_REPORT),true)
LOCAL_CFLAGS += -DFULL_REPORT
endif

ifeq ($(CRASHLOGD_FACTORY_CHECKSUM),true)
LOCAL_CFLAGS += -DCONFIG_FACTORY_CHECKSUM
endif

ifeq ($(CRASHLOGD_APLOG),true)
LOCAL_CFLAGS += -DCONFIG_APLOG
endif

ifeq ($(CRASHLOGD_COREDUMP),true)
LOCAL_CFLAGS += -DCONFIG_COREDUMP
endif

ifeq ($(CRASHLOGD_USE_SD),false)
LOCAL_CFLAGS += -DCONFIG_USE_SD=FALSE
else
LOCAL_CFLAGS += -DCONFIG_USE_SD=TRUE
endif

ifeq ($(CRASHLOGD_EFILINUX),true)
LOCAL_CFLAGS += -DCONFIG_EFILINUX
LOCAL_STATIC_LIBRARIES += \
    libdmi \
    libuefivar
endif

ifeq ($(CRASHLOGD_FDK),true)
LOCAL_CFLAGS += -DCONFIG_FDK
endif

ifeq ($(CRASHLOGD_EFILINUX),true)
ifeq ($(CRASHLOGD_FDK),true)
$(error CRASHLOGD_EFILINUX and CRASHLOGD_FDK options are exclusive.)
endif
endif

ifneq ($(CRASHLOGD_LOGS_PATH),)
LOCAL_CFLAGS += -DCONFIG_LOGS_PATH='$(CRASHLOGD_LOGS_PATH)'
endif

# Modules

ifeq ($(CRASHLOGD_MODULE_IPTRAK),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_IPTRAK
LOCAL_SRC_FILES += iptrak.c
endif

ifeq ($(CRASHLOGD_MODULE_SPID),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_SPID
LOCAL_SRC_FILES += spid.c
endif

# Enabled by default
ifneq ($(CRASHLOGD_MODULE_BTDUMP),false)
LOCAL_CFLAGS += -DCONFIG_BTDUMP
LOCAL_STATIC_LIBRARIES += libbtdump
LOCAL_SHARED_LIBRARIES += libbacktrace
include external/stlport/libstlport.mk
endif

ifeq ($(CRASHLOGD_MODULE_KCT),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_KCT
LOCAL_SRC_FILES += \
    ct_utils.c \
    kct_netlink.c \
    lct_link.c \
    ct_eventintegrity.c
endif

ifeq ($(CRASHLOGD_MODULE_MODEM),true)
ifeq ($(BOARD_HAVE_MODEM),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_MODEM
LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/IFX-modem \
    $(TARGET_OUT_HEADERS)/libtcs
LOCAL_SHARED_LIBRARIES += \
    libmmgrcli \
    libtcs
LOCAL_SRC_FILES += \
    modem.c \
    tcs_wrapper.c \
    mmgr_source.c
endif
endif

ifeq ($(CRASHLOGD_MODULE_FABRIC),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_FABRIC
LOCAL_SRC_FILES += fabric.c
endif

ifeq ($(CRASHLOGD_MODULE_FW_UPDATE),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_FW_UPDATE
LOCAL_SRC_FILES += fw_update.c
endif

ifeq ($(CRASHLOGD_MODULE_RAMDUMP),true)
LOCAL_CFLAGS += -DCRASHLOGD_MODULE_RAMDUMP
LOCAL_SRC_FILES += ramdump.c
endif

ifneq ($(CRASHLOGD_LOGS_PATH),)
LOCAL_CFLAGS += -DCONFIG_LOGS_PATH='$(CRASHLOGD_LOGS_PATH)'
endif

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := lct.h
LOCAL_COPY_HEADERS_TO := crashlog
include $(BUILD_COPY_HEADERS)
