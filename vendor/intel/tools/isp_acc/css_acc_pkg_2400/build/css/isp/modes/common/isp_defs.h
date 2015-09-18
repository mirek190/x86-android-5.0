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

#ifndef _COMMON_ISP_DEFS_H_
#define _COMMON_ISP_DEFS_H_

#ifdef __HOST
#error __FILE__ "contains isp specific defines"
#endif

#ifdef __ISP
#include "isp_defs_for_hive.h"
#endif

#include "isp_exprs.h"
#include "isp_sync.h"

#ifndef PIPE_GENERATION
#include "defs.h"
#include "math_support.h"	/* min(), max() */
#endif

#define DECI_FACTOR                (1<<DECI_FACTOR_LOG2)
#define VF_DOWNSCALE               (1<<VF_LOG_DOWNSCALE)

#ifndef SUPPORTS_VIDEO_FULL_RANGE_FLAG
#define SUPPORTS_VIDEO_FULL_RANGE_FLAG  0 /**< determine if the formats kernel will support different signal ranges
												currently we only need to do this for video so disable*/
#endif

#if ISP_NUM_STRIPES == 1
#define ISP_OUTPUT_WIDTH_VECS \
	CEIL_DIV(ISP_OUTPUT_WIDTH, ISP_VEC_NELEMS)
#else
#if defined(HAS_RES_MGR)
#define ISP_OUTPUT_WIDTH_VECS VECTORS_PER_LINE
#else
#define ISP_OUTPUT_WIDTH_VECS \
	MAX(CEIL_DIV(ISP_OUTPUT_WIDTH, ISP_VEC_NELEMS*ISP_NUM_STRIPES), ISP_MIN_STRIPE_WIDTH)
#endif
#endif

/* Internal UV resolutions (based on yuv 4:2:0) */
#if VARIABLE_RESOLUTION && !__HOST
#define ISP_UV_INTERNAL_WIDTH_VECS      isp_uv_internal_width_vecs
#else
#define ISP_UV_INTERNAL_WIDTH_VECS      _ISP_UV_INTERNAL_WIDTH_VECS
#endif

/* ******************
 * extra information 
 * ******************/
#ifndef __HIVECC
#  define info(fmt,s...)                do {printf(fmt, ## s); fflush(stdout);} while(0)
#else
#  define info(fmt,s...)
#endif

/* ***************************************
 * 	information to check DMA schedule 
 * ***************************************/
#define ENABLE_DMA_INFO                 0
#if ENABLE_DMA_INFO && !__HIVECC
#  define info_dma_title()              {printf("IFwait  IFreq  DMAwait    DMAreq    GDCwait    GDCreq\n"); fflush(stdout);}
#  define info_if_wait()                {printf("in\n"); fflush(stdout);}
#  define info_if_req()                 {printf("        in\n"); fflush(stdout);}
#  define info_dma_wait(s)              {printf("               %s\n", s); fflush(stdout);}
#  define info_dma_req_sbuf(s)          {printf("                          %s\n", s); fflush(stdout);}
#  define info_dma_req_dbuf(s,i)        {printf("                          %s[%d]\n", s, i); fflush(stdout);}
#  define info_gdc_wait(s)              {printf("                                    %s\n", s); fflush(stdout);}
#  define info_gdc_req_sbuf(s)          {printf("                                               %s\n", s); fflush(stdout);}
#  define info_gdc_req_dbuf(s,i)        {printf("                                               %s[%d]\n", s, i); fflush(stdout);}
#else
#  define info_dma_title()
#  define info_if_wait()
#  define info_if_req()
#  define info_dma_wait(s)
#  define info_dma_req_sbuf(s)
#  define info_dma_req_dbuf(s,i)
#  define info_gdc_wait(s)
#  define info_gdc_req_sbuf(s)
#  define info_gdc_req_dbuf(s,i)
#endif /* ENABLE_DMA_INFO */

#if defined(HAS_std_pass_v) && (MODE == IA_CSS_BINARY_MODE_PRIMARY || MODE==IA_CSS_BINARY_MODE_CAPTURE_PP)
#define VC(x) OP_std_pass_v(x)
#else
#define VC(x) (x)
#endif

#if defined(HAS_std_pass_v) && MODE == IA_CSS_BINARY_MODE_VIDEO
#define VCV(x) OP_std_pass_v(x)
#else
#define VCV(x) (x)
#endif

#if defined(__isp2400_mamoiada) && defined(HAS_slice_pass) && MODE == IA_CSS_BINARY_MODE_PRIMARY
#define SCOPY(x) OP_slice_pass(x)
#else
#define SCOPY(x) (x)
#endif

#if ISP_CSTORE_MUX_ADDRESS
#define CSTORE(a,v,c)  (*((c) ? &(a) : &_dummy_scalar) = (v))
#else
#define CSTORE(a,v,c)  ((a) = (c) ? (v) : (a SPECULATED))
#endif

#if ISP_CVSTORE_MUX_ADDRESS
#define CVSTORE(a,v,c) (*((c) ? &(a) : &_dummy_vector) = (v))
#else
#define CVSTORE(a,v,c) ((a) = vector_mux ((v), (a SPECULATED), c))
#endif

/* plane: plane which has the same size of input frame */
#define PLANE_FRAME_SIMDWIDTH         VECTORS_PER_INPUT_LINE

#ifndef HIVE_ISP_MAX_BURST_LENGTH
#error "HIVE_ISP_MAX_BURST_LENGTH undefined"
#endif

#ifndef PIPE_GENERATION
#include "isp_types.h"
#endif

#if OUTPUT_NUM_CHUNKS>1 && ISP_NUM_STRIPES>1
#error "Striping and chunking not supported in the same binary"
#endif

#define ASSERT_POWER_OF_2(x) \
  OP___assert(((x) & (~(x) + 1)) == (x))

#endif /* _COMMON_ISP_DEFS_H_ */
