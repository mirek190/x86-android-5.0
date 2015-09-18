LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#######################################################################
# Common variables

adstreamer_cmd_src_files := \
    ad_streamer_cmd.c

adstreamer_cmd_includes_dir := \
    $(KERNEL_HEADERS) \
    $(TARGET_OUT_HEADERS)/full_rw

#######################################################################
# ad_streamer command


LOCAL_SRC_FILES := \
    $(adstreamer_cmd_src_files) \
    ad_streamer_cmd_main.c

LOCAL_C_INCLUDES := \
    $(adstreamer_cmd_includes_dir)

LOCAL_SHARED_LIBRARIES :=  \
    libsysutils \
    libcutils \
    libhardware_legacy

LOCAL_STATIC_LIBRARIES := \
    libfull_rw \
    libadstreamer

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_ERROR_FLAGS += \
    -Wall \
    -Werror

LOCAL_MODULE := ad_streamer
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

#######################################################################
# Build for test with and without gcov for host and target


# Compile macro
define make_adstreamer_cmd_test_lib
$( \
    $(eval LOCAL_SRC_FILES += $(adstreamer_cmd_src_files)) \
    $(eval LOCAL_MODULE_TAGS := optional) \
    $(eval LOCAL_C_INCLUDES := $(adstreamer_cmd_includes_dir)) \
    $(eval LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)) \
)
endef
define add_gcov_adstreamer_cmd_test_lib
$( \
    $(eval LOCAL_CFLAGS += -O0 -fprofile-arcs -ftest-coverage) \
    $(eval LOCAL_LDFLAGS += -lgcov) \
)
endef

# Build for host test with gcov
ifeq ($(audiocomms_test_gcov_host),true)

include $(CLEAR_VARS)
$(call make_adstreamer_cmd_test_lib)
$(call add_gcov_adstreamer_cmd_test_lib)
LOCAL_MODULE := libadstreamer_cmd_static_gcov_host
LOCAL_STATIC_LIBRARIES := libadstreamer_host
LOCAL_C_INCLUDES += $(HOST_OUT_HEADERS)/android_to_host_stub
include $(BUILD_HOST_STATIC_LIBRARY)

endif

# Build for target test with gcov
ifeq ($(audiocomms_test_gcov),true)

include $(CLEAR_VARS)
$(call make_adstreamer_cmd_test_lib)
$(call add_gcov_adstreamer_cmd_test_lib)
LOCAL_MODULE := libadstreamer_cmd_static_gcov
LOCAL_STATIC_LIBRARIES := libadstreamer
include $(BUILD_STATIC_LIBRARY)

endif

# Build for host test
ifeq ($(audiocomms_test_host),true)

include $(CLEAR_VARS)
$(call make_adstreamer_cmd_test_lib)
LOCAL_MODULE := libadstreamer_cmd_static_host
LOCAL_STATIC_LIBRARIES := libadstreamer_host
LOCAL_C_INCLUDES += $(HOST_OUT_HEADERS)/android_to_host_stub
include $(BUILD_HOST_STATIC_LIBRARY)

endif

# Build for target test
ifeq ($(audiocomms_test_target),true)

include $(CLEAR_VARS)
LOCAL_MODULE := libadstreamer_cmd_static
LOCAL_STATIC_LIBRARIES := libadstreamer
$(call make_adstreamer_cmd_test_lib)
include $(BUILD_STATIC_LIBRARY)

endif

