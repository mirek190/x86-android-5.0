LOCAL_SRC_FILES += \
    common/CameraAreas.cpp \
    common/LogHelper.cpp \
    common/DebugFrameRate.cpp \
    common/PerformanceTraces.cpp \
    common/Camera3GFXFormat.cpp \
    common/Utils.cpp

ifeq ($(BOARD_GRAPHIC_IS_GEN),true)
LOCAL_SRC_FILES += common/GFXFormatGen.cpp
else
ifeq ($(BOARD_GRAPHIC_IS_MALI),true)
LOCAL_SRC_FILES += common/GFXFormatMali.cpp
else
# IMG gets to be the default
LOCAL_SRC_FILES += common/GFXFormatImg.cpp
endif #mali
endif #gen

LOCAL_STATIC_LIBRARIES += \
    libscheduling_policy

# enable R&D features only in R&D builds
ifneq ($(filter userdebug eng tests, $(TARGET_BUILD_VARIANT)),)
LOCAL_SRC_FILES += \
    common/MemoryDumper.cpp
endif
