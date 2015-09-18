# Copyright 2006 The Android Open Source Project

# XXX using libutils for simulator build only...
#
#ifeq ($(BOARD_HAVE_MODEM),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES:= \
    sev-ril.c \
    atchannel.c \
    misc.c \
    at_tok.c \
    voice.c \
    sim.c \
    sms.c \
    zte_getdevinfo.c \
    network.c 


LOCAL_SHARED_LIBRARIES := \
    libcutils libutils libril libnetutils

# for asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_CFLAGS += -DHAVE_DATA_DEVICE

#LOCAL_CFLAGS += -pie -fPIE
#LOCAL_LDFLAGS += -pie -fPIE


ifeq (foo,foo)
  #build shared library
  LOCAL_SHARED_LIBRARIES += \
      libcutils libutils
  LOCAL_CFLAGS += -DRIL_SHLIB
  LOCAL_MODULE:= libsev-ril
  include $(BUILD_SHARED_LIBRARY)
else
  #build executable
  LOCAL_SHARED_LIBRARIES += \
      libril
  LOCAL_MODULE:= sev-ril
  include $(BUILD_EXECUTABLE)
endif
#endif
