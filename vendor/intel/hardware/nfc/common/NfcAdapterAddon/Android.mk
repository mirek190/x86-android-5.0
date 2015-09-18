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

#ifeq ($(TARGET_ARCH),x86)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := com.intel.nfc.adapteraddon_library

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/src
LOCAL_SRC_FILES := $(call all-java-files-under, src)  \
                   $(call all-Iaidl-files-under, src)

include $(BUILD_STATIC_JAVA_LIBRARY)

################################################################################

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := com.intel.nfc.adapteraddon

ifneq (, $(TARGET_VENDOR_ADAPTERADDON))
    LOCAL_REQUIRED_MODULES := $(TARGET_VENDOR_ADAPTERADDON)
    LOCAL_STATIC_JAVA_LIBRARIES := $(TARGET_VENDOR_ADAPTERADDON)
else
    $(info com.intel.nfc.adapteraddon doesn't embedd any vendor specific library! )
endif

    LOCAL_STATIC_JAVA_LIBRARIES += com.intel.nfc.adapteraddon_library
    LOCAL_REQUIRED_MODULES += com.intel.nfc.adapteraddon_library

include $(BUILD_JAVA_LIBRARY)

#endif ## TARGET_ARCH == x86
