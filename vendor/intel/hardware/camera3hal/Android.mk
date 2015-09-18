ifeq ($(USE_CAMERA_HAL_3),true)
ifeq ($(USE_CAMERA_STUB),false)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := Camera3HALModule.cpp

include $(LOCAL_PATH)/AAL/Android.mk
LOCAL_C_INCLUDES := $(LOCAL_PATH)/AAL

# IPU4 HW
ifeq ($(BOARD_CAMERA_IPU4_SUPPORT), true)
SUB_PATH:= $(LOCAL_PATH)/ipu4
include $(SUB_PATH)/Android.mk
LOCAL_C_INCLUDES += $(SUB_PATH)
LOCAL_CFLAGS += -DCAMERA_IPU4_SUPPORT
endif

# IPU2(CSS24xx) HW
ifeq ($(BOARD_CAMERA_IPU2_SUPPORT), true)
SUB_PATH:= $(LOCAL_PATH)/ipu2
include $(SUB_PATH)/Android.mk
LOCAL_C_INCLUDES += $(SUB_PATH)
LOCAL_CFLAGS += -DCAMERA_IPU2_SUPPORT
endif

# CIF HW
ifeq ($(BOARD_CAMERA_CIF_SUPPORT), true)
SUB_PATH:= $(LOCAL_PATH)/cifpu
include $(SUB_PATH)/Android.mk
LOCAL_C_INCLUDES += $(SUB_PATH)
LOCAL_CFLAGS += -DCAMERA_CIF_SUPPORT
endif

# common header files
#$(call include-path-for, jpeg)
LOCAL_C_INCLUDES += \
    system/media/camera/include \
    system/core/include \
    external/sqlite/dist \
    frameworks/av/services/audioflinger \
    $(call include-path-for, frameworks-base) \
    $(call include-path-for, frameworks-base)/binder \
    $(call include-path-for, frameworks-av)/camera \
    external/jpeg \
    $(call include-path-for, libhardware) \
    $(call include-path-for, skia)/core \
    $(call include-path-for, skia)/images \
    $(TARGET_OUT_HEADERS)/libtbd \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/cameralibs \
    $(TARGET_OUT_HEADERS)/libmfldadvci

# common libraries from AOSP
LOCAL_SHARED_LIBRARIES += \
    libcamera_client \
    libutils \
    libcutils \
    libbinder \
    libcamera_metadata \
    libjpeg \
    libui \
    libsqlite \
    libdl \
    libgui \
    libexpat \
    libhardware

# platform specific part
ifeq ($(USE_INTEL_METABUFFER),true)
LOCAL_CFLAGS += -DENABLE_INTEL_METABUFFER
endif

ifeq ($(BOARD_GRAPHIC_IS_GEN),true)
ifeq ($(UFO_ENABLE_GEN),gen8)
LOCAL_CFLAGS += -DGRAPHIC_GEN8
endif
LOCAL_CFLAGS += -DGRAPHIC_IS_GEN
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
LOCAL_SHARED_LIBRARIES += libivp
else
LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/pvr/hal
endif

# Intel camera extras (HDR, face detection, etc.)
ifeq ($(USE_INTEL_CAMERA_EXTRAS),true)
LOCAL_CFLAGS += -DENABLE_INTEL_EXTRAS
endif

ifeq ($(USE_INTEL_JPEG), true)
LOCAL_CFLAGS += -DUSE_INTEL_JPEG
LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_imageencoder

LOCAL_SHARED_LIBRARIES += \
    libmix_imageencoder
endif

# enable R&D features only in R&D builds
ifneq ($(filter userdebug eng tests, $(TARGET_BUILD_VARIANT)),)
LOCAL_CFLAGS += -DLIBCAMERA_RD_FEATURES -Werror -Wno-error=unused-parameter -UNDEBUG -Wno-error=non-virtual-dtor -Wno-error=reorder
# uncomment it to enable memory leak check
# LOCAL_CFLAGS += -DMEM_LEAK_CHECK
endif

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_OWNER := intel
LOCAL_MULTILIB := 32

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := camera3_profiles.xml

include $(BUILD_SHARED_LIBRARY)

#for camera3_profiles.xml
include $(LOCAL_PATH)/config/Android.mk

endif #ifeq ($(USE_CAMERA_STUB),false)
endif #ifeq ($(USE_CAMERA_HAL_3),true)

