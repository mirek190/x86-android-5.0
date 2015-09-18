/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2013 Intel Corporation.
 * All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or licensors. Title to the Material remains with Intel
 * Corporation or its licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No License under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#include "isp_globals.hive.h"

#include "isp_defs.h"

#if ENABLE_SDIS && SDIS_VERSION==1
#define G_SDIS_HORIPROJ_TBL_ADR		(int*)(&isp_dmem_parameters.sdis_horiproj)
#define G_SDIS_VERTPROJ_TBL_ADR		(int*)(&isp_dmem_parameters.sdis_vertproj)
#define G_SDIS_HORIPROJ_TBL_SIZE	sizeof(isp_dmem_parameters.sdis_horiproj)
#define G_SDIS_VERTPROJ_TBL_SIZE	sizeof(isp_dmem_parameters.sdis_vertproj)
#else
#define G_SDIS_HORIPROJ_TBL_ADR		NULL
#define G_SDIS_VERTPROJ_TBL_ADR		NULL
#define G_SDIS_HORIPROJ_TBL_SIZE	0
#define G_SDIS_VERTPROJ_TBL_SIZE	0
#endif

#if ENABLE_OUTPUT && MODE != IA_CSS_BINARY_MODE_COPY
#define OUTPUT_DMA_INFO_ADR		&output_dma_info[0][0][0]
#else
#define OUTPUT_DMA_INFO_ADR		NULL
#endif

#if ENABLE_OUTPUT && MODE != IA_CSS_BINARY_MODE_COPY && VARIABLE_OUTPUT_FORMAT
#define VERTICAL_UPSAMPLED_ADR 		&vertical_upsampled
#else
#define VERTICAL_UPSAMPLED_ADR 		NULL
#endif

#if ENABLE_XNR
#define ISP_ENABLE_XNR &isp_enable_xnr
#else
#define ISP_ENABLE_XNR NULL
#endif

#if MODE == IA_CSS_BINARY_MODE_GDC
#define ISP_GDAC_CONFIG &isp_gdcac_config
#else
#define ISP_GDAC_CONFIG NULL
#endif

#if ENABLE_FLIP
#define ENABLE_HFLIP_ADDR &isp_dmem_parameters.output.enable_hflip
#define ENABLE_VFLIP_ADDR &isp_dmem_parameters.output.enable_vflip
#else
#define ENABLE_HFLIP_ADDR NULL
#define ENABLE_VFLIP_ADDR NULL
#endif

#if ENABLE_OUTPUT
#if NUM_OUTPUT_PINS > 1
#define ISP_OUTPUT_STRIDE_B &isp_dmem_configurations.output0.port_b.stride
#else
#define ISP_OUTPUT_STRIDE_B &isp_dmem_configurations.output.port_b.stride
#endif
#else
#define ISP_OUTPUT_STRIDE_B NULL
#endif

#define ISP_DMEM_ADDRESSES \
{ \
  uds_params, \
  &isp_deci_log_factor, \
  &isp_frames, \
  &isp_globals, \
  &isp_online, \
  OUTPUT_DMA_INFO_ADR, \
  VERTICAL_UPSAMPLED_ADR, \
  &xmem_base, \
  &s3a_data, \
  &sdis_data, \
  (sh_dma_cmd**)&sh_dma_cmd_ptr, \
  &g_isp_do_zoom, \
  &uds_config, \
  G_SDIS_HORIPROJ_TBL_ADR, \
  G_SDIS_VERTPROJ_TBL_ADR, \
  ISP_ENABLE_XNR, \
  ISP_GDAC_CONFIG, \
  &stripe_id, \
  &stripe_row_id, \
  &isp_continuous, \
  &required_bds_factor, \
  &isp_raw_stride_b, \
  &isp_raw_block_width_b, \
  &isp_raw_line_width_b, \
  &isp_raw_stripe_offset_b, \
  ISP_OUTPUT_STRIDE_B, \
  ENABLE_HFLIP_ADDR, \
  ENABLE_VFLIP_ADDR, \
}

/* Disabled, since it is not an constant expression for fish */
#if 0 &&defined(__HIVECC)
#define VECTOR_ADDRESS(v) ((PVECTOR)((unsigned)(&v) * ISP_VMEM_ALIGN))
#else
#define VECTOR_ADDRESS(v) ((PVECTOR)(&v))
#endif

#if !defined(IS_ISP_2500_SYSTEM) && \
   (MODE == IA_CSS_BINARY_MODE_PRIMARY  || \
    MODE == IA_CSS_BINARY_MODE_PREVIEW  || \
    MODE == IA_CSS_BINARY_MODE_VIDEO    || \
    MODE == IA_CSS_BINARY_MODE_PRE_ISP  || \
    MODE == IA_CSS_BINARY_MODE_PRE_DE   || \
    ENABLE_CONTINUOUS)
input_line_type SYNC_WITH(INPUT_BUF) MEM(VMEM) AT(INPUT_BUF_ADDR) input_buf;
#define INPUT_BUF_ADR VECTOR_ADDRESS(input_buf)
#else
#define INPUT_BUF_ADR NULL
#endif

#if ENABLE_MACC
#define G_MACC_COEF_ADR VECTOR_ADDRESS(g_macc_coef)
#else
#define G_MACC_COEF_ADR NULL
#endif

#if ENABLE_UDS || ENABLE_DS || CAN_SCALE
#define UDS_DATA_VIA_SP_ADR	VECTOR_ADDRESS(uds_data_via_sp)
#else
#define UDS_DATA_VIA_SP_ADR	NULL
#endif

#if ISP_NWAY==16 && (ENABLE_UDS || ENABLE_DS || CAN_SCALE)
#define UDS_IPXS_VIA_SP_ADR     VECTOR_ADDRESS(uds_ipxs_via_sp)
#define UDS_IBUF_VIA_SP_ADR     VECTOR_ADDRESS(uds_ibuf_via_sp)
#define UDS_OBUF_VIA_SP_ADR     VECTOR_ADDRESS(uds_obuf_via_sp)
#else
#define UDS_IPXS_VIA_SP_ADR     NULL
#define UDS_IBUF_VIA_SP_ADR     NULL
#define UDS_OBUF_VIA_SP_ADR     NULL
#endif

#if NEED_BDS_OTHER_THAN_1_00
#define RAW_FIR_BUF_ADR 	VECTOR_ADDRESS(raw_fir_buf)
#define RAW_FIR_BUF_SIZE	sizeof(raw_fir_buf)
#if NEED_BDS_CASCADE
#define RAW_FIR1_BUF_ADR 	VECTOR_ADDRESS(raw_fir1_buf)
#define RAW_FIR1_BUF_SIZE	sizeof(raw_fir1_buf)
#define RAW_FIR2_BUF_ADR 	VECTOR_ADDRESS(raw_fir2_buf)
#define RAW_FIR2_BUF_SIZE	sizeof(raw_fir2_buf)
#else
#define RAW_FIR1_BUF_ADR 	NULL
#define RAW_FIR1_BUF_SIZE	0
#define RAW_FIR2_BUF_ADR 	NULL
#define RAW_FIR2_BUF_SIZE	0
#endif // NEED_BDS_CASCADE
#else
#define RAW_FIR_BUF_ADR 	NULL
#define RAW_FIR_BUF_SIZE	0
#define RAW_FIR1_BUF_ADR 	NULL
#define RAW_FIR1_BUF_SIZE	0
#define RAW_FIR2_BUF_ADR 	NULL
#define RAW_FIR2_BUF_SIZE	0
#endif //NEED_BDS_OTHER_THAN_1_00

#define ISP_VMEM_ADDRESSES \
{ \
  INPUT_BUF_ADR, \
  G_MACC_COEF_ADR, \
  UDS_DATA_VIA_SP_ADR, \
  UDS_IPXS_VIA_SP_ADR, \
  UDS_IBUF_VIA_SP_ADR, \
  UDS_OBUF_VIA_SP_ADR, \
  RAW_FIR_BUF_ADR, \
  RAW_FIR1_BUF_ADR, \
  RAW_FIR2_BUF_ADR, \
}

#define ISP_SIZES \
{ \
  sizeof(uds_params), \
  G_SDIS_HORIPROJ_TBL_SIZE, \
  G_SDIS_VERTPROJ_TBL_SIZE, \
  RAW_FIR_BUF_SIZE, \
  RAW_FIR1_BUF_SIZE, \
  RAW_FIR2_BUF_SIZE, \
}

/* Initialized by the SP: binary dependent */
s_isp_globals   NO_SYNC NO_HOIST isp_globals;
s_isp_addresses NO_SYNC NO_HOIST isp_addresses = { ISP_DMEM_ADDRESSES, ISP_VMEM_ADDRESSES, ISP_SIZES };

unsigned isp_deci_log_factor;
unsigned isp_online;

struct sh_css_ddr_address_map xmem_base;
struct ia_css_isp_3a_statistics s3a_data;
struct ia_css_isp_dvs_statistics sdis_data;
struct s_isp_frames NO_SYNC isp_frames;
struct isp_uds_config uds_config;

unsigned g_isp_do_zoom;

/* *****************************************************************
 * 		isp parameters
 *				1value per 32bit
 * *****************************************************************/
NO_HOIST struct sh_css_sp_uds_params uds_params[SH_CSS_MAX_STAGES];

/* DMA proxy buffer */
sh_dma_cmd SYNC_WITH(DMA_PROXY_BUF) sh_dma_cmd_buffer[MAX_DMA_CMD_BUFFER];
unsigned     NO_SYNC sh_dma_cmd_buffer_idx;
unsigned     NO_SYNC sh_dma_cmd_buffer_cnt;
unsigned     NO_SYNC sh_dma_cmd_buffer_need_ack;
unsigned     NO_SYNC sh_dma_cmd_buffer_enabled = 0;
sh_dma_cmd *sh_dma_cmd_ptr = (sh_dma_cmd *)(&sh_dma_cmd_buffer[0]);

/* striped-ISP information */
unsigned stripe_id;
unsigned stripe_row_id;

unsigned isp_continuous;
unsigned required_bds_factor;
unsigned isp_raw_stride_b;
unsigned isp_raw_block_width_b;
unsigned isp_raw_line_width_b;
unsigned isp_raw_stripe_offset_b;

