# Name of the reference design
# Should be changed with the original values when starting customization
REF_DEVICE_NAME ?= $(TARGET_DEVICE)
REF_PRODUCT_NAME ?= $(TARGET_PRODUCT)

TARGET_USE_DROIDBOOT := true

# Set ISU as signing tool
TARGET_OS_SIGNING_METHOD := isu

########## Add plus features here ##########

# Android Security Framework (IDPT-SE)
# and Licensed Permission
-include $(COMMON_PLUS_PATH)/asf/BoardConfig.mk

########## End of plus features ##########

include $(PLATFORM_PATH)/BoardConfig.mk

# IAFW component to build for this board
BOARD_IAFW_COMPONENT := brd_baylake

#Modem
ifeq ($(BOARD_HAVE_MODEM), true)
BOARD_MODEM_LIST := 7160_flashless
BOARD_HAVE_ATPROXY := true
ADDITIONAL_BUILD_PROPERTIES += rild.libpath=/system/lib/librapid-ril-core.so

# enable modem throttling through itux
ITUX_MODEM_ZONE := true

# Adding DSDS enabling/disabling property
ADDITIONAL_DEFAULT_PROPERTIES += persist.dual_sim=none

ifeq ($(TARGET_RIL_DISABLE_STATUS_POLLING),true)
ADDITIONAL_BUILD_PROPERTIES += ro.ril.status.polling.enable=0
endif
endif

# Connectivity
BOARD_HAVE_WIFI := true
INTEL_WIDI := true
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(DEVICE_CONF_PATH)
TARGET_HAS_ISV := true
TARGET_VPP_USE_GEN := true

# MultiDisplay service
TARGET_HAS_MULTIPLE_DISPLAY := true

USE_INTEL_IPP := true

# WiDi
INTEL_WIDI_BAYTRAIL := true

# Power_HAL
POWERHAL_BYT := true

#Energy Management fuel gauge algorithm
USE_FG_ALGO := true

# Audio
BOARD_USES_ALSA_AUDIO := false
BOARD_USES_TINY_ALSA_AUDIO := true
BOARD_USES_AUDIO_HAL_XML := true
ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
# Enable ALSA utils for eng and user debug builds
BUILD_WITH_ALSA_UTILS := true
endif
BOARD_USES_GENERIC_AUDIO := false

#BCU HAL
BCUHAL_BYT := true

#GEN is one graphic and video engine
# Baytrail uses the GEN for the graphic and video
BOARD_GRAPHIC_IS_GEN := true

# Camera
# Set USE_CAMERA_STUB to 'true' for Fake Camera builds,
# 'false' for libcamera builds to use Camera Imaging(CI) supported by intel.
BOARD_CAMERA_IPU2_SUPPORT := true
USE_CAMERA_STUB := false
USE_CAMERA_HAL2 := false
USE_CAMERA_HAL_3 := true

USE_INTEL_METABUFFER := true

USE_CSS_2_1 := true

# Enabled HW accelerated JPEG encoder using VA API
USE_INTEL_JPEG := false

# Enable Intel accelerated skia ImageDecoder
ADD_INTEL_SKIMAGEDECODER := true

ifeq ($(BOARD_KERNEL_CMDLINE),)
DEBUG_KERNEL_CMDLINE := console=ttyS0,115200 console=logk0 earlyprintk=nologger \
                       ptrace.ptrace_can_access=1 panic_on_bad_page=1 panic_on_list_corruption=1
ifeq ($(TARGET_BUILD_VARIANT),eng)
DEBUG_KERNEL_CMDLINE := $(DEBUG_KERNEL_CMDLINE) loglevel=8 drm.debug=0x0
else ifeq ($(TARGET_BUILD_VARIANT),userdebug)
DEBUG_KERNEL_CMDLINE := $(DEBUG_KERNEL_CMDLINE) loglevel=4
else
DEBUG_KERNEL_CMDLINE := loglevel=0
endif
BOARD_KERNEL_CMDLINE := $(DEBUG_KERNEL_CMDLINE) androidboot.bootmedia=$(BOARD_BOOTMEDIA) \
                        androidboot.hardware=$(TARGET_DEVICE) $(cmdline_extra) vmalloc=172M \
                        debug_locks=0
endif

# Limit slub order on CRv2 wiht 1G RAM
BOARD_KERNEL_CMDLINE += slub_max_order=2

# Graphics
USE_OPENGL_RENDERER := true
BOARD_KERNEL_CMDLINE += vga=current i915.modeset=1 drm.vblankoffdelay=1 \
                        acpi_backlight=vendor

# System's VSYNC phase offsets in nanoseconds
VSYNC_EVENT_PHASE_OFFSET_NS := 7500000
SF_VSYNC_EVENT_PHASE_OFFSET_NS := 5000000

# Allow HWC to perform a final CSC on virtual displays
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true

# Graphics - MIPI
# List of panel ids supported:
#
# 1 - Reserved
# 2 - AUO_B101UAN01
# 3 - PANASONIC_VXX09F006A00
# 4 - AUO_B080XAT
# 5 - JDI_LPM070W425B
#
# The default is eDP (i.e., none of the above).
#
# Uncomment the following to enable support for AUO Mango mipi panel
# BOARD_KERNEL_CMDLINE += i915.mipi_panel_id=4

# Normal panel for FFRD8.  This is no longer the default.
#BOARD_KERNEL_CMDLINE += i915.mipi_panel_id=3

BOARD_USES_LIBPSS := false

INTEL_VA:=true
USE_INTEL_VA:=true
BOARD_USES_WRS_OMXIL_CORE:=true
BOARD_USES_MRST_OMX:=true
USE_INTEL_ASF_EXTRACTOR:=true
# enabled to use Intel secure AVC Stagefright HW decoder
USE_INTEL_SECURE_AVC := true
# enabled to use hardware VP8 decoder
USE_HW_VP8 := true

BOARD_USE_LIBVA_INTEL_DRIVER := true
BOARD_USE_LIBVA := true
BOARD_USE_LIBMIX := true

#Support background music playback for Widi Multitasking
ENABLE_BACKGROUND_MUSIC := true

# Settings for the Media SDK library and plug-ins:
# - USE_MEDIASDK: use Media SDK support or not
# - MFX_IPP: sets IPP library optimization to use
USE_MEDIASDK := true
# Settings for the Intel-optimized codecs and plug-ins:
USE_INTEL_MDP := true

ifeq ($(DOLBY_DAP),true)
# Disabled NXP Premium Audio Effect Libraries
USE_INTEL_LVSE := false
else
# Enabled NXP Premium Audio Effect Libraries
USE_INTEL_LVSE := false
#USE_INTEL_LVSE := true
endif

MFX_IPP := p8
# enabled to use Intel audio SRC (sample rate conversion)
USE_INTEL_SRC := true
# enabled to use ALAC
USE_FEATURE_ALAC := true

# Defines Intel library for GPU accelerated Renderscript:
OVERRIDE_RS_DRIVER := libRSDriver_intel7.so

# usb stick installer support
BOARD_KERNEL_DROIDBOOT_EXTRA_CMDLINE +=  droidboot.use_installer=1 droidboot.installer_usb=/dev/block/sda1


# Use shared object of ia_face
USE_SHARED_IA_FACE := true

# Use panorama v1.1
IA_PANORAMA_VERSION := 1.1

# Define Platform Sensor Hub firmware name
SENSORHUB_FW_NAME := psh_baylake.bin

# Sensor
USE_GENERAL_SENSOR_DRIVER := true

# disable libsensorhub
ENABLE_SENSOR_HUB := false

