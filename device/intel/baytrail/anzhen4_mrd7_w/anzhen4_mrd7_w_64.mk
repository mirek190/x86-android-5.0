PRODUCT_NAME := anzhen4_mrd7_w_64
REF_PRODUCT_NAME := byt_t_crv2

BOARD_USE_64BIT_KERNEL := true

SUPPORT_3G_DONGLE_ONLY := true

ifeq ($(SUPPORT_3G_DONGLE_ONLY),true)
# Dongle files
PRODUCT_COPY_FILES += \
               $(LOCAL_PATH)/dongle/connect-chat:system/etc/ppp/connect-chat \
               $(LOCAL_PATH)/dongle/ip-up:system/etc/ppp/ip-up \
               $(LOCAL_PATH)/dongle/ip-down:system/etc/ppp/ip-down \
               $(LOCAL_PATH)/dongle/chat:system/xbin/chat \
               $(LOCAL_PATH)/dongle/usb_modeswitch:system/xbin/usb_modeswitch \
                $(LOCAL_PATH)/dongle/gsm_ppp_dialer:system/etc/ppp/gsm_ppp_dialer \
                $(LOCAL_PATH)/dongle/cdma_ppp_dialer:system/etc/ppp/cdma_ppp_dialer \
                $(LOCAL_PATH)/dongle/init.gprs-pppd:system/etc/init.gprs-pppd  \
                $(LOCAL_PATH)/dongle/usb_modeswitch.conf:system/etc/usb_modeswitch.conf
endif
include $(LOCAL_PATH)/anzhen4_mrd7_w_path.mk

include $(LOCAL_PATH)/anzhen4_mrd7_w.mk
