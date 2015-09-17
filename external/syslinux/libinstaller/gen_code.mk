SOURCE := $(LOCAL_PATH)/../core/ldlinux.bss
GENSOURCE := bootsect_bin.c

SL_BS_BIN_C := $(intermediates)/$(GENSOURCE)
$(SL_BS_BIN_C): PRIVATE_PATH := $(LOCAL_PATH)
$(SL_BS_BIN_C): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/bin2c.pl syslinux_bootsect < $< > $@
$(SL_BS_BIN_C): $(SOURCE) $(LOCAL_PATH)/bin2c.pl
	$(transform-generated-source)

SOURCE := $(LOCAL_PATH)/../core/ldlinux.sys
GENSOURCE := ldlinux_bin.c

SL_BS_BIN_C := $(intermediates)/$(GENSOURCE)
$(SL_BS_BIN_C): PRIVATE_PATH := $(LOCAL_PATH)
$(SL_BS_BIN_C): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/bin2c.pl syslinux_ldlinux 512 < $< > $@
$(SL_BS_BIN_C): $(SOURCE) $(LOCAL_PATH)/bin2c.pl
	$(transform-generated-source)

SOURCE := $(LOCAL_PATH)/../mbr/mbr.bin
GENSOURCE := mbr_bin.c

SL_BS_BIN_C := $(intermediates)/$(GENSOURCE)
$(SL_BS_BIN_C): PRIVATE_PATH := $(LOCAL_PATH)
$(SL_BS_BIN_C): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/bin2c.pl syslinux_mbr < $< > $@
$(SL_BS_BIN_C): $(SOURCE) $(LOCAL_PATH)/bin2c.pl
	$(transform-generated-source)

SOURCE := $(LOCAL_PATH)/../mbr/gptmbr.bin
GENSOURCE := gptmbr_bin.c

SL_BS_BIN_C := $(intermediates)/$(GENSOURCE)
$(SL_BS_BIN_C): PRIVATE_PATH := $(LOCAL_PATH)
$(SL_BS_BIN_C): PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/bin2c.pl syslinux_gptmbr < $< > $@
$(SL_BS_BIN_C): $(SOURCE) $(LOCAL_PATH)/bin2c.pl
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES := \
	$(intermediates)/bootsect_bin.c \
	$(intermediates)/ldlinux_bin.c \
	$(intermediates)/mbr_bin.c \
	$(intermediates)/gptmbr_bin.c
