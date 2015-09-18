
containing = $(strip $(foreach v,$2,$(if $(findstring $1,$v),$v)))

include $(COMMON_PATH)/gps/ChipVendor.mk

ifeq (,$(filter none,$(GPS_CHIP_VENDOR) $(GPS_CHIP)))

##################################################

-include $(COMMON_PATH)/gps/$(GPS_CHIP_VENDOR)/BoardConfig.mk

##################################################

ifneq (,$(call containing,cpd,$(filter gps_%,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES))))
GPS_USES_CP_DAEMON := true
endif

ifneq (,$(call containing,extlna,$(filter gps_%,$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES))))
GPS_USES_EXTERNAL_LNA := true
endif

##################################################

endif
