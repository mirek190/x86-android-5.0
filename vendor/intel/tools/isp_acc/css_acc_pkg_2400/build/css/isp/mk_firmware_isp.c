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

#include "mk_firmware.h"

#define MK_FIRMWARE

#if defined(TARGET_crun)
#define C_RUN
#endif

#define _STR(x) #x
#define STR(x) _STR(x)

#define NO_HOIST

#ifdef CELL
#include STR(CAT(CELL,_params.h))
#endif

#ifdef __ACC
#include "isp_const.h"
#include "isp_types.h"
#include "isp_defs_for_hive.h"
#if defined(IS_ISP_2500_SYSTEM)
//#include "isp_defs.h"
#include "isp_defaults.h"
#include "defs.h"
#endif
#else
#include "vmem.h"
#include "vamem.h"
#include "isp_exprs.h"
#include "isp_defs_for_hive.h"
#include "isp_const.h"
#include "isp_types.h"
#if defined(IS_ISP_2500_SYSTEM)
#include "isp_defs.h"
#endif
#include "dma_settings.isp.h"
/* Include generated files */
#include "isp_parameters.isp.c"
#include "isp_configuration.isp.c"
#include "isp_states.isp.c"
#include "isp_param_defs.h"
#include "isp_config_defs.h"
#include "isp_state_defs.h"
#endif /* __ACC */

#include "sh_css_defs.h"
#include "dma_global.h"
#include "isp_formats.isp.h"

/*
 DMA_INVALID_CHANNEL is used in crun to check if a DMA channel used by the isp through the proxy
 is properly configured and implicitely claimed by the ISP Binary.
 Old behavior was that a non configured channel maps to channel 0, this makes it impossible to determine if a channel
 was configured correctly. For CRUN non allocated channels are mapped to an non existant channel.
 On sched simulation the old behaviour is maintaned, non configured channels map to channel 0.
 */
#if defined(TARGET_crun)
#define DMA_INVALID_CHANNEL HIVE_ISP_NUM_DMA_CHANNELS
#else
#define DMA_INVALID_CHANNEL 0
#endif

#if !defined(TARGET_crun)
#include STR(APPL.map.h)
#endif

/**************/

#if !defined(__ACC)
/* Include generated files */
#include "ia_css_isp_params.h"
#include "ia_css_isp_configs.h"
#include "ia_css_isp_states.h"
#endif

#include "ia_css_isp_param.mk_fw.h"

/**************/

#if defined(TARGET_crun)

#define HIVE_ADDR_isp_binary_main_entry 0x0

#endif /* defined(TARGET_crun) */

#ifndef HIVE_ADDR_isp_addresses
#define HIVE_ADDR_isp_addresses		0x0
#endif

#ifndef HIVE_ADDR_in_frame
#define HIVE_ADDR_in_frame		0x0
#endif

#ifndef HIVE_ADDR_out_frame
#define HIVE_ADDR_out_frame		0x0
#endif

#ifndef HIVE_ADDR_in_data
#define HIVE_ADDR_in_data		0x0
#endif

#ifndef HIVE_ADDR_out_data
#define HIVE_ADDR_out_data		0x0
#endif

#if defined(IS_ISP_2400_SYSTEM) || defined(IS_ISP_2500_SYSTEM)

#ifndef HIVE_ADDR_sh_dma_cmd_ptr
#define HIVE_ADDR_sh_dma_cmd_ptr	0x0
#endif

#ifndef _S3ATBL_USE_DMEM
#define _S3ATBL_USE_DMEM(x)		0x0
#endif

#ifndef ENABLE_INPUT_CHUNKING
#define ENABLE_INPUT_CHUNKING		0x0
#endif

#ifndef VFDEC_BITS_PER_PIXEL
#define VFDEC_BITS_PER_PIXEL		0x0
#endif

#endif /* defined(IS_ISP_2400_SYSTEM) || defined(IS_ISP_2500_SYSTEM) */

#ifndef ISP_MIN_INPUT_WIDTH
#define ISP_MIN_INPUT_WIDTH	0
#endif
#ifndef ISP_MIN_INPUT_HEIGHT
#define ISP_MIN_INPUT_HEIGHT	0
#endif

#if !defined(IS_ISP_2500_SYSTEM)

#define INIT_INFO() \
{ \
  .sp = { \
	.id                             = BINARY_ID, \
	.pipeline.mode                  = MODE, \
	.pipeline.isp_pipe_version      = ISP_PIPE_VERSION, \
	.bds.supported_bds_factors      = SUPPORTED_BDS_FACTORS, \
	.block.block_width              = BLOCK_WIDTH, \
	.block.block_height             = BLOCK_HEIGHT, \
	.block.output_block_height      = OUTPUT_BLOCK_HEIGHT, \
	.enable.reduced_pipe            = ENABLE_REDUCED_PIPE, \
	.enable.bayer_fir_6db           = ENABLE_BAYER_FIR_6DB, \
	.enable.raw_binning             = ENABLE_RAW_BINNING, \
	.enable.continuous              = ENABLE_CONTINUOUS, \
	.enable.uds                     = ENABLE_UDS, \
	.enable.dvs_6axis               = ENABLE_DVS_6AXIS, \
	.enable.streaming_dma           = ENABLE_STREAMING_DMA, \
	.enable.block_output            = ENABLE_BLOCK_OUTPUT, \
	.enable.sc                      = ENABLE_SC, \
	.enable.fpnr                    = ENABLE_FPNR, \
	.enable.s3a                     = ENABLE_S3A, \
	.enable.ds                      = ENABLE_DS, \
	.enable.dis                     = ENABLE_SDIS, \
	.enable.dvs_envelope            = ENABLE_DVS_ENVELOPE, \
	.enable.vf_veceven              = ENABLE_VF_VECEVEN, \
	.enable.macc                    = ENABLE_MACC, \
	.enable.output                  = ENABLE_OUTPUT, \
	.enable.ref_frame               = ENABLE_REF_FRAME, \
	.enable.tnr                     = ENABLE_TNR, \
	.enable.xnr                     = ENABLE_XNR, \
	.enable.params                  = ENABLE_PARAMS, \
	.enable.ca_gdc                  = ENABLE_CA_GDC, \
	.enable.isp_addresses           = ENABLE_ISP_ADDRESSES, \
	.enable.in_frame                = ENABLE_IN_FRAME, \
	.enable.out_frame               = ENABLE_OUT_FRAME, \
	.enable.high_speed              = ENABLE_HIGH_SPEED, \
	.dma.vfdec_bits_per_pixel       = VFDEC_BITS_PER_PIXEL, \
	.pipeline.left_cropping         = ISP_LEFT_CROPPING, \
	.pipeline.top_cropping          = ISP_TOP_CROPPING, \
	.pipeline.c_subsampling         = ISP_C_SUBSAMPLING, \
	.pipeline.pipelining            = ISP_PIPELINING, \
	.pipeline.variable_resolution   = VARIABLE_RESOLUTION, \
	.s3a.fixed_s3a_deci_log         = ISP_FIXED_S3A_DECI_LOG, \
	.input.source                   = ISP_INPUT, \
	.dvs.max_envelope_width 	= ISP_MAX_DVS_ENVELOPE_WIDTH, \
	.dvs.max_envelope_height	= ISP_MAX_DVS_ENVELOPE_HEIGHT, \
	.output.min_width		= ISP_MIN_OUTPUT_WIDTH, \
	.output.min_height		= ISP_MIN_OUTPUT_HEIGHT, \
	.output.max_width		= ISP_MAX_OUTPUT_WIDTH, \
	.output.max_height		= ISP_MAX_OUTPUT_HEIGHT, \
	.output.variable_format         = VARIABLE_OUTPUT_FORMAT, \
	.internal.max_width		= ISP_MAX_INTERNAL_WIDTH, \
	.internal.max_height		= ISP_MAX_INTERNAL_HEIGHT, \
	.input.min_width		= ISP_MIN_INPUT_WIDTH, \
	.input.min_height		= ISP_MIN_INPUT_HEIGHT, \
	.input.max_width		= ISP_MAX_INPUT_WIDTH, \
	.input.max_height		= ISP_MAX_INPUT_HEIGHT, \
	.vf_dec.max_log_downscale       = MAX_VF_LOG_DOWNSCALE, \
	.output.num_chunks              = OUTPUT_NUM_CHUNKS, \
	.iterator.num_stripes           = ISP_NUM_STRIPES, \
	.iterator.row_stripes_height    = ISP_ROW_STRIPES_HEIGHT, \
	.iterator.row_stripes_overlap_lines = ISP_ROW_STRIPES_OVERLAP_LINES, \
	.uds.bpp                        = UDS_BPP, \
	.uds.use_bci                    = UDS_USE_BCI, \
	.uds.use_str                    = UDS_USE_STR, \
	.uds.woix                       = UDS_WOIX, \
	.uds.woiy                       = UDS_WOIY, \
	.uds.extra_out_vecs             = UDS_EXTRA_OUT_VECS, \
	.uds.vectors_per_line_in        = UDS_VECTORS_PER_LINE_IN, \
	.uds.vectors_per_line_out       = UDS_VECTORS_PER_LINE_OUT, \
	.uds.vectors_c_per_line_in      = UDS_VECTORS_C_PER_LINE_IN, \
	.uds.vectors_c_per_line_out     = UDS_VECTORS_C_PER_LINE_OUT, \
	.uds.vmem_gdc_in_block_height_y = UDS_VMEM_GDC_IN_BLOCK_HEIGHT_Y, \
	.uds.vmem_gdc_in_block_height_c = UDS_VMEM_GDC_IN_BLOCK_HEIGHT_C, \
	.addresses.main_entry           = HIVE_ADDR_isp_binary_main_entry, \
	.addresses.isp_addresses        = HIVE_ADDR_isp_addresses, \
	.addresses.in_frame		= HIVE_ADDR_in_frame, \
	.addresses.out_frame		= HIVE_ADDR_out_frame, \
	.addresses.in_data		= HIVE_ADDR_in_data, \
	.addresses.out_data		= HIVE_ADDR_out_data, \
	.addresses.sh_dma_cmd_ptr	= HIVE_ADDR_sh_dma_cmd_ptr, \
	.mem_initializers               = MEM_INITIALIZERS, \
  }, \
	.type                           = IA_CSS_ACC_NONE, \
	.num_output_pins                = NUM_OUTPUT_PINS, \
	.mem_offsets.offsets            = { NULL, NULL, NULL }, \
}

#else /* if defined(IS_ISP_2500_SYSTEM) */

#define INIT_INFO() \
{ \
  .sp = { \
	.id                             = BINARY_ID, \
	.pipeline.mode                  = MODE, \
	.pipeline.isp_pipe_version      = ISP_PIPE_VERSION, \
	.enable.ds                      = ENABLE_DS, \
	.bds.supported_bds_factors      = SUPPORTED_BDS_FACTORS, \
	.enable.raw_binning             = ENABLE_RAW_BINNING, \
	.enable.continuous              = ENABLE_CONTINUOUS, \
	.enable.uds                     = ENABLE_UDS, \
	.enable.dvs_6axis               = ENABLE_DVS_6AXIS, \
	.enable.block_output            = ENABLE_BLOCK_OUTPUT, \
	.block.block_width              = BLOCK_WIDTH, \
	.block.block_height             = BLOCK_HEIGHT, \
	.block.output_block_height      = OUTPUT_BLOCK_HEIGHT, \
	.enable.dis                     = ENABLE_SDIS, \
	.enable.dvs_envelope            = ENABLE_DVS_ENVELOPE, \
	.enable.vf_veceven              = ENABLE_VF_VECEVEN, \
	.enable.obgrid                  = ENABLE_OBGRID, \
	.enable.lin                     = ENABLE_LIN, \
	.enable.s3a                     = ENABLE_S3A, \
	.enable.dpc_acc                 = ENABLE_DPC_ACC, \
	.enable.bds_acc                 = ENABLE_BDS_ACC, \
	.enable.shd_acc                 = ENABLE_SHD_ACC, \
	.enable.shd_ff                  = ENABLE_SHD_FF, \
	.enable.input_feeder            = ENABLE_INPUT_FEEDER, \
	.enable.output_system           = ENABLE_OUTPUT_SYSTEM, \
	.enable.stats_3a_raw_buffer     = ENABLE_STATS_3A_RAW_BUFF, \
	.enable.acc_bayer_denoise       = ENABLE_ACC_BAYER_DENOISE, \
	.enable.bnr_ff                  = ENABLE_BAYER_DENOISE_FF, \
	.enable.awb_acc                 = ENABLE_AWB_ACC, \
	.enable.awb_fr_acc              = ENABLE_AWB_FR_ACC, \
	.enable.rgbpp_acc               = ENABLE_RGBPP_ACC, \
	.enable.rgbpp_ff                = ENABLE_RGBPP_FF, \
	.enable.demosaic_acc            = ENABLE_DEMOSAIC_ACC, \
	.enable.dvs_stats               = ENABLE_DVS_STATS, \
	.enable.lace_stats              = ENABLE_LACE_STATS, \
	.enable.yuvp1_b0_acc            = ENABLE_YUVP1_B0_ACC, \
	.enable.yuvp1_c0_acc            = ENABLE_YUVP1_C0_ACC, \
	.enable.yuvp2_acc               = ENABLE_YUVP2_ACC, \
	.enable.ae                      = ENABLE_AE, \
	.enable.af                      = ENABLE_AF, \
	.enable.anr_acc                 = ENABLE_ANR_ACC, \
	.enable.routing_shd_to_bnr      = CONNECT_SHD_TO_BNR, \
	.enable.routing_bnr_to_anr      = CONNECT_BNR_TO_ANR, \
	.enable.routing_anr_to_de       = CONNECT_ANR_TO_DE, \
	.enable.routing_rgb_to_yuvp1    = CONNECT_RGB_TO_YUVP1, \
	.enable.routing_yuvp1_to_yuvp2  = CONNECT_YUVP1_TO_YUVP2, \
	.enable.dergb                   = ENABLE_DE_RGB, \
	.enable.rgb2yuv                 = ENABLE_RGB2YUV, \
	.enable.output                  = ENABLE_OUTPUT, \
	.enable.ref_frame               = ENABLE_REF_FRAME, \
	.enable.tnr                     = ENABLE_TNR, \
	.enable.xnr                     = ENABLE_XNR, \
	.enable.input_feeder            = ENABLE_INPUT_FEEDER, \
	.enable.params                  = ENABLE_PARAMS, \
	.enable.ca_gdc                  = ENABLE_CA_GDC, \
	.enable.isp_addresses           = ENABLE_ISP_ADDRESSES, \
	.enable.in_frame                = ENABLE_IN_FRAME, \
	.enable.out_frame               = ENABLE_OUT_FRAME, \
	.enable.high_speed              = ENABLE_HIGH_SPEED, \
	.dma.vfdec_bits_per_pixel       = VFDEC_BITS_PER_PIXEL, \
	.pipeline.left_cropping         = ISP_LEFT_CROPPING, \
	.pipeline.top_cropping          = ISP_TOP_CROPPING, \
	.pipeline.c_subsampling         = ISP_C_SUBSAMPLING, \
	.pipeline.pipelining            = ISP_PIPELINING, \
	.input.source                   = ISP_INPUT, \
	.dvs.max_envelope_width 	= ISP_MAX_DVS_ENVELOPE_WIDTH, \
	.dvs.max_envelope_height	= ISP_MAX_DVS_ENVELOPE_HEIGHT, \
	.output.min_width		= ISP_MIN_OUTPUT_WIDTH, \
	.output.min_height		= ISP_MIN_OUTPUT_HEIGHT, \
	.output.max_width		= ISP_MAX_OUTPUT_WIDTH, \
	.output.max_height		= ISP_MAX_OUTPUT_HEIGHT, \
	.internal.max_width		= ISP_MAX_INTERNAL_WIDTH, \
	.internal.max_height		= ISP_MAX_INTERNAL_HEIGHT, \
	.input.min_width		= ISP_MIN_INPUT_WIDTH, \
	.input.min_height		= ISP_MIN_INPUT_HEIGHT, \
	.input.max_width		= ISP_MAX_INPUT_WIDTH, \
	.input.max_height		= ISP_MAX_INPUT_HEIGHT, \
	.vf_dec.max_log_downscale       = MAX_VF_LOG_DOWNSCALE, \
	.pipeline.variable_resolution   = VARIABLE_RESOLUTION, \
	.output.variable_format         = VARIABLE_OUTPUT_FORMAT, \
	.output.num_chunks              = OUTPUT_NUM_CHUNKS, \
	.iterator.num_stripes           = ISP_NUM_STRIPES, \
	.iterator.row_stripes_height    = ISP_ROW_STRIPES_HEIGHT, \
	.iterator.row_stripes_overlap_lines = ISP_ROW_STRIPES_OVERLAP_LINES, \
	.uds.bpp                        = UDS_BPP, \
	.uds.use_bci                    = UDS_USE_BCI, \
	.uds.woix                       = UDS_WOIX, \
	.uds.woiy                       = UDS_WOIY, \
	.uds.extra_out_vecs             = UDS_EXTRA_OUT_VECS, \
	.uds.vectors_per_line_in        = UDS_VECTORS_PER_LINE_IN, \
	.uds.vectors_per_line_out       = UDS_VECTORS_PER_LINE_OUT, \
	.uds.vectors_c_per_line_in      = UDS_VECTORS_C_PER_LINE_IN, \
	.uds.vectors_c_per_line_out     = UDS_VECTORS_C_PER_LINE_OUT, \
	.uds.vmem_gdc_in_block_height_y = UDS_VMEM_GDC_IN_BLOCK_HEIGHT_Y, \
	.uds.vmem_gdc_in_block_height_c = UDS_VMEM_GDC_IN_BLOCK_HEIGHT_C, \
	.addresses.main_entry           = HIVE_ADDR_isp_binary_main_entry, \
	.addresses.isp_addresses        = HIVE_ADDR_isp_addresses, \
	.addresses.in_frame             = HIVE_ADDR_in_frame, \
	.addresses.out_frame            = HIVE_ADDR_out_frame, \
	.addresses.in_data              = HIVE_ADDR_in_data, \
	.addresses.out_data             = HIVE_ADDR_out_data, \
	.addresses.sh_dma_cmd_ptr       = HIVE_ADDR_sh_dma_cmd_ptr, \
	.mem_initializers               = MEM_INITIALIZERS, \
  }, \
	.type                           = IA_CSS_ACC_NONE, \
	.num_output_pins                = NUM_OUTPUT_PINS, \
	.mem_offsets.offsets            = { NULL, NULL, NULL }, \
}

#endif  /* if !defined(IS_ISP_2500_SYSTEM) */

#define BINARY_HEADER(n, bin_id) \
{ \
    .type     = ia_css_isp_firmware, \
    .info.isp = INIT_INFO(), \
    .blob     = BLOB_HEADER(n), \
}

#if DUMMY_BLOBS
const char *blob = NULL;
#else
const char *blob = (const char *)&CAT(_hrt_blob_, APPL);
#endif

struct ia_css_fw_info firmware_header = BINARY_HEADER(APPL, BINARY_ID);

/****************/

#ifndef __ACC
#ifdef MEM_OFFSETS
struct ia_css_memory_offsets _mem_offsets = MEM_OFFSETS;
#else
struct ia_css_memory_offsets _mem_offsets;
#endif
struct ia_css_memory_offsets *mem_offsets = &_mem_offsets;
size_t mem_offsets_size = sizeof(_mem_offsets);
#else
struct ia_css_memory_offsets *mem_offsets = NULL;
size_t mem_offsets_size = 0;
#endif

/****************/

#ifndef __ACC
#ifdef CONFIG_MEM_OFFSETS
struct ia_css_config_memory_offsets _conf_mem_offsets = CONFIG_MEM_OFFSETS;
#else
struct ia_css_config_memory_offsets _conf_mem_offsets;
#endif
struct ia_css_config_memory_offsets *conf_mem_offsets = &_conf_mem_offsets;
size_t conf_mem_offsets_size = sizeof(_conf_mem_offsets);
#else
struct ia_css_config_memory_offsets *conf_mem_offsets = NULL;
size_t conf_mem_offsets_size = 0;
#endif

/****************/

#ifndef __ACC
#ifdef STATE_MEM_OFFSETS
struct ia_css_state_memory_offsets _state_mem_offsets = STATE_MEM_OFFSETS;
#else
struct ia_css_state_memory_offsets _state_mem_offsets;
#endif
struct ia_css_state_memory_offsets *state_mem_offsets = &_state_mem_offsets;
size_t state_mem_offsets_size = sizeof(_state_mem_offsets);
#else
struct ia_css_state_memory_offsets *state_mem_offsets = NULL;
size_t state_mem_offsets_size = 0;
#endif

/****************/

static void
fill_dma(struct ia_css_binary_info *info)
{
#if ENABLE_REF_FRAME
	info->dma.ref_y_channel = DMA_REF_BUF_Y_CHANNEL;
	info->dma.ref_c_channel = DMA_REF_BUF_C_CHANNEL;
#else
	info->dma.ref_y_channel = DMA_INVALID_CHANNEL;
	info->dma.ref_c_channel = DMA_INVALID_CHANNEL;
#endif
#if ENABLE_TNR
	info->dma.tnr_channel     = DMA_TNR_BUF_CHANNEL;
	info->dma.tnr_out_channel = DMA_TNR_BUF_OUT_CHANNEL;
#else
	info->dma.tnr_channel     = DMA_INVALID_CHANNEL;
	info->dma.tnr_out_channel = DMA_INVALID_CHANNEL;
#endif
#ifdef DMA_DVS_COORDS_CHANNEL
	info->dma.dvs_coords_channel = DMA_DVS_COORDS_CHANNEL;
#else
	info->dma.dvs_coords_channel = DMA_INVALID_CHANNEL;
#endif
#ifdef DMA_OUTPUT_CHANNEL
	info->dma.output_channel = DMA_OUTPUT_CHANNEL;
#else
	info->dma.output_channel = DMA_INVALID_CHANNEL;
#endif
#ifdef DMA_C_CHANNEL
	info->dma.c_channel = DMA_C_CHANNEL;
#else
	info->dma.c_channel = DMA_INVALID_CHANNEL;
#endif
#ifdef DMA_VFOUT_CHANNEL
	info->dma.vfout_channel = DMA_VFOUT_CHANNEL;
#else
	info->dma.vfout_channel = DMA_INVALID_CHANNEL;
#endif
#ifdef DMA_VFOUT_C_CHANNEL
	info->dma.vfout_c_channel = DMA_VFOUT_C_CHANNEL;
#else
	info->dma.vfout_c_channel = DMA_INVALID_CHANNEL;
#endif
#ifdef DMA_CLAIMED_BY_ISP
	info->dma.claimed_by_isp = DMA_CLAIMED_BY_ISP;
#endif
}

/*
 * For ISP, just fill the header
 */
struct ia_css_blob_info fill_header(void) {
	struct ia_css_blob_info blob_original = firmware_header.blob;

	int i;
	unsigned int max_internal_width;
	struct ia_css_binary_xinfo *xinfo = &firmware_header.info.isp;
	struct ia_css_binary_info *info = &xinfo->sp;
	unsigned offset = sizeof(firmware_header);

#ifdef SUPPORTED_OUTPUT_FORMATS
	/* the SUPPORTED_OUTPUT_FORMATS macro is the new way of specifying which
	 * formats are supported. but not all binaries are using this yet, and the
	 * binary info is still using the old format so for the binaries that
	 * already use the new format, we need to convert it to the old format here.
	 * once all the binaries use the new format, we can use this new format
	 * (bitmask) also in the binaryinfo and update the check funciton in the
	 * binary selection. */
	xinfo->num_output_formats = 0;
	for (i = 0; i < IA_CSS_FRAME_FORMAT_NUM; i++) {
		if (SUPPORTED_OUTPUT_FORMATS & (1<<i)) {
			xinfo->output_formats[xinfo->num_output_formats] = format_decode(i);
			xinfo->num_output_formats++;
		}
	}
	for (i = xinfo->num_output_formats; i < IA_CSS_FRAME_FORMAT_NUM; i++)
	xinfo->output_formats[i] = -1;

#else
	enum ia_css_frame_format out_fmts[] = ISP_OUTPUT_FORMATS;

	xinfo->num_output_formats = sizeof(out_fmts) / sizeof(*out_fmts);
	for (i = 0; i < xinfo->num_output_formats; i++)
		xinfo->output_formats[i] = out_fmts[i];
	for (i = xinfo->num_output_formats; i < IA_CSS_FRAME_FORMAT_NUM; i++)
		xinfo->output_formats[i] = -1;
#endif

	firmware_header.header_size = sizeof(firmware_header);
	info->s3a.s3atbl_use_dmem =
			_S3ATBL_USE_DMEM(info->output.min_width != info->output.max_width);
	info->vf_dec.is_variable = info->pipeline.mode == IA_CSS_BINARY_MODE_COPY;
	max_internal_width = __ISP_INTERNAL_WIDTH(info->output.max_width,
			info->dvs.max_envelope_width,
			info->pipeline.left_cropping,
			info->pipeline.mode,
			info->pipeline.c_subsampling,
			info->output.num_chunks,
			info->pipeline.pipelining);
	/* LA: Hack to fix pitbull issues.
	 Should be fixed by PR 1821. */
#if ((BINARY_ID == SH_CSS_BINARY_ID_POST_ISP) || \
     (BINARY_ID == SH_CSS_BINARY_ID_PREVIEW_DEC))
	/* Needed for offline, e.g. post_isp and primary offline */
	info->input.max_width = ISP_MAX_INPUT_WIDTH;
#else
	/* Needed for */
	info->input.max_width = MAX_VECTORS_PER_INPUT_LINE * ISP_VEC_NELEMS;
#endif
    xinfo->xmem_addr = (unsigned)NULL;
    xinfo->next      = NULL;
    fill_dma(info);

    if (verbose) {

	fprintf(stdout, "ID = %d\n", info->id);
	/* for (i = 0; i < IA_CSS_NUM_ISP_PORTS; i++) { */
	    fprintf(stdout, "max_input[0] = [%u][%u]\n",
		    info->input.max_width, info->input.max_height);
	    fprintf(stdout, "min_input[0] = [%u][%u]\n",
		    info->input.min_width, info->input.min_height);
	    fprintf(stdout, "max_output[0]= [%u][%u]\n",
		    info->output.max_width, info->output.max_height);
	    fprintf(stdout, "min_output[0]= [%u][%u]\n",
		    info->output.min_width, info->output.min_height);
	/* } */
	ia_css_isp_param_print_mem_initializers(info);
    }
#ifdef MEM_OFFSETS
    firmware_header.blob.memory_offsets.offsets[IA_CSS_PARAM_CLASS_PARAM] = offset;
    offset += mem_offsets_size;
#endif
#ifdef CONFIG_MEM_OFFSETS
    firmware_header.blob.memory_offsets.offsets[IA_CSS_PARAM_CLASS_CONFIG] = offset;
    offset += conf_mem_offsets_size;
#endif
#ifdef STATE_MEM_OFFSETS
    firmware_header.blob.memory_offsets.offsets[IA_CSS_PARAM_CLASS_STATE] = offset;
    offset += state_mem_offsets_size;
#endif
	firmware_header.blob.prog_name_offset = offset;

	return blob_original;
}
