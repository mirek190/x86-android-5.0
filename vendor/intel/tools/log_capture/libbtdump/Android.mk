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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libbtdump
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := intel

LOCAL_SRC_FILES:= libbtdump.cpp \
    CProcInfo.cpp \
    CThreadInfo.cpp \
    CFrameInfo.cpp

LOCAL_CPPFLAGS := \
    -std=gnu++11 \
    -W -Wall -Wextra \
    -Wunused \
    -Werror \
    -DUSE_LIBBACKTRACE \

LOCAL_STATIC_LIBRARIES := \
    libcutils \
    libc

LOCAL_SHARED_LIBRARIES := \
    libbacktrace

include external/stlport/libstlport.mk

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE:= btdump
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := intel

LOCAL_SRC_FILES:= btdump.cpp

LOCAL_CPPFLAGS := \
    -std=gnu++11 \
    -W -Wall -Wextra \
    -Wunused \
    -Werror \

LOCAL_STATIC_LIBRARIES += \
    libbtdump

LOCAL_SHARED_LIBRARIES += \
    libbacktrace

include external/stlport/libstlport.mk
include $(BUILD_EXECUTABLE)