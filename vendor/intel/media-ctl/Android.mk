LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/mediactl.c
LOCAL_MODULE := libmediactl
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/android
LOCAL_COPY_HEADERS_TO := mediactl
LOCAL_COPY_HEADERS := src/mediactl.h 
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/v4l2subdev.c
LOCAL_MODULE := libv4l2subdev
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/android
LOCAL_SHARED_LIBRARIES := libmediactl
LOCAL_COPY_HEADERS_TO := mediactl
LOCAL_COPY_HEADERS := src/v4l2subdev.h
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/mediatext.c
LOCAL_MODULE := libmediatext
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/android
LOCAL_SHARED_LIBRARIES := libmediactl libv4l2subdev
LOCAL_COPY_HEADERS_TO := mediactl
LOCAL_COPY_HEADERS := src/mediatext.h
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/main.c src/options.c src/options.h src/tools.h
LOCAL_MODULE := media-ctl
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/android
LOCAL_SHARED_LIBRARIES := libmediactl libv4l2subdev
include $(BUILD_EXECUTABLE)
