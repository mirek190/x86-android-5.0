LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ARCH := ia32
# ARCH := x86_64

LOCAL_C_INCLUDES += \
$(TOP)/external/gnu-efi-3.0/inc \
$(TOP)/external/gnu-efi-3.0/inc/protocol \
$(TOP)/external/gnu-efi-3.0/inc/$(ARCH)

LOCAL_SRC_FILES := \
boxdraw.c \
console.c \
crc.c \
data.c \
debug.c \
dpath.c \
error.c \
event.c \
guid.c \
hand.c \
hw.c \
init.c \
lock.c \
misc.c \
print.c \
smbios.c \
sread.c \
str.c \
runtime/efirtlib.c \
runtime/rtdata.c \
runtime/rtlock.c \
runtime/rtstr.c \
runtime/vm.c \
$(ARCH)/initplat.c \
$(ARCH)/math.c


ifeq ($(ARCH),x86_64)
LOCAL_SRC_FILES += $(ARCH)/callwrap.c $(ARCH)/efi_stub.S
endif

LOCAL_MODULE := libefi
LOCAL_MODULE_TAGS := optional
include $(BUILD_SYSTEM)/raw_static_library.mk
