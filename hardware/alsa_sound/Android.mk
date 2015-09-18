ifeq ($(BOARD_USES_ALSA_AUDIO),true)
ifeq ($(BOARD_USES_AUDIO_HAL_CONFIGURABLE),true)

# Recursive call sub-folder Android.mk

include $(call all-subdir-makefiles)

endif #ifeq ($(BOARD_USES_AUDIO_HAL_CONFIGURABLE),true)
endif #ifeq ($(BOARD_USES_ALSA_AUDIO),true)


