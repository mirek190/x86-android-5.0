ifndef com32_build_prebuilt

# $(1) prebuilt module (really filename) to install
# in $(HOST_OUT)/usr/lib/syslinux
define com32_build_prebuilt
include $(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(HOST_OUT)/usr/lib/syslinux
LOCAL_MODULE_STEM := $(1)
LOCAL_SRC_FILES := $(1)
include $(BUILD_PREBUILT)
endef

endif
