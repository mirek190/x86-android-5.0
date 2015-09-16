# All catalog files found get automatically installed on the target
FOUND_CONFIGS := $(shell find $(LOCAL_PATH)/catalog -type f)
PRODUCT_COPY_FILES += $(foreach _conf, $(FOUND_CONFIGS), \
     $(_conf):system/etc/$(patsubst $(LOCAL_PATH)/%,%,$(_conf)))

# Add config service
PRODUCT_COPY_FILES += \
    $(COMMON_PATH)/config_init.sh:system/etc/catalog/config_init.sh \
    $(COMMON_PATH)/config_props.sh:system/etc/catalog/config_props.sh \
    $(COMMON_PATH)/init.config_init.rc:root/init.config_init.rc
