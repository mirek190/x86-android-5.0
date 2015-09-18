################################################################################
#
# Copyright 2014 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

################################################################################
#
# The configuration of MAMGR for XMM depends on
# - The number of XMM modem (one XMM MAMGR instance per XMM Modem)
# - The number of SIM for each modem (one TTY per SIM for a given XMM MAMGR/Modem)
#
# Refer to MAMGR HLD and MAMGR-XMM HLD for configuration file format & naming conventions.
#
# This makefile has to be used on platforms including two (and only two) XMM modems, each one
# handling a single SIM.
#
# Two XMM MAMGR instances are configured:
#
# A XMM MAMGR instance name is "xmm1"
#     "xmm1" XMM MAMGR uses:
#       * MMGR instance '1'
#       * tty: /dev/gsmtty13
#
# A XMM MAMGR instance name is "xmm2"
#     "xmm2" XMM MAMGR uses:
#       * MMGR instance '2'
#       * tty: /dev/gsmtty77
#
################################################################################
LOCAL_PATH := $(call my-dir)

# global variables
target_mamgr_config_files_path := $(TARGET_OUT_ETC)/mamgr

######### XMM First Modem Single SIM #########

include $(CLEAR_VARS)
LOCAL_MODULE := mamgr
LOCAL_REQUIRED_MODULES := \
    libmamgr-xmm \
    mamgr-second-xmm-modem
LOCAL_MODULE_STEM := xmm1.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(target_mamgr_config_files_path)
LOCAL_SRC_FILES := xmm-smss.xml
include $(BUILD_PREBUILT)

######### XMM Second Modem Single SIM #########

include $(CLEAR_VARS)
LOCAL_MODULE := mamgr-second-xmm-modem
LOCAL_MODULE_STEM := xmm2.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(target_mamgr_config_files_path)
LOCAL_SRC_FILES := xmm-smss-second.xml
include $(BUILD_PREBUILT)
