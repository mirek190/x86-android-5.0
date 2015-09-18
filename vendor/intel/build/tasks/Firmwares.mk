# Find all ifwi selected through PRODUCT_PACKAGES
# Define capsule and dediprog target to be used by json file to generate
# blankphone and flashfiles.

INTERNAL_FIRMWARE_FILES := $(filter $(PRODUCT_OUT)/ifwi/%, \
       $(ALL_PREBUILT) \
       $(ALL_COPIED_HEADERS) \
       $(ALL_GENERATED_SOURCES) \
       $(ALL_DEFAULT_INSTALLED_MODULES))

# dediprogs are in $(PRODUCT_OUT)/ifwi/PACKAGE_NAME/dediprog/
INSTALLED_DEDIPROG_TARGET := $(strip $(foreach fw,$(INTERNAL_FIRMWARE_FILES),\
    $(if $(filter %/dediprog/,$(dir $(fw))),$(fw))))
INSTALLED_CAPSULE_TARGET := $(strip $(foreach fw,$(INTERNAL_FIRMWARE_FILES),\
    $(if $(filter %/capsule/,$(dir $(fw))),$(fw))))
INSTALLED_STAGE2_TARGET := $(strip $(foreach fw,$(INTERNAL_FIRMWARE_FILES),\
    $(if $(filter %/stage2/,$(dir $(fw))),$(fw))))

ifneq (,$(INSTALLED_CAPSULE_TARGET))
INSTALLED_OTA_CAPSULE_TARGET := $(PRODUCT_OUT)/capsule.bin

INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_OUT)/capsule.bin
$(BUILT_TARGET_FILES_PACKAGE): $(INSTALLED_OTA_CAPSULE_TARGET)

$(INSTALLED_OTA_CAPSULE_TARGET): $(INSTALLED_CAPSULE_TARGET) | $(ACP)
	$(ACP) $^ $@
endif #INSTALLED_CAPSULE_TARGET
