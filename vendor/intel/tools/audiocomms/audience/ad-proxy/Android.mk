LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#######################################################################
# Common variables

ad_proxy_src_files := \
    ad_proxy.c \
    ad_i2c.c \
    ad_usb_tty.c

ad_proxy_includes_dir := \
    $(TARGET_OUT_HEADERS)/full_rw

#######################################################################
# Build for target

LOCAL_C_INCLUDES += \
    $(ad_proxy_includes_dir)

LOCAL_SRC_FILES := \
    $(ad_proxy_src_files) \
    main.c

LOCAL_SHARED_LIBRARIES := \
    libsysutils \
    libcutils

LOCAL_STATIC_LIBRARIES := \
    libfull_rw

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE := ad_proxy
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

#######################################################################
# Build for test with and without gcov for host and target

# Compile macro
define make_ad_proxy_test_lib
$( \
    $(eval LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)) \
    $(eval LOCAL_SRC_FILES := $(ad_proxy_src_files)) \
    $(eval LOCAL_MODULE_TAGS := optional) \
    $(eval LOCAL_C_INCLUDES := $(ad_proxy_includes_dir)) \
)
endef
define add_gcov_ad_proxy_test_lib
$( \
    $(eval LOCAL_CFLAGS += -O0 -fprofile-arcs -ftest-coverage) \
    $(eval LOCAL_LDFLAGS += -lgcov) \
)
endef

# Build for host test with gcov
ifeq ($(audiocomms_test_gcov_host),true)

include $(CLEAR_VARS)
$(call make_ad_proxy_test_lib)
$(call add_gcov_ad_proxy_test_lib)
LOCAL_MODULE := libad_proxy_static_gcov_host
include $(BUILD_HOST_STATIC_LIBRARY)

endif

# Build for target test with gcov
ifeq ($(audiocomms_test_gcov_target),true)

include $(CLEAR_VARS)
$(call make_ad_proxy_test_lib)
$(call add_gcov_ad_proxy_test_lib)
LOCAL_MODULE := libad_proxy_static_gcov
include $(BUILD_STATIC_LIBRARY)

endif

# Build for host test
ifeq ($(audiocomms_test_host),true)

include $(CLEAR_VARS)
$(call make_ad_proxy_test_lib)
LOCAL_MODULE := libad_proxy_static_host
include $(BUILD_HOST_STATIC_LIBRARY)

endif

# Build for target test
ifeq ($(audiocomms_test_target),true)

include $(CLEAR_VARS)
LOCAL_MODULE := libad_proxy_static
$(call make_ad_proxy_test_lib)
include $(BUILD_STATIC_LIBRARY)

endif

