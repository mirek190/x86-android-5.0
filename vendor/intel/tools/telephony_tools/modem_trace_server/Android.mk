LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH) $(TARGET_OUT_HEADERS)/IFX-modem
LOCAL_SRC_FILES := mts.c
LOCAL_MODULE := mts
LOCAL_SHARED_LIBRARIES := \
    libc \
    libcutils \
    liblog \
    libutils \
    libmmgrcli
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

