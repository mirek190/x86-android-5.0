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

#ifndef __GDC_PRIVATE_H_INCLUDED__
#define __GDC_PRIVATE_H_INCLUDED__

#ifndef __INLINE_GDC__
#error "gdc_private.h: Inlining required"
#endif

/*
 * Although the ISP is slave, and has no visible address map,
 * it needs global device ID information, but the pipe tool
 * does not accept the system_global file
 *
typedef enum {
	GDC0_ID = 0,
	GDC1_ID,
	N_GDC_ID
} gdc_ID_t;
 */

#include "isp.h"
/*
 * We should use the event interface for all token transfer
 * but because of the irregularity of the GDC interface that
 * would obfuscate only more
 *
#define __INLINE_EVENT__
#include "event_fifo.h"
 */

#ifndef __HIVECC
#define GDC_DATA_ALIGN	1
#else
#define GDC_DATA_ALIGN	ISP_VEC_ALIGN
#endif

typedef __register struct {
	tvector           packed_param;
} gdc_scale_param_t;

typedef __register struct {
	unsigned int      origin_x;
	unsigned int      origin_y;
	unsigned int      in_addr_offset;
	unsigned int      in_block_width;
	unsigned int      in_block_height;
	unsigned int      p0_x;
	unsigned int      p0_y;
	unsigned int      p1_x;
	unsigned int      p1_y;
	unsigned int      p2_x;
	unsigned int      p2_y;
	unsigned int      p3_x;
	unsigned int      p3_y;
} gdc_warp_param_t;

//typedef volatile tmemvectoru MEM(ISP_VMEM)			gdc_data_ptr_t;
#define gdc_data_ptr_t		volatile tmemvectoru MEM(ISP_VMEM)

#include "gdc_public.h"

#include "assert_support.h"


STORAGE_CLASS_INLINE void gdc_send_token(
	const gdc_ID_t		ID,
	const hrt_data		token);

STORAGE_CLASS_GDC_C gdc_warp_param_t gdc_warp_translate(
	gdc_warp_param_t		param)
{
/* Compute the bounding box */
	unsigned int	xmin = ((param.p0_x < param.p2_x)?param.p0_x:param.p2_x);
	unsigned int	xmax = ((param.p1_x >=param.p3_x)?param.p1_x:param.p3_x);
	unsigned int	ymin = ((param.p0_y < param.p1_y)?param.p0_y:param.p1_y);
	unsigned int	ymax = ((param.p2_y >=param.p3_y)?param.p2_y:param.p3_y);

	unsigned int	yi = ymin/HRT_GDC_COORD_ONE;
/* The "x" coordinate is vector aligned */
	unsigned int	xi = xmin / (HRT_GDC_COORD_ONE * ISP_VEC_NELEMS);
	xi *= ISP_VEC_NELEMS;

/* Coordinates for frame access */
	param.origin_y = yi;
	param.origin_x = xi;

	yi *= HRT_GDC_COORD_ONE;
	xi *= HRT_GDC_COORD_ONE;

/* Coordinates for buffer access */
	param.p0_x -= xi;
	param.p0_y -= yi;
	param.p1_x -= xi;
	param.p1_y -= yi;
	param.p2_x -= xi;
	param.p2_y -= yi;
	param.p3_x -= xi;
	param.p3_y -= yi;

	param.p0_x *= HRT_GDC_COORD_SCALE;
	param.p0_y *= HRT_GDC_COORD_SCALE;
	param.p1_x *= HRT_GDC_COORD_SCALE;
	param.p1_y *= HRT_GDC_COORD_SCALE;
	param.p2_x *= HRT_GDC_COORD_SCALE;
	param.p2_y *= HRT_GDC_COORD_SCALE;
	param.p3_x *= HRT_GDC_COORD_SCALE;
	param.p3_y *= HRT_GDC_COORD_SCALE;

return param;
}

STORAGE_CLASS_GDC_C void gdc_warp(
	const gdc_ID_t			ID,
	gdc_warp_param_t		param,
	const gdc_data_ptr_t	*in,
	gdc_data_ptr_t			*out)
{
OP___assert(ID < N_GDC_ID);
/* MW: Since we do packing we must also assert on ranges */
	gdc_send_token(ID, HRT_GDC_PACK_WARP(param.p0_x));
	gdc_send_token(ID, param.p0_y);
	gdc_send_token(ID, param.p1_x);
	gdc_send_token(ID, param.p1_y);
	gdc_send_token(ID, param.p2_x);
	gdc_send_token(ID, param.p2_y);
	gdc_send_token(ID, param.p3_x);
	gdc_send_token(ID, param.p3_y);
#ifdef __HIVECC
	gdc_send_token(ID, (hrt_data)(in)*GDC_DATA_ALIGN);
	gdc_send_token(ID, (hrt_data)(out)*GDC_DATA_ALIGN);
#else
	gdc_send_token(ID, (hrt_data)in);
	gdc_send_token(ID, (hrt_data)out);
	gdc_send_token(ID, (hrt_data)0);
	gdc_send_token(ID, (hrt_data)0);
#endif

return;
}

STORAGE_CLASS_GDC_C void gdc_scale(
	const gdc_ID_t			ID,
	gdc_scale_param_t		param,
	const gdc_data_ptr_t	*in,
	const unsigned short	N,
	const unsigned short	M,
	const unsigned short	y_int,
	const unsigned short	y_frac,
	gdc_data_ptr_t			*out)
{
    unsigned int	fragment;
    unsigned int	N_fragment          = OP_vec_get(param.packed_param, GDC_PARAM_CHUNK_CNT_IDX);
    unsigned int	x_woi				= OP_vec_get(param.packed_param, GDC_PARAM_WOIX_IDX);
    unsigned int	x_woi_last          = OP_vec_get(param.packed_param, GDC_PARAM_WOIX_LAST_IDX);
    unsigned int	x_odim_last         = OP_vec_get(param.packed_param, GDC_PARAM_OXDIM_LAST_IDX);
    unsigned int	x_odim_floored      = OP_vec_get(param.packed_param, GDC_PARAM_OXDIM_FLOORED_IDX);
    unsigned int	x_stride			= OP_vec_get(param.packed_param, GDC_PARAM_VECTORS_PER_LINE_IN_IDX);

OP___assert(ID < N_GDC_ID);
OP___assert(HRT_GDC_N <= (1U << (ISP_VEC_ELEMBITS-1)));
OP___assert(N_GDC_PARAM <= (ISP_VEC_NELEMS >> 2));
OP___assert(N_fragment < N_GDC_PARAM);
/* MW: Since we do packing we must also assert on ranges */

	gdc_reg_store(ID, HRT_GDC_IXDIM_IDX, x_woi);
	gdc_reg_store(ID, HRT_GDC_OXDIM_IDX, x_odim_floored);
	gdc_reg_store(ID, HRT_GDC_SRC_STRIDE_IDX, M*ISP_VEC_ALIGN);

	for (fragment = 0 ; fragment < N_fragment ; fragment++) {
		unsigned int    ipx_start_array = OP_vec_get(param.packed_param, N_GDC_PARAM*1 + fragment);
		unsigned int    ibuf_offset     = OP_vec_get(param.packed_param, N_GDC_PARAM*2 + fragment);
		unsigned int    obuf_offset     = OP_vec_get(param.packed_param, N_GDC_PARAM*3 + fragment);

		ipx_start_array += (ibuf_offset%ISP_VEC_NELEMS)*HRT_GDC_N;
		ibuf_offset /= ISP_VEC_NELEMS;

/* The "config" commands can be generalised to register stores */
#ifdef __HIVECC
		gdc_reg_store(ID, HRT_GDC_SRC_END_ADDR_IDX, (hrt_data)(&in[N*M+ibuf_offset])*GDC_DATA_ALIGN);
		gdc_reg_store(ID, HRT_GDC_SRC_WRAP_ADDR_IDX, (hrt_data)(&in[ibuf_offset])*GDC_DATA_ALIGN);
#else
/* NOTE: Maintain sequence */
		gdc_reg_store(ID, HRT_GDC_SRC_END_ADDR_IDX, (hrt_data)-1);
		gdc_send_token(ID, (hrt_data)(&in[N*M+ibuf_offset]));
		gdc_reg_store(ID, HRT_GDC_SRC_WRAP_ADDR_IDX, (hrt_data)-1);
		gdc_send_token(ID, (hrt_data)(&in[ibuf_offset]));
#endif

		cnd_gdc_reg_store(ID, HRT_GDC_IXDIM_IDX, x_woi_last, (fragment == (N_fragment-1)));
		cnd_gdc_reg_store(ID, HRT_GDC_OXDIM_IDX, x_odim_last, (fragment == (N_fragment-1)));

/* The "run" command is explicitly modelled as a token transfer, although there are register indices, they are not available from here */
		gdc_send_token(ID, HRT_GDC_PACK_SCALE(ipx_start_array, y_frac));
#ifdef __HIVECC
		gdc_send_token(ID, (hrt_data)(&in[y_int*M+ibuf_offset])*GDC_DATA_ALIGN);
		gdc_send_token(ID, (hrt_data)(&out[obuf_offset])*GDC_DATA_ALIGN);
#else
		gdc_send_token(ID, (hrt_data)(&in[y_int*M+ibuf_offset]));
		gdc_send_token(ID, (hrt_data)(&out[obuf_offset]));
		gdc_send_token(ID, (hrt_data)0);
		gdc_send_token(ID, (hrt_data)0);
#endif

#pragma hivecc hoist=off
	}

return;
}

STORAGE_CLASS_GDC_C void cnd_gdc_scale(
	const gdc_ID_t			ID,
	const gdc_scale_param_t	param,
	const gdc_data_ptr_t	*in,
	const unsigned short	N,
	const unsigned short	M,
	const unsigned short	y_int,
	const unsigned short	y_frac,
	gdc_data_ptr_t			*out,
	const bool				cnd)
{
	if (cnd) {
		gdc_scale(ID, param, in, N, M, y_int, y_frac, out);
	}
return;
}


STORAGE_CLASS_GDC_C void gdc_reg_store(
	const gdc_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value)
{
/*	unsigned char	fifo_addr = ((ID==GDC0_ID)?GDC0_FIFO_ADDRESS:GDC1_FIFO_ADDRESS); */
	hrt_data		token = HRT_GDC_PACK(reg, value);
OP___assert(ID < N_GDC_ID);
	if (ID == GDC0_ID) OP_std_snd(GDC0_FIFO_ADDRESS, token);
	if (ID == GDC1_ID) OP_std_snd(GDC1_FIFO_ADDRESS, token);
return;
}

STORAGE_CLASS_GDC_C void cnd_gdc_reg_store(
	const gdc_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value,
	const bool			cnd)
{
	if (cnd)
		gdc_reg_store(ID, reg, value);
return;
}

STORAGE_CLASS_GDC_C hrt_data gdc_reg_load(
	const gdc_ID_t		ID,
	const unsigned int	reg)
{
	unsigned char	fifo_addr = ((ID==GDC0_ID)?GDC0_FIFO_ADDRESS:GDC1_FIFO_ADDRESS);
/* Here we should need a read command or something alike */
	hrt_data		token = HRT_GDC_PACK(reg, 0);
OP___assert(ID < N_GDC_ID);
/*	OP_std_snd(fifo_addr, token); */
	token = OP_std_rcv(fifo_addr);
return HRT_GDC_UNPACK(reg, token);
}

STORAGE_CLASS_INLINE void gdc_send_token(
	const gdc_ID_t		ID,
	const hrt_data		token)
{
OP___assert(ID < N_GDC_ID);
	if (ID == GDC0_ID) OP_std_snd(GDC0_FIFO_ADDRESS, token);
	if (ID == GDC1_ID) OP_std_snd(GDC1_FIFO_ADDRESS, token);
return;
}

#endif /* __GDC_PRIVATE_H_INCLUDED__ */
