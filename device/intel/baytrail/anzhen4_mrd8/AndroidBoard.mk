include $(PLATFORM_PATH)/AndroidBoard.mk

# parameter-framework
include $(DEVICE_CONF_PATH)/parameter-framework/AndroidBoard.mk

# mamgr
include $(COMMON_PATH)/mamgr/xmm_single_modem_single_sim.mk

# Common config
include $(COMMON_PATH)/config/AndroidBoard.mk

# FG config file
include $(DEVICE_CONF_PATH)/fg_config/config1/AndroidBoard.mk
include $(DEVICE_CONF_PATH)/fg_config/config2/AndroidBoard.mk

