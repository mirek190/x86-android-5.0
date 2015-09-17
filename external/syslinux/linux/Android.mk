LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := android_syslinux
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/syslinux/bin/

LOCAL_SRC_FILES := \
	syslinux.c \
	../libinstaller/syslxopt.c \
	../libinstaller/syslxcom.c \
	../libinstaller/setadv.c \
	../libinstaller/advio.c \

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libfat
LOCAL_STATIC_LIBRARIES := syslinux_libfat

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libinstaller
LOCAL_STATIC_LIBRARIES += syslinux_libinstaller

LOCAL_CFLAGS := \
	-g -O0 -Dalloca=malloc \
	-W -Wall -Wstrict-prototypes \
	-D_FILE_OFFSET_BITS=64 -m32 \
	-D_PATH_MOUNT=\"/system/bin/mount\" \
	-D_PATH_UMOUNT=\"/system/bin/umount\" \

include $(BUILD_EXECUTABLE)

# Create an explicit dependency with full path
.PHONY: $(LOCAL_MODULE_PATH)$(LOCAL_MODULE)
$(LOCAL_MODULE_PATH)$(LOCAL_MODULE): $(LOCAL_MODULE)

# android_syslinux can be included in OTA/recovery if needed
INSTALLED_SYSLINUX_TARGET_EXEC := $(PRODUCT_OUT)/syslinux/bin/android_syslinux
