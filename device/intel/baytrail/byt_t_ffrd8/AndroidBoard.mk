include $(PLATFORM_PATH)/AndroidBoard.mk

# parameter-framework
include $(DEVICE_CONF_PATH)/parameter-framework/AndroidBoard.mk

# mamgr (Modem Audio Manager)
include $(COMMON_PATH)/mamgr/xmm_single_modem_single_sim.mk

# FG config file
include $(DEVICE_CONF_PATH)/fg_config/AndroidBoard.mk

# Common config
include $(COMMON_PATH)/config/AndroidBoard.mk
