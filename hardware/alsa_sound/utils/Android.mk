LOCAL_PATH := $(call my-dir)

# Common variables
##################

alsa_utils_exported_includes_folder := audio_hal_utils
alsa_utils_exported_includes_files := \
    SyncSemaphore.h \
    SyncSemaphoreList.h \
    Utils.h \
    Tokenizer.h

alsa_utils_src_files := \
    SyncSemaphore.cpp \
    SyncSemaphoreList.cpp \
    Tokenizer.cpp


# Build for target
##################
include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := $(alsa_utils_exported_includes_folder)
LOCAL_COPY_HEADERS := $(alsa_utils_exported_includes_files)

LOCAL_CFLAGS := -DDEBUG -Wall -Werror

LOCAL_SRC_FILES := $(alsa_utils_src_files)

LOCAL_C_INCLUDES += \
    external/stlport/stlport \
    bionic

LOCAL_SHARED_LIBRARIES := libstlport

LOCAL_MODULE := libaudiohalutils
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# Build for host test
#####################

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := $(alsa_utils_exported_includes_folder)
LOCAL_COPY_HEADERS := $(alsa_utils_exported_includes_files)

LOCAL_SRC_FILES := $(alsa_utils_src_files)

LOCAL_MODULE := libaudiohalutils_host
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)

