ifeq (bcm,$(GPS_CHIP_VENDOR))
ifneq (,$(filter $(GPS_CHIP),4752 47521))
ifneq (true,$(GPS_SDK))

LOCAL_PATH := $(call my-dir)

###############################################################################
# gps.conf
###############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

###############################################################################
# gps.xml
###############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := gps.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
ifeq ($(GPS_USES_EXTERNAL_LNA),true)
LOCAL_SRC_FILES := gpsconfig_$(GPS_CHIP)_extlna.xml
else
LOCAL_SRC_FILES := gpsconfig_$(GPS_CHIP)_intlna.xml
endif
include $(BUILD_PREBUILT)

###############################################################################
# libgps
###############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
ifndef MODULE.TARGET.STATIC_LIBRARIES.libgpsso_proxy
LOCAL_PREBUILT_LIBS += libgpsso_proxy.a
endif
ifndef MODULE.TARGET.STATIC_LIBRARIES.libgpsso
LOCAL_PREBUILT_LIBS += libgpsso.a
endif
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := gps.$(TARGET_DEVICE)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_PRELINK_MODULE := false

LOCAL_WHOLE_STATIC_LIBRARIES := libgpsso_proxy \
                                libgpsso \
                                libxml2

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libicuuc libstlport

# L avoid build issue "warning: shared library text segment is not shareable"
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel

include $(BUILD_SHARED_LIBRARY)

###############################################################################
# libflp
###############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
ifndef MODULE.TARGET.STATIC_LIBRARIES.libflpso_proxy
LOCAL_PREBUILT_LIBS += libflpso_proxy.a
endif
ifndef MODULE.TARGET.STATIC_LIBRARIES.libflpso
LOCAL_PREBUILT_LIBS += libflpso.a
endif
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := flp.$(TARGET_DEVICE)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_PRELINK_MODULE := false

LOCAL_WHOLE_STATIC_LIBRARIES := libflpso_proxy \
                                libflpso \
                                libxml2

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libicuuc libstlport

# L avoid build issue "warning: shared library text segment is not shareable"
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel

include $(BUILD_SHARED_LIBRARY)

###############################################################################
# gpsd
###############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LIBGPSD_CUSTOM := libgpsd_custom_nomodem
ifeq ($(BOARD_HAVE_MODEM),true)
# Exception for factory build which must avoid complex modem interface.
ifneq ($(GPSD_VARIANT),factory)
LIBGPSD_CUSTOM := libgpsd_custom
endif
endif

ifndef MODULE.TARGET.STATIC_LIBRARIES.$(LIBGPSD_CUSTOM)
LOCAL_PREBUILT_LIBS += $(LIBGPSD_CUSTOM).a
endif
ifndef MODULE.TARGET.STATIC_LIBRARIES.libgpsd
LOCAL_PREBUILT_LIBS += libgpsd.a
endif
ifndef MODULE.TARGET.STATIC_LIBRARIES.liblto
LOCAL_PREBUILT_LIBS += liblto.a
endif
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := gpsd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

# We have a cross-dependency between libgpsd_custom and libgpsd.
# Hence we need to list each library twice otherwise the linker
# is not able to resolve all the symbols.
LOCAL_STATIC_LIBRARIES := $(LIBGPSD_CUSTOM) \
                          libgpsd \
                          $(LIBGPSD_CUSTOM) \
                          libgpsd \
                          liblto \
                          libxml2

LOCAL_SHARED_LIBRARIES := liblog libicuuc libssl libcrypto libutils libhardware_legacy libcutils libstlport libgui
ifeq ($(LIBGPSD_CUSTOM),libgpsd_custom)
LOCAL_SHARED_LIBRARIES += libmmgrcli
endif

include $(BUILD_EXECUTABLE)

###############################################################################
# gpscerd
###############################################################################

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
ifndef MODULE.TARGET.STATIC_LIBRARIES.libgpscerd
LOCAL_PREBUILT_LIBS += libgpscerd.a
endif
include $(BUILD_MULTI_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := gpscerd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

LOCAL_STATIC_LIBRARIES := libgpscerd

LOCAL_SHARED_LIBRARIES := liblog libcutils libstlport libkeystore_binder libbinder libutils
include $(BUILD_EXECUTABLE)

endif
endif
endif
