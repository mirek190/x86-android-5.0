LOCAL_PATH:= $(call my-dir)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := regulatory.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/wifi/crda
LOCAL_SRC_FILES := wireless-regdb/regulatory.bin
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := linville.key.pub.pem
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/wifi/crda/pubkeys
LOCAL_SRC_FILES := pubkeys/linville.key.pub.pem
include $(BUILD_PREBUILT)


##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := intel.key.pub.pem
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/wifi/crda/pubkeys
LOCAL_SRC_FILES := pubkeys/intel.key.pub.pem
include $(BUILD_PREBUILT)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := crda
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto libc
LOCAL_STATIC_LIBRARIES := libnl_2
LOCAL_SRC_FILES := \
	reglib.c \
	crda.c
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	external/libnl-headers \
	external/openssl/include

LOCAL_CFLAGS := -DUSE_OPENSSL -DPUBKEY_DIR=\"/system/etc/wifi/crda/pubkeys\" -DCONFIG_LIBNL20 -DCONFIG_GOOGLE_LIBNL20
include $(BUILD_EXECUTABLE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := regdbdump
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto libc
LOCAL_STATIC_LIBRARIES := libnl_2
LOCAL_SRC_FILES := \
        reglib.c \
        regdbdump.c \
        print-regdom.c
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	external/libnl-headers \
	external/openssl/include
LOCAL_CFLAGS := -DUSE_OPENSSL -DPUBKEY_DIR=\"/system/etc/wifi/crda/pubkeys\" -DCONFIG_LIBNL20 -DCONFIG_GOOGLE_LIBNL20
include $(BUILD_EXECUTABLE)

##################################################

include $(CLEAR_VARS)
LOCAL_MODULE := intersect
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcrypto libc
LOCAL_STATIC_LIBRARIES := libnl_2
LOCAL_SRC_FILES := \
        reglib.c \
        intersect.c \
        print-regdom.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	external/libnl-headers \
	external/openssl/include
LOCAL_CFLAGS := -DUSE_OPENSSL -DPUBKEY_DIR=\"/system/etc/wifi/crda/pubkeys\" -DCONFIG_LIBNL20 -DCONFIG_GOOGLE_LIBNL20
include $(BUILD_EXECUTABLE)

