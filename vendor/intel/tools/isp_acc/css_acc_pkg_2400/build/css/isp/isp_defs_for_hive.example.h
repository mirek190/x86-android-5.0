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

#ifndef _MY_APPLICATION_DEFS_FOR_HIVE_H
#define _MY_APPLICATION_DEFS_FOR_HIVE_H

/*===========================================================
  Examples isp_defs_for_hive.h
  Lists all properties of an isp binary to be used in
  a firmware package generation.
===========================================================*/

#define BINARY_ID	      	     0
#define BINARY  	      	     simple_extension
#define MODE		      	     IA_CSS_BINARY_NUM_MODES
#define ENABLE_DS	      	     0
#define ENABLE_FPNR	      	     0
#define ENABLE_SC	      	     0
#define ENABLE_S3A	      	     0
#define ENABLE_SDIS	      	     0
#define ENABLE_OUTPUT    	     1
#define ENABLE_VF_VECEVEN     	     0
#define ENABLE_UDS	      	     0
#define ENABLE_RGBA                  0
#define ENABLE_CONTINUOUS            1
#define ENABLE_PARAMS	             0
#define ENABLE_OVERLAY		     0
#define ENABLE_ISP_ADDRESSES	     0
#define ENABLE_IN_FRAME		     1
#define ENABLE_OUT_FRAME	     1
#define ISP_DMEM_PARAMETERS_SIZE     sizeof(isp_dmem_parameters)
#define ISP_PIPELINING  	     1
#define OUTPUT_NUM_CHUNKS     	     1
#define DUMMY_BUF_VECTORS     	     0
#define ISP_LEFT_CROPPING	     0
#define ISP_TOP_CROPPING	     0
#define ENABLE_DVS_ENVELOPE   	     0
#define ISP_MAX_DVS_ENVELOPE_WIDTH   0
#define ISP_MAX_DVS_ENVELOPE_HEIGHT  0
#define ISP_MAX_OUTPUT_WIDTH	     SH_CSS_MAX_VF_WIDTH
#define ISP_MAX_OUTPUT_HEIGHT	     SH_CSS_MAX_VF_HEIGHT
#define ISP_MIN_OUTPUT_WIDTH	     SH_CSS_MIN_SENSOR_WIDTH
#define ISP_MIN_OUTPUT_HEIGHT	     SH_CSS_MIN_SENSOR_HEIGHT
#define SUPPORTED_OUTPUT_FORMATS  PACK_FMT(FRAME_FORMAT_NV12)
#define MAX_VF_LOG_DOWNSCALE         0
#define ISP_INPUT		     IA_CSS_BINARY_INPUT_MEMORY
#define ISP_C_SUBSAMPLING	     DEFAULT_C_SUBSAMPLING
#define ISP_FIXED_S3A_DECI_LOG       0

/* *************************
 * Cropping 
 * ************************* */
#define OUT_CROP_MAX_POS_X            ISP_VEC_NELEMS
#define OUT_CROP_MAX_POS_Y            ISP_VEC_NELEMS
#define VF_CROP_MAX_POS_X             ISP_VEC_NELEMS
#define VF_CROP_MAX_POS_Y             ISP_VEC_NELEMS

#define UDS_BPP                 8
#define UDS_USE_BCI             0
#define UDS_WOIY                2

/* Two chunks in ASIC since the max input and output resolutions are 1280-64 */
#if ISP_VEC_NELEMS == 16
#define UDS_NUMBER_OF_CHUNKS    4
#define UDS_WOIX                (1024/UDS_WOIY)
#else
#define UDS_NUMBER_OF_CHUNKS    2
#define UDS_WOIX                (2048/UDS_WOIY)
#endif

/* To reduce vmem bandwidth, since gdc is bottleneck */
#define ISP_CVSTORE_MUX_ADDRESS 1

#define _ISP_MAX_VF_INPUT_WIDTH \
	__ISP_MAX_VF_OUTPUT_WIDTH(2*ISP_MAX_OUTPUT_WIDTH,\
				  ISP_LEFT_CROPPING)
#define ISP_MAX_VF_INPUT_VECS CEIL_DIV(_ISP_MAX_VF_INPUT_WIDTH, ISP_VEC_NELEMS)
#define MAX_VFPP_INPUT_VECTORS_PER_LINE     ISP_MAX_VF_INPUT_VECS
#define MAX_VFPP_INPUT_C_VECTORS_PER_LINE   CEIL_DIV(ISP_MAX_VF_INPUT_VECS,2)

#define _ISP_MAX_VFPP_OUTPUT_WIDTH \
	__ISP_MAX_VF_OUTPUT_WIDTH(2*SH_CSS_MAX_VF_WIDTH, ISP_LEFT_CROPPING)
#define ISP_MAX_VFPP_OUTPUT_VECS CEIL_DIV(_ISP_MAX_VFPP_OUTPUT_WIDTH, ISP_VEC_NELEMS)

/* output size = vga */
#define MAX_VFPP_OUTPUT_VECTORS_PER_LINE    ISP_MAX_VFPP_OUTPUT_VECS
#define MAX_VFPP_OUTPUT_C_VECTORS_PER_LINE  CEIL_DIV(ISP_MAX_VFPP_OUTPUT_VECS,2)

#define VF_PP_VMEM_IN_DIMY                  4
#define VF_PP_VMEM_IN_DIMU                  2

#define UDS_VECTORS_PER_LINE_IN           MAX_VFPP_INPUT_VECTORS_PER_LINE
#define UDS_VECTORS_PER_LINE_OUT          MAX_VFPP_OUTPUT_VECTORS_PER_LINE
#define UDS_VECTORS_C_PER_LINE_IN         MAX_VFPP_INPUT_C_VECTORS_PER_LINE
#define UDS_VECTORS_C_PER_LINE_OUT        MAX_VFPP_OUTPUT_C_VECTORS_PER_LINE
#define UDS_VMEM_GDC_IN_BLOCK_HEIGHT_Y    VF_PP_VMEM_IN_DIMY 
#define UDS_VMEM_GDC_IN_BLOCK_HEIGHT_C    VF_PP_VMEM_IN_DIMU

#include "../common/defs.h"

/* ************************
 * DMA 
 * ************************/
#define NUM_LUT_SUB_XFERS             1
#define NUM_SDIS_SUB_XFERS            1
#define NUM_ISPPARM_XFERS             1
#define NUM_FPNTBL_SUB_XFERS          1
#define NUM_SCTBL_SUB_XFERS           1
#define NUM_OUTPUT_SUB_XFERS          1
#define NUM_OUTPUT2L_SUB_XFERS        1
#define NUM_C_SUB_XFERS               1
#define NUM_PIXEL_SUB_XFERS           1
#define NUM_TNR_SUB_XFERS             1
#define NUM_VF_GDCOUT_SUB_XFERS       1
#define NUM_RAW_SUB_XFERS             1
#define NUM_VFOUT_SUB_XFERS           1
#define NUM_VFC_SUB_XFERS             1
#define NUM_QPLANE_SUB_XFERS          1
#define NUM_PLANE_SUB_XFERS           2

#include "extension.hive.h"

#endif /* _MY_APPLICATION_DEFS_FOR_HIVE_H */
