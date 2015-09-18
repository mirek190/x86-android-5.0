# Common variable

PUBLISH_PATH := pub
PUBLISH_TOOLS_PATH := $(PUBLISH_PATH)/tools

TARGET_PUBLISH_PATH ?= $(shell echo $(TARGET_PRODUCT) | tr '[:lower:]' '[:upper:]')
PUBLISH_TARGET := $(PUBLISH_PATH)/$(TARGET_PUBLISH_PATH)

ifeq ($(LEGACY_PUBLISH),false)

PUB_FLASHFILES := $(PUBLISH_TARGET)/flash_files/build-$(TARGET_BUILD_VARIANT)
PUB_SIGNATURE := $(PUBLISH_TARGET)/signing_keys/$(TARGET_BUILD_VARIANT)
PUB_BLANKFILES := $(PUBLISH_TARGET)/flash_files/blankphone

FLASHFILES_CONFIG ?= $(COMMON_PATH)/default_flashfiles.json
BLANK_FLASHFILES_CONFIG ?= $(COMMON_PATH)/default_blankflashfiles.json
OTA_CONFIG ?= $(COMMON_PATH)/default_ota_flashfiles.json

$(eval FLASHFILES_T2F := $(shell ./vendor/intel/support/flashdep.py $(FLASHFILES_CONFIG)))
$(eval BLANK_T2F := $(shell ./vendor/intel/support/flashdep.py $(BLANK_FLASHFILES_CONFIG)))
$(eval OTA_T2F := $(shell ./vendor/intel/support/flashdep.py $(OTA_CONFIG)))

FLASHFILES_IMAGES := $(foreach dep,$(FLASHFILES_T2F),$(word 2,$(subst :, ,$(dep))))
BLANK_IMAGES := $(foreach dep,$(BLANK_T2F),$(word 2,$(subst :, ,$(dep))))
OTA_IMAGES := $(foreach dep,$(OTA_T2F),$(word 2,$(subst :, ,$(dep))))

FLASHFILES_XML := $(shell ./vendor/intel/support/flashtarget.py $(FLASHFILES_CONFIG))
FLASHFILES_XML := $(addprefix $(PRODUCT_OUT)/,$(FLASHFILES_XML))
FIRST_FLASHFILES_XML := $(firstword $(FLASHFILES_XML))
OTHER_FLASHFILES_XML := $(wordlist 2,9,$(FLASHFILES_XML))

BLANK_XML := $(shell ./vendor/intel/support/flashtarget.py $(BLANK_FLASHFILES_CONFIG))
BLANK_XML := $(addprefix $(PUB_BLANKFILES)/,$(BLANK_XML))
FIRST_BLANK_XML := $(firstword $(BLANK_XML))
OTHER_BLANK_XML := $(wordlist 2,9,$(BLANK_XML))

# Flashfiles
.PHONY: flashfiles
flashfiles: fastboot_flashfile

# Makefile cannot handle 1 command with multiple output, so use the first one
$(FIRST_FLASHFILES_XML): $(FLASHFILES_CONFIG) $(FLASHFILES_IMAGES)
	$(hide) ./vendor/intel/support/flashxml.py -c $< -p $(REF_PRODUCT_NAME) -b $(FILE_NAME_TAG) -d $(@D) -t "$(FLASHFILES_T2F)"

# Add dependency for other xmls
$(OTHER_FLASHFILES_XML): $(FIRST_FLASHFILES_XML)

.PHONY: pub_flashfiles
pub_flashfiles: $(FLASHFILES_IMAGES) $(FLASHFILES_XML)| $(ACP)
	@echo copying flashfiles: $(notdir $^)
	$(hide) mkdir -p $(PUB_FLASHFILES)
	$(hide) $(ACP) $^ $(PUB_FLASHFILES)/

.PHONY: flashfiles_nozip
flashfiles_nozip: $(FLASHFILES_XML) $(FLASHFILES_IMAGES)

PUB_ZIP := $(PUB_FLASHFILES)/$(TARGET_PRODUCT)-$(TARGET_BUILD_VARIANT)-fastboot-$(FILE_NAME_TAG).zip

.PHONY: fastboot_flashfile
fastboot_flashfile: $(PUB_ZIP)

$(PUB_ZIP): $(FLASHFILES_IMAGES) $(FLASHFILES_XML)
	$(hide) mkdir -p $(@D)
	$(hide) rm -f $@
	@echo generating $@
	$(hide) zip -j $@ $^

PUB_SIG_FILES := $(addprefix $(KERNEL_OUT_DIR)/,signing_key.priv signing_key.x509)

.PHONY: publish_signature
publish_signature: build_kernel | $(ACP)
	$(hide) mkdir -p $(PUB_SIGNATURE)
	-$(hide) $(ACP) $(PUB_SIG_FILES) $(PUB_SIGNATURE)/

ifneq (,$(filter $(TARGET_BUILD_VARIANT),eng userdebug))
flashfiles: publish_signature
endif

# Blankphone
$(FIRST_BLANK_XML): $(BLANK_FLASHFILES_CONFIG) $(BLANK_IMAGES)
	$(hide) mkdir -p $(@D)
	$(hide) ./vendor/intel/support/flashxml.py -c $< -p $(REF_PRODUCT_NAME) -b $(FILE_NAME_TAG) -d $(@D) -t "$(BLANK_T2F)"

$(OTHER_BLANK_XML): $(FIRST_BLANK_XML)

.PHONY: pub_blank_flashfiles
pub_blank_flashfiles: $(BLANK_IMAGES) | $(BLANK_XML) $(ACP)
	@echo copying blank_flashfiles: $(notdir $^)
	$(hide) mkdir -p $(PUB_BLANKFILES)
	$(hide) $(ACP) $^ $(PUB_BLANKFILES)

PUB_BLANK_ZIP := $(PUB_BLANKFILES)/$(TARGET_PRODUCT)-blankphone.zip
$(PUB_BLANK_ZIP): $(BLANK_IMAGES) $(BLANK_XML)
	$(hide) mkdir -p $(@D)
	$(hide) rm -f $@
	@echo generating $@
	$(hide) zip -j $@ $^
	$(hide) rm -f $(BLANK_XML)

.PHONY: blank_flashfiles
blank_flashfiles: $(PUB_BLANK_ZIP)

# OTA
PUB_OTA_INPUT := $(PUBLISH_TARGET)/ota_inputs/$(TARGET_BUILD_VARIANT)/$(notdir $(BUILT_TARGET_FILES_PACKAGE))
$(PUB_OTA_INPUT): $(BUILT_TARGET_FILES_PACKAGE) | $(ACP)
	$(hide) mkdir -p $(@D)
	$(hide) $(ACP) $^ $@

PUB_OTA_FILE := $(PUBLISH_TARGET)/fastboot-images/$(TARGET_BUILD_VARIANT)/$(notdir $(INTERNAL_OTA_PACKAGE_TARGET))
$(PUB_OTA_FILE): $(INTERNAL_OTA_PACKAGE_TARGET) | $(ACP)
	$(hide) mkdir -p $(@D)
	$(hide) $(ACP) $^ $@

PUB_OTA_XML := $(PUBLISH_TARGET)/fastboot-images/$(TARGET_BUILD_VARIANT)/flash.xml
$(PUB_OTA_XML): $(OTA_CONFIG)
	$(hide) mkdir -p $(@D)
	$(hide) ./vendor/intel/support/flashxml.py -c $< -p $(REF_PRODUCT_NAME) -b $(FILE_NAME_TAG) -d $(@D) -t "$(OTA_T2F)"

PUB_OTA_ZIP := $(PUB_FLASHFILES)/$(TARGET_PRODUCT)-$(TARGET_BUILD_VARIANT)-ota-$(FILE_NAME_TAG).zip
$(PUB_OTA_ZIP): $(PUB_OTA_XML) $(INTERNAL_OTA_PACKAGE_TARGET)
	$(hide) mkdir -p $(@D)
	$(hide) zip -j $@ $^

.PHONY: ota_flashfile
ota_flashfile: $(PUB_OTA_ZIP) $(PUB_OTA_INPUT) $(PUB_OTA_FILE)

PUB_IFWI_DIR := $(PUBLISH_TARGET)/IFWI
PUB_IFWI := $(subst $(PRODUCT_OUT)/ifwi,$(PUB_IFWI_DIR),$(INTERNAL_FIRMWARE_FILES))

$(PUB_IFWI_DIR)/%: $(PRODUCT_OUT)/ifwi/% | $(ACP)
	@echo "Publish ifwi $(notdir $@)"
	$(hide) mkdir -p $(@D)
	$(hide) $(ACP) $< $@

.PHONY: publish_ifwi
publish_ifwi: $(PUB_IFWI)

PUB_INTEL_PREBUILTS := $(PUBLISH_TARGET)/prebuilts.zip

EXTERNAL_CUSTOMER ?= "g"

INTEL_PREBUILTS_LIST := $(shell repo forall -g bsp-priv -a $(EXTERNAL_CUSTOMER)_external=bin -p -c echo 2> /dev/null)
INTEL_PREBUILTS_LIST := $(filter-out project,$(INTEL_PREBUILTS_LIST))
INTEL_PREBUILTS_LIST := $(addprefix prebuilts/intel/, $(subst /PRIVATE/,/prebuilts/$(REF_PRODUCT_NAME)/,$(INTEL_PREBUILTS_LIST)))
INTEL_PREBUILTS_LIST += prebuilts/intel/Android.mk

$(PUB_INTEL_PREBUILTS): generate_intel_prebuilts
	@echo "Publish prebuilts for external release"
	$(hide) rm -f $@
	$(hide) cd $(PRODUCT_OUT) && zip -r $(abspath $@) $(INTEL_PREBUILTS_LIST)

# publish external if buildbot set EXTERNAL_BINARIES env variable
# and only for userdebug
ifeq (userdebug,$(TARGET_BUILD_VARIANT))
ifeq ($(EXTERNAL_BINARIES),true)
publish_intel_prebuilts: $(PUB_INTEL_PREBUILTS)
endif
endif

ifneq ($(PUBLISH_CONF),)
BUILDBOT_PUBLISH_DEPS := $(shell python -c 'import json,os ; print " ".join(json.loads(os.environ["PUBLISH_CONF"]).get("$(TARGET_BUILD_VARIANT)",[]))')

# Translate buildbot target to makefile target
flashfiles: $(BUILDBOT_PUBLISH_DEPS)

full_ota: $(PUB_OTA_FILE)
full_ota_flashfile: $(PUB_OTA_ZIP)
ota_target_files: $(PUB_OTA_INPUT)
system_img:
endif # PUBLISH_CONF

endif # LEGACY_PUBLSH != true

# Common Publish

# System symbols
PUB_SYSTEM_SYMBOLS := $(PUBLISH_TARGET)/fastboot-images/$(TARGET_BUILD_VARIANT)/symbols.tar.gz

$(PUB_SYSTEM_SYMBOLS): systemtarball
	@echo "Publish system symbols"
	$(hide) mkdir -p $(@D)
	$(hide) tar czf $@ $(PRODUCT_OUT)/symbols

.PHONY: publish_system_symbols
publish_system_symbols: $(PUB_SYSTEM_SYMBOLS)

# Kernel debug
PUB_KERNEL_DBG := .config.bz2 vmlinux.bz2 System.map.bz2
PUB_KERNEL_DBG_PATH := $(PUBLISH_TARGET)/kernel
PUB_KERNEL_DBG := $(addprefix $(PUB_KERNEL_DBG_PATH)/,$(PUB_KERNEL_DBG))

$(PUB_KERNEL_DBG_PATH)/%: build_kernel | $(ACP)
	$(hide) mkdir -p $(@D)
	$(hide) bzip2 -c $(PRODUCT_OUT)/linux/kernel/$(basename $(@F)) > $@

PUB_KERNEL_MODULES = $(PUB_KERNEL_DBG_PATH)/kernel_modules-$(TARGET_BUILD_VARIANT).tar.bz2

$(PUB_KERNEL_MODULES): build_kernel
	$(hide) mkdir -p $(@D)
	$(hide) tar -cjf $@ -C $(PRODUCT_OUT)/root/lib/modules .

.PHONY: publish_kernel_debug
publish_kernel_debug: $(PUB_KERNEL_DBG) $(PUB_KERNEL_MODULES)
	@echo "Publish kernel debug: $(notdir $^)"

# Linux tools
PUB_LINUX_TOOLS_DIR := $(PUBLISH_TOOLS_PATH)/linux-x86/bin
PUB_LINUX_TOOLS := adb fastboot
PUB_LINUX_TOOLS := $(addprefix $(PUB_LINUX_TOOLS_DIR)/,$(PUB_LINUX_TOOLS))

$(PUB_LINUX_TOOLS_DIR)/%: $(HOST_OUT_EXECUTABLES)/%
	$(hide) mkdir -p $(@D)
	$(hide) $(ACP) $^ $@

.PHONY: publish_linux_tools
publish_linux_tools: $(PUB_LINUX_TOOLS)
	@echo "Publish linux tools"
