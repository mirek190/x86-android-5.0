#
## Copyright (C) 2013 The Android Open Source Project
#
## Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# # You may obtain a copy of the License at
# #
# #      http://www.apache.org/licenses/LICENSE-2.0
# #
# # Unless required by applicable law or agreed to in writing, software
# # distributed under the License is distributed on an "AS IS" BASIS,
# # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# # See the License for the specific language governing permissions and
# # limitations under the License.
# #

LOCAL_PATH := $(call my-dir)
UEFIVAR_ADDITIONNAL_CFLAGS := -Wall

ifeq ($(BOARD_USE_64BIT_KERNEL),true)
UEFIVAR_ADDITIONNAL_CFLAGS += -DKERNEL_X86_64
endif

include $(CLEAR_VARS)
LOCAL_CFLAGS += $(UEFIVAR_ADDITIONNAL_CFLAGS)

LOCAL_SRC_FILES:= \
	libuefivar.c

LOCAL_MODULE := libuefivar
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS += $(UEFIVAR_ADDITIONNAL_CFLAGS)

LOCAL_SRC_FILES:= \
	uefivar.c \
	libuefivar.c

LOCAL_MODULE:= uefivar
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/sbin
LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_EXECUTABLE)
