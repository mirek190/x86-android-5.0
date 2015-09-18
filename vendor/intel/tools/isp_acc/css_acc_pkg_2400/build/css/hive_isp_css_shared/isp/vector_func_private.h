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

#ifndef __VECTOR_FUNC_PRIVATE_H_INCLUDED__
#define __VECTOR_FUNC_PRIVATE_H_INCLUDED__

#ifndef __INLINE_VECTOR_FUNC__
#error "vector_func_private.h: Inlining required"
#endif

#include "isp.h"

#ifndef __INLINE_VECTOR_OPS__
#define __INLINE_VECTOR_OPS__
#endif
#include "vector_ops.h"

#include "vector_func_public.h"

#include "filters/filters_1.0/filter_9x9.h" // for the matrix types
#include "filters/filters_1.0/filter_3x3.c"
#include "filters/filters_1.0/filter_4x4.c"
#include "filters/filters_1.0/filter_5x5.c"
#include "filters/filters_1.0/matrix_4x4.h"
#include "filters/filters_1.0/matrix_6x6.h"
#include "isp_bma_bfa_types.h"
#include "bbb_config.h"

#include "assert_support.h"

#define MAX_POS_VAL  ~(1<<(ISP_VEC_ELEMBITS-1))
#define MAX_NEG_VAL  1<<(ISP_VEC_ELEMBITS-1)

#define ONE_IN_Q14 (1<<(ISP_VEC_ELEMBITS-2))
#define Q29_TO_Q15_SHIFT_VAL (ISP_VEC_ELEMBITS-2)
#define Q28_TO_Q15_SHIFT_VAL (ISP_VEC_ELEMBITS-3)


/*
 * Basic operations, these should move to the SDK api files
 */

STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec2w_asl(
		tvector2w _a,
		tvector1w _b)
{
	tvector2w _c, _d, _sat, _res, max, min;
	tflags _flag1, _flag2;
	tscalar2w zero = {0, 0};
	tscalar2w _min_signed_val = {0, OP_std_lsl(1, ISP_VEC_ELEMBITS-1)};
#ifdef IS_ISP_2600_SYSTEM
	tscalar2w _max_signed_val = {~0, ~_min_signed_val.scalar1};
#else
	tscalar2w _max_signed_val = {~0, ~_min_signed_val.d1};
#endif
	max    = OP_vec2w_clone(_max_signed_val);
	min    = OP_vec2w_clone(_min_signed_val);
	_c     = OP_vec2w_lsl(_a, _b);
	_d     = OP_vec2w_asr(_c, _b);
	_flag1 = OP_vec2w_eq(_a, _d);
	_flag2 = OP_vec2w_lt_c(_a, zero);
	_sat   = OP_vec2w_mux(min, max, _flag2);
	_res   = OP_vec2w_mux(_c, _sat, _flag1);
	return _res;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_asl(
		tvector1w _a,
		tvector1w _b)
{
	tvector1w _c, _d, _sat, _res, max, min;
	tflags _flag1, _flag2;
	max    = OP_vec_clone(MAX_POS_VAL);
	min    = OP_vec_clone(MAX_NEG_VAL);
	_c     = OP_vec1w_lsl(_a, _b);
	_d     = OP_vec1w_asr(_c, _b);
	_flag1 = OP_vec1w_eq(_a, _d);
	_flag2 = OP_vec1w_lt_c(_a, 0);
	_sat   = OP_vec1w_mux(min,max,_flag2);
	_res   = OP_vec1w_mux(_c,_sat,_flag1);
	return _res;
}

STORAGE_CLASS_VECTOR_FUNC_C tflags
OP_vec2w_ne(
		tvector2w _a,
		tvector2w _b)
{
	tflags _c;
	_c = OP_vec2w_neq(_a, _b);
	return _c;
}

STORAGE_CLASS_VECTOR_FUNC_C tflags
OP_vec1w_ne(
		tvector1w _a,
		tvector1w _b)
{
	tflags _c;
	_c = OP_vec1w_neq(_a, _b);
	return _c;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec2w_subasr1(
		tvector2w _a,
	       	tvector2w _b)
{
#ifdef IS_ISP_2600_SYSTEM
#if HAS_vec_sgnext
	tvector signa = OP_vec_sgnext(CASTVEC1W(_a.vec1));
	tvector signb = OP_vec_sgnext(OP_vec_inv(CASTVEC1W(_b.vec1)));
#else
	tvector signa = OP_vec_asr_c(CASTVEC1W(_a.vec1), ISP_VEC_ELEMBITS-1);
	tvector signb = OP_vec_asr_c(OP_vec_inv(CASTVEC1W(_b.vec1)), ISP_VEC_ELEMBITS-1);
#endif
#if ISP_NWAY == 32
	/* for NWAY 32, the scalar datapath can be used to initialize the flag to all ones */
	tflags carry = OP_std_inv(0);
#else
	/* for all other cores, we need another way to initialize the flag to all ones. */
	tflags carry = OP_vec_eq_c(OP_vec_clone(0), 0);
#endif

#else
#if HAS_vec_sgnext
	tvector signa = OP_vec_sgnext(CASTVEC1W(_a.d1));
	tvector signb = OP_vec_sgnext(OP_vec_inv(CASTVEC1W(_b.d1)));
#else
	tvector signa = OP_vec_asr_c(CASTVEC1W(_a.d1), ISP_VEC_ELEMBITS-1);
	tvector signb = OP_vec_asr_c(OP_vec_inv(CASTVEC1W(_b.d1)), ISP_VEC_ELEMBITS-1);
#endif
	tflags carry = OP_vec_eq_c(OP_vec_clone(0), 0);
#endif  /* IS_ISP_2600_SYSTEM */

	tvector s;
	tvector2w _c;
	tvector2w _r;
	/* substract the inputs by adding with the inverted input. */
#ifdef IS_ISP_2600_SYSTEM
	vec_add_cincout(Any, CASTVEC1W(_a.vec0), OP_vec_inv(CASTVEC1W(_b.vec0)), carry, _c.vec0, carry);
	vec_add_cincout(Any, CASTVEC1W(_a.vec1), OP_vec_inv(CASTVEC1W(_b.vec1)), carry, _c.vec1, carry);
#else
	vec_add_cincout(Any, CASTVEC1W(_a.d0), OP_vec_inv(CASTVEC1W(_b.d0)), carry, _c.d0, carry);
	vec_add_cincout(Any, CASTVEC1W(_a.d1), OP_vec_inv(CASTVEC1W(_b.d1)), carry, _c.d1, carry);
#endif
	vec_add_cincout(Any, signa, signb, carry, s, carry);

	/* rounding shiftright by 1 */
#ifdef IS_ISP_2600_SYSTEM
	vec_rasrh_c_cout(Any, CASTVEC1W(_c.vec1), CASTVEC1W(_c.vec0), 1, _r.vec0, carry);
	vec_asrh_c_cin(Any, s, CASTVEC1W(_c.vec1), 1, carry, _r.vec1);
#else
	vec_rasrh_c_cout(Any, CASTVEC1W(_c.d1), CASTVEC1W(_c.d0), 1, _r.d0, carry);
	vec_asrh_c_cin(Any, s, CASTVEC1W(_c.d1), 1, carry, _r.d1);
#endif


	return _r;
}

/*
 * To Be Done: Replace tvector2w2 by tvector4w
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec2w_mul(
		tvector2w _a,
		tvector2w _b)
{
	tvector2w2 _c;
	_c = OP_vec2w_muld(_a, _b);
#ifdef IS_ISP_2600_SYSTEM
	return _c.vec0;
#else
	return _c.d0;
#endif
}

STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec2w_qmul(
		tvector2w _a,
		tvector2w _b)
{
	tvector2w2 _c;
#if defined IS_ISP_2600_SYSTEM
	_c      = OP_vec2w_muld(_a, _b);
	_c.vec1 = OP_vec2w_lsl_c(_c.vec1,1);
	_c.vec0 = OP_vec2w_lsr_c(_c.vec0,(ISP_VEC_ELEMBITS<<1)-1);
	_c.vec1 = OP_vec2w_add(_c.vec0,_c.vec1);
	return _c.vec1;
#else
	_c    = OP_vec2w_muld(_a, _b);
	_c.d1 = OP_vec2w_lsl_c(_c.d1, 1);
	_c.d0 = OP_vec2w_lsr_c(_c.d0, (ISP_VEC_ELEMBITS << 1) - 1);
	_c.d1 = OP_vec2w_add(_c.d0, _c.d1);
	return _c.d1;
#endif
}


STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec2w_qrmul(
		tvector2w _a,
		tvector2w _b)
{
	tvector2w2 _c;
	tvector2w _d;
#if defined IS_ISP_2600_SYSTEM
	_c      = OP_vec2w_muld(_a, _b);
	_c.vec1 = OP_vec2w_addsat(_c.vec1,_c.vec1);         /* Instead of shift we use (a+a) = a<<1 */
	_d.vec1 = OP_vec_clone(0);
	_d.vec0 = OP_vec_lsrrnd_wsplit_cu_rndout(_c.vec0);    /* Get the rounding bit */
	_c.vec1 = OP_vec2w_add(_d,_c.vec1);                  /* set the rounding bit */
	return _c.vec1;
#else
	_c    = OP_vec2w_muld(_a, _b);
	_c.d1 = OP_vec2w_addsat(_c.d1, _c.d1);
	_d.d1 = OP_vec_clone(0);
	_d.d0 = OP_vec_lsrrnd_wsplit_cu_rndout(_c.d0);    /* Get the rounding bit */
	_c.d1 = OP_vec2w_add(_d, _c.d1);
	return _c.d1;
#endif
}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_input_scaling_offset_clamping(
	tvector1w x,
	int input_scale,
	int input_offset)
{
	int offset_value   = OP_std_lsl(1, OP_std_absnosat(input_offset));
	int clamp_value    = (1 << (ISP_VEC_ELEMBITS-1))-1;
	tvector1w tmp1     = OP_vec1w_asr_c(x, OP_std_absnosat(input_scale));
	tvector1w tmp2     = OP_vec1w_lsl_c(x, OP_std_absnosat(input_scale));
	tvector1w tmp3;
	if (input_offset != 0) {
		if (input_scale > 0) {
			if (input_offset > 0)
				tmp3 = OP_vec1w_add_c(tmp1, offset_value);

			else

				tmp3 = OP_vec1w_sub_c(tmp1, offset_value);
		} else {
			if (input_offset > 0)
				tmp3 = OP_vec1w_add_c(tmp2, offset_value);
			else
				tmp3 = OP_vec1w_sub_c(tmp2, offset_value);

		}
	} else
		tmp3 = tmp2;

	return OP_vec1w_clipz_c(tmp3, clamp_value);

}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_output_scaling_clamping(
	tvector1w x,
	int output_scale)
{

	int clamp_value = (1 << (ISP_VEC_ELEMBITS-1))-1;
	tvector1w tmp1  = OP_vec1w_asr_c(x, OP_std_absnosat(output_scale));
	tvector1w tmp2  = OP_vec1w_lsl_c(x, OP_std_absnosat(output_scale));
	if (output_scale > 0)
		return OP_vec1w_clipz_c(tmp1, clamp_value);

	else
		return OP_vec1w_clipz_c(tmp2, clamp_value);
}

#if defined IS_ISP_2600_SYSTEM
STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_piecewise_estimation(
	tvector1w x,
	isp_config_points test_isp_config_points)
{
	int j;
	tvector1w a, b, c, d;
	tvector1w tmpy UNINIT;
	tvector1w tmpx UNINIT;
	tvector1w tmpslope UNINIT;
	tvector1w y UNINIT;
	tflags flag1, flag2, flag3, flag4, flag5;

#ifdef C_RUN
	for (j=0; j < ISP_MAX_CONFIG_POINTS-1; j++) {
		assert(test_isp_config_points.x_cord[j] < test_isp_config_points.x_cord[j+1]);
	}
#endif

	/*Input Processing*/
	x = OP_vec1w_input_scaling_offset_clamping(x, ISP_INPUT_SCALE_FACTOR, ISP_INPUT_OFFSET_FACTOR);

	/*Piecewise linear estimation */

	flag1 = OP_vec_lt_c(x, test_isp_config_points.x_cord[0]);
	flag2 = OP_vec_ge_c(x, test_isp_config_points.x_cord[ISP_MAX_CONFIG_POINTS-1]);

	for (j = 0; j < ISP_MAX_CONFIG_POINTS-1; j++) {
#pragma hivecc unroll
		flag3 = OP_vec_ge_c(x, test_isp_config_points.x_cord[j]);
		flag4 = OP_vec_lt_c(x, test_isp_config_points.x_cord[j+1]);
		flag5 = flag3 & flag4;
		tmpx = OP_vec_rmux_c(tmpx, test_isp_config_points.x_cord[j], flag5);
		tmpy = OP_vec_rmux_c(tmpy, test_isp_config_points.y_offset[j], flag5);
		tmpslope = OP_vec_rmux_c(tmpslope, test_isp_config_points.slope[j], flag5);
	}

	a = OP_vec_rmux_c(x, test_isp_config_points.x_cord[ISP_MAX_CONFIG_POINTS-1], flag2);
	b = OP_vec_rmux_c(tmpx, test_isp_config_points.x_cord[ISP_MAX_CONFIG_POINTS-2], flag2);
	c = OP_vec_rmux_c(tmpy, test_isp_config_points.y_offset[ISP_MAX_CONFIG_POINTS-2], flag2);
	d = OP_vec_rmux_c(tmpslope, test_isp_config_points.slope[ISP_MAX_CONFIG_POINTS-2], flag2);

	y = OP_vec1w_mul_realigning(d, OP_vec1w_sub(a,b), ISP_SLOPE_A_RESOLUTION);
	y = OP_vec1w_add(y, c);

	y = OP_vec_rmux_c(y, test_isp_config_points.y_offset[0], flag1);

	/*Output Processing */
	y = OP_vec1w_output_scaling_clamping(y, ISP_OUTPUT_SCALE_FACTOR);
	return y;
}

/* XCU Init function to generate vectors to be used as LUT */
STORAGE_CLASS_VECTOR_FUNC_C config_point_vectors slope_offset_LUT_init(
	isp_config_points input_config_points)
{
	int range, tmp_range;
	int max_config_pt, min_config_pt;
	int i = 0;
	int j;
	short exp_of_2 = 0;
	config_point_vectors init_vectors UNINIT;

	for ( j = 0; j < ISP_MAX_CONFIG_POINTS-1; j++) {
		OP___assert(input_config_points.x_cord[j] < input_config_points.x_cord[j+1]);
	}

	/* config points are monotically increasing */
	max_config_pt = input_config_points.x_cord[ISP_MAX_CONFIG_POINTS-1];
	min_config_pt = input_config_points.x_cord[0];

	/* Divide the distance between x0 and xn into LUTSIZE intervals */
	range = ((max_config_pt - min_config_pt)/LUTSIZE);
	tmp_range = range;

	/* Convert this range to nearest power of 2 that is >= range */
	while (tmp_range > 1)
	{
		tmp_range = tmp_range >> 1;
		exp_of_2++;
	}

	tmp_range = (1 << exp_of_2);
	if ((range - tmp_range) > 0){
		exp_of_2++;
	}
	init_vectors.exponent = exp_of_2;
	tmp_range = (1 << exp_of_2);

	/* Calculate the LUT based on range. Range is divided into ISP_NWAY 
	 * equal intervals and for case x<x1 and x>=xn, data is not filled in the 
	 * LUT table. They are used as special cases. */
	for (j = 0; j < LUTSIZE; j++) {
#pragma hivecc unroll
		if ((input_config_points.x_cord[0] + (j*range)) < input_config_points.x_cord[i+1]){
			init_vectors.x_cord_vec = OP_vec_set (init_vectors.x_cord_vec, j, input_config_points.x_cord[i]);
			init_vectors.slope_vec  = OP_vec_set (init_vectors.slope_vec, j, input_config_points.slope[i]);
			init_vectors.offset_vec = OP_vec_set (init_vectors.offset_vec, j, input_config_points.y_offset[i]);
		}
		else {
			init_vectors.x_cord_vec = OP_vec_set (init_vectors.x_cord_vec, j, input_config_points.x_cord[i+1]);
			init_vectors.slope_vec  = OP_vec_set (init_vectors.slope_vec, j, input_config_points.slope[i+1]);
			init_vectors.offset_vec = OP_vec_set (init_vectors.offset_vec, j, input_config_points.y_offset[i+1]);
			i++;
		}
	}
	return init_vectors;

}

/* XCU Core function */
STORAGE_CLASS_VECTOR_FUNC_C tvector1w X_piecewise_estimation(
	tvector1w x,
	isp_config_points test_isp_config_points,
	config_point_vectors init_vectors)
{
	tvector1w y;
	tvector1w x_val;
	tvector1w tmpx;
	tvector1w x_val_prev;
	tvector1w offset;
	tvector1w slope;
	tflags pts_lt_x0, pts_ge_xn;
	tvector idxarray; 

	/*Input Processing*/
	x = OP_vec1w_input_scaling_offset_clamping(x, ISP_INPUT_SCALE_FACTOR, ISP_INPUT_OFFSET_FACTOR);

	/*Piecewise linear estimation */

	pts_lt_x0 = OP_vec_lt_c(x, test_isp_config_points.x_cord[0]);
	pts_ge_xn = OP_vec_ge_c(x, test_isp_config_points.x_cord[ISP_MAX_CONFIG_POINTS-1]);

	/* Subtract x0 from all points to find the interval based on range of (xn-x0)/32 */
	tmpx       = OP_vec_sub_c(x, test_isp_config_points.x_cord[0]);
	/* values greater than xn will have index greater than 31, shuffle operation replaces these values to 0 */
	idxarray   = OP_vec_asr_c(tmpx, init_vectors.exponent);

	x_val      = OP_vec_rmux_c (x, test_isp_config_points.x_cord[ISP_MAX_CONFIG_POINTS-1], pts_ge_xn);
	x_val_prev = OP_vec_shuffle (init_vectors.x_cord_vec, idxarray); //0 is stored in indices >31
	x_val_prev = OP_vec_rmux_c (x_val_prev, test_isp_config_points.x_cord[ISP_MAX_CONFIG_POINTS-2], pts_ge_xn); /*x(n-1) stored for points >=x(n) */
	offset     = OP_vec_shuffle (init_vectors.offset_vec, idxarray);
	offset     = OP_vec_rmux_c (offset, test_isp_config_points.y_offset[ISP_MAX_CONFIG_POINTS-2], pts_ge_xn);
	slope      = OP_vec_shuffle (init_vectors.slope_vec, idxarray);
	slope      = OP_vec_rmux_c (slope, test_isp_config_points.slope[ISP_MAX_CONFIG_POINTS-2], pts_ge_xn);

	y = OP_vec1w_mul_realigning (slope, OP_vec1w_sub(x_val, x_val_prev), ISP_SLOPE_A_RESOLUTION);
	y = OP_vec1w_add (y, offset);

	y = OP_vec_rmux_c (y, test_isp_config_points.y_offset[0], pts_lt_x0);/* y values for x<x0 */

	/*Output Processing */
	y = OP_vec1w_output_scaling_clamping(y, ISP_OUTPUT_SCALE_FACTOR);
	return y;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_XCU (tvector1w x,
	isp_config_points test_isp_config_points)
{
	tvector1w y;
	config_point_vectors init_vectors UNINIT;
	init_vectors = slope_offset_LUT_init (test_isp_config_points);
	y = X_piecewise_estimation (x, test_isp_config_points, init_vectors);
	return y;
}
#endif

STORAGE_CLASS_VECTOR_FUNC_C tvector2w OP_vec1w_cast_vec2w_u(
	tvector1w _a)
{
	tvector2w _r = {_a, OP_vec_clone(0)};
	return _r;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_qrmul(
	tvector1w _a,
	tvector1w _b)
{
#if !HAS_vec_mulsrNrndsat
#error "This implementation needs to get the correct shift value and needs to be validated"
	tvector2w tmp = OP_vec1w_muld(_a, _b);
	return = OP_vec_asrrnd_wsplit_ic (tmp.d1, tmp.d0, coreNbits - 1);
#else
	return OP_vec_mulsrNrndsat(_a, _b);
#endif
}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_qmul(
	tvector1w _a,
	tvector1w _b)
{
	tvector2w tmp = _OP_vec1w_muld(_a, _b);
#ifdef IS_ISP_2600_SYSTEM
	tvector1w res = OP_vec_asrsat_wsplit_ic (tmp.vec1, tmp.vec0, ISP_VEC_ELEMBITS-1);
#else
	tvector1w res = OP_vec_asrsat_wsplit_ic (tmp.d1, tmp.d0, ISP_VEC_ELEMBITS-1);
#endif
	return res;

}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_qmul_c(
	tvector1w _a,
	tscalar1w _b)
{
	tvector2w tmp = _OP_vec1w_muld_c(_a, _b);
#ifdef IS_ISP_2600_SYSTEM
	tvector1w res = OP_vec_asrsat_wsplit_ic(tmp.vec1, tmp.vec0, ISP_VEC_ELEMBITS-1);
#else
	tvector1w res = OP_vec_asrsat_wsplit_ic(tmp.d1, tmp.d0, ISP_VEC_ELEMBITS-1);
#endif
	return res;

}

STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_qrmul_c(
	tvector1w _a,
	tscalar1w _b)
{
#if !HAS_vec_mulsrNrndsat_c
#error "This implementation needs to get the correct shift value and needs to be validated"
	tvector2w tmp = OP_vec1w_muld_c(_a, _b);
	return = OP_vec_asrrnd_wsplit_ic (tmp.d1, tmp.d0, coreNbits - 1);
#else
	return OP_vec_mulsrNrndsat_c(_a, _b);
#endif
}

// realining multiply.
STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_mul_realigning(
	tvector1w _a,
	tvector1w _b,
	tscalar1w _c)
{
	tvector2w tmp = _OP_vec1w_muld(_a, _b);
	return OP_vec_2w_asrrndh_c (tmp, _c);
}

// realining multiply with clone
STORAGE_CLASS_VECTOR_FUNC_C tvector1w
OP_vec1w_mul_realigning_c(
	tvector1w _a,
	tscalar1w _b,
	tscalar1w _c)
{
	tvector2w tmp = _OP_vec1w_muld_c(_a, _b);
	return OP_vec_2w_asrrndh_c (tmp, _c);
}

// doubling multiply accumulate
STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec1w_maccd_sat(
	tvector2w acc,
	tvector1w _a,
	tvector1w _b)
{
	tvector2w tmp = _OP_vec1w_muld(_a, _b);
	tvector2w out = OP_vec2w_addsat(acc, tmp);
	return out;
}

// doubling multiply accumulate
STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec1w_maccd_sat_c(
	tvector2w acc,
	tvector1w _a,
	tscalar1w _b)
{
	tvector2w tmp = _OP_vec1w_muld_c(_a, _b);
	tvector2w out = OP_vec2w_addsat(acc, tmp);
	return out;
}

// doubling multiply accumulate
STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec1w_maccd(
	tvector2w acc,
	tvector1w _a,
	tvector1w _b)
{
	tvector2w tmp = _OP_vec1w_muld(_a, _b);
	tvector2w out = OP_vec2w_add(acc, tmp);
	return out;
}

// doubling multiply accumulate
STORAGE_CLASS_VECTOR_FUNC_C tvector2w
OP_vec1w_maccd_c(
	tvector2w acc,
	tvector1w _a,
	tscalar1w _b)
{
	tvector2w tmp = _OP_vec1w_muld_c(_a, _b);
	tvector2w out = OP_vec2w_add(acc, tmp);
	return out;
}

/*
 * Filter functions
 * - Normalized decimated FIR with coefficients[1 2 1]
 * - Normalized decimated FIR with coefficients[1 4 6 4 1]
 * - Normalised vs. non-normalised (scaling on output)
 * - 6dB vs. 9dB attentuation at Fs/2 (coefficients [1;2;1], vs. [1;1;1])
 * - Filters for spatial shift (fractional pixel group delay)
 * - Single vs. dual line outputs
 * - full rate vs. decimated
 * - factorised vs. direct
 */

/** @brief Factor 2 decimation filter
 *
 * @param[in] input0     vector of input pixels
 * @param[in] input1     second vector of input pixels adjacent to the first one
 * @param[in] slice      slice of pixels. Input0 should be adjacent to this slice
 *
 * @return               vector of decimated and filtered pixels
 *
 * This function will do a horizontal normalized Decimated by 2 FIR with coefficients [1,2,1]
 *
 */

STORAGE_CLASS_VECTOR_FUNC_C tvector
firdec2_1x3m_6dB_nrm(   tvector input0,
			tvector input1,
			tvectorslice slice)
{

	tvector1x2 out    = OP_vec_evenodd(input0, input1);
	s_slice_vector sv = OP_slvec_sliceavgrnd(slice, out.v1, slice, out.v1);
	tvector avg       = OP_vec_avgrnd(out.v0, sv.v);
	return avg;
}


/** @brief Factor 4 decimation filter
 *
 * @param[in] input0     vector of input pixels
 * @param[in] input1     second vector of input pixels adjacent to the first one
 * @param[in] input2     Third vector of input pixels adjacent to the second one
 * @param[in] input3     Fourth vector of input pixels adjacent to the third one
 * @param[in] slice      slice of pixels. Input0 should be adjacent to this slice
 *
 * @return               vector of decimated and filtered pixels
 *
 * This function will do a horizontal normalized Decimated by 4 FIR with coefficients [1,4,6,4,1]
 *
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector
firdec4_1x5m_12dB_nrm(   tvector input0,
			tvector input1,
			tvector input2,
			tvector input3,
			tvectorslice slice)
{
	tvector1x2 out0 = OP_vec_evenodd(input0,input1);
	tvector1x2 out1 = OP_vec_evenodd(input2,input3);
	tvector1x2 out2 = OP_vec_evenodd(out0.v0,out1.v0);
	tvector1x2 out3 = OP_vec_evenodd(out0.v1,out1.v1);

	tvector vec_b = out2.v0;
	tvector vec_c = out3.v0;
	tvector vec_d = out2.v1;
	tvector vec_e = out3.v1;
	/* vec_a is slice vector of vec_e with the slice */

	/* implement [1 4 6 4 1] filter with cascaed of averages */
	s_slice_vector avg_ae = OP_slvec_sliceavgrnd( slice, vec_e, slice, vec_e);
	tvector avg_bc        = OP_vec_avgrnd(vec_b, vec_c);
	tvector avg_ae_c      = OP_vec_avgrnd(avg_ae.v, vec_c);
	tvector avg_ae_c_d    = OP_vec_avgrnd(avg_ae_c, vec_d);
	tvector avg           = OP_vec_avgrnd(avg_ae_c_d, avg_bc);

/* it can also be implemented like this. that is more symetrical.
 * but changing this would also require the reference implementation
 * to be changed. and also the fir1x5_12db function below in this file
 * w = avg(a,e)
 * x = avg(w,c)
 * y = avg(x,c)
 * z = avg(b,d)
 * r = avg(y,z)
 * return r;
*/

	return avg;
}

/** @brief Coring
 *
 * @param[in] coring_vec            Amount of coring based on brightness level
 * @param[in] filt_input            Vector of input pixels on which Coring is applied
 * @param[in] m_CnrCoring0          Coring Level0
 *
 * @return                          vector of filtered pixels after coring is applied
 *
 * This function will perform adaptive coring based on brightness level to remove noise
 *
 */

STORAGE_CLASS_VECTOR_FUNC_C tvector coring (    tvector coring_vec,
						tvector filt_input,
						int m_CnrCoring0 )
{
	tvector out, Coring_W, tmp1, tmp2;
	int  zero = 0, minus1=-1;

	Coring_W = OP_vec_addsat_c (coring_vec , m_CnrCoring0);

	tmp1 = OP_vec_addsat(filt_input, Coring_W);
	tmp1 = OP_vec_min_c (tmp1, zero);

	tmp2 = OP_vec_subsat(filt_input, Coring_W);
	tmp2 = OP_vec_max_c (tmp2, zero);

	out   = OP_vec_mux (tmp1, tmp2, OP_vec_le_c(filt_input, minus1));

	return out;
}


/*
 * Normalised FIR with coefficients [3,4,1], -5dB at Fs/2, -90 degree phase shift (quarter pixel)
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_5dB_m90_nrm (
		const s_1x3_matrix	m)
{
	tvector out11 = OP_vec_avgrnd(CASTVEC1W(m.v02), CASTVEC1W(m.v00));
	tvector out1 = OP_vec_avgrnd(out11, CASTVEC1W(m.v00));
	tvector out = OP_vec_avgrnd(out1, CASTVEC1W(m.v01));

	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3_5dB_m90_nrm (
		const s_slice_vector	sv)
{
	s_1x3_matrix	m = get_1x3_matrix(sv);
	tvector	out = fir1x3m_5dB_m90_nrm (m);
	return out;
}

/*
 * Normalised FIR with coefficients [1,4,3], -5dB at Fs/2, +90 degree phase shift (quarter pixel)
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_5dB_p90_nrm (
		const s_1x3_matrix		m)
{
	tvector out11 = OP_vec_avgrnd(CASTVEC1W(m.v00),CASTVEC1W(m.v02));
	tvector out1 = OP_vec_avgrnd(out11, CASTVEC1W(m.v02));
	tvector out = OP_vec_avgrnd(out1, CASTVEC1W(m.v01));
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3_5dB_p90_nrm (
		const s_slice_vector	sv)
{
	s_1x3_matrix	m = get_1x3_matrix(sv);
	tvector	out = fir1x3m_5dB_p90_nrm (m);
	return out;
}

/*
 * Normalised FIR with coefficients [1,2,1], -6dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_6dB_nrm (
		const s_1x3_matrix		m)
{
	tvector	out = OP_vec_avgrnd(OP_vec_avgrnd (CASTVEC1W(m.v00), CASTVEC1W(m.v02)), CASTVEC1W(m.v01));
	return out;
}
/*
 * Normalised FIR for 1.5/1.25
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_6dB_nrm_ph0 (
		const s_1x3_matrix		m)
{
	tvector out = ( m.v00*13 + m.v01*16 + m.v02*3 ) >> 5;
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_6dB_nrm_ph1 (
		const s_1x3_matrix		m)
{
	tvector out = ( m.v00*9 + m.v01*16 + m.v02*7 ) >> 5;
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_6dB_nrm_ph2 (
		const s_1x3_matrix		m)
{
	tvector out = ( m.v00*5 + m.v01*16 + m.v02*11 ) >> 5;
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_6dB_nrm_ph3 (
		const s_1x3_matrix		m)
{
	tvector out = ( m.v00*1 + m.v01*16 + m.v02*15 ) >> 5;
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_6dB_nrm_calc_coeff (
		const s_1x3_matrix		m, int coeff)
{

	tvector out = ( m.v00*(8-coeff) + m.v01*16 + m.v02*(8+coeff) ) >> 5;
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3_6dB_nrm (
		const s_slice_vector	sv)
{
	s_1x3_matrix	m = get_1x3_matrix(sv);
	tvector	out = fir1x3m_6dB_nrm (m);
	return out;
}

/*
 * Normalised FIR with coefficients [1,1,1], -9dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3m_9dB_nrm (
		const s_1x3_matrix		m)
{
	tvector	out =OP_vec_addsat (OP_vec_avgrnd (CASTVEC1W(m.v00), CASTVEC1W(m.v02)), (CASTVEC1W(m.v01) >> 1));
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3_9dB_nrm (
		const s_slice_vector	sv)
{
	s_1x3_matrix	m = get_1x3_matrix(sv);
	tvector	out = fir1x3m_9dB_nrm (m);
	return out;
}

/*
 * FIR with coefficients [1,2,1], -6dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3_6dB (
		const s_slice_vector	sv)
{
#if HAS_vec_slice && HAS_vecsl_addnextsat
	s_slice_vector vtmp0 = OP_vecsl_addnextsat (CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v), CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v));
	s_slice_vector vtmp1 = OP_vecsl_addnextsat (vtmp0.s, vtmp0.v, vtmp0.s, vtmp0.v);
	tvector out          = OP_vec_slice(vtmp1.s, vtmp1.v, 1-ISP_SLICE_NELEMS);
#else
	s_slice_vector vtmp0 = OP_slvec_sliceaddsat (CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v),1, CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v),0);
	s_slice_vector vtmp1 = OP_slvec_sliceaddsat (vtmp0.s, vtmp0.v, ISP_SLICE_NELEMS-2, vtmp0.s, vtmp0.v, ISP_SLICE_NELEMS-3);
	tvector out = vtmp1.v;
#endif


	return (out>>2);
}

/*
 * FIR with coefficients [1,1,1], -9dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x3_9dB (
		const s_slice_vector	sv)
{
#if HAS_vec_slice && HAS_vecsl_addnextsat
	s_slice_vector vtmp0 = OP_vecsl_addnextsat (CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v), CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v));
	s_slice_vector vtmp1 = OP_vecsl_addnextsat (CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v), vtmp0.s, vtmp0.v);
	tvector out = OP_vec_slice(vtmp1.s, vtmp1.v, 1-ISP_SLICE_NELEMS);
#else
	s_slice_vector vtmp0 = OP_slvec_sliceaddsat (CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v),1, CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v),0);
	s_slice_vector vtmp1 = OP_slvec_sliceaddsat (CASTVECSLICE1W(sv.s), CASTVEC1W(sv.v), ISP_SLICE_NELEMS-1, vtmp0.s, vtmp0.v, ISP_SLICE_NELEMS-3);
	tvector out          = vtmp1.v;
#endif


	return (out>>2);
}

/*
 * Normalised FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Unity gain filter through repeated scaling and rounding
 *	- 6 rotate operations per output
 *	- 8 vector operations per output
 * _______
 *   14
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir3x3m_6dB_nrm (
		const s_3x3_matrix		m)
{
	s_1x3_matrix in0 = {m.v00, m.v01, m.v02};
	s_1x3_matrix in1 = {m.v10, m.v11, m.v12};
	s_1x3_matrix in2 = {m.v20, m.v21, m.v22};
	s_1x3_matrix tmp;
	tvector out;

	tmp.v00 = fir1x3m_6dB_nrm(in0);
	tmp.v01 = fir1x3m_6dB_nrm(in1);
	tmp.v02 = fir1x3m_6dB_nrm(in2);

	out = fir1x3m_6dB_nrm(tmp);
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir3x3_6dB_nrm (
		const s_3_slice_vector	sv)
{
	s_3x3_matrix	m = get_3x3_matrix(sv);
	tvector out = fir3x3m_6dB_nrm(m);
	return out;
}

/*
 * Normalised FIR with coefficients [1;1;1] * [1,1,1]
 *
 * (near) Unity gain filter through repeated scaling and rounding
 *	- 6 rotate operations per output
 *	- 8 vector operations per output
 * _______
 *   14
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir3x3m_9dB_nrm (
		const s_3x3_matrix		m)
{
	tvector	vtmp0 = OP_vec_avgrnd (OP_vec_avgrnd (CASTVEC1W(m.v00), CASTVEC1W(m.v02)), OP_vec_avgrnd (CASTVEC1W(m.v20), CASTVEC1W(m.v22)));
	tvector vtmp1 = OP_vec_avgrnd (OP_vec_avgrnd (CASTVEC1W(m.v01), CASTVEC1W(m.v21)), OP_vec_avgrnd (CASTVEC1W(m.v10), CASTVEC1W(m.v12)));
	tvector out = OP_vec_addsat (OP_vec_avgrnd (vtmp0, vtmp1), (CASTVEC1W(m.v11) >> 3));
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir3x3_9dB_nrm (
		const s_3_slice_vector	sv)
{
	s_3x3_matrix	m = get_3x3_matrix(sv);
	tvector out = fir3x3m_9dB_nrm(m);
	return out;
}


/*
 * Normalised dual output FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Unity gain filter through repeated scaling and rounding
 * compute two outputs per call to re-use common intermediates
 *	- 4 rotate operations per output
 *	- 6 vector operations per output (alternative possible, but in this
 *	    form it's not obvious to re-use variables)
 * _______
 *   10
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector2x1 fir3x3m_6dB_out2x1_nrm (
		const s_4x3_matrix		m)
{
	s_1x3_matrix in0 = {m.v00, m.v01, m.v02};
	s_1x3_matrix in1 = {m.v10, m.v11, m.v12};
	s_1x3_matrix in2 = {m.v20, m.v21, m.v22};
	s_1x3_matrix in3 = {m.v30, m.v31, m.v32};
	s_1x3_matrix tmp0, tmp1;
	tvector2x1	out;

	tmp0.v00 = fir1x3m_6dB_nrm(in0);
	tmp0.v01 = fir1x3m_6dB_nrm(in1);
	tmp0.v02 = fir1x3m_6dB_nrm(in2);
	tmp1.v00 = tmp0.v01;
	tmp1.v01 = tmp0.v02;
	tmp1.v02 = fir1x3m_6dB_nrm(in3);

	out.v0 = fir1x3m_6dB_nrm(tmp0);
	out.v1 = fir1x3m_6dB_nrm(tmp1);

	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector2x1 fir3x3_6dB_out2x1_nrm (
		const s_4_slice_vector	sv)
{
	s_4x3_matrix	m = get_4x3_matrix(sv);
	tvector2x1	out = fir3x3m_6dB_out2x1_nrm(m);
	return out;
}

/*
 * Normalised dual output FIR with coefficients [1;1;1] * [1,1,1]
 *
 * (near) Unity gain filter through repeated scaling and rounding
 * compute two outputs per call to re-use common intermediates
 *	- 4 rotate operations per output
 *	- 7 vector operations per output (alternative possible, but in this
 *	    form it's not obvious to re-use variables)
 * _______
 *   11
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector2x1 fir3x3m_9dB_out2x1_nrm (
		const s_4x3_matrix		m)
{
	tvector	vtmp0 = OP_vec_avgrnd (CASTVEC1W(m.v00), CASTVEC1W(m.v02));
	tvector	vtmp1 = OP_vec_avgrnd (CASTVEC1W(m.v10), CASTVEC1W(m.v12));
	tvector	vtmp2 = OP_vec_avgrnd (CASTVEC1W(m.v20), CASTVEC1W(m.v22));
	tvector	vtmp3 = OP_vec_avgrnd (CASTVEC1W(m.v30), CASTVEC1W(m.v32));

	tvector	vtmp4 = OP_vec_avgrnd (vtmp0, vtmp2);
	tvector vtmp5 = OP_vec_avgrnd (OP_vec_avgrnd (CASTVEC1W(m.v01), CASTVEC1W(m.v21)), vtmp1);
	tvector	vtmp6 = OP_vec_avgrnd (vtmp1, vtmp3);
	tvector vtmp7 = OP_vec_avgrnd (OP_vec_avgrnd (CASTVEC1W(m.v11), CASTVEC1W(m.v31)), vtmp2);

	tvector out0 = OP_vec_addsat (OP_vec_avgrnd (vtmp4, vtmp5), (CASTVEC1W(m.v11) >> 3));
	tvector out1 = OP_vec_addsat (OP_vec_avgrnd (vtmp6, vtmp7), (CASTVEC1W(m.v21) >> 3));
	tvector2x1	out = {out0, out1};

	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector2x1 fir3x3_9dB_out2x1_nrm (
		const s_4_slice_vector	sv)
{
	s_4x3_matrix	m = get_4x3_matrix(sv);
	tvector2x1	out = fir3x3m_9dB_out2x1_nrm(m);
	return out;
}


/*
 * FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Filter with gain correction on output
 *	-  1 rotate operations per output
 *	-  6 vector operations per output
 * _______
 *     7
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir3x3_6dB (
		const s_3_slice_vector	sv)
{
	s_slice_vector vtmp0 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv0.s), CASTVEC1W(sv.sv0.v), CASTVECSLICE1W(sv.sv1.s), CASTVEC1W(sv.sv1.v));
	s_slice_vector vtmp1 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv2.s), CASTVEC1W(sv.sv2.v), CASTVECSLICE1W(sv.sv1.s), CASTVEC1W(sv.sv1.v));
	s_slice_vector vtmp2 = OP_vecsl_addsat (vtmp0.s, vtmp0.v, vtmp1.s, vtmp1.v);

#if HAS_vec_slice && HAS_vecsl_addnextsat
	s_slice_vector vtmp3 = OP_vecsl_addnextsat (vtmp2.s, vtmp2.v, vtmp2.s, vtmp2.v);
	s_slice_vector vtmp4 = OP_vecsl_addnextsat (vtmp3.s, vtmp3.v, vtmp3.s, vtmp3.v);
	tvector out = OP_vec_slice(vtmp4.s, vtmp4.v, 1-ISP_SLICE_NELEMS);
#else
	s_slice_vector vtmp3 = OP_slvec_sliceaddsat (vtmp2.s, vtmp2.v,1, vtmp2.s, vtmp2.v,0);
	s_slice_vector vtmp4 = OP_slvec_sliceaddsat (vtmp3.s, vtmp3.v, ISP_SLICE_NELEMS-2, vtmp3.s, vtmp3.v,ISP_SLICE_NELEMS-3);
	tvector out = vtmp4.v;
#endif

	return (out>>4);
}

/*
 * FIR with coefficients [1;1;1] * [1,1,1]
 *
 * Filter with gain correction on output
 *	-  1 rotate operations per output
 *	-  5 vector operations per output
 * _______
 *     6
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir3x3_9dB (
		const s_3_slice_vector	sv)
{
	s_slice_vector vtmp0 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv0.s), CASTVEC1W(sv.sv0.v), CASTVECSLICE1W(sv.sv1.s), CASTVEC1W(sv.sv1.v));
	s_slice_vector vtmp1 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv2.s), CASTVEC1W(sv.sv2.v), vtmp0.s, vtmp0.v);

#if HAS_vec_slice & HAS_vecsl_addnextsat
	s_slice_vector vtmp2 = OP_vecsl_addnextsat (vtmp1.s, vtmp1.v, vtmp1.s, vtmp1.v);
	s_slice_vector vtmp3 = OP_vecsl_addnextsat (vtmp1.s, vtmp1.v, vtmp2.s, vtmp2.v);
	tvector out = OP_vec_slice(vtmp3.s, vtmp3.v, 1-ISP_SLICE_NELEMS);
#else
	s_slice_vector vtmp2 = OP_slvec_sliceaddsat (vtmp1.s, vtmp1.v,1, vtmp1.s, vtmp1.v,0);
	s_slice_vector vtmp3 = OP_slvec_sliceaddsat (vtmp1.s, vtmp1.v, ISP_SLICE_NELEMS-1, vtmp2.s, vtmp2.v,ISP_SLICE_NELEMS-3);
	tvector out = vtmp3.v;
#endif


	return (out>>3);
}


/*
 * Dual output FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Filter with gain correction on output
 *	-  1   rotate operations per output
 *	-  5.5 vector operations per output
 * ________
 *     6.5
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector2x1 fir3x3_6dB_out2x1 (
		const s_4_slice_vector	sv)
{
	s_slice_vector vtmp0 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv0.s), CASTVEC1W(sv.sv0.v), CASTVECSLICE1W(sv.sv1.s), CASTVEC1W(sv.sv1.v));
	s_slice_vector vtmp1 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv2.s), CASTVEC1W(sv.sv2.v), CASTVECSLICE1W(sv.sv1.s), CASTVEC1W(sv.sv1.v));
	s_slice_vector vtmp2 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv2.s), CASTVEC1W(sv.sv2.v), CASTVECSLICE1W(sv.sv3.s), CASTVEC1W(sv.sv3.v));
	s_slice_vector vtmp3 = OP_vecsl_addsat (vtmp0.s, vtmp0.v, vtmp1.s, vtmp1.v);
	s_slice_vector vtmp4 = OP_vecsl_addsat (vtmp2.s, vtmp2.v, vtmp1.s, vtmp1.v);

#if HAS_vecsl_addnextsat
	s_slice_vector vtmp5 = OP_vecsl_addnextsat (vtmp3.s, vtmp3.v, vtmp3.s, vtmp3.v);
	s_slice_vector vtmp6 = OP_vecsl_addnextsat (vtmp4.s, vtmp4.v, vtmp4.s, vtmp4.v);
	s_slice_vector vtmp7 = OP_vecsl_addnextsat (vtmp5.s, vtmp5.v, vtmp5.s, vtmp5.v);
	s_slice_vector vtmp8 = OP_vecsl_addnextsat (vtmp6.s, vtmp6.v, vtmp6.s, vtmp6.v);

	tvector out0 = OP_vec_slice(vtmp7.s, vtmp7.v, 1-ISP_SLICE_NELEMS);
	tvector out1 = OP_vec_slice(vtmp8.s, vtmp8.v, 1-ISP_SLICE_NELEMS);
#else
	s_slice_vector vtmp5 = OP_slvec_sliceaddsat (vtmp3.s, vtmp3.v,1, vtmp3.s, vtmp3.v,0);
	s_slice_vector vtmp6 = OP_slvec_sliceaddsat (vtmp4.s, vtmp4.v,1, vtmp4.s, vtmp4.v,0);
	s_slice_vector vtmp7 = OP_slvec_sliceaddsat (vtmp5.s, vtmp5.v, ISP_SLICE_NELEMS-2, vtmp5.s, vtmp5.v,ISP_SLICE_NELEMS-3);
	s_slice_vector vtmp8 = OP_slvec_sliceaddsat (vtmp6.s, vtmp6.v, ISP_SLICE_NELEMS-2, vtmp6.s, vtmp6.v,ISP_SLICE_NELEMS-3);
	tvector out0= vtmp7.v;
	tvector out1= vtmp8.v;

#endif

	tvector2x1	out = {out0>>4, out1>>4};

	return out;
}

/*
 * Dual output FIR with coefficients [1;1;1] * [1,1,1]
 *
 * Filter with gain correction on output
 *	-  1   rotate operations per output
 *	-  3.5 vector operations per output
 * ________
 *     4.5
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector2x1 fir3x3_9dB_out2x1 (
		const s_4_slice_vector	sv)
{

	s_slice_vector vtmp0 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv2.s), CASTVEC1W(sv.sv2.v), CASTVECSLICE1W(sv.sv1.s), CASTVEC1W(sv.sv1.v));
	s_slice_vector vtmp1 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv0.s), CASTVEC1W(sv.sv0.v), vtmp0.s, vtmp0.v);
	s_slice_vector vtmp2 = OP_vecsl_addsat (CASTVECSLICE1W(sv.sv3.s), CASTVEC1W(sv.sv3.v), vtmp0.s, vtmp0.v);

#if HAS_vec_slice && HAS_vecsl_addnextsat
	s_slice_vector vtmp5 = OP_vecsl_addnextsat (vtmp1.s, vtmp1.v, vtmp1.s, vtmp1.v);
	s_slice_vector vtmp6 = OP_vecsl_addnextsat (vtmp2.s, vtmp2.v, vtmp2.s, vtmp2.v);
	s_slice_vector vtmp7 = OP_vecsl_addnextsat (vtmp1.s, vtmp1.v, vtmp5.s, vtmp5.v);
	s_slice_vector vtmp8 = OP_vecsl_addnextsat (vtmp2.s, vtmp2.v, vtmp6.s, vtmp6.v);

	tvector out0 = OP_vec_slice(vtmp7.s, vtmp7.v, 1-ISP_SLICE_NELEMS);
	tvector out1 = OP_vec_slice(vtmp8.s, vtmp8.v, 1-ISP_SLICE_NELEMS);
#else
	s_slice_vector vtmp5 = OP_slvec_sliceaddsat (vtmp1.s, vtmp1.v,1, vtmp1.s, vtmp1.v,0);
	s_slice_vector vtmp6 = OP_slvec_sliceaddsat (vtmp2.s, vtmp2.v,1, vtmp2.s, vtmp2.v,0);
	s_slice_vector vtmp7 = OP_slvec_sliceaddsat (vtmp1.s, vtmp1.v, ISP_SLICE_NELEMS-1, vtmp5.s, vtmp5.v,ISP_SLICE_NELEMS-3);
	s_slice_vector vtmp8 = OP_slvec_sliceaddsat (vtmp6.s, vtmp6.v, ISP_SLICE_NELEMS-1, vtmp6.s, vtmp6.v,ISP_SLICE_NELEMS-3);
	tvector out0= vtmp7.v;
	tvector out1= vtmp8.v;
#endif


	tvector2x1	out = {out0>>3, out1>>3};

	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir5x5m_15dB_nrm (
		s_5x5_matrix	m)
{
	tvector	out;
	/* 1 */
	tvector vtmp0_1 = OP_vec_avgrnd (m.v00, m.v04);
	tvector vtmp1_1 = OP_vec_avgrnd (m.v40, m.v44);
	tvector vtmp2_1 = OP_vec_avgrnd (vtmp0_1, vtmp1_1);
	/* 4 */
	tvector vtmp0_4 = OP_vec_avgrnd (m.v02, m.v42);
	tvector vtmp1_4 = OP_vec_avgrnd (m.v20, m.v24);
	tvector vtmp2_4 = OP_vec_avgrnd (vtmp0_4, vtmp1_4);
	/* 3 : 4 - 1 */
	tvector vtmp0_3 = OP_vec_avgrnd (m.v01, m.v03);
	tvector vtmp1_3 = OP_vec_avgrnd (m.v10, m.v14);
	tvector vtmp2_3 = OP_vec_avgrnd (m.v30, m.v34);
	tvector vtmp3_3 = OP_vec_avgrnd (m.v41, m.v43);
	tvector vtmp4_3 = OP_vec_avgrnd (vtmp0_3, vtmp1_3);
	tvector vtmp5_3 = OP_vec_avgrnd (vtmp2_3, vtmp3_3);
	/* 9 : 8 + 1 */
	tvector vtmp0_9 = OP_vec_avgrnd (m.v11, m.v13);
	tvector vtmp1_9 = OP_vec_avgrnd (m.v31, m.v33);
	tvector vtmp2_9 = OP_vec_avgrnd (vtmp0_9, vtmp1_9);
	/* 12 : 8 + 4 */
	tvector vtmp0_12 = OP_vec_avgrnd (m.v12, m.v32);
	tvector vtmp1_12 = OP_vec_avgrnd (m.v21, m.v23);
	tvector vtmp2_12 = OP_vec_avgrnd (vtmp0_12, vtmp1_12);

	tvector vtmpa = OP_vec_subhalfrnd (vtmp2_1, vtmp4_3);
	tvector vtmpb = OP_vec_subhalfrnd (vtmp2_9, vtmp5_3);
	tvector vtmpc = OP_vec_avgrnd (vtmpa, vtmpb);

	tvector vtmpd = OP_vec_subhalfrnd (m.v22, vtmp2_12);
	tvector vtmpe = OP_vec_avgrnd (vtmpc, vtmp2_4);
	tvector vtmpf = OP_vec_avgrnd (vtmp4_3, vtmp5_3);

	tvector vtmpg = OP_vec_avgrnd (vtmpd, vtmpe);
	tvector vtmph = OP_vec_avgrnd (vtmpf, vtmp2_9);

	tvector vtmpi = OP_vec_avgrnd (vtmpg, vtmph);
	tvector vtmpj = vtmpi + (vtmp2_12 >> 1);

	//twidevector	wvtmpk  = OP_vec_mul_c(vtmpj , 7282);	/* 128/144 */
	twidevector	wvtmpk;
	vec_mul_c(Any, vtmpj , 7282, wvtmpk.hi,wvtmpk.lo);	/* 128/144 */
	out = OP_vec_asrsat_wsplit_c(wvtmpk.hi, wvtmpk.lo, 13);

	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir5x5_15dB_nrm (
		const s_5_slice_vector	sv)
{
	s_5x5_matrix	m = get_5x5_matrix(sv);
	tvector out = fir5x5m_15dB_nrm(m);
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x5m_12dB_nrm (
	s_1x5_matrix m)
{
	tvector out111 = OP_vec_avgrnd(m.v04, m.v00);
	tvector out11 = OP_vec_avgrnd(out111, m.v02);
	tvector out1 = OP_vec_avgrnd(out11, m.v03);
	tvector out2 = OP_vec_avgrnd(m.v02, m.v01);
	tvector out = OP_vec_avgrnd(out1, out2);

	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir5x5m_12dB_nrm (
	s_5x5_matrix m)
{
	s_1x5_matrix tmp;
	tvector out;

	tmp.v00 = fir1x5m_12dB_nrm(matrix_5x5_sel_1x5_0(m));
	tmp.v01 = fir1x5m_12dB_nrm(matrix_5x5_sel_1x5_1(m));
	tmp.v02 = fir1x5m_12dB_nrm(matrix_5x5_sel_1x5_2(m));
	tmp.v03 = fir1x5m_12dB_nrm(matrix_5x5_sel_1x5_3(m));
	tmp.v04 = fir1x5m_12dB_nrm(matrix_5x5_sel_1x5_4(m));

	out = fir1x5m_12dB_nrm(tmp);
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x5m_box (
	s_1x5_matrix m)
{
	tvector vtmp1 =  OP_vec_avgrnd    ( m.v00  ,  m.v01 );
	tvector vtmp2 =  OP_vec_avgrnd    ( m.v03   , m.v04 );
	tvector vtmp3 =  OP_vec_asrrnd_ic ( m.v02, 2);
	tvector vavg  =  OP_vec_avgrnd    ( vtmp1 ,  vtmp2) ;
	tvector out   =  OP_vec_avgrnd    ( vavg  ,  vtmp3) ;
	return out;
}

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x9m_box (
	s_1x9_matrix m)
{
	tvector vtmp1 =  OP_vec_avgrnd    ( m.v00  ,  m.v01 );
	tvector vtmp2 =  OP_vec_avgrnd    ( m.v02   , m.v03 );
	tvector vtmp3 =  OP_vec_avgrnd    ( m.v05   , m.v06 );
	tvector vtmp4 =  OP_vec_avgrnd    ( m.v07   , m.v08 );
	tvector vtmp5 =  OP_vec_asrrnd_ic ( m.v04, 3);

	tvector vavg1 =  OP_vec_avgrnd    ( vtmp1 ,  vtmp2) ;
	tvector vavg2 =  OP_vec_avgrnd    ( vtmp3 ,  vtmp4) ;
	tvector vavg  =  OP_vec_avgrnd    ( vavg1 ,  vavg2) ;
	tvector out   =  OP_vec_avgrnd    ( vavg  ,  vtmp5) ;

	return out;
}

/** @brief Mean of 1x3 matrix
 *
 *  @param[in] m 1x3 matrix with pixels
 *
 *  @return mean of 1x3 matrix
 *
 * This function calculates the mean of 1x3 pixels,
 * with a factor of 4/3.
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector mean1x3m(
	s_1x3_matrix m)
{
	tvector vavg1 = OP_vec_avgrnd(m.v00, m.v02);
	tvector vtmp1 = OP_vec_asrrnd_ic(m.v01, 1);
	tvector out   = OP_vec_avgrnd(vavg1, vtmp1);

	return out;
}

/** @brief Mean of 3x3 matrix
 *
 *  @param[in] m 3x3 matrix with pixels
 *
 *  @return mean of 3x3 matrix
 *
 * This function calculates the mean of 3x3 pixels,
 * with a factor of 16/9.
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector mean3x3m(
	s_3x3_matrix m)
{
	tvector vtmp1 = OP_vec_avgrnd(m.v00, m.v01);
	tvector vtmp2 = OP_vec_avgrnd(m.v10, m.v11);
	tvector vtmp3 = OP_vec_avgrnd(m.v20, m.v21);
	tvector vtmp4 = OP_vec_avgrnd(m.v02, m.v12);
	tvector vtmp5 = OP_vec_asrrnd_ic(m.v22, 3);

	tvector vavg1 = OP_vec_avgrnd(vtmp1, vtmp2);
	tvector vavg2 = OP_vec_avgrnd(vtmp3, vtmp4);

	tvector out11 = OP_vec_avgrnd(vavg1, vavg2);
	tvector out   = OP_vec_avgrnd(out11, vtmp5);

	return out;
}

/** @brief Mean of 1x4 matrix
 *
 *  @param[in] m 1x4 matrix with pixels
 *
 *  @return mean of 1x4 matrix
 *
 * This function calculates the mean of 1x4 pixels
*/

/*
 * Function to calculate the mean of 1x4 matrix
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector mean1x4m(
	s_1x4_matrix m)
{
	tvector vavg1 = OP_vec_avgrnd(m.v00, m.v01);
	tvector vavg2 = OP_vec_avgrnd(m.v02, m.v03);
	tvector out   = OP_vec_avgrnd(vavg1, vavg2);

	return out;
}

/** @brief Mean of 4x4 matrix
 *
 *  @param[in] m 4x4 matrix with pixels
 *
 *  @return mean of 4x4 matrix
 *
 * This function calculates the mean of 4x4 matrix with pixels
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector mean4x4m(
	s_4x4_matrix m)
{
	s_1x4_matrix in0 = {m.v00, m.v01, m.v02, m.v03};
	s_1x4_matrix in1 = {m.v10, m.v11, m.v12, m.v13};
	s_1x4_matrix in2 = {m.v20, m.v21, m.v22, m.v23};
	s_1x4_matrix in3 = {m.v30, m.v31, m.v32, m.v33};
	s_1x4_matrix tmp;
	tvector out;

	tmp.v00 = mean1x4m(in0);
	tmp.v01 = mean1x4m(in1);
	tmp.v02 = mean1x4m(in2);
	tmp.v03 = mean1x4m(in3);
	out     = mean1x4m(tmp);

	return out;
}

/** @brief Mean of 2x3 matrix
 *
 *  @param[in] m 2x3 matrix with pixels
 *
 *  @return mean of 2x3 matrix
 *
 * This function calculates the mean of 2x3 matrix with pixels
 * with a factor of 8/6.
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector mean2x3m(
	s_2x3_matrix m)
{
	tvector vtmp1 = OP_vec_avgrnd(m.v00, m.v01);
	tvector vtmp2 = OP_vec_avgrnd(m.v10, m.v11);
	tvector vtmp3 = OP_vec_avgrnd(m.v02, m.v12);

	tvector vavg1 = OP_vec_avgrnd(vtmp1, vtmp2);
	tvector vavg2 = OP_vec_asrrnd_ic(vtmp3, 1);

	tvector out = OP_vec_avgrnd(vavg1, vavg2);

	return out;
}

/** @brief Mean of 1x6 matrix
 *
 *  @param[in] m 1x6 matrix with pixels
 *
 *  @return mean of 1x6 matrix
 *
 * This function calculates the mean of 1x6 matrix with pixels
 * with a factor of 8/6.
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector mean1x6m(
	s_1x6_matrix m)
{
	tvector vtmp1 = OP_vec_avgrnd(m.v00, m.v01);
	tvector vtmp2 = OP_vec_avgrnd(m.v02, m.v03);
	tvector vtmp3 = OP_vec_avgrnd(m.v04, m.v05);

	tvector vavg1 = OP_vec_avgrnd(vtmp1, vtmp2);
	tvector vavg2 = OP_vec_asrrnd_ic(vtmp3, 1);

	tvector out = OP_vec_avgrnd(vavg1, vavg2);

	return out;
}

/** @brief Mean of 5x5 matrix
 *
 *  @param[in] m 5x5 matrix with pixels
 *
 *  @return mean of 5x5 matrix
 *
 * This function calculates the mean of 5x5 pixels,
 * with a factor of 32/25.
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector mean5x5m(
	s_5x5_matrix m)
{
	tvector vt00 = OP_vec_avgrnd(m.v00, m.v01);
	tvector vt01 = OP_vec_avgrnd(m.v02, m.v03);
	tvector vt02 = OP_vec_avgrnd(m.v10, m.v11);
	tvector vt03 = OP_vec_avgrnd(m.v12, m.v13);
	tvector vt04 = OP_vec_avgrnd(m.v20, m.v21);
	tvector vt05 = OP_vec_avgrnd(m.v22, m.v23);
	tvector vt06 = OP_vec_avgrnd(m.v30, m.v31);
	tvector vt07 = OP_vec_avgrnd(m.v32, m.v33);
	tvector vt08 = OP_vec_avgrnd(m.v40, m.v41);
	tvector vt09 = OP_vec_avgrnd(m.v42, m.v43);
	tvector vt0A = OP_vec_avgrnd(m.v04, m.v14);
	tvector vt0B = OP_vec_avgrnd(m.v24, m.v34);

	tvector vt10 = OP_vec_avgrnd(vt00, vt01);
	tvector vt11 = OP_vec_avgrnd(vt02, vt03);
	tvector vt12 = OP_vec_avgrnd(vt04, vt05);
	tvector vt13 = OP_vec_avgrnd(vt06, vt07);
	tvector vt14 = OP_vec_avgrnd(vt08, vt09);
	tvector vt15 = OP_vec_avgrnd(vt0A, vt0B);

	tvector vt20 = OP_vec_avgrnd(vt10, vt11);
	tvector vt21 = OP_vec_avgrnd(vt12, vt13);
	tvector vt22 = OP_vec_avgrnd(vt14, vt15);
	tvector vt23 = OP_vec_asrrnd_ic(m.v44, 3);

	tvector vt30 = OP_vec_avgrnd(vt20, vt21);
	tvector vt31 = OP_vec_avgrnd(vt22, vt23);

	tvector vout = OP_vec_avgrnd(vt30, vt31);

	return vout;
}

/** @brief Mean of 6x6 matrix
 *
 *  @param[in] m 6x6 matrix with pixels
 *
 *  @return mean of 6x6 matrix
 *
 * This function calculates the mean of 6x6 matrix with pixels
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector mean6x6m(
	s_6x6_matrix m)
{
	s_1x6_matrix in0 = {m.v00, m.v01, m.v02, m.v03, m.v04, m.v05};
	s_1x6_matrix in1 = {m.v10, m.v11, m.v12, m.v13, m.v14, m.v15};
	s_1x6_matrix in2 = {m.v20, m.v21, m.v22, m.v23, m.v24, m.v25};
	s_1x6_matrix in3 = {m.v30, m.v31, m.v32, m.v33, m.v34, m.v35};
	s_1x6_matrix in4 = {m.v40, m.v41, m.v42, m.v43, m.v44, m.v45};
	s_1x6_matrix in5 = {m.v50, m.v51, m.v52, m.v53, m.v54, m.v55};
	s_1x6_matrix tmp;
	tvector out;

	tmp.v00 = mean1x6m(in0);
	tmp.v01 = mean1x6m(in1);
	tmp.v02 = mean1x6m(in2);
	tmp.v03 = mean1x6m(in3);
	tmp.v04 = mean1x6m(in4);
	tmp.v05 = mean1x6m(in5);
	out     = mean1x6m(tmp);

	return out;
}

/** @brief Minimum of 4x4 matrix
 *
 *  @param[in] m 4x4 matrix with pixels
 *
 *  @return minimum of 4x4 matrix
 *
 * This function calculates element wise minimum
 * vector from the input 4x4 matrix pixels.
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector min4x4m(
	s_4x4_matrix m)
{
	tvector vt00 = OP_vec_min(m.v00, m.v01);
	tvector vt01 = OP_vec_min(m.v02, m.v03);
	tvector vt02 = OP_vec_min(m.v10, m.v11);
	tvector vt03 = OP_vec_min(m.v12, m.v13);
	tvector vt04 = OP_vec_min(m.v20, m.v21);
	tvector vt05 = OP_vec_min(m.v22, m.v23);
	tvector vt06 = OP_vec_min(m.v30, m.v31);
	tvector vt07 = OP_vec_min(m.v32, m.v33);

	tvector vt10 = OP_vec_min(vt00, vt01);
	tvector vt11 = OP_vec_min(vt02, vt03);
	tvector vt12 = OP_vec_min(vt04, vt05);
	tvector vt13 = OP_vec_min(vt06, vt07);

	tvector vt20 = OP_vec_min(vt10, vt11);
	tvector vt21 = OP_vec_min(vt12, vt13);

	tvector vout = OP_vec_min(vt20, vt21);

	return vout;
}

/** @brief Maximum of 4x4 matrix
 *
 *  @param[in] m 4x4 matrix with pixels
 *
 *  @return maximum of 4x4 matrix
 *
 * This function calculates element wise maximum
 * vector from the input 4x4 matrix pixels.
*/
STORAGE_CLASS_VECTOR_FUNC_C tvector max4x4m(
	s_4x4_matrix m)
{
	tvector vt00 = OP_vec_max(m.v00, m.v01);
	tvector vt01 = OP_vec_max(m.v02, m.v03);
	tvector vt02 = OP_vec_max(m.v10, m.v11);
	tvector vt03 = OP_vec_max(m.v12, m.v13);
	tvector vt04 = OP_vec_max(m.v20, m.v21);
	tvector vt05 = OP_vec_max(m.v22, m.v23);
	tvector vt06 = OP_vec_max(m.v30, m.v31);
	tvector vt07 = OP_vec_max(m.v32, m.v33);

	tvector vt10 = OP_vec_max(vt00, vt01);
	tvector vt11 = OP_vec_max(vt02, vt03);
	tvector vt12 = OP_vec_max(vt04, vt05);
	tvector vt13 = OP_vec_max(vt06, vt07);

	tvector vt20 = OP_vec_max(vt10, vt11);
	tvector vt21 = OP_vec_max(vt12, vt13);

	tvector vout = OP_vec_max(vt20, vt21);

	return vout;
}

#ifdef IS_ISP_2600_SYSTEM
/** @brief SAD between two 3x3 matrices
 *
 *  @param[in] a 3x3 matrix with pixels
 *
 *  @param[in] b 3x3 matrix with pixels
 *
 *  @return 3x3 matrix SAD
 *
 * This function calculates the sum of absolute difference between two matrices.
 * Both input pixels and SAD are normalized by a factor of SAD3x3_IN_SHIFT and
 * SAD3x3_OUT_SHIFT respectively.
 * Computed SAD is 1/(2 ^ (SAD3x3_IN_SHIFT + SAD3x3_OUT_SHIFT)) ie 1/16 factor
 * of original SAD and it's more precise than sad3x3m()
*/

STORAGE_CLASS_VECTOR_FUNC_C tvector sad3x3m_precise(
	s_3x3_matrix a,
	s_3x3_matrix b)
{
	int in_shift = SAD3x3_IN_SHIFT;
	int out_shift = SAD3x3_OUT_SHIFT;
	tvector2w wsad;
	tvector vd00 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v00, in_shift), OP_vec1w_asrrnd_c(b.v00, in_shift));
	tvector vd01 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v01, in_shift), OP_vec1w_asrrnd_c(b.v01, in_shift));
	tvector vd02 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v02, in_shift), OP_vec1w_asrrnd_c(b.v02, in_shift));
	tvector vd10 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v10, in_shift), OP_vec1w_asrrnd_c(b.v10, in_shift));
	tvector vd11 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v11, in_shift), OP_vec1w_asrrnd_c(b.v11, in_shift));
	tvector vd12 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v12, in_shift), OP_vec1w_asrrnd_c(b.v12, in_shift));
	tvector vd20 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v20, in_shift), OP_vec1w_asrrnd_c(b.v20, in_shift));
	tvector vd21 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v21, in_shift), OP_vec1w_asrrnd_c(b.v21, in_shift));
	tvector vd22 = OP_vec1w_subabssat(OP_vec1w_asrrnd_c(a.v22, in_shift), OP_vec1w_asrrnd_c(b.v22, in_shift));
	tvector vsad1 = OP_vec1w_add(vd00, vd01);
	tvector vsad2 = OP_vec1w_add(vd10, vd11);
	tvector vsad3 = OP_vec1w_add(vd20, vd21);
	tvector vsad4 = OP_vec1w_add(vd02, vd12);
	vsad1 = OP_vec1w_add(vsad1, vd22);
	vsad2 = OP_vec1w_add(vsad2, vsad3);
	/* This implementation assumes input shift is >= 2
	 * Otherwise, it would cause overflow during above add operations.
	 * Range of normalized difference (vd00 to vd22) is:
	 * [0 - ((2 ^ (ISP_VEC_ELEMBITS - in_shift) - 1)] i.e. 0 - 0x3fff for
	 * ISP_VEC_ELEMBITS = 16, in_shift = 2. So, at most 4 vd-s can be
	 * accumulated by unsaturated addition without overflow */
	OP___assert(((1<<(ISP_VEC_ELEMBITS-in_shift))-1)*4 < ((1<<ISP_VEC_ELEMBITS)-1));
	/* vsad1 => vd00 + vd01 + vd22        *
	 * vsad2 => vd10 + vd11 + vd20 + vd21 *
	 * vsad4 => vd02 + vd12               */
	wsad = OP_vec1w_cast_vec2w_u(vsad4);
	wsad = OP_vec2w_add(wsad, OP_vec1w_cast_vec2w_u(vsad1));
	wsad = OP_vec2w_add(wsad, OP_vec1w_cast_vec2w_u(vsad2));
#ifdef IS_ISP_2600_SYSTEM
	vsad1 = OP_vec_asrrnd_wsplit_c(wsad.vec1, wsad.vec0, out_shift);
#else
	vsad1 = OP_vec_asrrnd_wsplit_c(wsad.d1, wsad.d0, out_shift);
#endif
	return vsad1;
}

/** @brief SAD between two 3x3 matrices
 *
 *  @param[in] a 3x3 matrix with pixels
 *
 *  @param[in] b 3x3 matrix with pixels
 *
 *  @return 3x3 matrix SAD
 *
 * This function calculates the sum of absolute difference between two matrices.
 * This version saves cycles by avoiding input normalization and wide vector
 * operation during sum computation
 * Input pixel differences are computed by absolute of rounded, halved
 * subtraction. Normalized sum is computed by rounded averages.
 * Computed SAD is (1/2)*(1/16) = 1/32 factor of original SAD. Factor 1/2 comes
 * from input halving operation and factor 1/16 comes from mean operation
*/

STORAGE_CLASS_VECTOR_FUNC_C tvector sad3x3m(
	s_3x3_matrix a,
	s_3x3_matrix b)
{
	s_3x3_matrix m;
	tvector vsad;
	m.v00 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v00, b.v00));
	m.v01 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v01, b.v01));
	m.v02 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v02, b.v02));
	m.v10 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v10, b.v10));
	m.v11 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v11, b.v11));
	m.v12 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v12, b.v12));
	m.v20 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v20, b.v20));
	m.v21 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v21, b.v21));
	m.v22 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v22, b.v22));
	vsad = mean3x3m(m);
	return vsad;
}

/** @brief SAD between two 5x5 matrices
 *
 *  @param[in] a 5x5 matrix with pixels
 *
 *  @param[in] b 5x5 matrix with pixels
 *
 *  @return 5x5 matrix SAD
 *
 * Input pixel differences are computed by absolute of rounded, halved
 * subtraction. Normalized sum is computed by rounded averages.
 * Computed SAD is (1/2)*(1/32) = 1/64 factor of original SAD. Factor 1/2 comes
 * from input halving operation and factor 1/32 comes from mean operation
*/

STORAGE_CLASS_VECTOR_FUNC_C tvector sad5x5m(
	s_5x5_matrix a,
	s_5x5_matrix b)
{
	s_5x5_matrix m;
	tvector vsad;

	m.v00 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v00, b.v00));
	m.v01 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v01, b.v01));
	m.v02 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v02, b.v02));
	m.v03 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v03, b.v03));
	m.v04 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v04, b.v04));

	m.v10 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v10, b.v10));
	m.v11 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v11, b.v11));
	m.v12 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v12, b.v12));
	m.v13 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v13, b.v13));
	m.v14 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v14, b.v14));

	m.v20 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v20, b.v20));
	m.v21 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v21, b.v21));
	m.v22 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v22, b.v22));
	m.v23 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v23, b.v23));
	m.v24 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v24, b.v24));

	m.v30 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v30, b.v30));
	m.v31 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v31, b.v31));
	m.v32 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v32, b.v32));
	m.v33 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v33, b.v33));
	m.v34 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v34, b.v34));

	m.v40 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v40, b.v40));
	m.v41 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v41, b.v41));
	m.v42 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v42, b.v42));
	m.v43 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v43, b.v43));
	m.v44 = OP_vec1w_abs(OP_vec1w_subhalfrnd(a.v44, b.v44));

	vsad = mean5x5m(m);
	return vsad;
}
#endif
/*
.*.
 * This function will do bi-linear Interpolation on
 * inputs a and b using cloned weight factor w
 * The bilinear interpolation equation is (a*w) + b*(1-w)
 * But this is implemented as (a-b)*w + b for optimization on number of multipliers
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *.
 * 8 operations
 */

STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_bilinear_interpol_approx_c(
	tvector1w a,
	tvector1w b,
	int w)
{
	tvector1w out, diff;
	tvector2w slope, acc, b2w;
	tflags    cin = OP_flag_clone(0);

	diff = OP_vec1w_subhalfrnd(a, b);
	slope = _OP_vec1w_muld_c(diff, w); /* out0 is Q.28 */

	/* shift left by Q28_TO_Q15_SHIFT_VAL */
#ifdef IS_ISP_2600_SYSTEM
	b2w.vec1 = OP_vec_lsrh_c_cin(OP_vec_sgnext(b), CASTVEC1W(b), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
	b2w.vec0 = OP_vec_lsrh_c_cin(CASTVEC1W(b), OP_vec_clone(0), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
#else
	b2w.d1 = OP_vec_lsrh_c_cin(OP_vec_sgnext(b), CASTVEC1W(b), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
	b2w.d0 = OP_vec_lsrh_c_cin(CASTVEC1W(b), OP_vec_clone(0), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
#endif

	acc = OP_vec2w_add(slope, b2w);

#ifdef IS_ISP_2600_SYSTEM
	out = OP_vec_asrrnd_wsplit_ic(acc.vec1, acc.vec0, Q28_TO_Q15_SHIFT_VAL);
#else
	out = OP_vec_asrrnd_wsplit_ic(acc.d1, acc.d0, Q28_TO_Q15_SHIFT_VAL);
#endif

	return out;
}

/*
.*.
 * This function will do bi-linear Interpolation on
 * inputs a and b using weight factor w
 * The bilinear interpolation equation is (a*w) + b*(1-w)
 * But this is implemented as (a-b)*w + b for optimization on number of multipliers
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *.
 */

STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_bilinear_interpol_approx(
	tvector1w a,
	tvector1w b,
	tvector1w w)
{
	tvector1w out, diff;
	tvector2w slope, acc, b2w;
	tflags		cin = OP_flag_clone(0);

	diff = OP_vec1w_subhalfrnd(a, b);
	slope = _OP_vec1w_muld(diff, w); /* out0 is Q.28 */

	/* shift left by Q28_TO_Q15_SHIFT_VAL */
#ifdef IS_ISP_2600_SYSTEM
	b2w.vec1 = OP_vec_lsrh_c_cin(OP_vec_sgnext(b), CASTVEC1W(b), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
	b2w.vec0 = OP_vec_lsrh_c_cin(CASTVEC1W(b), OP_vec_clone(0), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
#else
	b2w.d1 = OP_vec_lsrh_c_cin(OP_vec_sgnext(b), CASTVEC1W(b), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
	b2w.d0 = OP_vec_lsrh_c_cin(CASTVEC1W(b), OP_vec_clone(0), ISP_VEC_ELEMBITS - Q28_TO_Q15_SHIFT_VAL, cin);
#endif

	acc = OP_vec2w_add(slope, b2w);

#ifdef IS_ISP_2600_SYSTEM
	out = OP_vec_asrrnd_wsplit_ic(acc.vec1, acc.vec0, Q28_TO_Q15_SHIFT_VAL);
#else
	out = OP_vec_asrrnd_wsplit_ic(acc.d1, acc.d0, Q28_TO_Q15_SHIFT_VAL);
#endif
	return out;
}

/*
.*.
 * This function will do bi-linear Interpolation on
 * inputs a and b using weight factor w
 * The bilinear interpolation equation is (a*w) + b*(1-w)
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *.
 * 5 operations
 */

STORAGE_CLASS_VECTOR_FUNC_C tvector1w OP_vec1w_bilinear_interpol(
	tvector1w a,
	tvector1w b,
	tscalar1w w)
{
	tvector1w out;
	tvector2w out0,out1,out2;

	out1 = _OP_vec1w_muld_c(a, w); /*out1 is Q.29 */
	out2 = _OP_vec1w_muld_c(b, (ONE_IN_Q14 - w)); /*out2 is Q.29 */
	out0 = OP_vec2w_add(out1, out2); /*out0 is Q.29 */

#ifdef IS_ISP_2600_SYSTEM
	out = OP_vec_asrrnd_wsplit_ic(out0.vec1, out0.vec0, Q29_TO_Q15_SHIFT_VAL); /*out is Q.15 */
#else
	out = OP_vec_asrrnd_wsplit_ic (out0.d1, out0.d0, Q29_TO_Q15_SHIFT_VAL); /*out is Q.15 */
#endif
	return out;
}

/** @brief Block matching algorithm for a block size of 16x16 and reference block 8x8
 *  @param[in] 2D block of 16x16 pixels
 *  @param[in] 2D block of 8x8 pixels
 *  @param[in] right shift value
 *
 *  @return Vector(s) with number of total SADs generated as the valid elements
 *  This algorithm performs pixel matching of the reference block with the search block
 *  and calculates sum of absolute difference (SADs) of each pixel of reference block with the
 *  search block. The search block is traversed with the reference block comparison
 *  depending on the search window size and pixel shift.
 *  The number of SADS calculated by this algorithm depends on the search winow, reference block
 *  and pixel shift and is given by the following formula:
 *  num_sads = ((search_window_ht - reference_block_ht)/pixel_shift + 1) *
 *             ((search_window_wd - reference_block_wd)/pixel_shift + 1)
 *
 *  @details
 *  The search block size is fixed at 16x16 pixels, however the search window can be either
 *  16x16 or 14x14 pixels. Depending on the search window size, appropriate ISP instruction
 *  should be used. Four ISP instructions are available based on the pxiel shift used to
 *  traverse the search window and search window. These are:
 *  1: OP_asp_bma_16_2_32way ==> 16x16 search window, 2 pixel shift traversal, 25 SADs as o/p
 *  2: OP_asp_bma_16_1_32way ==> 16x16 search window, 1 pixel shift traversal, 81 SADs as o/p
 *  3: OP_asp_bma_14_2_32way ==> 14x14 search window, 2 pixel shift traversal, 16 SADs as o/p
 *  4: OP_asp_bma_14_1_32way ==> 14x14 search window, 1 pixel shift traversal, 49 SADs as o/p
 *
 *  These instructions are designed for ISP_NWAY = 64. They also work for ISP_NWAY=32, but a
 * block of 16x16 pixels is stored in 8 vectors instead of 4 as in the case of ISP_NWAY=64 and
 * a block of 8x8 pixels is stored in 2 vectors rather than 1 vector. The way a block of 16x16
 * is stored in 8 vectors for ISP_NWAY=32 is as follows:
 *
 *  	block of 16x8 pixels                block of 16x16 pixels
 *    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7       _________
 *  0|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|      |A_1|B_1|
 *  1|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|      |A_2|B_2|
 *  2|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|      |C_1|D_1|
 *  3|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|      |C_2|D_2|
 *   |<-----A_1----->|<-----B_1----->|      |_______|
 *    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  4|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
 *  5|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
 *  6|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
 *  7|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
 *   |<-----A_2----->|<-----B_2----->|
 *
 *   A block of 8x4 (32 elements) (1 vector for ISP_NWAY=32)
 *   0 1 2 3 4 5 6 7
 * 0|-|-|-|-|-|-|-|-|
 * 1|-|-|-|-|-|-|-|-|
 * 2|-|-|-|-|-|-|-|-|
 * 3|-|-|-|-|-|-|-|-|
 *  |<--block_8x4-->|
 *
 * The above instructions produce output as vectors and the number of vectors given as output
 * depends on the ISP_NWAY. For ISP_NWAY = 32, OP_asp_bma_16_2_32way gives 1 vector as output
 * with 25 valid elements, OP_asp_bma_16_1_32way produces output in 3 vectors with 81 valid
 * elements, OP_asp_bma_14_2_32way produces 1 vector output with 16 valid elements and
 * OP_asp_bma_14_1_32way produces 2 vectors as output with 49 valid elements.
 */

/** This function performs Block Matching Algorithm search comparing the reference block of size
 * 8x8 with all the blocks of size 8x8 in the search window of 16x16.
 * The step between each search is 2 pixel.
 * Number of SADs calculated for 2 pixel step is 25.
 * These sads are stored in a vector with only 25 valid elements.
 */

#ifdef IS_ISP_2600_SYSTEM
#ifndef BXT_C0 /* Does not get compiled for Broxton C0 because of the different interface */
STORAGE_CLASS_VECTOR_FUNC_C bma_output_16_2 OP_vec1w_asp_bma_16_2_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_16_2 output;
	output = OP_vec_clone(0);

#if ISP_NWAY == 32
	OP___assert(ISP_NWAY % 32 == 0);
	output = OP_asp_bma_16_2_32way(search_area.A_1, search_area.A_2,
						search_area.B_1, search_area.B_2,
						search_area.C_1, search_area.C_2,
						search_area.D_1, search_area.D_2,
						input_block.Ref_1, input_block.Ref_2,
						shift);

#else
#error "Only ISP_NWAY = 32 supported"
#endif /*ISP_NWAY == 32 */
	return output;
}

/* This function performs Block Matching Algorithm search comparing the reference block of size
 * 8x8 with all the blocks of size 8x8 in the search window of 16x16.
 * The step between each search is 1 pixel.
 * Number of SADs calculated for 1 pixel step is 81.
 * These sads are stored in 3 vectors with 2 complete vectors and one
 * with only 17 valid elements.
 */

STORAGE_CLASS_VECTOR_FUNC_C bma_output_16_1 OP_vec1w_asp_bma_16_1_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_16_1 output;
	output.bma_lower = OP_vec_clone(0);
	output.bma_middle = OP_vec_clone(0);
	output.bma_upper = OP_vec_clone(0);

	OP___assert(ISP_NWAY % 32 == 0);
#if ISP_NWAY == 32
	asp_bma_16_1_32way(Any, search_area.A_1, search_area.A_2,
						search_area.B_1, search_area.B_2,
						search_area.C_1, search_area.C_2,
						search_area.D_1, search_area.D_2,
						input_block.Ref_1, input_block.Ref_2,
						shift,
						output.bma_lower, output.bma_middle, output.bma_upper);

#else
#error "Only ISP_NWAY = 32 supported"
#endif /*ISP_NWAY == 32 */
	return output;
}

/* This function performs Block Matching Algorithm search comparing the reference block of size
 * 8x8 with all the blocks of size 8x8 in the search window of 14x14. The block chosen to create
 * a search window is 16x16.
 * The step between each search is 2 pixels.
 * Number of SADs calculated for 2 pixel step is 16.
 * These sads are stored in a single vector with only 16 valid elements.
 */

STORAGE_CLASS_VECTOR_FUNC_C bma_output_14_2 OP_vec1w_asp_bma_14_2_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_14_2 output;
	output = OP_vec_clone(0);
	OP___assert(ISP_NWAY % 32 == 0);
#if ISP_NWAY == 32
	output = OP_asp_bma_14_2_32way(search_area.A_1, search_area.A_2,
						search_area.B_1, search_area.B_2,
						search_area.C_1, search_area.C_2,
						search_area.D_1, search_area.D_2,
						input_block.Ref_1, input_block.Ref_2,
						shift);
#else
#error "Only ISP_NWAY = 32 supported"
#endif /*ISP_NWAY == 32 */
	return output;
}

/* This function performs Block Matching Algorithm search comparing the reference block of size
 * 8x8 with all the blocks of size 8x8 in the search window of 14x14. The block chosen to create
 * a search window is 16x16.
 * The step between each search is 1 pixels.
 * Number of SADs calculated for 1 pixel step is 49.
 * These sads are stored in two vectors with one complete vector and the other with
 * only 17 valid elements.
 */

STORAGE_CLASS_VECTOR_FUNC_C bma_output_14_1 OP_vec1w_asp_bma_14_1_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_14_1 output;
	output.bma_lower = OP_vec_clone(0);
	output.bma_upper = OP_vec_clone(0);
	OP___assert(ISP_NWAY % 32 == 0);
#if ISP_NWAY == 32
	asp_bma_14_1_32way(Any, search_area.A_1, search_area.A_2,
						search_area.B_1, search_area.B_2,
						search_area.C_1, search_area.C_2,
						search_area.D_1, search_area.D_2,
						input_block.Ref_1, input_block.Ref_2,
						shift,
						output.bma_lower, output.bma_upper);

#else
#error "Only ISP_NWAY = 32 supported"
#endif /*ISP_NWAY == 32 */
	return output;
}
#else /* BXT_C0 */

/** @brief OP_vec1w_single_bfa_9x9_reduced
 *
 * @param[in] weights - spatial and range weight lut
 * @param[in] threshold - threshold plane, for range weight scaling
 * @param[in] central_pix - central pixel plane
 * @param[in] src_plane - src pixel plane
 *
 * @return   Bilateral filter output pixel
 *
 * This function implements, reduced 9x9 single bilateral filter.
 * Output = sum(pixel * weight) / sum(weight)
 * Where sum is summation over reduced 9x9 block set. Reduced because few
 * corner pixels are not taken.
 * weight = spatial weight * range weight
 * spatial weights are loaded from spatial_weight_lut depending on src pixel
 * position in the 9x9 block
 * range weights are computed by table look up from range_weight_lut depending
 * on scaled absolute difference between src and central pixels.
 * threshold is used as scaling factor. range_weight_lut consists of
 * BFA_RW_LUT_SIZE numbers of LUT entries to model any distribution function.
 * Piecewise linear approximation technique is used to compute range weight
 * It computes absolute difference between central pixel and 61 src pixels.
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector1wb4x8 OP_vec1w_single_bfa_9x9_reduced(
	bfa_weights weights,
	tvector1wb4x8 threshold,
	tvector1wb4x8 central_pix,
	s_12x16packed_matrix src)
{
	tvector1wb4x8 sop0, sop1, sow;
	tvector2w sop;
	tvector1wb4x8 out;
	OP___assert(ISP_NWAY % 32 == 0);
	asp_bfa_s_12_16_32way(Any, src.b00, src.b40, src.b80, src.b08, src.b48, src.b88,
			      central_pix, threshold,
			      weights.rw, weights.sw0, weights.sw1, weights.sw2,
			      sop0, sop1, sow);

	sop.vec0 = OP_vec_even(sop0, sop1);
	sop.vec1 = OP_vec_odd(sop0, sop1);
	out = OP_vec_div_wsplit(sop.vec1, sop.vec0, sow);
	return out;
}

/** @brief OP_1w_joint_bfa_9x9_reduced
 *
 * @param[in] weights - spatial and range weight lut
 * @param[in] threshold0 - 1st threshold plane, for range weight scaling
 * @param[in] central_pix0 - 1st central pixel plane
 * @param[in] src0_plane - 1st pixel plane
 * @param[in] threshold1 - 2nd threshold plane, for range weight scaling
 * @param[in] central_pix1 - 2nd central pixel plane
 * @param[in] src1_plane - 2nd pixel plane
 *
 * @return   Joint bilateral filter output pixel
 *
 * This function implements, reduced 9x9 joint bilateral filter.
 * It computes absolute difference between central pixel and 2 sets of src pixels.
 * Sum of absolute values are used to range weight table look up, which is piecewise
 * linear approximation of any given distribution function.
 * 61 spatial weights are multiplied with computed range weights to get weights.
 * Final output is computed as = sum(pixel * weight) / sum(weight)
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector1wb4x8 OP_vec1w_joint_bfa_9x9_reduced(
	bfa_weights weights,
	tvector1wb4x8 threshold0,
	tvector1wb4x8 central_pix0,
	s_12x16packed_matrix src0,
	tvector1wb4x8 threshold1,
	tvector1wb4x8 central_pix1,
	s_12x16packed_matrix src1)
{
	tvector1wb4x8 sop0, sop1, sow;
	tvector2w sop;
	tvector1wb4x8 out;
	OP___assert(ISP_NWAY % 32 == 0);
	asp_bfa_cj_12_16_32way(Any, src0.b00, src0.b40, src0.b80, src0.b08, src0.b48, src0.b88,
			       central_pix0, threshold0,
			       src1.b00, src1.b40, src1.b80, src1.b08, src1.b48, src1.b88,
			       central_pix1, threshold1,
			       weights.rw, weights.sw0, weights.sw1, weights.sw2,
			       sop0, sop1, sow);

	sop.vec0 = OP_vec_even(sop0, sop1);
	sop.vec1 = OP_vec_odd(sop0, sop1);
	out = OP_vec_div_wsplit(sop.vec1, sop.vec0, sow);
	return out;
}

#endif /* BXT_C0 */
#endif /* IS_ISP_2600_SYSTEM */
#endif /* __VECTOR_FUNC_PRIVATE_H_INCLUDED__ */
