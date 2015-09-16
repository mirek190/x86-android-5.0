###########################################################
## Look for config files
###########################################################
# Get all existing files from generic to specific
# #   -> common -> platform -> device -> product
# # $(1): [path/]file_name (can include wildcards)
define get-all-config-files
 $(wildcard $(COMMON_PATH)/$(1)) \
     $(wildcard device/intel/$(TARGET_BOARD_PLATFORM)/$(1)) \
     $(wildcard $(TARGET_DEVICE_DIR)/$(1)) \
     $(wildcard $(TARGET_DEVICE_DIR)/$(1).$(TARGET_PRODUCT))
endef

# # Get the most specific version of a config file available
# # $(1): [path/]file_name (can include wildcards)
define get-specific-config-file
$(lastword $(call get-all-config-files,$(1)))
endef
