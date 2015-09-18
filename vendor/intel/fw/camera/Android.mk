LOCAL_PATH:= $(call my-dir)

# ISP and FR
camera_libs := shisp_css15.bin shisp_2400b0_v21.bin shisp_2401a0_legacy_v21.bin shisp_2401a0_v21.bin shisp_2401a0_v21_bxtpoc.bin

# HDR FW CSS1.5 ISP2300
hdr_css15_2300_libs := isp_acc_multires_css15.bin isp_acc_warping_css15.bin isp_acc_deghosting_css15.bin \
isp_acc_fusion_css15.bin isp_acc_postproc_css15.bin

# HDR v2 FW CSS2.1 ISP2400B0
hdr_v2_css21_2400b0_libs := isp_acc_multires_v2_css21_2400b0.bin isp_acc_warping_v2_css21_2400b0.bin isp_acc_deghosting_v2_css21_2400b0.bin \
isp_acc_lumaproc_css21_2400b0.bin isp_acc_chromaproc_css21_2400b0.bin
# HDR v2 FW CSS2.1 ISP2401
hdr_v2_css21_2401_libs := isp_acc_multires_v2_css21_2401.bin isp_acc_warping_v2_css21_2401.bin isp_acc_deghosting_v2_css21_2401.bin \
isp_acc_lumaproc_css21_2401.bin isp_acc_chromaproc_css21_2401.bin
# ULL v1.5 FW CSS2.1 ISP2400B0
ull_v15_css21_2400b0_libs := isp_acc_warping_v2_em_css21_2400b0.bin isp_acc_mfnr_em_css21_2400b0.bin isp_acc_sce_em_css21_2400b0.bin
# ULL v1.5 FW CSS2.1 ISP2401
ull_v15_css21_2401_libs := isp_acc_warping_v2_em_css21_2401.bin isp_acc_mfnr_em_css21_2401.bin isp_acc_sce_em_css21_2401.bin

# function to copy firmware libraries to /etc/firmware
define camera-prebuilt-boilerplate
$(foreach t,$(1), \
  $(eval include $(CLEAR_VARS)) \
  $(eval tw := $(subst :, ,$(strip $(t)))) \
  $(eval LOCAL_MODULE := $(tw)) \
  $(eval LOCAL_MODULE_OWNER := intel) \
  $(eval LOCAL_MODULE_TAGS := optional) \
  $(eval LOCAL_MODULE_CLASS := ETC) \
  $(eval LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware) \
  $(eval LOCAL_SRC_FILES := $(tw)) \
  $(eval include $(BUILD_PREBUILT)) \
)
endef

# build ISP and FW
$(call camera-prebuilt-boilerplate, \
    $(camera_libs))

# CSS1.5 ISP2300
include $(CLEAR_VARS)
LOCAL_MODULE := hdr_fw_css15_2300
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(hdr_css15_2300_libs)
include $(BUILD_PHONY_PACKAGE)

$(call camera-prebuilt-boilerplate, \
    $(hdr_css15_2300_libs))

# build HDR v2 FW CSS2.1 ISP2400B0
include $(CLEAR_VARS)
LOCAL_MODULE := hdr_v2_fw_css21_2400b0
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(hdr_v2_css21_2400b0_libs)
include $(BUILD_PHONY_PACKAGE)

$(call camera-prebuilt-boilerplate, \
    $(hdr_v2_css21_2400b0_libs))

# build HDR v2 FW CSS2.1 ISP2401
include $(CLEAR_VARS)
LOCAL_MODULE := hdr_v2_fw_css21_2401
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(hdr_v2_css21_2401_libs)
include $(BUILD_PHONY_PACKAGE)

$(call camera-prebuilt-boilerplate, \
    $(hdr_v2_css21_2401_libs))

# build ULL v1.5 FW CSS2.1 ISP2400B0
include $(CLEAR_VARS)
LOCAL_MODULE := ull_v15_fw_css21_2400b0
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(ull_v15_css21_2400b0_libs)
include $(BUILD_PHONY_PACKAGE)

$(call camera-prebuilt-boilerplate, \
    $(ull_v15_css21_2400b0_libs))

# build ULL v1.5 FW CSS2.1 ISP2401
include $(CLEAR_VARS)
LOCAL_MODULE := ull_v15_fw_css21_2401
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(ull_v15_css21_2401_libs)
include $(BUILD_PHONY_PACKAGE)

$(call camera-prebuilt-boilerplate, \
    $(ull_v15_css21_2401_libs))

include $(CLEAR_VARS)
LOCAL_MODULE := ap1302_fw.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := ov680_fw.bin
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
