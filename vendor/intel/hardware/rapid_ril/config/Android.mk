LOCAL_PATH := $(call my-dir)

include $(call first-makefiles-under, $(LOCAL_PATH))

include $(CLEAR_VARS)
LOCAL_MODULE := rril_cfg
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES :=\
    rril_6360_cfg\
    rril_7160_cfg\
    rril_7260_cfg\
    rril_2230_cfg\

include $(BUILD_PHONY_PACKAGE)
