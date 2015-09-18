LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_USE_DROIDBOOT),true)

LOCAL_SRC_FILES := \
	volumeutils/roots.c \
	volumeutils/ufdisk.c \
	aboot.c \
	fastboot.c \
	util.c \
	droidboot_installer.c \
	droidboot.c \
	charger/charger.c

LOCAL_C_INCLUDES += bootable/recovery/mtdutils \
		system/extras/ext4_utils \
		external/zlib \
		external/libpng \
		system/core/libsparse \
		system/core/libsparse/include

DROIDBOOT_OTA_UPDATE_FILE ?= /cache/update.zip
LOCAL_CFLAGS := -DDEVICE_NAME=\"$(TARGET_BOOTLOADER_BOARD_NAME)\" \
	-W -Wall -Wno-unused-parameter -Werror \
	-D OTA_UPDATE_FILE='"$(DROIDBOOT_OTA_UPDATE_FILE)"'

ifeq ($(TARGET_RECOVERY_PIXEL_FORMAT),"RGBX_8888")
LOCAL_CFLAGS += -DRECOVERY_RGBX
endif
ifeq ($(TARGET_RECOVERY_PIXEL_FORMAT),"BGRA_8888")
LOCAL_CFLAGS += -DRECOVERY_BGRA
endif

LOCAL_MODULE := droidboot
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libext4_utils libz libcutils libsparse libc
LOCAL_STATIC_LIBRARIES += libmtdutils libpng libpixelflinger_static libmtdutils
LOCAL_STATIC_LIBRARIES += $(TARGET_DROIDBOOT_LIBS) $(TARGET_DROIDBOOT_EXTRA_LIBS) libminzip libselinux

#libpixelflinger_static for x86 is using encoder under hardware/intel/apache-harmony
ifeq ($(TARGET_ARCH),x86)
LOCAL_STATIC_LIBRARIES += libenc
endif

# Each library in TARGET_DROIDBOOT_LIBS should have a function
# named "<libname>_init()".  Here we emit a little C function that
# gets #included by aboot.c.  It calls all those registration
# functions.

# Devices can also add libraries to TARGET_DROIDBOOT_EXTRA_LIBS.
# These libs are also linked in with droidboot, but we don't try to call
# any sort of registration function for these.  Use this variable for
# any subsidiary static libraries required for your registered
# extension libs.

inc := $(call intermediates-dir-for,PACKAGING,droidboot_extensions)/register.inc

# During the first pass of reading the makefiles, we dump the list of
# extension libs to a temp file, then copy that to the ".list" file if
# it is different than the existing .list (if any).  The register.inc
# file then uses the .list as a prerequisite, so it is only rebuilt
# (and aboot.o recompiled) when the list of extension libs changes.

junk := $(shell mkdir -p $(dir $(inc));\
	        echo $(TARGET_DROIDBOOT_LIBS) > $(inc).temp;\
	        diff -q $(inc).temp $(inc).list 2>/dev/null || cp -f $(inc).temp $(inc).list)

$(inc) : libs := $(TARGET_DROIDBOOT_LIBS)
$(inc) : $(inc).list $(LOCAL_PATH)/Android.mk
	$(hide) mkdir -p $(dir $@)
	$(hide) echo "" > $@
	$(hide) $(foreach lib,$(libs),echo "extern void $(lib)_init(void);" >> $@)
	$(hide) echo "void register_droidboot_plugins() {" >> $@
	$(hide) $(foreach lib,$(libs),echo "  $(lib)_init();" >> $@)
	$(hide) echo "}" >> $@

$(call intermediates-dir-for,EXECUTABLES,droidboot)/aboot.o : $(inc)
LOCAL_C_INCLUDES += $(dir $(inc))

ifneq ($(DROIDBOOT_NO_GUI),true)
LOCAL_C_INCLUDES += $(call include-path-for, recovery)
LOCAL_SRC_FILES += charger/charger_ui.c \
		charger/power.c \
		droidboot_ui.c \
		ui.c \
		timer.c

LOCAL_STATIC_LIBRARIES += libminui libpng
LOCAL_CFLAGS += -DUSE_GUI
endif

ifeq ($(DROIDBOOT_USE_INSTALLER),true)
LOCAL_CFLAGS += -DUSE_INSTALLER
endif

include $(BUILD_EXECUTABLE)

endif # TARGET_USE_DROIDBOOT



# FPT
include $(CLEAR_VARS)
LOCAL_MODULE := FPT
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/FPT
include $(BUILD_PREBUILT)

# fparts.txt
include $(CLEAR_VARS)
LOCAL_MODULE := fparts.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/fparts.txt
include $(BUILD_PREBUILT)

# FWUpdLcl
include $(CLEAR_VARS)
LOCAL_MODULE := FWUpdLcl
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/FWUpdLcl
include $(BUILD_PREBUILT)

# TXEInfo
include $(CLEAR_VARS)
LOCAL_MODULE := TXEInfo
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/TXEInfo
include $(BUILD_PREBUILT)

# TXEManuf
include $(CLEAR_VARS)
LOCAL_MODULE := TXEManuf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/TXEManuf
include $(BUILD_PREBUILT)

# TXEManuf.cfg
include $(CLEAR_VARS)
LOCAL_MODULE := TXEManuf.cfg
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/TXEManuf.cfg
include $(BUILD_PREBUILT)

# vsccommn.bin
include $(CLEAR_VARS)
LOCAL_MODULE := vsccommn.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLE
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/fpt-tools-package
LOCAL_SRC_FILES := fpt-tools-package/vsccommn.bin
include $(BUILD_PREBUILT)
