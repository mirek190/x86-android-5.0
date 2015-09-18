# Copyright (C) 2012 The Android Open Source Project
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

LOCAL_PATH := $(my-dir)

################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := init.nfc.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := init.nfc.rc
include $(BUILD_PREBUILT)

################################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := smartcard_api
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    SmartcardService \
    nfcee_access \
    org.simalliance.openmobileapi \
    org.simalliance.openmobileapi.xml
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := nfcee_access
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_SUFFIX := .xml
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE).xml
include $(BUILD_PREBUILT)

################################################################################

# include $(call first-makefiles-under,$(LOCAL_PATH))

################################################################################

LOCAL_PATH := $(ANDROID_BUILD_TOP)
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.nfc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_SUFFIX := .xml
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := frameworks/native/data/etc/$(LOCAL_MODULE).xml
include $(BUILD_PREBUILT)

################################################################################

LOCAL_PATH := $(ANDROID_BUILD_TOP)
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.nfc.hce
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_SUFFIX := .xml
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := frameworks/native/data/etc/$(LOCAL_MODULE).xml
include $(BUILD_PREBUILT)

