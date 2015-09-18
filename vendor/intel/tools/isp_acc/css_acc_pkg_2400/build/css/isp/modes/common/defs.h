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

#ifndef _DEFS_H
#define _DEFS_H

#ifndef HRTCAT
#define _HRTCAT(m, n)     m##n
#define HRTCAT(m, n)      _HRTCAT(m, n)
#endif

#ifndef HRTSTR
#define _HRTSTR(x)   #x
#define HRTSTR(x)    _HRTSTR(x)
#endif

#ifndef HRTMIN
#define HRTMIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef HRTMAX
#define HRTMAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#if !defined(PIPE_GENERATION) && !defined(__ACC)
#include <sh_css_defs.h>
#endif

#ifdef __ISP
#include "isp_defs_for_hive.h"
#endif

#define _ISP_CAT(m, n)     m##n
#define ISP_CAT(m, n)      _ISP_CAT(m, n)

/* default settings */

#if !defined(USE_DMA_PROXY)
#define USE_DMA_PROXY 1
#endif

#define USE_DMA_CMD_BUFFER USE_DMA_PROXY

#if !defined(SUPPORTS_UV_INTERLEAVED_INPUT)
#define SUPPORTS_UV_INTERLEAVED_INPUT	0
#endif

#if !defined(SUPPORTS_YUV_INTERLEAVED_INPUT)
#define SUPPORTS_YUV_INTERLEAVED_INPUT	0
#endif

#ifndef ISP_OUTPUT_FRAME_FORMAT
#define VARIABLE_OUTPUT_FORMAT  1
#define ISP_OUTPUT_FRAME_FORMAT isp_output_image_format
#else
#define VARIABLE_OUTPUT_FORMAT  0
#endif

#define ISP_OUTPUT1_FRAME_FORMAT isp_output1_image_format

#if !defined(ISP_MAX_INPUT_WIDTH)
#define ISP_MAX_INPUT_WIDTH \
	_ISP_MAX_INPUT_WIDTH(ISP_MAX_INTERNAL_WIDTH, ENABLE_DS, \
				ENABLE_FIXED_BAYER_DS, ENABLE_RAW_BINNING, ENABLE_CONTINUOUS)
#endif

#define OUT_CROP_POS_Y                    8U

#define VARIABLE_RESOLUTION     (ISP_MAX_OUTPUT_WIDTH != ISP_MIN_OUTPUT_WIDTH)

#if defined(PIPE_GENERATION) || defined(MK_FIRMWARE) /* Not evaluatable yet */
#define ISP_MAX_INTERNAL_HEIGHT \
	(ISP_MAX_OUTPUT_HEIGHT + OUT_CROP_POS_Y + ISP_MAX_DVS_ENVELOPE_HEIGHT)
#elif VARIABLE_RESOLUTION
#define ISP_MAX_INTERNAL_HEIGHT \
	(ISP_MAX_OUTPUT_HEIGHT + OUT_CROP_POS_Y + ISP_MAX_DVS_ENVELOPE_HEIGHT)
#else
#define ISP_MAX_INTERNAL_HEIGHT          ISP_INTERNAL_HEIGHT
#endif

#if !defined(ISP_MAX_INPUT_HEIGHT)
#define ISP_MAX_INPUT_HEIGHT \
	_ISP_MAX_INPUT_HEIGHT(ISP_MAX_INTERNAL_HEIGHT, ENABLE_DS, \
				ENABLE_FIXED_BAYER_DS, ENABLE_RAW_BINNING, ENABLE_CONTINUOUS)
#endif

#if !defined(ITERATOR_VECTOR_INCREMENT)
#define ITERATOR_VECTOR_INCREMENT	2
#endif

#if !defined(ITERATOR_LINE_INCREMENT)
#define ITERATOR_LINE_INCREMENT		2
#endif

#if !defined(ISP_PIPE_VERSION)
#define ISP_PIPE_VERSION 	1
#endif

#if !defined(ENABLE_LINELOOP)
#define ENABLE_LINELOOP  0
#endif

#if !defined(ENABLE_BLOCKLOOP)
#define ENABLE_BLOCKLOOP  0
#endif

#if !defined(ENABLE_DUALLOOP)
#define ENABLE_DUALLOOP   0
#endif

#if !defined(ISP_NUM_LOOPS)
#define ISP_NUM_LOOPS 	1
#endif

#if !defined(ENABLE_DIS_CROP)
#define ENABLE_DIS_CROP 0
#endif

#if !defined(ENABLE_UDS)
#define ENABLE_UDS 0
#endif

#if !defined(ISP_FIXED_S3A_DECI_LOG)
#define ISP_FIXED_S3A_DECI_LOG 0
#endif

/* Duplicates from "isp/interface/isp_defaults.h" */
#if !defined(ENABLE_FIXED_BAYER_DS)
#define ENABLE_FIXED_BAYER_DS 0
#endif

#if !defined(SUPPORTED_BDS_FACTORS)
#define SUPPORTED_BDS_FACTORS PACK_BDS_FACTOR(SH_CSS_BDS_FACTOR_1_00)
#endif

#if !defined(ENABLE_BAYER_FIR_9DB)
#define ENABLE_BAYER_FIR_9DB 0
#endif

#if !defined(ENABLE_BAYER_FIR_6DB)
#define ENABLE_BAYER_FIR_6DB 0
#endif

#if !defined(ENABLE_BAYER_FIR_PAR)
#define ENABLE_BAYER_FIR_PAR 0
#endif

#if !defined(ENABLE_RAW_BINNING)
#define ENABLE_RAW_BINNING 0
#endif

#if !defined(ENABLE_AA_BEFORE_RAW_BINNING)
#define ENABLE_AA_BEFORE_RAW_BINNING 0
#endif

#if !defined(ENABLE_YUV_AA)
#define ENABLE_YUV_AA           0
#endif

#if !defined(ENABLE_VF_BINNING)
#define ENABLE_VF_BINNING       0
#endif

#if !defined(ENABLE_VF_VAR_FORMATS)
#define ENABLE_VF_VAR_FORMATS   0
#endif

#if !defined(ENABLE_PARAMS)
#define ENABLE_PARAMS		1
#endif

#if !defined(MAX_VF_LOG_DOWNSCALE)
#define MAX_VF_LOG_DOWNSCALE   0
#endif

#if !defined(ENABLE_RAW)
#define ENABLE_RAW		0
#endif

#if !defined(ENABLE_RAW_INPUT)
#define ENABLE_RAW_INPUT	ENABLE_RAW
#endif

#if !defined(ENABLE_INPUT_YUV)
#define ENABLE_INPUT_YUV	0
#endif

#if !defined(ENABLE_INPUT_YUV_BLOCK)
#define ENABLE_INPUT_YUV_BLOCK  0
#endif

#if ENABLE_INPUT_YUV && ENABLE_INPUT_YUV_BLOCK
#error "INPUT_YUV and INPUT_YUV_BLOCK both can't be enabled together"
#endif

#if !defined(ENABLE_REDUCED_PIPE)
#define ENABLE_REDUCED_PIPE	0
#endif

#if !defined(ENABLE_DVS_6AXIS)
#define ENABLE_DVS_6AXIS	0
#endif

#if !defined(ENABLE_STREAMING_DMA)
#define ENABLE_STREAMING_DMA	0
#endif

#if !defined(ENABLE_BLOCK_OUTPUT)
#define ENABLE_BLOCK_OUTPUT 0
#endif

#if !defined(ENABLE_FLIP)
#define ENABLE_FLIP    0
#endif

#if !defined(BLOCK_WIDTH)
#define BLOCK_WIDTH         MAX_VECTORS_PER_CHUNK
#endif

#if !defined(BLOCK_HEIGHT)
#define BLOCK_HEIGHT     2
#endif

#if !defined(OUTPUT_BLOCK_HEIGHT)
#define OUTPUT_BLOCK_HEIGHT     2
#endif

#if !defined(ENABLE_INPUT_FEEDER)
#define ENABLE_INPUT_FEEDER		0
#endif

#if !defined(ENABLE_DPC_ACC)
#define ENABLE_DPC_ACC 0
#endif

#if !defined(ENABLE_BDS_ACC)
#define ENABLE_BDS_ACC 0
#endif


#if ENABLE_INPUT_FEEDER || ENABLE_DPC_ACC || ENABLE_BDS_ACC

	#if !ENABLE_INPUT_FEEDER
		#error "DPC must be activated when INPUT FEEDER/BDS is"
	#endif
	#if !ENABLE_DPC_ACC
		#error "INPUT FEEDER must be activated when DPC/BDS is"
	#endif
	#if !ENABLE_BDS_ACC
		#error "BDS must be activated when INPUT FEEDER/DPC is"
	#endif

#endif

#if !defined(DVS_IN_BLOCK_HEIGHT)
#define DVS_IN_BLOCK_HEIGHT     0
#endif

#if !defined(DVS_IN_BLOCK_WIDTH)
#define DVS_IN_BLOCK_WIDTH       0  // [vectors] -> 128
#endif

#if !defined(ENABLE_TNR)
#define ENABLE_TNR              0
#endif

#if !defined(ENABLE_OUTPUT) && !defined(ENABLE_OUTPUT_SYSTEM)
#undef NUM_OUTPUT_PINS
#define NUM_OUTPUT_PINS         0
#endif

#if !defined(ENABLE_OUTPUT)
#define ENABLE_OUTPUT           0
#endif

#if !defined(NUM_OUTPUT_PINS)
#define NUM_OUTPUT_PINS         1
#endif

#if !defined(ENABLE_BAYER_OUTPUT)
#define ENABLE_BAYER_OUTPUT     0
#endif

#if !defined(ENABLE_OUTPUT_SYSTEM)
#define ENABLE_OUTPUT_SYSTEM    0
#endif

#if !defined(ENABLE_OUTPUT_SPLIT)
#define ENABLE_OUTPUT_SPLIT     0
#endif

#if !defined(ENABLE_YUV_DOWNSCALE)
#define ENABLE_YUV_DOWNSCALE  0
#endif

#if !defined(ENABLE_YUV_SHIFT)
#define ENABLE_YUV_SHIFT      0
#endif

#if ENABLE_YUV_DOWNSCALE && ENABLE_YUV_SHIFT
#error cannot define both ENABLE_YUV_DOWNSCALE and ENABLE_YUV_SHIFT
#endif

#if !defined(ENABLE_MACC)
#define ENABLE_MACC           0
#endif

#if !defined(ENABLE_SS)
#define ENABLE_SS             0
#endif

#if !defined(ENABLE_OBGRID)
#define ENABLE_OBGRID         0
#endif

#if !defined(ENABLE_LIN)
#define ENABLE_LIN            0
#endif

#if !defined(ENABLE_OB)
#define ENABLE_OB		1
#endif

#if !defined(ENABLE_DP)
#define ENABLE_DP		1
#endif

#if !defined(ENABLE_DPC)
#define ENABLE_DPC		1
#endif

#if !defined(ENABLE_DPCBNR)
#define ENABLE_DPCBNR         1
#endif

#if !defined(ENABLE_YEE)
#define ENABLE_YEE            0
#endif

#if !defined(ENABLE_PIXELNOISE_IN_DE_MOIRE_CORING)
#define ENABLE_PIXELNOISE_IN_DE_MOIRE_CORING 0
#endif

#if !defined(ENABLE_DE_C1C2_CORING)
#define ENABLE_DE_C1C2_CORING 0
#endif

#if !defined(ENABLE_CNR)
#define ENABLE_CNR            1
#endif

#if !defined(ENABLE_AF)
#define ENABLE_AF             0
#endif

#if !defined(ENABLE_AE)
#define ENABLE_AE             0
#endif

#if !defined(ENABLE_AWB_ACC)
#define ENABLE_AWB_ACC        0
#endif

#if !defined(ENABLE_AWB_FR_ACC)
#define ENABLE_AWB_FR_ACC     0
#endif

#if !defined(ENABLE_SHD_FF)
#define ENABLE_SHD_FF         0
#endif

#if !defined(ENABLE_BAYER_DENOISE_FF)
#define ENABLE_BAYER_DENOISE_FF 0
#endif

#if !defined(ENABLE_ANR_ACC)
#define ENABLE_ANR_ACC        0
#endif

#if !defined(ENABLE_DEMOSAIC_FF)
#define ENABLE_DEMOSAIC_FF    0
#endif

#if !defined(ENABLE_RGBPP_FF)
#define ENABLE_RGBPP_FF       0
#endif


#if !defined(ENABLE_YUVP1_B0_ACC)
#define ENABLE_YUVP1_B0_ACC   0
#endif

#if !defined(ENABLE_YUVP1_C0_ACC)
#define ENABLE_YUVP1_C0_ACC   0
#endif

#if !defined(ENABLE_YUVP2_ACC)
#define ENABLE_YUVP2_ACC      0
#endif

#if !defined(ENABLE_DVS_STATS)
#define ENABLE_DVS_STATS      0
#endif

#if !defined(ENABLE_XNR)
#define ENABLE_XNR            0
#endif

#if !defined(ENABLE_GAMMA)
#define ENABLE_GAMMA          0
#endif

#if !defined(ENABLE_CTC)
#define ENABLE_CTC            0
#endif

#if !defined(ENABLE_DEMOSAIC)
#define ENABLE_DEMOSAIC       1
#endif

#if !defined(ENABLE_LACE_STATS)
#define ENABLE_LACE_STATS 0
#endif

#if !defined(ENABLE_CLIPPING_IN_YEE)
#define ENABLE_CLIPPING_IN_YEE 0
#endif

#if !defined(ENABLE_GAMMA_UPGRADE)
#define ENABLE_GAMMA_UPGRADE  0
#endif

#if !defined(ENABLE_BAYER_HIST)
#if ISP_PIPE_VERSION == 2
#define ENABLE_BAYER_HIST  0 /* should be 1 */
#else
#define ENABLE_BAYER_HIST  0
#endif
#endif

#if !defined(ENABLE_REF_FRAME)
#define ENABLE_REF_FRAME        (ENABLE_DIS_CROP \
				|| ENABLE_UDS || ENABLE_DVS_6AXIS)
#endif

#if !defined(CONNECT_BNR_TO_ANR)
#define CONNECT_BNR_TO_ANR 0
#endif

#if !defined(CONNECT_SHD_TO_BNR)
#define CONNECT_SHD_TO_BNR 1
#endif

#if !defined(CONNECT_ANR_TO_DE)
#define CONNECT_ANR_TO_DE 0
#endif

#if !defined(CONNECT_RGB_TO_YUVP1)
#define CONNECT_RGB_TO_YUVP1 0
#endif

#if !defined(CONNECT_YUVP1_TO_YUVP2)
#define CONNECT_YUVP1_TO_YUVP2 0
#endif

#if !defined(ENABLE_RGB2YUV)
#define ENABLE_RGB2YUV         0
#endif

#if !defined(ENABLE_DE_RGB)
#define ENABLE_DE_RGB         0
#endif

/* note: the following #defines are dependent on the previous ones
 *       they should be kept after them */
#define ENABLE_SHD_ACC                  (ENABLE_SHD_FF || ENABLE_BAYER_DENOISE_FF || \
                                         ENABLE_AE || ENABLE_AF || ENABLE_AWB_ACC || ENABLE_AWB_FR_ACC)

#if !defined(ENABLE_S3A)
#define ENABLE_S3A                      (ENABLE_AE || ENABLE_AF || ENABLE_AWB_ACC || ENABLE_AWB_FR_ACC)
#endif

/* ae & awb require the stats-3a component */
#define ENABLE_STATS_3A_RAW_BUFF        (ENABLE_AE || ENABLE_AF || ENABLE_AWB_ACC || ENABLE_AWB_FR_ACC)

#if !defined(ENABLE_RGBPP_ACC)
#define ENABLE_RGBPP_ACC                (ENABLE_DEMOSAIC_FF || ENABLE_RGBPP_FF)
#endif

#define ENABLE_DEMOSAIC_ACC             (ENABLE_DEMOSAIC_FF || ENABLE_RGBPP_FF)

/* we can make it more complicated later */
#define ENABLE_ACC_BAYER_DENOISE        (ENABLE_SHD_ACC)
#if (ENABLE_SHD_ACC || ENABLE_ACC_BAYER_DENOISE) && !CONNECT_SHD_TO_BNR
#error "SHD and BNR accelerators must always be connected"
#endif

/* Str2vec and vec2str enable flags */
#define ENABLE_S2V_BAYER1               (ENABLE_INPUT_FEEDER && !CONNECT_BDS_TO_SHD)
#define ENABLE_S2V_BAYER2               (ENABLE_ACC_BAYER_DENOISE && !CONNECT_BNR_TO_ANR)
#define ENABLE_S2V_BAYER3               (ENABLE_ANR_ACC && !CONNECT_ANR_TO_DE)
#define ENABLE_S2V_YUV1                 (ENABLE_RGBPP_ACC && !CONNECT_RGB_TO_YUVP1)
#define ENABLE_S2V_YUV2                 ((ENABLE_YUVP1_B0_ACC || ENABLE_YUVP1_C0_ACC) && !CONNECT_YUVP1_TO_YUVP2)
#define ENABLE_S2V_YUV3                 (ENABLE_YUVP2_ACC)

#define ENABLE_V2S_BAYER1               (ENABLE_SHD_ACC && !CONNECT_BDS_TO_SHD)
#define ENABLE_V2S_BAYER2               (ENABLE_ANR_ACC && !CONNECT_BNR_TO_ANR)
#define ENABLE_V2S_BAYER3               (ENABLE_DEMOSAIC_ACC && !CONNECT_ANR_TO_DE)
#define ENABLE_V2S_YUV1                 ((ENABLE_YUVP1_B0_ACC || ENABLE_YUVP1_C0_ACC) && !CONNECT_RGB_TO_YUVP1)
#define ENABLE_V2S_YUV2                 (ENABLE_YUVP2_ACC && !CONNECT_YUVP1_TO_YUVP2)


#if !defined(ENABLE_RGBA)
#define ENABLE_RGBA 0
#endif

#if !defined(ENABLE_HIGH_SPEED)
#define ENABLE_HIGH_SPEED 0
#endif

/* sdis2 supports 64/32/16BQ grid */
#if !defined(ENABLE_SDIS2_64BQ_32BQ_16BQ_GRID)
#define ENABLE_SDIS2_64BQ_32BQ_16BQ_GRID  0
#endif

/* sdis2 supports 64/32BQ grid */
#if !defined(ENABLE_SDIS2_64BQ_32BQ_GRID)
#define ENABLE_SDIS2_64BQ_32BQ_GRID       0
#endif

#ifndef ENABLE_CONTINUOUS
#define ENABLE_CONTINUOUS       0
#endif

#ifndef ENABLE_CA_GDC
#define ENABLE_CA_GDC           0
#endif

#ifndef ENABLE_OVERLAY
#define ENABLE_OVERLAY          0
#endif

#ifndef ENABLE_ISP_ADDRESSES
#define ENABLE_ISP_ADDRESSES    1
#endif

#ifndef ENABLE_IN_FRAME
#define ENABLE_IN_FRAME		0
#endif

#ifndef ENABLE_OUT_FRAME
#define ENABLE_OUT_FRAME	0
#endif

#if !defined(USE_BNR_LITE)
#if ISP_PIPE_VERSION == 2
#define USE_BNR_LITE  1 /* should be 0 */
#else
#define USE_BNR_LITE  1
#endif
#endif

#if !defined(USE_YEEYNR_LITE)
#if ISP_PIPE_VERSION == 2
#define USE_YEEYNR_LITE  0
#else
#define USE_YEEYNR_LITE  1
#endif
#endif

#if !defined(SPLIT_DP)
#define SPLIT_DP 0
#endif

#if MODE==IA_CSS_BINARY_MODE_PRE_ISP || MODE==IA_CSS_BINARY_MODE_POST_ISP
#define ENABLE_STORE_BAYER 1
#else
#define ENABLE_STORE_BAYER 0
#endif

#ifndef UDS_BPP
#define UDS_BPP                 0
#endif

#ifndef UDS_USE_BCI
#define UDS_USE_BCI             0
#endif

#ifndef UDS_USE_STR
#define UDS_USE_STR             0
#endif

#ifndef UDS_WOIX
#define UDS_WOIX                0
#endif

#ifndef UDS_WOIY
#define UDS_WOIY                0
#endif

#ifndef UDS_EXTRA_OUT_VECS
#define UDS_EXTRA_OUT_VECS      0
#endif

#ifndef ISP_NUM_STRIPES
#define ISP_NUM_STRIPES         1
#endif

#ifndef ISP_ROW_STRIPES_HEIGHT
#define ISP_ROW_STRIPES_HEIGHT         0
#endif

#ifndef ISP_ROW_STRIPES_OVERLAP_LINES
#define ISP_ROW_STRIPES_OVERLAP_LINES         0
#endif

#ifndef ISP_NUM_GDCS
#define ISP_NUM_GDCS		1
#endif

#ifndef ISP_USE_VEC_ITERATORS
#define ISP_USE_VEC_ITERATORS		1
#endif

#if !defined(ISP_MAX_INTERNAL_WIDTH)
#define ISP_MAX_INTERNAL_WIDTH \
	__ISP_INTERNAL_WIDTH(ISP_MAX_OUTPUT_WIDTH, ISP_MAX_DVS_ENVELOPE_WIDTH, \
			     ISP_LEFT_CROPPING, MODE, ISP_C_SUBSAMPLING, \
			     OUTPUT_NUM_CHUNKS, ISP_PIPELINING)
#endif

#if !defined(ISP_MAX_INTERNAL_HEIGHT)
#define ISP_MAX_INTERNAL_HEIGHT \
	__ISP_INTERNAL_HEIGHT(ISP_MAX_OUTPUT_HEIGHT, ISP_MAX_DVS_ENVELOPE_HEIGHT, \
			      ISP_TOP_CROPPING)
#endif

#define VARIABLE_RESOLUTION     (ISP_MAX_OUTPUT_WIDTH != ISP_MIN_OUTPUT_WIDTH)

#if !defined(UDS_VECTORS_PER_LINE_IN)
#define UDS_VECTORS_PER_LINE_IN           MAX_VECTORS_PER_LINE
#define UDS_VECTORS_PER_LINE_OUT          MAX_VECTORS_PER_OUTPUT_LINE
#define UDS_VECTORS_C_PER_LINE_IN         (MAX_VECTORS_PER_LINE/2)
#define UDS_VECTORS_C_PER_LINE_OUT        (MAX_VECTORS_PER_OUTPUT_LINE/2)
#define UDS_VMEM_GDC_IN_BLOCK_HEIGHT_Y    7
#define UDS_VMEM_GDC_IN_BLOCK_HEIGHT_C    5
#endif

/* As an optimization we use DMEM to store the 3A statistics for fixed
 * resolution primary binaries on the ASIC system (not on FPGA). */
#define _S3ATBL_USE_DMEM(var_res)       (!(var_res))
#define S3ATBL_USE_DMEM 		_S3ATBL_USE_DMEM(VARIABLE_RESOLUTION)

#endif /* _DEFS_H */
