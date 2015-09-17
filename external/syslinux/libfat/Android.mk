LOCAL_PATH:= $(call my-dir)

# build static library for host
include $(CLEAR_VARS)
LOCAL_MODULE := syslinux_libfat_host

LOCAL_SRC_FILES := \
	cache.c \
	fatchain.c \
	open.c \
	searchdir.c

include $(BUILD_HOST_STATIC_LIBRARY)

# build static library for target device
include $(CLEAR_VARS)
LOCAL_MODULE := syslinux_libfat

LOCAL_SRC_FILES := \
	cache.c \
	fatchain.c \
	open.c \
	searchdir.c

include $(BUILD_STATIC_LIBRARY)
