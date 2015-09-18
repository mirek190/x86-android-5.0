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

#include "isp_op1w.h"
#include "isp_op2w.h"
#include "ref_vector_func.h"
#include "isp_config.h"
#include "isp_op_count.h"
#include <assert_support.h>
#include <type_support.h>


/* when inlined the C file is empty because it is already included inside the H file */
#ifndef VECTOR_FUNC_INLINED

/*
 * Filter functions
 *
 * - Coring Functionality
 * - Normalised vs. non-normalised (scaling on output)
 * - 6dB vs. 9dB attentuation at Fs/2 (coefficients [1;2;1], vs. [1;1;1])
 * - Filters for spatial shift (fractional pixel group delay)
 * - Single vs. dual line outputs
 * - full rate vs. decimated
 * - factorised vs. direct
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector2w OP_1w_maccd_sat(
	tvector2w acc,
	tvector1w a,
	tvector1w b )
{
	tvector2w tmp = OP_1w_muld(a, b);
	tvector2w out = OP_2w_addsat(acc, tmp);
	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector2w OP_1w_maccd(
	tvector2w acc,
	tvector1w a,
	tvector1w b )
{
	tvector2w tmp = OP_1w_muld(a, b);
	tvector2w out = OP_2w_add(acc, tmp);
	return out;
}

#define OP_1W_MUL_REALIGNING_OP_CNT 2

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_mul_realigning(
	tvector1w a,
	tvector1w b,
	tscalar1w shift )
{
	tvector2w tmp, tmp2;
	tvector1w out;

	/* this building block is counted independently because the operation count
	   is smaller than the sum of the cost of the independent reference functions
	   that are used */
	inc_bbb_count_ext(bbb_func_OP_1w_mul_realigning, OP_1W_MUL_REALIGNING_OP_CNT);
	disable_bbb_count();
	tmp = OP_1w_muld(a, b);
	tmp2 = OP_2w_asrrnd(tmp, shift);
	out = OP_2w_sat_cast_to_1w(tmp2);
	enable_bbb_count();
	return out;
}


STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_input_scaling_offset_clamping(
	tvector1w x,
	tscalar1w_5bit_signed input_scale,
	tscalar1w_5bit_signed input_offset)
{

	assert((input_offset <= MAX_SHIFT_1W) && (input_offset >= -MAX_SHIFT_1W));
	assert((input_scale <= MAX_SHIFT_1W) && (input_scale >= -MAX_SHIFT_1W));
	int offset_value   = OP_1w_lsl(1, OP_1w_abs(input_offset));
	int clamp_value    = (1 << (NUM_BITS-1))-1;
	tvector1w tmp1     = OP_1w_asr(x, OP_1w_abs(input_scale));
	tvector1w tmp2     = OP_1w_lsl(x, OP_1w_abs(input_scale));
	tvector1w tmp3;
	if (input_offset != 0) {
		if (input_scale > 0) {
			if (input_offset > 0)
				tmp3 = OP_1w_add(tmp1, offset_value);

			else

				tmp3 = OP_1w_sub(tmp1, offset_value);
		} else {
			if (input_offset > 0)
				tmp3 = OP_1w_add(tmp2, offset_value);
			else
				tmp3 = OP_1w_sub(tmp2, offset_value);

		}
	} else
		tmp3 = tmp2;

	return OP_1w_clipz(tmp3, clamp_value);
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_output_scaling_clamping(
	tvector1w x,
	tscalar1w_5bit_signed output_scale)
{

	int clamp_value    = (1 << (NUM_BITS-1))-1;
	assert((output_scale <= MAX_SHIFT_1W) && (output_scale >= -MAX_SHIFT_1W));
	tvector1w tmp1     = OP_1w_asr(x, OP_1w_abs(output_scale));
	tvector1w tmp2     = OP_1w_lsl(x, OP_1w_abs(output_scale));
	if (output_scale > 0)
		return OP_1w_clipz(tmp1, clamp_value);

	else
		return OP_1w_clipz(tmp2, clamp_value);
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_piecewise_estimation(
	tvector1w x,
	ref_config_points test_config_points)
{
	int i, idx;
	tvector1w a, b, c, d, y;
	x = OP_1w_input_scaling_offset_clamping(x, INPUT_SCALE_FACTOR, INPUT_OFFSET_FACTOR);

	for (i = 0; i < MAX_CONFIG_POINTS-1; i++) {
		assert(test_config_points.x_cord[i] < test_config_points.x_cord[i+1]);
	}

	if (x < test_config_points.x_cord[0]) {
		y = test_config_points.y_offset[0];
	} else {
		if (x >= test_config_points.x_cord[MAX_CONFIG_POINTS-1]) {

			a = test_config_points.x_cord[MAX_CONFIG_POINTS-1];
			b = test_config_points.x_cord[MAX_CONFIG_POINTS-2];
			c = test_config_points.slope[MAX_CONFIG_POINTS-2];
			d = test_config_points.y_offset[MAX_CONFIG_POINTS-2];
		} else {
			/*
			   Find i such that x∈[x_(i-1),x_i ),i≤n
			 */
			for (i = 0; i < (MAX_CONFIG_POINTS-1); i++) {
				if ((x >= test_config_points.x_cord[i]) &&
						(x < test_config_points.x_cord[i+1])) {
					idx = i;
					break;
				}
			}
			a = x;
			b = test_config_points.x_cord[idx];
			c = test_config_points.slope[idx];
			d = test_config_points.y_offset[idx];
		}
		y = OP_1w_mul_realigning(c, OP_1w_sub(a,b), SLOPE_A_RESOLUTION);
		y = OP_1w_add(y, d);
	}
	y = OP_1w_output_scaling_clamping(y, OUTPUT_SCALE_FACTOR);

	return y;
}

/* XCU Core function */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_XCU(
	tvector1w x,
	ref_config_points test_config_points)
{
	int range, tmp_range;
	int max_config_pt, min_config_pt;
	int j;
	int i = 0;
	int exp_of_2 = 0;

	tvector1w x_val, x_val_prev, y, slope, offset, tmpx;
	/*init_vectors contains the LUT for interpolation function */
	ref_config_point_vectors init_vectors;
	tscalar1w_16bit idx;

	for ( j = 0; j < MAX_CONFIG_POINTS-1; j++) {
		assert (test_config_points.x_cord[j] < test_config_points.x_cord[j+1]);
	}
	/*config points are monotically increasing*/
	max_config_pt = test_config_points.x_cord[MAX_CONFIG_POINTS-1];
	min_config_pt = test_config_points.x_cord[0];

	/*Divide the distance between x0 and xn into CONFIG_UNIT_LUT_SIZE (ISP_NWAY = 32) intervals */
	range = ((max_config_pt - min_config_pt) >> 5);
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
	tmp_range   = (1 << exp_of_2);

	/* Calculate LUT vectors. Range is divided into ISP_NWAY equal intervals
	 * It contains the config point data for interval (x1,xn). Cases for x<x1
	 * and x>=xn are treated as special cases */

	for (j = 0; j < CONFIG_UNIT_LUT_SIZE; j++) {
		if ((test_config_points.x_cord[0] + (j*range)) < test_config_points.x_cord[i+1]){
			init_vectors.x_cord_vec[j] = test_config_points.x_cord[i];
			init_vectors.slope_vec[j]  = test_config_points.slope[i];
			init_vectors.offset_vec[j] = test_config_points.y_offset[i];
		}
		else {
			init_vectors.x_cord_vec[j] = test_config_points.x_cord[i+1];
			init_vectors.slope_vec[j]  = test_config_points.slope[i+1];
			init_vectors.offset_vec[j] = test_config_points.y_offset[i+1];
			i++;
		}
	}

	/*Input Processing*/
	x = OP_1w_input_scaling_offset_clamping(x, INPUT_SCALE_FACTOR, INPUT_OFFSET_FACTOR);

	/*Piecewise linear estimation */
	if (x < test_config_points.x_cord[0]) {
		y = test_config_points.y_offset[0];
	} else {
		if (x >= test_config_points.x_cord[MAX_CONFIG_POINTS-1]) {
			slope      = test_config_points.slope[MAX_CONFIG_POINTS-2];
			offset     = test_config_points.y_offset[MAX_CONFIG_POINTS-2];
			x_val      = test_config_points.x_cord[MAX_CONFIG_POINTS-1];
			x_val_prev = test_config_points.x_cord[MAX_CONFIG_POINTS-2];
		} else {
			tmpx       = x - test_config_points.x_cord[0];
			idx        = tmpx >> init_vectors.exponent;
			slope      = ((idx > (CONFIG_UNIT_LUT_SIZE - 1)) ?
								test_config_points.slope[MAX_CONFIG_POINTS-2] : init_vectors.slope_vec[idx]);
			offset     = ((idx > (CONFIG_UNIT_LUT_SIZE - 1)) ?
								test_config_points.y_offset[MAX_CONFIG_POINTS-2] : init_vectors.offset_vec[idx]);
			x_val_prev = ((idx > (CONFIG_UNIT_LUT_SIZE - 1)) ?
								test_config_points.x_cord[MAX_CONFIG_POINTS-2] : init_vectors.x_cord_vec[idx]);
			x_val      = x;
			}
		y = OP_1w_mul_realigning (slope, OP_1w_sub(x, x_val_prev), SLOPE_A_RESOLUTION);
		y = OP_1w_add (y, offset);
	}

	/*Output Processing */
	y = OP_1w_output_scaling_clamping(y, OUTPUT_SCALE_FACTOR);
	return y;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w coring (  tvector1w coring_vec,
		tvector1w filt_input,
		tscalar1w m_CnrCoring0)
{
	tvector1w out, Coring_W, tmp1, tmp2;

	Coring_W = OP_1w_addsat(coring_vec, /*OP_int_cast_to_1w(700)*/m_CnrCoring0);

	tmp1 = OP_1w_addsat(filt_input, Coring_W);
	tmp1 = OP_1w_min (tmp1,OP_int_cast_to_1w(0));

	tmp2 = OP_1w_subsat(filt_input, Coring_W);
	tmp2 = OP_1w_max (tmp2, OP_int_cast_to_1w(0));

	out   = OP_1w_mux (tmp1, tmp2, OP_1w_le(filt_input, OP_int_cast_to_1w(-1)));

	return out;
}

/*
 * Normalised FIR with coefficients [3,4,1], -5dB at Fs/2, -90 degree phase shift (quarter pixel)
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_5dB_m90_nrm (
	const s_1w_1x3_matrix		m)
{
  tvector1w out11 = OP_1w_avgrnd(m.v02, m.v00);
  tvector1w out1  = OP_1w_avgrnd(out11, m.v00);
  tvector1w out   = OP_1w_avgrnd(out1, m.v01);

  return out;
}

/*
 * Normalised FIR with coefficients [1,4,3], -5dB at Fs/2, +90 degree phase shift (quarter pixel)
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_5dB_p90_nrm (
	const s_1w_1x3_matrix		m)
{
	tvector1w out11 = OP_1w_avgrnd(m.v00, m.v02);
	tvector1w out1 = OP_1w_avgrnd(out11, m.v02);
	tvector1w out = OP_1w_avgrnd(out1, m.v01);
  return out;
}

/*
 * Normalised FIR with coefficients [1,2,1], -6dB at Fs/2
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_6dB_nrm (
	const s_1w_1x3_matrix		m)
{
	tvector1w	out = OP_1w_avgrnd(OP_1w_avgrnd (m.v00, m.v02), m.v01);
    return out;
}

/*
 * Normalised FIR for 1.5/1.25
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_6dB_nrm_ph0 (
	const s_1w_1x3_matrix		m)
{
    tvector1w out = OP_2w_asrrnd((OP_1w_muld(m.v00, OP_int_cast_to_1w(13))
				+ OP_1w_muld(m.v01,OP_int_cast_to_1w(16))
				+ OP_1w_muld(m.v02, OP_int_cast_to_1w(3))), OP_int_cast_to_1w(5));

return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_6dB_nrm_ph1 (
	const s_1w_1x3_matrix		m)
{
	tvector1w out = OP_2w_asrrnd((OP_1w_muld(m.v00, OP_int_cast_to_1w(9))
				+ OP_1w_muld(m.v01, OP_int_cast_to_1w(16))
				+ OP_1w_muld(m.v02, OP_int_cast_to_1w(7))), OP_int_cast_to_1w(5));
return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_6dB_nrm_ph2 (
	const s_1w_1x3_matrix		m)
{
	tvector1w out = OP_2w_asrrnd((OP_1w_muld(m.v00, OP_int_cast_to_1w(5))
				+ OP_1w_muld(m.v01, OP_int_cast_to_1w(16))
				+ OP_1w_muld(m.v02, OP_int_cast_to_1w(11))), OP_int_cast_to_1w(5));
return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_6dB_nrm_ph3 (
	const s_1w_1x3_matrix		m)
{
	tvector1w out = OP_2w_asrrnd((OP_1w_muld(m.v00,OP_int_cast_to_1w(1) )
				+ OP_1w_muld(m.v01, OP_int_cast_to_1w(16))
				+ OP_1w_muld(m.v02,OP_int_cast_to_1w(15))), OP_int_cast_to_1w(5));
return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_6dB_nrm_calc_coeff (
	const s_1w_1x3_matrix		m,
	tscalar1w_3bit coeff)
{

	tvector1w out = OP_2w_asrrnd((OP_1w_muld(m.v00, OP_int_cast_to_1w(8-coeff))
				+ OP_1w_muld(m.v01, OP_int_cast_to_1w(16))
				+ OP_1w_muld(m.v02, OP_int_cast_to_1w(8+coeff))), OP_int_cast_to_1w(5));
return out;
}

/*
 * Normalised FIR with coefficients [1,1,1], -9dB at Fs/2
 */
STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x3m_9dB_nrm (
	const s_1w_1x3_matrix		m)
{
	tvector1w	out = OP_1w_addsat (OP_1w_avgrnd (m.v00, m.v02), OP_1w_asrrnd(m.v01,OP_int_cast_to_1w(1)));
    return out;
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

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir3x3m_6dB_nrm (
	const s_1w_3x3_matrix		m)
{
	s_1w_1x3_matrix in0 = {m.v00, m.v01, m.v02};
	s_1w_1x3_matrix in1 = {m.v10, m.v11, m.v12};
	s_1w_1x3_matrix in2 = {m.v20, m.v21, m.v22};
	s_1w_1x3_matrix tmp;
	tvector1w out;

	tmp.v00 = fir1x3m_6dB_nrm(in0);
	tmp.v01 = fir1x3m_6dB_nrm(in1);
	tmp.v02 = fir1x3m_6dB_nrm(in2);

	out = fir1x3m_6dB_nrm(tmp);
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
STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir3x3m_9dB_nrm (
	const s_1w_3x3_matrix		m)
{
	tvector1w vtmp0 = OP_1w_avgrnd (OP_1w_avgrnd (m.v00, m.v02), OP_1w_avgrnd (m.v20, m.v22));
	tvector1w vtmp1 = OP_1w_avgrnd (OP_1w_avgrnd (m.v01, m.v21), OP_1w_avgrnd (m.v10, m.v12));
	tvector1w out   = OP_1w_addsat (OP_1w_avgrnd(vtmp0, vtmp1), OP_1w_asrrnd(m.v11,OP_int_cast_to_1w(3)));
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
STORAGE_CLASS_REF_VECTOR_FUNC_C s_1w_2x1_matrix fir3x3m_6dB_out2x1_nrm (
	const s_1w_4x3_matrix		m)
{
	s_1w_1x3_matrix in0 = {m.v00, m.v01, m.v02};
	s_1w_1x3_matrix in1 = {m.v10, m.v11, m.v12};
	s_1w_1x3_matrix in2 = {m.v20, m.v21, m.v22};
	s_1w_1x3_matrix in3 = {m.v30, m.v31, m.v32};
	s_1w_1x3_matrix tmp0, tmp1;
	s_1w_2x1_matrix out;

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
STORAGE_CLASS_REF_VECTOR_FUNC_C s_1w_2x1_matrix fir3x3m_9dB_out2x1_nrm (
	const s_1w_4x3_matrix		m)
{
	tvector1w vtmp0 = OP_1w_avgrnd(m.v00, m.v02);
	tvector1w vtmp1 = OP_1w_avgrnd(m.v10, m.v12);
	tvector1w vtmp2 = OP_1w_avgrnd(m.v20, m.v22);
	tvector1w vtmp3 = OP_1w_avgrnd(m.v30, m.v32);

	tvector1w vtmp4 = OP_1w_avgrnd(vtmp0, vtmp2);
	tvector1w vtmp5 = OP_1w_avgrnd(OP_1w_avgrnd(m.v01, m.v21), vtmp1);
	tvector1w vtmp6 = OP_1w_avgrnd(vtmp1, vtmp3);
	tvector1w vtmp7 = OP_1w_avgrnd(OP_1w_avgrnd(m.v11, m.v31), vtmp2);

	tvector1w out0 = OP_1w_addsat (OP_1w_avgrnd(vtmp4, vtmp5), OP_1w_asrrnd(m.v11 ,OP_int_cast_to_1w(3)));
	tvector1w out1 = OP_1w_addsat (OP_1w_avgrnd(vtmp6, vtmp7), OP_1w_asrrnd(m.v21 ,OP_int_cast_to_1w(3)));
	s_1w_2x1_matrix out = {out0, out1};

return out;
}


STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir5x5m_15dB_nrm (
	const s_1w_5x5_matrix	m)
{
	tvector1w	out;
/* 1 */
	tvector1w vtmp0_1 = OP_1w_avgrnd (m.v00, m.v04);
	tvector1w vtmp1_1 = OP_1w_avgrnd (m.v40, m.v44);
	tvector1w vtmp2_1 = OP_1w_avgrnd (vtmp0_1, vtmp1_1);
/* 4 */
	tvector1w vtmp0_4 = OP_1w_avgrnd (m.v02, m.v42);
	tvector1w vtmp1_4 = OP_1w_avgrnd (m.v20, m.v24);
	tvector1w vtmp2_4 = OP_1w_avgrnd (vtmp0_4, vtmp1_4);
/* 3 : 4 - 1 */
	tvector1w vtmp0_3 = OP_1w_avgrnd (m.v01, m.v03);
	tvector1w vtmp1_3 = OP_1w_avgrnd (m.v10, m.v14);
	tvector1w vtmp2_3 = OP_1w_avgrnd (m.v30, m.v34);
	tvector1w vtmp3_3 = OP_1w_avgrnd (m.v41, m.v43);
	tvector1w vtmp4_3 = OP_1w_avgrnd (vtmp0_3, vtmp1_3);
	tvector1w vtmp5_3 = OP_1w_avgrnd (vtmp2_3, vtmp3_3);
/* 9 : 8 + 1 */
	tvector1w vtmp0_9 = OP_1w_avgrnd (m.v11, m.v13);
	tvector1w vtmp1_9 = OP_1w_avgrnd (m.v31, m.v33);
	tvector1w vtmp2_9 = OP_1w_avgrnd (vtmp0_9, vtmp1_9);
/* 12 : 8 + 4 */
	tvector1w vtmp0_12 = OP_1w_avgrnd (m.v12, m.v32);
	tvector1w vtmp1_12 = OP_1w_avgrnd (m.v21, m.v23);
	tvector1w vtmp2_12 = OP_1w_avgrnd (vtmp0_12, vtmp1_12);

	tvector1w vtmpa = OP_1w_subasr1 (vtmp2_1, vtmp4_3);
	tvector1w vtmpb = OP_1w_subasr1 (vtmp2_9, vtmp5_3);
	tvector1w vtmpc = OP_1w_avgrnd (vtmpa, vtmpb);

	tvector1w vtmpd = OP_1w_subasr1 (m.v22, vtmp2_12);
	tvector1w vtmpe = OP_1w_avgrnd (vtmpc, vtmp2_4);
	tvector1w vtmpf = OP_1w_avgrnd (vtmp4_3, vtmp5_3);

	tvector1w vtmpg = OP_1w_avgrnd (vtmpd, vtmpe);
	tvector1w vtmph = OP_1w_avgrnd (vtmpf, vtmp2_9);

	tvector1w vtmpi = OP_1w_avgrnd (vtmpg, vtmph);
	tvector1w vtmpj = OP_1w_addsat(vtmpi , OP_1w_asrrnd(vtmp2_12 ,OP_int_cast_to_1w(1)));

	tvector2w wvtmpk  = OP_1w_muld(vtmpj , OP_int_cast_to_1w(7282));	/* 128/144 */
	out = OP_2w_asr(wvtmpk,OP_int_cast_to_1w(13));
return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x5m_12dB_nrm (
	const s_1w_1x5_matrix m)
{
	tvector1w out111 = OP_1w_avgrnd(m.v04, m.v00);
	tvector1w out11 = OP_1w_avgrnd(out111, m.v02);
	tvector1w out1 = OP_1w_avgrnd(out11, m.v03);
	tvector1w out2 = OP_1w_avgrnd(m.v02, m.v01);
	tvector1w out = OP_1w_avgrnd(out1, out2);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir5x5m_12dB_nrm (
	const s_1w_5x5_matrix m)
{
	s_1w_1x5_matrix tmp;
	s_1w_1x5_matrix in0 = {m.v00, m.v01, m.v02, m.v03, m.v04};
	s_1w_1x5_matrix in1 = {m.v10, m.v11, m.v12, m.v13, m.v14};
	s_1w_1x5_matrix in2 = {m.v20, m.v21, m.v22, m.v23, m.v24};
	s_1w_1x5_matrix in3 = {m.v30, m.v31, m.v32, m.v33, m.v34};
	s_1w_1x5_matrix in4 = {m.v40, m.v41, m.v42, m.v43, m.v44};
	tvector1w out;

	tmp.v00 = fir1x5m_12dB_nrm(in0);
	tmp.v01 = fir1x5m_12dB_nrm(in1);
	tmp.v02 = fir1x5m_12dB_nrm(in2);
	tmp.v03 = fir1x5m_12dB_nrm(in3);
	tmp.v04 = fir1x5m_12dB_nrm(in4);
	out = fir1x5m_12dB_nrm(tmp);
	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x5m_box (
	s_1w_1x5_matrix m)
{
	tvector1w vtmp1 =  OP_1w_avgrnd( m.v00, m.v01);
	tvector1w vtmp2 =  OP_1w_avgrnd( m.v03, m.v04);
	tvector1w vtmp3 =  OP_1w_asrrnd( m.v02, 2);
	tvector1w vavg  =  OP_1w_avgrnd( vtmp1, vtmp2);
	tvector1w out   =  OP_1w_avgrnd( vavg,  vtmp3);
	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w fir1x9m_box (
	s_1w_1x9_matrix m)
{
	tvector1w vtmp1 =  OP_1w_avgrnd( m.v00, m.v01 );
	tvector1w vtmp2 =  OP_1w_avgrnd( m.v02, m.v03 );
	tvector1w vtmp3 =  OP_1w_avgrnd( m.v05, m.v06 );
	tvector1w vtmp4 =  OP_1w_avgrnd( m.v07, m.v08 );
	tvector1w vtmp5 =  OP_1w_asrrnd( m.v04, 3);

	tvector1w vavg1 =  OP_1w_avgrnd( vtmp1, vtmp2);
	tvector1w vavg2 =  OP_1w_avgrnd( vtmp3, vtmp4);
	tvector1w vavg  =  OP_1w_avgrnd( vavg1, vavg2);
	tvector1w out   =  OP_1w_avgrnd( vavg , vtmp5);

	return out;
}


STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean1x3m(
	s_1w_1x3_matrix m)
{
	tvector1w vavg1 = OP_1w_avgrnd(m.v00, m.v02);
	tvector1w vtmp1 = OP_1w_asrrnd(m.v01, 1);
	tvector1w out   = OP_1w_avgrnd(vavg1, vtmp1);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean3x3m(
	s_1w_3x3_matrix m)
{
	tvector1w vtmp1 = OP_1w_avgrnd(m.v00, m.v01);
	tvector1w vtmp2 = OP_1w_avgrnd(m.v10, m.v11);
	tvector1w vtmp3 = OP_1w_avgrnd(m.v20, m.v21);
	tvector1w vtmp4 = OP_1w_avgrnd(m.v02, m.v12);
	tvector1w vtmp5 = OP_1w_asrrnd(m.v22, 3);

	tvector1w vavg1 = OP_1w_avgrnd(vtmp1, vtmp2);
	tvector1w vavg2 = OP_1w_avgrnd(vtmp3, vtmp4);

	tvector1w out11 = OP_1w_avgrnd(vavg1, vavg2);
	tvector1w out   = OP_1w_avgrnd(out11, vtmp5);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean1x4m(
	s_1w_1x4_matrix m)
{
	tvector1w vavg1 = OP_1w_avgrnd(m.v00, m.v01);
	tvector1w vavg2 = OP_1w_avgrnd(m.v02, m.v03);
	tvector1w out   = OP_1w_avgrnd(vavg1, vavg2);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean4x4m(
	s_1w_4x4_matrix m)
{
	s_1w_1x4_matrix in0 = {m.v00, m.v01, m.v02, m.v03};
	s_1w_1x4_matrix in1 = {m.v10, m.v11, m.v12, m.v13};
	s_1w_1x4_matrix in2 = {m.v20, m.v21, m.v22, m.v23};
	s_1w_1x4_matrix in3 = {m.v30, m.v31, m.v32, m.v33};
	s_1w_1x4_matrix tmp;
	tvector1w out;

	tmp.v00 = mean1x4m(in0);
	tmp.v01 = mean1x4m(in1);
	tmp.v02 = mean1x4m(in2);
	tmp.v03 = mean1x4m(in3);
	out     = mean1x4m(tmp);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean2x3m(
	s_1w_2x3_matrix m)
{
	tvector1w vtmp1 = OP_1w_avgrnd(m.v00, m.v01);
	tvector1w vtmp2 = OP_1w_avgrnd(m.v10, m.v11);
	tvector1w vtmp3 = OP_1w_avgrnd(m.v02, m.v12);

	tvector1w vavg1 = OP_1w_avgrnd(vtmp1, vtmp2);
	tvector1w vavg2 = OP_1w_asrrnd(vtmp3, 1);

	tvector1w out = OP_1w_avgrnd(vavg1, vavg2);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean1x6m(
	s_1w_1x6_matrix m)
{
	tvector1w vtmp1 = OP_1w_avgrnd(m.v00, m.v01);
	tvector1w vtmp2 = OP_1w_avgrnd(m.v02, m.v03);
	tvector1w vtmp3 = OP_1w_avgrnd(m.v04, m.v05);

	tvector1w vavg1 = OP_1w_avgrnd(vtmp1, vtmp2);
	tvector1w vavg2 = OP_1w_asrrnd(vtmp3, 1);

	tvector1w out = OP_1w_avgrnd(vavg1, vavg2);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean5x5m(
	s_1w_5x5_matrix m)
{
	tvector1w t00 = OP_1w_avgrnd(m.v00, m.v01);
	tvector1w t01 = OP_1w_avgrnd(m.v02, m.v03);
	tvector1w t02 = OP_1w_avgrnd(m.v10, m.v11);
	tvector1w t03 = OP_1w_avgrnd(m.v12, m.v13);
	tvector1w t04 = OP_1w_avgrnd(m.v20, m.v21);
	tvector1w t05 = OP_1w_avgrnd(m.v22, m.v23);
	tvector1w t06 = OP_1w_avgrnd(m.v30, m.v31);
	tvector1w t07 = OP_1w_avgrnd(m.v32, m.v33);
	tvector1w t08 = OP_1w_avgrnd(m.v40, m.v41);
	tvector1w t09 = OP_1w_avgrnd(m.v42, m.v43);
	tvector1w t0A = OP_1w_avgrnd(m.v04, m.v14);
	tvector1w t0B = OP_1w_avgrnd(m.v24, m.v34);

	tvector1w t10 = OP_1w_avgrnd(t00, t01);
	tvector1w t11 = OP_1w_avgrnd(t02, t03);
	tvector1w t12 = OP_1w_avgrnd(t04, t05);
	tvector1w t13 = OP_1w_avgrnd(t06, t07);
	tvector1w t14 = OP_1w_avgrnd(t08, t09);
	tvector1w t15 = OP_1w_avgrnd(t0A, t0B);

	tvector1w t20 = OP_1w_avgrnd(t10, t11);
	tvector1w t21 = OP_1w_avgrnd(t12, t13);
	tvector1w t22 = OP_1w_avgrnd(t14, t15);
	tvector1w t23 = OP_1w_asrrnd(m.v44, 3);

	tvector1w t30 = OP_1w_avgrnd(t20, t21);
	tvector1w t31 = OP_1w_avgrnd(t22, t23);

	tvector1w out = OP_1w_avgrnd(t30, t31);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w mean6x6m(
	s_1w_6x6_matrix m)
{
	s_1w_1x6_matrix in0 = {m.v00, m.v01, m.v02, m.v03, m.v04, m.v05};
	s_1w_1x6_matrix in1 = {m.v10, m.v11, m.v12, m.v13, m.v14, m.v15};
	s_1w_1x6_matrix in2 = {m.v20, m.v21, m.v22, m.v23, m.v24, m.v25};
	s_1w_1x6_matrix in3 = {m.v30, m.v31, m.v32, m.v33, m.v34, m.v35};
	s_1w_1x6_matrix in4 = {m.v40, m.v41, m.v42, m.v43, m.v44, m.v45};
	s_1w_1x6_matrix in5 = {m.v50, m.v51, m.v52, m.v53, m.v54, m.v55};
	s_1w_1x6_matrix tmp;
	tvector1w out;

	tmp.v00 = mean1x6m(in0);
	tmp.v01 = mean1x6m(in1);
	tmp.v02 = mean1x6m(in2);
	tmp.v03 = mean1x6m(in3);
	tmp.v04 = mean1x6m(in4);
	tmp.v05 = mean1x6m(in5);
	out     = mean1x6m(tmp);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w min4x4m(
	s_1w_4x4_matrix m)
{
	tvector1w t00 = OP_1w_min(m.v00, m.v01);
	tvector1w t01 = OP_1w_min(m.v02, m.v03);
	tvector1w t02 = OP_1w_min(m.v10, m.v11);
	tvector1w t03 = OP_1w_min(m.v12, m.v13);
	tvector1w t04 = OP_1w_min(m.v20, m.v21);
	tvector1w t05 = OP_1w_min(m.v22, m.v23);
	tvector1w t06 = OP_1w_min(m.v30, m.v31);
	tvector1w t07 = OP_1w_min(m.v32, m.v33);

	tvector1w t10 = OP_1w_min(t00, t01);
	tvector1w t11 = OP_1w_min(t02, t03);
	tvector1w t12 = OP_1w_min(t04, t05);
	tvector1w t13 = OP_1w_min(t06, t07);

	tvector1w t20 = OP_1w_min(t10, t11);
	tvector1w t21 = OP_1w_min(t12, t13);

	tvector1w out = OP_1w_min(t20, t21);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w max4x4m(
	s_1w_4x4_matrix m)
{
	tvector1w t00 = OP_1w_max(m.v00, m.v01);
	tvector1w t01 = OP_1w_max(m.v02, m.v03);
	tvector1w t02 = OP_1w_max(m.v10, m.v11);
	tvector1w t03 = OP_1w_max(m.v12, m.v13);
	tvector1w t04 = OP_1w_max(m.v20, m.v21);
	tvector1w t05 = OP_1w_max(m.v22, m.v23);
	tvector1w t06 = OP_1w_max(m.v30, m.v31);
	tvector1w t07 = OP_1w_max(m.v32, m.v33);

	tvector1w t10 = OP_1w_max(t00, t01);
	tvector1w t11 = OP_1w_max(t02, t03);
	tvector1w t12 = OP_1w_max(t04, t05);
	tvector1w t13 = OP_1w_max(t06, t07);

	tvector1w t20 = OP_1w_max(t10, t11);
	tvector1w t21 = OP_1w_max(t12, t13);

	tvector1w out = OP_1w_max(t20, t21);

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w sad3x3m_precise(
	s_1w_3x3_matrix a,
	s_1w_3x3_matrix b)
{
	tvector1w sad;
	tvector1w in_shift = SAD3x3_IN_SHIFT;
	tvector1w out_shift = SAD3x3_OUT_SHIFT;
	tvector1w d00 = OP_1w_subabssat(OP_1w_asrrnd(a.v00, in_shift), OP_1w_asrrnd(b.v00, in_shift));
	tvector1w d01 = OP_1w_subabssat(OP_1w_asrrnd(a.v01, in_shift), OP_1w_asrrnd(b.v01, in_shift));
	tvector1w d02 = OP_1w_subabssat(OP_1w_asrrnd(a.v02, in_shift), OP_1w_asrrnd(b.v02, in_shift));
	tvector1w d10 = OP_1w_subabssat(OP_1w_asrrnd(a.v10, in_shift), OP_1w_asrrnd(b.v10, in_shift));
	tvector1w d11 = OP_1w_subabssat(OP_1w_asrrnd(a.v11, in_shift), OP_1w_asrrnd(b.v11, in_shift));
	tvector1w d12 = OP_1w_subabssat(OP_1w_asrrnd(a.v12, in_shift), OP_1w_asrrnd(b.v12, in_shift));
	tvector1w d20 = OP_1w_subabssat(OP_1w_asrrnd(a.v20, in_shift), OP_1w_asrrnd(b.v20, in_shift));
	tvector1w d21 = OP_1w_subabssat(OP_1w_asrrnd(a.v21, in_shift), OP_1w_asrrnd(b.v21, in_shift));
	tvector1w d22 = OP_1w_subabssat(OP_1w_asrrnd(a.v22, in_shift), OP_1w_asrrnd(b.v22, in_shift));
	tvector2w sad1 = OP_2w_add(OP_1w_cast_to_2w(d00), OP_1w_cast_to_2w(d01));
	tvector2w sad2 = OP_2w_add(OP_1w_cast_to_2w(d10), OP_1w_cast_to_2w(d11));
	tvector2w sad3 = OP_2w_add(OP_1w_cast_to_2w(d20), OP_1w_cast_to_2w(d21));
	tvector2w sad4 = OP_2w_add(OP_1w_cast_to_2w(d02), OP_1w_cast_to_2w(d12));
	sad1 = OP_2w_add(sad1, OP_1w_cast_to_2w(d22));
	sad2 = OP_2w_add(sad2, sad3);
	sad1 = OP_2w_add(sad1, sad4);
	sad1 = OP_2w_add(sad1, sad2);
	sad = OP_2w_cast_to_1w(OP_2w_asrrnd(sad1, out_shift));
	return sad;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w sad3x3m(
	s_1w_3x3_matrix a,
	s_1w_3x3_matrix b)
{
	s_1w_3x3_matrix m;
	tvector1w sad;
	m.v00 = OP_1w_abs(OP_1w_subasr1(a.v00, b.v00));
	m.v01 = OP_1w_abs(OP_1w_subasr1(a.v01, b.v01));
	m.v02 = OP_1w_abs(OP_1w_subasr1(a.v02, b.v02));
	m.v10 = OP_1w_abs(OP_1w_subasr1(a.v10, b.v10));
	m.v11 = OP_1w_abs(OP_1w_subasr1(a.v11, b.v11));
	m.v12 = OP_1w_abs(OP_1w_subasr1(a.v12, b.v12));
	m.v20 = OP_1w_abs(OP_1w_subasr1(a.v20, b.v20));
	m.v21 = OP_1w_abs(OP_1w_subasr1(a.v21, b.v21));
	m.v22 = OP_1w_abs(OP_1w_subasr1(a.v22, b.v22));
	sad = mean3x3m(m);
	return sad;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w sad5x5m(
	s_1w_5x5_matrix a,
	s_1w_5x5_matrix b)
{
	s_1w_5x5_matrix m;
	tvector1w sad;

	m.v00 = OP_1w_abs(OP_1w_subasr1(a.v00, b.v00));
	m.v01 = OP_1w_abs(OP_1w_subasr1(a.v01, b.v01));
	m.v02 = OP_1w_abs(OP_1w_subasr1(a.v02, b.v02));
	m.v03 = OP_1w_abs(OP_1w_subasr1(a.v03, b.v03));
	m.v04 = OP_1w_abs(OP_1w_subasr1(a.v04, b.v04));

	m.v10 = OP_1w_abs(OP_1w_subasr1(a.v10, b.v10));
	m.v11 = OP_1w_abs(OP_1w_subasr1(a.v11, b.v11));
	m.v12 = OP_1w_abs(OP_1w_subasr1(a.v12, b.v12));
	m.v13 = OP_1w_abs(OP_1w_subasr1(a.v13, b.v13));
	m.v14 = OP_1w_abs(OP_1w_subasr1(a.v14, b.v14));

	m.v20 = OP_1w_abs(OP_1w_subasr1(a.v20, b.v20));
	m.v21 = OP_1w_abs(OP_1w_subasr1(a.v21, b.v21));
	m.v22 = OP_1w_abs(OP_1w_subasr1(a.v22, b.v22));
	m.v23 = OP_1w_abs(OP_1w_subasr1(a.v23, b.v23));
	m.v24 = OP_1w_abs(OP_1w_subasr1(a.v24, b.v24));

	m.v30 = OP_1w_abs(OP_1w_subasr1(a.v30, b.v30));
	m.v31 = OP_1w_abs(OP_1w_subasr1(a.v31, b.v31));
	m.v32 = OP_1w_abs(OP_1w_subasr1(a.v32, b.v32));
	m.v33 = OP_1w_abs(OP_1w_subasr1(a.v33, b.v33));
	m.v34 = OP_1w_abs(OP_1w_subasr1(a.v34, b.v34));

	m.v40 = OP_1w_abs(OP_1w_subasr1(a.v40, b.v40));
	m.v41 = OP_1w_abs(OP_1w_subasr1(a.v41, b.v41));
	m.v42 = OP_1w_abs(OP_1w_subasr1(a.v42, b.v42));
	m.v43 = OP_1w_abs(OP_1w_subasr1(a.v43, b.v43));
	m.v44 = OP_1w_abs(OP_1w_subasr1(a.v44, b.v44));

	sad = mean5x5m(m);
	return sad;
}

/*
 *
 * This function will do bi-linear Interpolation on
 * inputs a and b using cloned weight factor c
 *
 * The bilinear interpolation equation is (a*w) + b*(1-w)
 * But this is implemented as (a-b)*w + b for optimization
 *
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *
 */
STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_bilinear_interpol_approx_c(
	tvector1w a,
	tvector1w b,
	tscalar1w_weight w)
{
	return(OP_1w_bilinear_interpol_approx(a, b, (tvector1w_weight)w));
}

/*
 *
 * This function will do bi-linear Interpolation on
 * inputs a and b using weight factor w
 *
 * The bilinear interpolation equation is (a*w) + b*(1-w)
 * But this is implemented as (a-b)*w + b for optimization
 *
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_bilinear_interpol_approx(
	tvector1w a,
	tvector1w b,
	tvector1w_weight w)
{
	tvector1w out1, out, out2;
	tvector2w out0, b_double;
	out2 = OP_1w_subasr1(a, b); /*out2 is Q.14 */
	out0 = OP_1w_muld(out2, w); /* out0 is Q.28 */
	b_double = OP_2w_lsl(b, Q28_TO_Q15_SHIFT_VAL);
	out1 = OP_2w_asrrnd(OP_2w_add(out0, b_double), OP_int_cast_to_1w(Q28_TO_Q15_SHIFT_VAL)); /*out1 is Q.15 */
	out  = OP_2w_sat_cast_to_1w(out1); /* out is Q.15 */

	return out;
}

/*
 *
 * This function will do bi-linear Interpolation on
 * inputs a and b using weight factor w
 *
 * The bilinear interpolation equation is (a*w) + b*(1-w)
 *
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_bilinear_interpol(
	tvector1w a,
	tvector1w b,
	tscalar1w_weight w)
{
	tscalar1w_weight w1 = OP_1w_sub(ONE_IN_Q14 , w);
	tvector2w out0;
	tvector1w out;

	out0 = OP_2w_add(OP_1w_muld(a, w), OP_1w_muld(b, w1)); /*out0 is Q.29 */
	out  = OP_2w_cast_to_1w(OP_2w_asrrnd(out0, OP_int_cast_to_1w(Q29_TO_Q15_SHIFT_VAL))); /*out is Q.15 */

	return out;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C int sad(
	tscalar1w **search_window_ptr,
	tscalar1w **ref_block_ptr,
	tscalar1w *sads,
	int ref_sz,
	int search_block_sz,
	tscalar1w_4bit_bma_shift shift)
{
	int i, j;
	tscalar1w elem_ref, elem_search;
	tscalar1w *row_ref, *row_search;
	tvector2w tmp_sum = 0;
	for (i = 0; i < ref_sz; i++) {
		row_ref = (tscalar1w *)ref_block_ptr + i*ref_sz;
		row_search = (tscalar1w *)search_window_ptr + i*search_block_sz;
		for (j = 0; j < ref_sz; j++) {
			elem_search = *(row_search + j);
			elem_ref = *(row_ref + j);
			tmp_sum = OP_2w_add(tmp_sum, OP_2w_subabssat(elem_search, elem_ref));
		}
	}
	tmp_sum = OP_2w_asr(tmp_sum, shift);
	*sads = OP_2w_cast_to_1w(tmp_sum);
	return 0;
}
/* Generic implementation of block matching algorithm
 * Search area size is MXM
 * Reference block size is NXN
 */

STORAGE_CLASS_REF_VECTOR_FUNC_C int generic_block_matching_algorithm(
	tscalar1w **search_window_ptr,
	tscalar1w **ref_block_ptr,
	tscalar1w *sads,
	int search_sz,
	int ref_sz,
	int pixel_shift,
	int search_block_sz,
	tscalar1w_4bit_bma_shift shift)
{
	int i, j;
	tscalar1w *search_window_block;
	int sad_num = 0;
	assert(search_sz > ref_sz);

	for (i = 0; (i + ref_sz) <= search_sz; i += pixel_shift) {
		for (j = 0; (j + ref_sz) <= search_sz; j += pixel_shift) {
			search_window_block = (tscalar1w *)search_window_ptr + (i*search_block_sz) + j; /* Initialize the ptr to search window row */
			sad((tscalar1w **)search_window_block, ref_block_ptr, &sads[sad_num], ref_sz, search_block_sz, shift);
			sad_num++;
		}
	}

	return 0;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C bma_output_16_1 OP_1w_asp_bma_16_1_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_16_1 output;
	generic_block_matching_algorithm((tscalar1w **)(&(search_area.search)),
		(tscalar1w **)&(input_block.ref), (tscalar1w *)&(output.sads), BMA_SEARCH_BLOCK_SZ_16,
		BMA_REF_BLOCK_SZ_8,PIXEL_SHIFT_1, BMA_SEARCH_BLOCK_SZ_16, shift);

	return output;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C bma_output_16_2 OP_1w_asp_bma_16_2_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_16_2 output;

	generic_block_matching_algorithm((tscalar1w **)(&(search_area.search)),
		(tscalar1w **)(&(input_block.ref)), (tscalar1w *)(&(output.sads)), BMA_SEARCH_BLOCK_SZ_16,
		BMA_REF_BLOCK_SZ_8,PIXEL_SHIFT_2, BMA_SEARCH_BLOCK_SZ_16, shift);

	return output;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C bma_output_14_1 OP_1w_asp_bma_14_1_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_14_1 output;

	generic_block_matching_algorithm((tscalar1w **)(&(search_area.search)),
		(tscalar1w **)(&(input_block.ref)), (tscalar1w *)(&(output.sads)), BMA_SEARCH_WIN_SZ_14,
		BMA_REF_BLOCK_SZ_8, PIXEL_SHIFT_1, BMA_SEARCH_BLOCK_SZ_16, shift);

	return output;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C bma_output_14_2 OP_1w_asp_bma_14_2_32way(
	bma_16x16_search_window search_area,
	ref_block_8x8 input_block,
	tscalar1w_4bit_bma_shift shift)
{
	bma_output_14_2 output;
	generic_block_matching_algorithm((tscalar1w **)&(search_area.search),
		(tscalar1w **)&(input_block.ref), (tscalar1w *) &(output.sads), BMA_SEARCH_WIN_SZ_14,
		BMA_REF_BLOCK_SZ_8, PIXEL_SHIFT_2, BMA_SEARCH_BLOCK_SZ_16, shift);

	return output;
}

#ifdef HAS_bfa_unit
STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_extract(tvector1w data, int msb_index, int lsb_index)
{
	/* Extract bits from msb to lsb index */
	tvector1w mask = OP_int_cast_to_1w((MAX_ELEM(msb_index - lsb_index + 1)));
	return OP_1w_and(OP_1w_lsr(data, OP_int_cast_to_1w(lsb_index)), mask);
}

 /* This function computes range weight
  * Normalized pixel absolute difference and range weight LUT is used as input
  * */
STORAGE_CLASS_REF_VECTOR_FUNC_C unsigned char bbb_bfa_calc_range_weight(
	tvector1w * range_weight_lut, tvector1w value)
{
	tvector1w bin_index;
	tvector1w bin_frac;

	assert(NUM_BITS >= 16);

	/* range_weight_lut is piecewise linear approximation of distribution
	 * function, every entry contains slope and offset. For adaptive range
	 * extension, integer and fractional parts are determined by taking
	 * data from different bit position
	 * bin_index: nearest higher x[N+1] value, for whichy[N] and y[N+1] are known
	 * bin_frac: x[N] - x */
	if (OP_1w_lt(value, BFA_RW_LUT_THRESHOLD)) {
		bin_index = OP_1w_extract(value, BFA_RW_LUT0_IDX_END_BIT, BFA_RW_LUT0_IDX_START_BIT);
		bin_frac = OP_1w_extract(value, BFA_RW_LUT0_FRAC_END_BIT, BFA_RW_LUT0_FRAC_START_BIT);
	} else {
		bin_index = OP_1w_extract(value, BFA_RW_LUT1_IDX_END_BIT, BFA_RW_LUT1_IDX_START_BIT);
		bin_index = OP_1w_add(bin_index, BFA_RW_LUT1_IDX_OFFSET);
		bin_frac = OP_1w_extract(value, BFA_RW_LUT1_FRAC_END_BIT, BFA_RW_LUT1_FRAC_START_BIT);
	}

	/* validate data width - to prevent out of boundary read */
	assert(isValidMpudata(bin_frac, BFA_RW_FRAC_BIT_CNT));
	assert(isValidMpudata(bin_index, BFA_RW_IDX_BIT_CNT));

	/* range_weight_lut is compressed as:
	 * entry-N = (range_weight[N+1] - range_weight[N] : MSB;
	 * range_weight[N]: LSB */
	tvector1w rw_lut_entry = range_weight_lut[bin_index];
	tvector1w slope = OP_1w_and(OP_1w_lsr(rw_lut_entry, BFA_RW_SLOPE_BIT_POS), BFA_RW_MASK);
	tvector1w start_value = OP_1w_and(rw_lut_entry, BFA_RW_MASK);
	/* Linear interpolation:
	 * y = y[N] -(x[N] - x)*(y[N+1] - y[N]); x[N+1] - x[N] = 1 */
	tvector1w drop = OP_1w_lsr(OP_1w_mul(bin_frac, slope), BFA_RW_SLOPE_BIT_SHIFT);
	assert(isValidMpudata(drop, BFA_RW_BIT_CNT));
	tvector1w rw = OP_1w_sub(start_value, drop);
	assert(isValidMpudata(rw, BFA_RW_BIT_CNT));
	return rw;
}

STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w bbb_bfa_get_scaled_subabs(
	tvector1w src, tvector1w center_pixel, tvector1w threshold)
{
	tvector1w cp_mask, subabs_out, scaled_subabs;

	assert(NUM_BITS >= 16);

	/* Discard 7lsbs, central pixel is s0.8 and src is in s0.15 */
	cp_mask = OP_int_cast_to_1w(BFA_CP_MASK);
	subabs_out = OP_1w_subabssat(src, OP_1w_and(center_pixel, cp_mask));
	/* Discard 6lsbs, and saturate to 9bits unsigned values */
	scaled_subabs = OP_1w_lsr(subabs_out, BFA_SUBABS_SHIFT);
	scaled_subabs = OP_1w_clipz(scaled_subabs, BFA_SUBABS_MAX);
	/* Pixel difference is multiplied with threshold( noise scale) before
	 * range weight table lookup */
	scaled_subabs = OP_1w_mul(scaled_subabs, threshold);
	return scaled_subabs;
}

/* Single bilateral function, scalar model of HW asp_bfa_s_12_16_32way
 * instruction with variable filter size */
STORAGE_CLASS_REF_VECTOR_FUNC_C void bbb_bfa_single(
	unsigned int filter_size, tvector1w *src_plane, tvector1w center_pixel,
	tvector1w *range_weight_lut, tvector1w *spatial_weights,
	tvector1w threshold, tvector2w *p_sop, tvector1w *p_sow)
{
	tvector2w sop = 0; /* sum(pix * spatial weight * range weight) */
	tvector1w sow = 0; /* sum(spatial weight * range weight) */
	unsigned int i;

	assert(NUM_BITS >= 16);

	assert(isValidMpudata(threshold, BFA_THRESHOLD_BIT_CNT));

	for (i = 0; i < filter_size; ++i) {
		tvector1w scaled_subabs = bbb_bfa_get_scaled_subabs(src_plane[i], center_pixel, threshold);
		scaled_subabs = OP_1w_clipz(scaled_subabs, BFA_SUBABSSAT_MAX);

		tvector1w range_weight = bbb_bfa_calc_range_weight(range_weight_lut, scaled_subabs);
		tvector1w spatial_weight = spatial_weights[i];
		assert(isValidMpudata(spatial_weight, BFA_SW_BIT_CNT));

		/* Weights are normalized by factor 1/64 to prevent possible
		 * overflow */
		tvector1w weight = OP_1w_lsr(OP_1w_muld(range_weight, spatial_weight), BFA_WEIGHT_SHIFT);
		tvector2w weighted_pixel = OP_1w_muld(weight, src_plane[i]);
		sop = OP_2w_add(sop, weighted_pixel);
		sow = OP_1w_add(sow, weight);
	}
	*p_sop = sop;
	*p_sow = sow;
}

/* Joint bilateral function, scalar model of HW asp_bfa_cj_12_16_32way
 * instruction with variable filter size */
STORAGE_CLASS_REF_VECTOR_FUNC_C void bbb_bfa_joint(
	unsigned int filter_size, tvector1w *src0_plane, tvector1w center_pixel0,
	tvector1w *src1_plane, tvector1w center_pixel1,
	tvector1w *range_weight_lut, tvector1w *spatial_weights,
	tvector1w threshold0, tvector1w threshold1, tvector2w *p_sop, tvector1w *p_sow)
{
	tvector2w sop = 0; /* sum(pix * spatial weight * range weight) */
	tvector1w sow = 0; /* sum(spatial weight * range weight) */
	unsigned int i;

	assert(NUM_BITS >= 16);
	assert(isValidMpudata(threshold0, BFA_THRESHOLD_BIT_CNT));
	assert(isValidMpudata(threshold1, BFA_THRESHOLD_BIT_CNT));

	for (i = 0; i < filter_size; ++i) {
		tvector1w scaled_subabs0, scaled_subabs1, scaled_subabs;
		scaled_subabs0 = bbb_bfa_get_scaled_subabs(src0_plane[i], center_pixel0, threshold0);
		scaled_subabs1 = bbb_bfa_get_scaled_subabs(src1_plane[i], center_pixel1, threshold1);
		scaled_subabs = OP_1w_add(scaled_subabs0, scaled_subabs1);
		scaled_subabs = OP_1w_clipz(scaled_subabs, BFA_SUBABSSAT_MAX);

		tvector1w range_weight = bbb_bfa_calc_range_weight(range_weight_lut, scaled_subabs);
		tvector1w spatial_weight = spatial_weights[i];
		assert(isValidMpudata(spatial_weight, BFA_SW_BIT_CNT));

		/* Weights are normalized by factor 1/64 to prevent possible
		 * overflow */
		tvector1w weight = OP_1w_lsr(OP_1w_muld(range_weight, spatial_weight), BFA_WEIGHT_SHIFT);
		tvector2w weighted_pixel = OP_1w_muld(weight, src0_plane[i]);
		sop = OP_2w_add(sop, weighted_pixel);
		sow = OP_1w_add(sow, weight);
	}
	*p_sop = sop;
	*p_sow = sow;
}
/* Copy BFA_MAX_KWAY (= 61) pixels from s_1w_9x9_matrix ( = 81 pixels) */
STORAGE_CLASS_REF_VECTOR_FUNC_C void bbb_bfa_gen_reduced9x9_buffer(s_1w_9x9_matrix in, tvector1w *p_src)
{
	/* HW does not yet support full 9x9 BFA.
	 * Only 61 points are supported at max.
	 * Here is a validity map for selecting 61 pixels out of 81 */
	const bool bfa_pix_valid[9][9] = {
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0},
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0},
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0},
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0},
		{ 0, 1, 1, 1, 1, 1, 1, 1, 0},
		{ 0, 0, 0, 1, 1, 1, 0, 0, 0} };
	int r, c, id = 0;
	tvector1w *in_ptr = &in.v00;
	for (r = 0; r < 9; r++) {
		for (c = 0; c < 9; c++) {
			if (bfa_pix_valid[r][c]) {
				assert(id <= BFA_MAX_KWAY);
				p_src[id] = in_ptr[c];
				id++;
			}
		}
		in_ptr += sizeof(s_1w_1x9_matrix)/sizeof(tvector1w);
	}
}

/** @brief bbb_bfa_gen_spatial_weight_lut
 *
 * @param[in] in - 9x9 matrix of spatial weights
 * @param[in] out - pointer to generated LUT
 *
 * @return   None
 *
 * This function implements, creates spatial weight look up table used
 * for bilaterl filter instruction.
 */
STORAGE_CLASS_REF_VECTOR_FUNC_C void bbb_bfa_gen_spatial_weight_lut(
	s_1w_9x9_matrix in,
	tvector1w out[BFA_MAX_KWAY])
{
	int i;
	bbb_bfa_gen_reduced9x9_buffer(in, &out[0]);
	for (i = 0; i < BFA_MAX_KWAY; i++) {
		out[i] &= BFA_SW_MASK;
	}
}

/** @brief bbb_bfa_gen_range_weight_lut
 *
 * @param[in] in - input range weight,
 * @param[in] out - generated LUT
 *
 * @return   None
 *
 * This function implements, creates range weight look up table used
 * for bilaterl filter instruction.
 * 8 unsigned 7b weights are represented in 7 16bits LUT
 * LUT formation is done as follows:
 * higher 8 bit: Point(N) = Point(N+1) - Point(N)
 * lower 8 bit: Point(N) = Point(N)
 * Weight function can be any monotonic decreasing function for x >= 0
 */
STORAGE_CLASS_REF_VECTOR_FUNC_C void bbb_bfa_gen_range_weight_lut(
	tvector1w in[BFA_RW_LUT_SIZE+1],
	tvector1w out[BFA_RW_LUT_SIZE])
{
	int i = 0;
	tvector1w range_weight, range_weight_nxt, slope;
	range_weight = in[i] & BFA_RW_MASK;
	for (i = 0; i < BFA_RW_LUT_SIZE; i++) {
		range_weight_nxt = in[i + 1] & BFA_RW_MASK;
		assert(range_weight_nxt <= range_weight);
		slope = (range_weight - range_weight_nxt) & BFA_RW_MASK;
		out[i] = ((slope<<BFA_RW_SLOPE_BIT_POS)|range_weight);
		range_weight = range_weight_nxt;
	}
}


/** @brief OP_1w_single_bfa_9x9_reduced
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
STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_single_bfa_9x9_reduced(
	bfa_weights weights,
	tvector1w threshold,
	tvector1w central_pix,
	s_1w_9x9_matrix src_plane)
{
	tvector2w sop;
	tvector1w sow;
	tvector1w src_buf[BFA_MAX_KWAY];

	bbb_bfa_gen_reduced9x9_buffer(src_plane, src_buf);
	bbb_bfa_single(BFA_MAX_KWAY, src_buf, central_pix,
		       weights.range_weight_lut, weights.spatial_weight_lut,
		       threshold, &sop, &sow);
	return OP_2w_divh(sop, sow);
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
 * Output = sum(pixel * weight) / sum(weight)
 * Where sum is summation over reduced 9x9 block set. Reduced because few
 * corner pixels are not taken.
 * weight = spatial weight * range weight
 * spatial weights are loaded from spatial_weight_lut depending on src pixel
 * position in the 9x9 block
 * range weights are computed by table look up from range_weight_lut depending
 * on sum of scaled absolute difference between central pixel and two src pixel
 * planes. threshold is used as scaling factor. range_weight_lut consists of
 * BFA_RW_LUT_SIZE numbers of LUT entries to model any distribution function.
 * Piecewise linear approximation technique is used to compute range weight
 * It computes absolute difference between central pixel and 61 src pixels.
 */
STORAGE_CLASS_REF_VECTOR_FUNC_C tvector1w OP_1w_joint_bfa_9x9_reduced(
	bfa_weights weights,
	tvector1w threshold0,
	tvector1w central_pix0,
	s_1w_9x9_matrix src0_plane,
	tvector1w threshold1,
	tvector1w central_pix1,
	s_1w_9x9_matrix src1_plane)
{
	tvector2w sop;
	tvector1w sow;
	tvector1w src0_buf[BFA_MAX_KWAY], src1_buf[BFA_MAX_KWAY];

	bbb_bfa_gen_reduced9x9_buffer(src0_plane, src0_buf);
	bbb_bfa_gen_reduced9x9_buffer(src1_plane, src1_buf);
	bbb_bfa_joint(BFA_MAX_KWAY,
		      src0_buf, central_pix0,
		      src1_buf, central_pix1,
		      weights.range_weight_lut, weights.spatial_weight_lut,
		      threshold0, threshold1,
		      &sop, &sow);
	return OP_2w_divh(sop, sow);
}
#endif
#endif /* VECTOR_FUNC_INLINED */
