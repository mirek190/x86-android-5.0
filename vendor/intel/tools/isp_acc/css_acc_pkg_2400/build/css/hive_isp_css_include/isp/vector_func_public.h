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

#ifndef __VECTOR_FUNC_PUBLIC_H_INCLUDED__
#define __VECTOR_FUNC_PUBLIC_H_INCLUDED__

#include "matrix_3x3.isp.h"
#include "filters/filters_1.0/matrix_4x4.h"
#include "filters/filters_1.0/matrix_5x5.h"

/*
 *  realining multiply.
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector1w
OP_vec1w_mul_realigning(
	tvector1w _a,
	tvector1w _b,
	tscalar1w _c);

/*
 * Normalised FIR with coefficients [3,4,1], -5dB at Fs/2, -90 degree phase shift (quarter pixel)
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3m_5dB_m90_nrm (
	const s_1x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3_5dB_m90_nrm (
	const s_slice_vector	sv);

/*
 * Normalised FIR with coefficients [1,4,3], -5dB at Fs/2, +90 degree phase shift (quarter pixel)
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3m_5dB_p90_nrm (
	const s_1x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3_5dB_p90_nrm (
	const s_slice_vector	sv);

/*
 * Normalised FIR with coefficients [1,2,1], -6dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3m_6dB_nrm (
	const s_1x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3_6dB_nrm (
	const s_slice_vector	sv);

/*
 * Normalised FIR with coefficients [1,1,1], -9dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3m_9dB_nrm (
	const s_1x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3_9dB_nrm (
	const s_slice_vector	sv);

/*
 * FIR with coefficients [1,2,1], -6dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3_6dB (
	const s_slice_vector	sv);

/*
 * FIR with coefficients [1,1,1], -9dB at Fs/2
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir1x3_9dB (
	const s_slice_vector	sv);

/*
 * Normalised FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Unity gain filter through repeated scaling and rounding
 *	- 6 rotate operations per output
 *	- 8 vector operations per output
 * _______
 *   14
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir3x3m_6dB_nrm (
	const s_3x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector fir3x3_6dB_nrm (
	const s_3_slice_vector	sv);

/*
 * Normalised FIR with coefficients [1;1;1] * [1,1,1]
 *
 * (near) Unity gain filter through repeated scaling and rounding
 *	- 6 rotate operations per output
 *	- 8 vector operations per output
 * _______
 *   14
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir3x3m_9dB_nrm (
	const s_3x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector fir3x3_9dB_nrm (
	const s_3_slice_vector	sv);

/*
 * Normalised dual output FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Unity gain filter through repeated scaling and rounding
 * compute two outputs per call to re-use common intermediates
 *	- 4 rotate operations per output
 *	- 7 vector operations per output (alternative possible, but in this
 *	    form it's not obvious to re-use variables)
 * _______
 *   11
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector2x1 fir3x3m_6dB_out2x1_nrm (
	const s_4x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector2x1 fir3x3_6dB_out2x1_nrm (
	const s_4_slice_vector	sv);

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
STORAGE_CLASS_VECTOR_FUNC_H tvector2x1 fir3x3m_9dB_out2x1_nrm (
	const s_4x3_matrix		m);

STORAGE_CLASS_VECTOR_FUNC_H tvector2x1 fir3x3_9dB_out2x1_nrm (
	const s_4_slice_vector	sv);

/*
 * FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Filter with gain correction on output
 *	-  1 rotate operations per output
 *	-  6 vector operations per output
 * _______
 *     7
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir3x3_6dB (
	const s_3_slice_vector	sv);

/*
 * FIR with coefficients [1;1;1] * [1,1,1]
 *
 * Filter with gain correction on output
 *	-  1 rotate operations per output
 *	-  5 vector operations per output
 * _______
 *     6
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector fir3x3_9dB (
	const s_3_slice_vector	sv);


/*
 * Dual output FIR with coefficients [1;2;1] * [1,2,1]
 *
 * Filter with gain correction on output
 *	-  1   rotate operations per output
 *	-  5.5 vector operations per output
 * ________
 *     6.5
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector2x1 fir3x3_6dB_out2x1 (
	const s_4_slice_vector	sv);

/*
 * Dual output FIR with coefficients [1;1;1] * [1,1,1]
 *
 * Filter with gain correction on output
 *	-  1   rotate operations per output
 *	-  3.5 vector operations per output
 * ________
 *     4.5
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector2x1 fir3x3_9dB_out2x1 (
	const s_4_slice_vector	sv);

/*
 * Normalised FIR with coefficients [1;1;1] * [1;2;1] * [1,2,1] * [1,1,1]
 *
 * (near) Unity gain filter through repeated scaling and rounding
 *	- 20 rotate operations per output
 *	- 28 vector operations per output
 * _______
 *   48
 */
STORAGE_CLASS_VECTOR_FUNC_C tvector fir5x5_15dB_nrm (
	const s_5_slice_vector	sv);

STORAGE_CLASS_VECTOR_FUNC_C tvector fir1x5m_12dB_nrm (
	s_1x5_matrix m);

STORAGE_CLASS_VECTOR_FUNC_C tvector fir5x5m_12dB_nrm (
	s_5x5_matrix m);

/*
 * SAD between two matrices.
 * Both input pixels and SAD are normalized by a factor of SAD3x3_IN_SHIFT and
 * SAD3x3_OUT_SHIFT respectively.
 * Computed SAD is 1/(2 ^ (SAD3x3_IN_SHIFT + SAD3x3_OUT_SHIFT)) ie 1/16 factor
 * of original SAD.
*/
STORAGE_CLASS_VECTOR_FUNC_H tvector sad3x3m_precise(
	s_3x3_matrix a,
	s_3x3_matrix b);

/*
 * SAD between two matrices.
 * This version saves cycles by avoiding input normalization and wide vector
 * operation during sum computation
 * Input pixel differences are computed by absolute of rounded, halved
 * subtraction. Normalized sum is computed by rounded averages.
 * Computed SAD is (1/2)*(1/16) = 1/32 factor of original SAD. Factor 1/2 comes
 * from input halving operation and factor 1/16 comes from mean operation
*/
STORAGE_CLASS_VECTOR_FUNC_H tvector sad3x3m(
	s_3x3_matrix a,
	s_3x3_matrix b);

/*
 * SAD between two matrices.
 * Computed SAD is 1/64 factor of original SAD.
*/
STORAGE_CLASS_VECTOR_FUNC_H tvector sad5x5m(
	s_5x5_matrix a,
	s_5x5_matrix b);


/** @brief Bi-linear Interpolation optimized(approximate)
 *
 * @param[in] a input0
 * @param[in] b input1
 * @param[in] c cloned weight factor
  *
 * @return		(a-b)*c + b
 *
 * This function will do bi-linear Interpolation on inputs a and b
 * using constant weight factor c for all elements of the input vectors
 *
 * Inputs a,b are assumed in S1.15 format
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *
 * The bilinear interpolation equation is (a*c) + b*(1-c),
 * But this is implemented as (a-b)*c + b for optimization
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector1w OP_vec1w_bilinear_interpol_approx_c(
	tvector1w a,
	tvector1w b,
	int c);

/** @brief Bi-linear Interpolation optimized(approximate)
 *
 * @param[in] a input0
 * @param[in] b input1
 * @param[in] c weight factor
  *
 * @return		(a-b)*c + b
 *
 * This function will do bi-linear Interpolation on
 * inputs a and b using weight factor c
 *
 * Inputs a,b are assumed in S1.15 format
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *
 * The bilinear interpolation equation is (a*c) + b*(1-c),
 * But this is implemented as (a-b)*c + b for optimization
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector1w OP_vec1w_bilinear_interpol_approx(
	tvector1w a,
	tvector1w b,
	tvector1w c);


/** @brief Bi-linear Interpolation
 *
 * @param[in] a input0
 * @param[in] b input1
 * @param[in] c weight factor
  *
 * @return		(a*c) + b*(1-c)
 *
 * This function will do bi-linear Interpolation on
 * inputs a and b using weight factor c
 *
 * Inputs a,b are assumed in S1.15 format
 * Weight factor has to be in range [0,1] and is assumed to be in S2.14 format
 *
 * The bilinear interpolation equation is (a*c) + b*(1-c),
 */
STORAGE_CLASS_VECTOR_FUNC_H tvector1w OP_vec1w_bilinear_interpol(
	tvector1w a,
	tvector1w b,
	tscalar1w c);

#endif /* __VECTOR_FUNC_PUBLIC_H_INCLUDED__ */
