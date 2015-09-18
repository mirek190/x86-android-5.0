AOBS_JSON := $(call get-specific-config-file ,scale/aobs.json)
CONFIGS_JSON := $(call get-specific-config-file ,scale/configs.json)
FRU_CONFIGS := $(PRODUCT_OUT)/fru_configs.xml
FRU_TOKEN_DIR := $(PRODUCT_OUT)/fru
FRU_TOKEN_TEMPLATE := $(call get-specific-config-file ,scale/fru_token_template.tmt)

build_fru:
	mkdir -p $(PRODUCT_OUT)/fru
	$(TOP)/vendor/intel/support/generate_fru.py \
	$(AOBS_JSON) \
	$(CONFIGS_JSON) \
	$(FRU_CONFIGS) \
	$(FRU_TOKEN_DIR) \
	"$(FRU_TOKEN_TEMPLATE)"

