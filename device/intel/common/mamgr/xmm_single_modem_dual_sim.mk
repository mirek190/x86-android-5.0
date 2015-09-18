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
# This makefile has to be used on platforms including one (and only one) XMM modem handling
# dual SIM (DSDS or DSDA).
#
# One XMM MAMGR instance is configured:
#
# The XMM MAMGR instance name is "xmm1"
#     "xmm1" XMM MAMGR uses:
#       * MMGR instance '1'
#       * tty: /dev/gsmtty13 (first SIM)
#       * tty: /dev/gsmtty21 (second SIM)
#
################################################################################
LOCAL_PATH := $(call my-dir)

# global variables
target_mamgr_config_files_path := $(TARGET_OUT_ETC)/mamgr

######### XMM Single Modem Dual SIM #########

include $(CLEAR_VARS)
LOCAL_MODULE := mamgr
LOCAL_REQUIRED_MODULES := libmamgr-xmm
LOCAL_MODULE_STEM := xmm1.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(target_mamgr_config_files_path)
LOCAL_SRC_FILES := xmm-smds.xml
include $(BUILD_PREBUILT)
