# The camera HAL depends on the XML configuration file camera3_profiles
# This sections ensures that the files is present on the build image
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#Please add the product name when a new config file was added

#default platform
PLATFORM_NAME := baytrail

ifeq ($(TARGET_DEVICE), ecs_e7)
PLATFORM_NAME := baytrail
endif

ifeq ($(TARGET_DEVICE), anzhen4_mrd7)
PLATFORM_NAME := baytrail
endif

ifeq ($(TARGET_DEVICE), anzhen4_mrd8)
PLATFORM_NAME := baytrail
endif

ifeq ($(TARGET_DEVICE), fxn_anchor8)
PLATFORM_NAME := baytrail
endif

ifeq ($(TARGET_DEVICE), byt_t_crv2)
PLATFORM_NAME := baytrail
endif

ifeq ($(TARGET_DEVICE), saltbay)
PLATFORM_NAME := merrifield
endif

ifeq ($(TARGET_DEVICE), mofd_v1)
ifeq ($(BOARD_CAMERA_IPU4_SUPPORT), true)
PLATFORM_NAME := moorefield_n1
else
PLATFORM_NAME := moorefield
endif
endif

ifeq ($(TARGET_DEVICE), Sf3g_mrd0_p0)
PLATFORM_NAME := sofia3g
endif

ifeq ($(TARGET_DEVICE), cht_rvp)
PLATFORM_NAME := cherrytrail
endif

LOCAL_MODULE := camera3_profiles.xml
#For IRDA project, it needs to be tested under both FULL mode
#and LIMITED mode at some time, so add camera3_profile.full.xml for FULL mode
#to make it easy to switch.
ifeq ($(TARGET_DEVICE), ecs_e7)
LOCAL_REQUIRED_MODULES := camera3_profiles.full.xml
endif
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(PLATFORM_NAME)/$(TARGET_DEVICE)/camera3_profiles.xml
include $(BUILD_PREBUILT)

ifeq ($(TARGET_DEVICE), ecs_e7)
include $(CLEAR_VARS)
LOCAL_MODULE := camera3_profiles.full.xml
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(PLATFORM_NAME)/$(TARGET_DEVICE)/camera3_profiles.full.xml
include $(BUILD_PREBUILT)
endif
