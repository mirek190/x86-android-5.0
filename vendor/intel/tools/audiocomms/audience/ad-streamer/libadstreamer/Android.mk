LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#######################################################################
# Common variables

libadstreamer_src_files := \
    ad_streamer.c

libadstreamer_includes_dir := \
    $(KERNEL_HEADERS) \
    $(TARGET_OUT_HEADERS)/full_rw

#######################################################################
# Build for target

LOCAL_SRC_FILES := \
    $(libadstreamer_src_files)

LOCAL_C_INCLUDES := \
    $(libadstreamer_includes_dir)

LOCAL_SHARED_LIBRARIES :=  \
    libsysutils \
    libcutils

LOCAL_STATIC_LIBRARIES := \
    libfull_rw

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_ERROR_FLAGS += \
    -Wall \
    -Werror

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_MODULE := libadstreamer
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

#######################################################################
# Build for test with and without gcov for host and target


# Compile macro
define make_libadstreamer_test_lib
$( \
    $(eval LOCAL_SRC_FILES += $(libadstreamer_src_files)) \
    $(eval LOCAL_MODULE_TAGS := optional) \
    $(eval LOCAL_C_INCLUDES := $(libadstreamer_includes_dir)) \
    $(eval LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)) \
)
endef
define add_gcov_libadstreamer_test_lib
$( \
    $(eval LOCAL_CFLAGS += -O0 -fprofile-arcs -ftest-coverage) \
    $(eval LOCAL_LDFLAGS += -lgcov) \
)
endef

# Build for host test with gcov
ifeq ($(audiocomms_test_gcov_host),true)

include $(CLEAR_VARS)
$(call make_libadstreamer_test_lib)
$(call add_gcov_libadstreamer_test_lib)
LOCAL_MODULE := libadstreamer_gcov_host
include $(BUILD_HOST_STATIC_LIBRARY)

endif

# Build for target test with gcov
ifeq ($(audiocomms_test_gcov),true)

include $(CLEAR_VARS)
$(call make_libadstreamer_test_lib)
$(call add_gcov_libadstreamer_test_lib)
LOCAL_MODULE := libadstreamer_gcov
include $(BUILD_STATIC_LIBRARY)

endif

# Build for host test
ifeq ($(audiocomms_test_host),true)

include $(CLEAR_VARS)
$(call make_libadstreamer_test_lib)
LOCAL_MODULE := libadstreamer_host
include $(BUILD_HOST_STATIC_LIBRARY)

endif

