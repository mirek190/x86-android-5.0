#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_DIR := $(call my-dir)

ifeq ($(COMBO_CHIP_VENDOR), intel) # only Intel vendor FM solution is supported
include  $(LOCAL_DIR)/AndroidFmCommon.mk
include  $(LOCAL_DIR)/AndroidFmIntel.mk
endif
ifeq ($(COMBO_CHIP_VENDOR), bcm) # only BCM vendor FM solution is supported
ifeq ($(BLUEDROID_ENABLE_V4L2),true)
include  $(LOCAL_DIR)/AndroidFmCommon.mk
include  $(LOCAL_DIR)/AndroidFmBCM.mk
endif
endif
