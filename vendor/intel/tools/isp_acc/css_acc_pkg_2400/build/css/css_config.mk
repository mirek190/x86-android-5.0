ifeq "$(SYSTEM)" "hive_isp_css_2400_system"
HIVEFLEX_SP = scalar_processor_2400
HIVEFLEX_ISP = isp2400_mamoiada
GDC = gdc_v2
RX = css_receiver_2400
DMA = dma_v2
IF = InputFormatter_sysblock
IFBIN = Stream2Memory
IFSW = input_switch_2400
ACQ = isp_acquisition
CAPT = isp_capture
ISYS = input_system_ctrl
endif
ifeq "$(SYSTEM)" "hive_isp_css_2401_system"
HIVEFLEX_SP = scalar_processor_2400
HIVEFLEX_ISP = isp2401_mamoiada
GDC = gdc_v2
RX = css_receiver_2400
IF = InputFormatter_sysblock
IFBIN = Stream2Memory
IFSW = input_switch_2400
DMA = dma_v2
ACQ = isp_acquisition
CAPT = isp_capture_2401
ISYS = input_system_ctrl
endif
ifeq ($(SYSTEM),$(filter $(SYSTEM), css_skycam_a0t_system css_skycam_c0_system))
HIVEFLEX_SP = scalar_processor_2500
HIVEFLEX_ISP = isp2500_skycam
GDC = gdc_v3
RX = none
DMA = dma_v2
IF = none
IFBIN = none
IFSW = none
ACQ = none
CAPT = none
ISYS = none
endif


# For 2401 the same DLI is used as for 2400
#
ifeq "$(SYSTEM)" "hive_isp_css_2401_system"
DLI_SYSTEM = hive_isp_css_2400_system
else
ifeq ($(SYSTEM),$(filter $(SYSTEM), css_skycam_a0t_system css_skycam_c0_system))
DLI_SYSTEM = css_skycam_system
else
DLI_SYSTEM = $(SYSTEM)
endif
endif

DIR_PREFIX = $(SYSTEM)_
# CSS_INSTALL_DIR = $(HIVE_CVSWORK)/$(UNAME)/host_$(SYSTEM)
SPLIB_INSTALL_DIR = $(HIVE_CVSWORK)/$(UNAME)/sp_$(SYSTEM)/$(TARGETS)/lib
ifeq "$(SYSTEM)" "hive_isp_css_2401_system"
  ifeq "$(CONFIG_CSI2_PLUS)" "1"
    # For 2401 with new input system
    ifeq "$(BXT_POC)" "1"
      DIR_PREFIX  = $(SYSTEM)_bxtpoc_
      SPLIB_INSTALL_DIR = $(HIVE_CVSWORK)/$(UNAME)/sp_$(SYSTEM)_bxtpoc/$(TARGETS)/lib
    else
      SPLIB_INSTALL_DIR = $(HIVE_CVSWORK)/$(UNAME)/sp_$(SYSTEM)_csi2p/$(TARGETS)/lib
      DIR_PREFIX = $(SYSTEM)_csi2p_
    endif
endif
endif
