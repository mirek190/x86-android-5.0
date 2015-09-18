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

#ifndef __VECTOR_OPS_PRIVATE_H_INCLUDED__
#define __VECTOR_OPS_PRIVATE_H_INCLUDED__

#ifndef __INLINE_VECTOR_OPS__
#error "vector_ops_private.h: Inlining required"
#endif

#include "isp.h"
#include "vector_ops_public.h"
#include "assert_support.h"

/* vector copy */
#ifndef VC
#define VC(x) (x)
#endif

/* Work around for Broxton C0 SDK */
#if !HAS_vec_shuffle
#define OP_vec_shuffle OP_vec_shuffle_e
#endif

/* Broxton SDK doesn't supply OP_vec1w_clone */
#if !HAS_vec1w_clone  && HAS_vec_clone
#define OP_vec1w_clone OP_vec_clone
#endif

/* OP_vec[1/2]w_subabs does sat(abs(sub(a,b))), for consistent naming with bbb
 * reference lets rename it to OP_vec[1/2]w_subabssat */
#ifndef OP_vec1w_subabssat
#define OP_vec1w_subabssat      OP_vec1w_subabs
#endif
#ifndef OP_vec2w_subabssat
#define OP_vec2w_subabssat      OP_vec2w_subabs
#endif

/* vector conditional store */
/* This macro is same as existing macro CVSTORE. Once CVSTORE is moved to this
 * file this macro should be removed */
#define COND_VSTORE(a, v, c) ((a) = vector_mux((v), (a SPECULATED), c))


/* This is a work around for a problem in the SDK, it has to be fixed in every SDK
 * as soon as the return type of the OP_vec1w_muld() is fixed in the SDK
 * this code can be removed, and the calls to _OP_vec1w_muld() can be replaced.
 * isp2600_vec1w uses a different version of the tvector1w2 type as the isp2400 and isp2500
 */

#if defined(IS_ISP_2600_SYSTEM) || defined(IS_WDSP_SDK)
STORAGE_CLASS_VECTOR_OPS_C tvector2w
cast1w2to2w(
	tvector1w2 in)
{
	tvector2w out = {in.vec0, in.vec1};
	return out;
}

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_mul(
	tvector1w a,
	tvector1w b)
{
	tvector1w2 mul = OP_vec1w_muld(a, b);
	return mul.vec0;
}

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_mul_c(
	tvector1w a,
	tscalar1w b)
{
	tvector1w2 mul = OP_vec1w_muld_c(a, b);
	return mul.vec0;
}

#else
STORAGE_CLASS_VECTOR_OPS_C tvector2w
cast1w2to2w(
	tvector1w2 in)
{
	tvector2w out = {in.d0, in.d1};
	return out;
}

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_mul(
	tvector1w a,
	tvector1w b)
{
	tvector1w2 mul = OP_vec1w_muld(a, b);
	return mul.d0;
}

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_mul_c(
	tvector1w a,
	tscalar1w b)
{
	tvector1w2 mul = OP_vec1w_muld_c(a, b);
	return mul.d0;
}
#endif

STORAGE_CLASS_VECTOR_OPS_C tvector2w
_OP_vec1w_muld(
	tvector1w a,
	tvector1w b)
{
	return cast1w2to2w(OP_vec1w_muld(a, b));
}

STORAGE_CLASS_VECTOR_OPS_C tvector2w
_OP_vec1w_muld_c(
	tvector1w a,
	tscalar1w b)
{
	return cast1w2to2w(OP_vec1w_muld_c(a, b));
}

/* End of work around for muld */

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_min_c(
	tvector1w _a,
	tscalar1w _b)
{
	return OP_vec_min_c(_a, _b);
}

#if !HAS_vec_max && HAS_vec_maxlt_o2
#define HAS_vec_max 2
STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec_max(
	tvector1w _a,
	tvector1w _b)
{
	tvector1w max;
	tflags f;

	vec_maxlt_o2 (Any, _a, _b, max, f);
	return max;
}
#endif

#if !HAS_vec_max_c && HAS_vec_maxlt_co2
#define HAS_vec_max_c 2
STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec_max_c(
	tvector1w _a,
	tscalar1w _b)
{
	tvector1w max;
	tflags f;

	vec_maxlt_co2 (Any, _a, _b, max, f);
	return max;
}
#endif

#if !HAS_vec_min && HAS_vec_minle_o2
#define HAS_vec_min 2
STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec_min(
	tvector1w _a,
	tvector1w _b)
{
	tvector1w min;
	tflags f;

	vec_minle_o2 (Any, _a, _b, min, f);
	return min;
}
#endif

#if !HAS_vec_min_c && HAS_vec_minle_co2
#define HAS_vec_min_c 2
STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec_min_c(
	tvector1w _a,
	tscalar1w _b)
{
	tvector1w min;
	tflags f;

	vec_minle_co2 (Any, _a, _b, min, f);
	return min;
}
#endif

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_max_c(
	tvector1w _a,
	tscalar1w _b)
{
	return OP_vec_max_c(_a, _b);
}

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_subabssat_c(
	tvector1w _a,
	tscalar1w _b)
{
	return OP_vec1w_subabssat(_a, OP_vec1w_clone(_b));
}

/* single precsion division*/
#if !HAS_vec_div

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_div(
	tvector1w _a,
	tvector1w _b)
{
	tvector1w zeros = OP_vec1w_clone(0);
	tvector1w lsbmask = OP_vec1w_clone(1);
	tvector1w _p = zeros;
	tvector1w _c = zeros;
	tvector1w xor_a_b = OP_vec1w_xor(_a, _b);
	tflags sign = OP_vec1w_ge(xor_a_b, zeros);

	tvector1w tmp_p, a_sr;
	tflags cond;
	int i;

	_a = OP_vec1w_abs(_a);
	_b = OP_vec1w_abs(_b);

	for (i = ISP_VEC_ELEMBITS-2; i >= 0; i--) {

		a_sr = OP_vec1w_lsr_c(_a, i);
		a_sr = OP_vec1w_and(a_sr, lsbmask);
		_p = OP_vec1w_lsl_c(_p, 1);
		_p = OP_vec1w_or(_p, a_sr);

		tmp_p = OP_vec1w_sub(_p, _b);
		cond = OP_vec1w_ge(tmp_p, zeros);
		/*Remainder*/
		_p = OP_vec1w_mux(tmp_p, _p, cond);
		/*Quotient*/
		_c = OP_vec1w_lsl_c(_c, 1);
		_c = OP_vec1w_or(_c, OP_flag_to_vec_u(cond));
	}
	_c = OP_vec_mux(_c, OP_vec1w_neg(_c), sign);

	return _c;
}
#endif

/* Single precision unsigned integer square root */
STORAGE_CLASS_VECTOR_OPS_C tvector1wu
OP_vec1w_sqrt_u(
	tvector1wu _a)
{
#if HAS_vec_sqrt
	/* NB: Despite the name, OP_vec1w_sqrt(_a) actually implements an
	 *    unsigned SQRT */
	return OP_vec1w_sqrt(_a);
#else
	unsigned int pow_4;
	tflags val_ge_res_plus_pow4;
	tvector1wu res_pl_pow_4;
	tvector1wu val = _a;
	tvector1wu valnew, resnew;
	tvector1wu res = OP_vec_clone(0);
	tflags carry_out; /* dummy */

	/* Init with highest power of four */
	const unsigned int max_pow_4 = 1 << ((ISP_VEC_ELEMBITS-1) & ~1);

	for (pow_4 = max_pow_4; pow_4 > 0; pow_4 >>= 2) {

		/* Note: no overflow can occur, so both vec_add and vec_addsat are fine.
     *  vec_addsat is used because it has a _c version on 240x */
		res_pl_pow_4 = OP_vec_addsat_c(res, pow_4);

		val_ge_res_plus_pow4 = OP_vec_le_u(res_pl_pow_4, val);

		/* In first iteration, val can be >= MAX_INT, therefore vec_subsat cannot
     * be used */
		vec_add_cout(Any, val, OP_vec_neg(res_pl_pow_4), valnew, carry_out);

		val = OP_vec_mux(valnew, val, val_ge_res_plus_pow4);

		res = OP_vec_lsr_c(res, 1);

		/* Note: no overflow can occur, so both vec_add and vec_addsat are fine */
		resnew = OP_vec_addsat_c(res, pow_4);

		res = OP_vec_mux(resnew, res, val_ge_res_plus_pow4);
	}

	return res;
#endif
}


STORAGE_CLASS_VECTOR_OPS_C tvector2w
OP_vec2w_div(
	tvector2w _a,
	tvector2w _b)
{
	tscalar2w zero = {0, 0};
	tvector2w zeros = OP_vec2w_clone(zero);
	tvector2w lsbmask = {OP_vec1w_clone(1), OP_vec1w_clone(0)};
	tvector2w _p = zeros;
	tvector2w _c = zeros;
	tvector2w xor_a_b = OP_vec2w_xor(_a, _b);
	tflags sign = OP_vec2w_ge(xor_a_b, zeros);
	tvector2w a_sr, tmp_p;
	tflags cond;
	int i;

	_a = OP_vec2w_abs(_a);
	_b = OP_vec2w_abs(_b);

	for (i = (ISP_VEC_ELEMBITS << 1)-2; i >= 0; i--) {

		a_sr = OP_vec2w_lsr_c(_a, i);
		a_sr = OP_vec2w_and(a_sr, lsbmask);
		_p = OP_vec2w_lsl_c(_p, 1);
		_p = OP_vec2w_or(_p, a_sr);

		tmp_p = OP_vec2w_sub(_p, _b);
		cond = OP_vec2w_ge(tmp_p, zeros);
		/*Remainder*/
		_p = OP_vec2w_mux(tmp_p, _p, cond);
		/*Quotient*/
		_c = OP_vec2w_lsl_c(_c, 1);
#if defined(IS_ISP_2600_SYSTEM) || defined(IS_WDSP_SDK)
		_c.vec0 = OP_vec1w_or(_c.vec0, OP_flag_to_vec_u(cond));
#else
		_c.d0 = OP_vec1w_or(_c.d0, OP_flag_to_vec_u(cond));
#endif
	}
	_c = OP_vec2w_mux(_c, OP_vec2w_neg(_c), sign);
	return _c;
}

/* double precision shift right with halving cast and saturation. */
STORAGE_CLASS_VECTOR_OPS_C tvector1w OP_vec_2w_asrrndh_c(
	tvector2w in,
	tscalar1w shift)
{
/* the ISP2600 system has a different definition of the tvector2w register struct
   This function should be moved to the API of the core such that we don't need
   this kind of #if switches in the code. */
#if defined(IS_ISP_2600_SYSTEM) || defined(IS_WDSP_SDK)
	return OP_vec_asrrnd_wsplit_c (in.vec1, in.vec0, shift);
#else
	return OP_vec_asrrnd_wsplit_c (in.d1, in.d0, shift);
#endif
}


/*Saturate & cast double precision to single precision*/
STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec2w_sat_cast_to_vec1w(tvector2w _a)
{
	return OP_vec_2w_asrrndh_c(_a, 0);
}

/*Halving Division*/
STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec2w_divh(
	tvector2w _a,
	tvector1w _b)
{
#if HAS_vec_div_wsplit
/* the ISP2600 system has a different definition of the tvector2w register struct
   This function should be moved to the API of the core such that we don't need
   this kind of #if switches in the code. */
#if defined(IS_ISP_2600_SYSTEM) || defined(IS_WDSP_SDK)
	tvector1w out = OP_vec_div_wsplit(_a.vec1, _a.vec0, _b);
	return out;
#else
	tvector1w out = OP_vec_div_wsplit(_a.d1, _a.d0, _b);
	return out;
#endif
#else
	tvector2w twide_b = {_b, OP_vec_sgnext(CASTVEC1W(_b))};
	tvector2w _c = OP_vec2w_div(_a, twide_b);
	return OP_vec2w_sat_cast_to_vec1w(_c);
#endif
}

STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec2w_sqrt_u(
	tvector2wu _a)
{
#if HAS_vec_sqrt_wsplit
	tvector1wu out = OP_vec_sqrt_wsplit(_a.vec1, _a.vec0);
	return out;
#else
	(void)_a;
	OP___assert(0);
	return OP_vec_clone(0);
#endif
}


STORAGE_CLASS_VECTOR_OPS_C tvector1w
OP_vec1w_qdiv(
	tvector1w _a,
	tvector1w _b)
{
	tvector2w a = {OP_vec1w_clone(0), _a};
	tvector2w in = OP_vec2w_asr_c(a,1);
	tvector1w out = OP_vec2w_divh(in, _b);
	return out;
}

STORAGE_CLASS_VECTOR_OPS_C tvector1x2 OP_vec_minmax (
		const tvector       _a,
		const tvector       _b)
{
#if !HAS_vec_minmax_o2
	tvector		vmin = OP_vec_min(CASTVEC1W(_a), CASTVEC1W(_b));
	tvector		vmax = OP_vec_max(CASTVEC1W(_a), CASTVEC1W(_b));
	tvector1x2	out = {vmin, vmax};
#else
	tvector1x2	out;
	vec_minmax_o2(Any, CASTVEC1W(_a), CASTVEC1W(_b), out.v0, out.v1);
#endif
	return out;
}

#if HAS_vec_even || HAS_vec_deint2_o2 || HAS_vec_deinterleaved_o2
STORAGE_CLASS_VECTOR_OPS_C tvector1x2 OP_vec_evenodd (
		const tvector       _a,
		const tvector       _b)
{
#if (!HAS_vec_deint2_o2 && !HAS_vec_deinterleaved_o2 && !HAS_vec_evenodd_o2)
	tvector		veven = OP_vec_even(CASTVEC1W(_a), CASTVEC1W(_b));
	tvector		vodd = OP_vec_odd(CASTVEC1W(_a), CASTVEC1W(_b));
	tvector1x2	out = {veven, vodd};
#else
#if HAS_vec_deint2_o2
	tvector1x2	out;
	vec_deint2_o2(Any, CASTVEC1W(_a), CASTVEC1W(_b), out.v0, out.v1);
#else
#if HAS_vec_deinterleaved_o2
	tvector1x2	out;
	vec_deinterleaved_o2(Any, CASTVEC1W(_a), CASTVEC1W(_b), out.v0, out.v1);
#else
	tvector1x2	out;
	vec_evenodd_o2(Any, CASTVEC1W(_a), CASTVEC1W(_b), out.v0, out.v1);
#endif
#endif
#endif
	return out;
}
#endif

#if HAS_vec_mergel || HAS_vec_mergelh_o2
STORAGE_CLASS_VECTOR_OPS_C tvector1x2 OP_vec_mergelh (
		const tvector       _a,
		const tvector       _b)
{
#if !HAS_vec_mergelh_o2
	tvector		vlow = OP_vec_mergel(CASTVEC1W(_a), CASTVEC1W(_b));
	tvector		vhigh = OP_vec_mergeh(CASTVEC1W(_a), CASTVEC1W(_b));
	tvector1x2	out = {vlow, vhigh};
#else
	tvector1x2	out;
	vec_mergelh_o2(Any, CASTVEC1W(_a), CASTVEC1W(_b), out.v0, out.v1);
#endif
	return out;
}
#endif

/*
 * From the multi-precision lib (that should replace the "widevector")
 *
 typedef tvectorslice		tslice1w;
 */

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_slvec_sliceavgrnd(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1 )
{
#if HAS_slvec_sliceavgrnd
	s_slice_vector  out;
	slvec_sliceavgrnd (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), 1, CASTVECSLICE1W(s1), CASTVEC1W(v1), 0, out.s, out.v);
#else
	s_slice_vector out;
	out.v=OP_vec_avgrnd(OP_vec_slice(CASTVECSLICE1W(s0),CASTVEC1W(v0), -1),CASTVEC1W(v1));
	out.s=CASTVECSLICE1W(s1); /*Dummy operation*/
#endif
	return out;
}

/*
 * Legacy isp_vecsl_... functions. They are wrappers around the OP_vecsl...
 * functions defined later in this file.
 */


STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_add (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_addsat
	return OP_vecsl_addsat(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_sub (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_subsat
	return OP_vecsl_subsat(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_subabs (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_subabs
	return OP_vecsl_subabs(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_addnextsat (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_addnextsat
	return OP_vecsl_addnextsat(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_nextsubsat (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_nextsubsat
	return OP_vecsl_nextsubsat(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_subnextsat (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_subnextsat
	return OP_vecsl_subnextsat(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_nextsubabs (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_nextsubabs
	return OP_vecsl_nextsubabs(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector
isp_vecsl_subnextabs (s_slice_vector a, s_slice_vector b)
{
#if HAS_vecsl_subnextabs
	return OP_vecsl_subnextabs(a.s, a.v, b.s, b.v);
#else
	OP___assert(0);
	return a; /* unreachable; keep compiler happy */
#endif
}


STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_addsat(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
#if HAS_vecsl_addsat
	s_slice_vector  out;
	vecsl_addsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
#else
	s_slice_vector  out;
	slvec_addsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
#endif
	return out;
}

#if HAS_vecsl_addnextsat
STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_addnextsat(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
	s_slice_vector  out;
	vecsl_addnextsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
	return out;
}

#else
STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_slvec_sliceaddsat(
		const tslice1w		s0,
		const tvector1w		v0,
		const int               offset0,
		const tslice1w		s1,
		const tvector1w		v1,
		const int               offset1)
{
	s_slice_vector  out;
	slvec_sliceaddsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), offset0, CASTVECSLICE1W(s1), CASTVEC1W(v1), offset1, out.s, out.v);
	return out;
}
#endif

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_subabs(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
#if HAS_vecsl_subabs
	s_slice_vector  out;
	vecsl_subabs (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
#else
	s_slice_vector  out;
	slvec_subabssat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
#endif
	return out;
}

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_subsat(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
#if HAS_vecsl_subsat
	s_slice_vector  out;
	vecsl_subsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
#else
	s_slice_vector  out;
	slvec_subsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
#endif
	return out;
}

#if HAS_vecsl_subnextabs
STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_subnextabs(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
	s_slice_vector  out;
	vecsl_subnextabs (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
	return out;
}
#endif

#if HAS_vecsl_subnextsat
STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_subnextsat(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
	s_slice_vector  out;
	vecsl_subnextsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
	return out;
}
#endif

#if HAS_vecsl_nextsubabs
STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_nextsubabs(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
	s_slice_vector  out;
	vecsl_nextsubabs (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
	return out;
}
#endif

#if HAS_vecsl_nextsubsat
STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vecsl_nextsubsat(
		const tslice1w		s0,
		const tvector1w		v0,
		const tslice1w		s1,
		const tvector1w		v1)
{
	s_slice_vector  out;
	vecsl_nextsubsat (Any, CASTVECSLICE1W(s0), CASTVEC1W(v0), CASTVECSLICE1W(s1), CASTVEC1W(v1), out.s, out.v);
	return out;
}

#endif

STORAGE_CLASS_VECTOR_OPS_C tvector1w OP_vec_lsrrnd_wsplit_cu_rndout(
		const tvector2w		v
		)
{
	tvector1w  out;
#if defined(IS_ISP_2600_SYSTEM) || defined(IS_WDSP_SDK)
	int rnd_out;
	vec_lsrrnd_wsplit_cu_rndout(Any,CASTVEC1W(v.vec1), CASTVEC1W(v.vec0),(ISP_VEC_ELEMBITS<<1)-1,out,rnd_out);
#else
	tflags rnd_out;
	vec_lsrrnd_wsplit_cu_rndout(Any,CASTVEC1W(v.d1), CASTVEC1W(v.d0),(ISP_VEC_ELEMBITS<<1)-1,out,rnd_out);
#endif
	return out;
}

/*
 * Note: twidevector != tvector2w
 */
STORAGE_CLASS_VECTOR_OPS_C twidevector OP_vec_mul_c(
		const tvector1w		v0,
		const short			c1)
{
	twidevector out;
	vec_mul_c(Any, CASTVEC1W(v0), c1, out.hi, out.lo);
	return out;
}

#if !HAS_vec_mux_csel
STORAGE_CLASS_VECTOR_OPS_C twidevector OP_widevec_mux_csel(
		const twidevector	v0,
		const twidevector	v1,
		const int			sel)
{
	tflags	vsel = OP_vec_neq_c (OP_vec_clone(sel), 0);
	twidevector out = { OP_vec_mux (CASTVEC1W(v0.hi), CASTVEC1W(v1.hi), vsel), OP_vec_mux (CASTVEC1W(v0.lo), CASTVEC1W(v1.lo), vsel) };
	return out;
}
#else
STORAGE_CLASS_VECTOR_OPS_C twidevector OP_widevec_mux_csel(
		const twidevector	v0,
		const twidevector	v1,
		const int		sel)
{
	twidevector out = { OP_vec_mux_csel (CASTVEC1W(v0.hi), CASTVEC1W(v1.hi), sel), OP_vec_mux_csel (CASTVEC1W(v0.lo), CASTVEC1W(v1.lo), sel) };
	return out;
}
#endif

#if !HAS_vec_slice
#define HAS_vec_slice 2
STORAGE_CLASS_VECTOR_OPS_C tvector1w OP_vec_slice(
		const tslice1w		s,
		const tvector1w		v,
		const int		idx)
{
	tvector1w out;
	out = OP_slvec_slice(CASTVECSLICE1W(s), CASTVEC1W(v), -idx);
	return out;
}
#endif

STORAGE_CLASS_VECTOR_OPS_C s_slice_vector OP_vec_slvec_select_high_pair(
		const tslice1w		s,
		const tvector1w		v,
		const int		idx)
{
	s_slice_vector out;
#if HAS_vec_slice_select_high
	vec_slice_select_high (Any, CASTVECSLICE1W(s), CASTVEC1W(v), idx, out.v, out.s);
#else
	slvec_slice_select_high (Any, CASTVECSLICE1W(s), CASTVEC1W(v), -idx, out.v, out.s);
#endif
	return out;
}

STORAGE_CLASS_VECTOR_OPS_C unsigned
OP_flag_get(
  tflags flag,
  unsigned id)
{
	tvector v = OP_flag_to_vec_u(flag);
	return OP_vec_get(v, id);
}

#if HAS_vec_asrrnd_wsplit_c_rndin
STORAGE_CLASS_VECTOR_OPS_C tvector vec_asr_no_rnd (twidevector w, unsigned i)
{
#if ISP_NWAY <= 32
  tflags cin = 0x0;
#else
  tflags cin = OP_flag_clone(0);
#endif
  return OP_vec_asrrnd_wsplit_c_rndin(w.hi, w.lo, i, cin);
}
#endif

#if HAS_vec_lsrrnd_wsplit_cu_rndin
STORAGE_CLASS_VECTOR_OPS_C tvector vec_lsr_no_rnd (twidevector w, unsigned i)
{
#if ISP_NWAY <= 32
  tflags cin = 0x0;
#else
  tflags cin = OP_flag_clone(0);
#endif
  return OP_vec_lsrrnd_wsplit_cu_rndin(w.hi, w.lo, i, cin);
}
#endif

#if HAS_vec_abs && !HAS_vec_abssat
#define HAS_vec_abssat 2
STORAGE_CLASS_VECTOR_OPS_C tvector OP_vec_abssat (tvector v)
{
  return OP_vec_abs(v);
}
#endif

#if HAS_vec_mux_c && !HAS_vec_rmux_c
#define HAS_vec_rmux_c 2
STORAGE_CLASS_VECTOR_OPS_C tvector OP_vec_rmux_c (tvector v, unsigned c, tflags f)
{
  return OP_vec_mux (OP_vec_clone(c), v, f);
}
#endif

#if !HAS_vec_gt_u
#define HAS_vec_gt_u 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_gt_u (tvectoru a, tvectoru b)
{
  return OP_vec_lt_u (b, a);
}
#endif

#if !HAS_vec_ge_u
#define HAS_vec_ge_u 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_ge_u (tvectoru a, tvectoru b)
{
  return OP_vec_le_u (b, a);
}
#endif

#if !HAS_vec_gt_cu
#define HAS_vec_gt_cu 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_gt_cu (tvectoru a, int b)
{
  return OP_vec_lt_u (OP_vec_clone(b), a);
}
#endif

#if !HAS_vec_ge_cu
#define HAS_vec_ge_cu 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_ge_cu (tvectoru a, int b)
{
  return OP_vec_le_u (OP_vec_clone(b), a);
}
#endif

#if !HAS_vec_gt && !defined(__isp2500_skycam) /* Skycam declares this without setting HAS */
#define HAS_vec_gt 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_gt (tvectors a, tvectors b)
{
  return OP_vec_lt (b, a);
}
#endif

#if !HAS_vec_ge
#define HAS_vec_ge 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_ge (tvectors a, tvectors b)
{
  //return OP_vec_le (b, a);
  return OP_vec_lt (b-1, a);
}
#endif

#if !HAS_vec_gt_c && !defined(__isp2500_skycam) /* Skycam declares this without setting HAS */
#define HAS_vec_gt_c 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_gt_c (tvectors a, int b)
{
  return OP_vec_lt (OP_vec_clone(b), a);
}
#endif

#if !HAS_vec_ge_c
#define HAS_vec_ge_c 2
STORAGE_CLASS_VECTOR_OPS_C tflags OP_vec_ge_c (tvectors a, int b)
{
  //return OP_vec_le (OP_vec_clone(b), a);
  return OP_vec_lt (OP_vec_clone(b-1), a);
}
#endif

STORAGE_CLASS_VECTOR_OPS_C twidevector isp_vec_mul_ecu(tvectoru a, unsigned b)
{
  twidevector r;
  vec_mul_cu(Any, a, (unsigned short)b, r.hi, r.lo);
  return r;
}

#endif /* __VECTOR_OPS_PRIVATE_H_INCLUDED__ */
