LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_HAVE_ATPROXY),true)

include $(CLEAR_VARS)
LOCAL_MODULE := proxy
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/IFX-modem \
    $(LOCAL_PATH)/apb \

LOCAL_CFLAGS += -Wall -Werror -std=c99
LOCAL_SRC_FILES := \
    proxy_main.c \
    serial.c \
    atp_util.c \
    apb/apb.c \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libc \
    libmmgrcli \

ifeq ($(BOARD_USES_GTI_FRAMEWORK),true)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/libgtiproxy
LOCAL_SHARED_LIBRARIES += libgtiproxy libgti libutaos
LOCAL_CFLAGS += -DUSE_FRAMEWORK_GTI
endif

include $(BUILD_EXECUTABLE)

# WORKAROUND - always rebuild proxy_main to avoid objects inconsistency
# due to USE_FRAMEWORK_GTI flag that MUST NOT be modified between build
# variants (eng, userdebug, user)
.PHONY: $(intermediates)/proxy_main.o
.PHONY: $(intermediates)/import_includes

include $(call first-makefiles-under,$(LOCAL_PATH))

endif
