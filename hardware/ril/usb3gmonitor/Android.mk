LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := usb3gmonitor
LOCAL_MODULE_TAGS := optional 
LOCAL_SRC_FILES := usb3gmonitor.c
LOCAL_SHARED_LIBRARIES := libcutils
$(shell mkdir -p $(TARGET_OUT)/etc/usb_modeswitch.d)
$(shell cp $(LOCAL_PATH)/usb_modeswitch.d/* $(TARGET_OUT)/etc/usb_modeswitch.d/)
include $(BUILD_EXECUTABLE)

