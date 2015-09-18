/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */



#include "isp_op1w.h"
#include "isp_config.h"
#include <assert_support.h>
#include <stdio.h>
#include "isp_op_count.h"

/* when inlined the C file is empty because it is already included inside the H file */
#ifndef ISP_OP1W_INLINED

/* Arithmetic */

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_and(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_and);
	return mp_and(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_or(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_or);
	return mp_or(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_xor(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_xor);
	return mp_xor(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_inv(
    const tvector1w     _a)
{
	inc_bbb_count(bbb_func_OP_1w_inv);
	return mp_compl(_a, NUM_BITS*1);
}

/* Additive */
STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_add(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	inc_bbb_count(bbb_func_OP_1w_add);
	inc_core_count_n(core_func_OP_add, 1);
	out = mp_add(_a, _b, NUM_BITS*1);
	return out;
}

#ifdef HAS_vec_sub
/* The operation count depends on the availability of a subtraction operation in the core */
#define OP_1W_SUB_OP_CNT 1
#else
#if defined(ISP2400) || defined(ISP2401)
#define OP_1W_SUB_OP_CNT 2 /* Emulated with vec_inv + vec_add_cincout */
#else
#define OP_1W_SUB_OP_CNT 0 /* Operation count unknown */
#endif
#endif
STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_sub(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	inc_bbb_count_ext(bbb_func_OP_1w_sub, OP_1W_SUB_OP_CNT);
	out = mp_sub(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_addsat(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	inc_bbb_count(bbb_func_OP_1w_addsat);
	out = mp_sadd(_a, _b, NUM_BITS*1);
	return out;
}


STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_subsat(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	inc_bbb_count(bbb_func_OP_1w_subsat);
	out = mp_ssub(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_subasr1(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_subasr1);
	return mp_subasr1(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_abs(
    const tvector1w     _a)
{
	inc_bbb_count(bbb_func_OP_1w_abs);
	return mp_abs(_a, NUM_BITS*1);
}


STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_subabssat(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_subabssat);
	return mp_abs(mp_ssub(_a, _b, NUM_BITS*1), NUM_BITS*1);
}


/* Multiplicative */

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector2w OP_1w_muld(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector2w out;
	inc_bbb_count(bbb_func_OP_1w_muld);
	out = mp_muld(_a, _b, NUM_BITS*1); // out is bitdepth x 2
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_mul(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_mul);
	return mp_mul(_a, _b, NUM_BITS*1);
}

#define OP_1W_QMUL_CNT 2 /* Emulated with 1w_muld() + vec_asrsat_wsplit() */
STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_qmul(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_1w_qmul, OP_1W_QMUL_CNT);
	return mp_qmul(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_qrmul(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_qrmul);
	return mp_qrmul(_a, _b, NUM_BITS*1);
}

/* Comparative */

STORAGE_CLASS_ISP_OP1W_FUNC_C tflags OP_1w_eq(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tflags out;
	inc_bbb_count(bbb_func_OP_1w_eq);
	out = mp_isEQ(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tflags OP_1w_ne(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tflags out;
	inc_bbb_count(bbb_func_OP_1w_ne);
	out = mp_isNE(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tflags OP_1w_le(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tflags out;
	inc_bbb_count(bbb_func_OP_1w_le);
	out = mp_isLE(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tflags OP_1w_lt(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tflags out;
	inc_bbb_count(bbb_func_OP_1w_lt);
	out = mp_isLT(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tflags OP_1w_ge(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tflags out;
	inc_bbb_count(bbb_func_OP_1w_ge);
	out = mp_isGE(_a, _b, NUM_BITS*1);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tflags OP_1w_gt(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tflags out;
	inc_bbb_count(bbb_func_OP_1w_gt);
	out = mp_isGT(_a, _b, NUM_BITS*1);
	return out;
}

/* Shift */

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_asr(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_1W);
	inc_bbb_count(bbb_func_OP_1w_asr);
	out = mp_asr(_a, b, NUM_BITS);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_asrrnd(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_1W);
	inc_bbb_count(bbb_func_OP_1w_asrrnd);
	out = mp_rasr(_a, b, NUM_BITS);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_asl(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_1W);
	inc_bbb_count(bbb_func_OP_1w_asl);
	out = mp_asl(_a, b, NUM_BITS);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_aslsat(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_1W);
	inc_bbb_count(bbb_func_OP_1w_aslsat);
	out = mp_asl(_a, b, NUM_BITS);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_lsl(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_1W);
	inc_bbb_count(bbb_func_OP_1w_lsl);
	out = mp_lsl(_a, b, NUM_BITS);
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_lsr(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_1W);
	inc_bbb_count(bbb_func_OP_1w_lsr);
	out = mp_lsr(_a, b, NUM_BITS);
	return out;
}

/* Cast */

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_int_cast_to_1w(
    const int           _a)
{
	tvector1w out = _a;
	mpsdata_t b = _a;
	inc_bbb_count(bbb_func_OP_int_cast_to_1w);
	assert(isValidMpsdata(b, NUM_BITS));
	return out;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C int OP_1w_cast_to_int(
    const tvector1w      _a)
{
	inc_bbb_count(bbb_func_OP_1w_cast_to_int);
	return (int)_a;
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector2w OP_1w_cast_to_2w(
    const tvector1w     _a)
{
	// doubling
	inc_bbb_count(bbb_func_OP_1w_cast_to_2w);
	return mp_castd(_a, NUM_BITS);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_2w_cast_to_1w(
    const tvector2w    _a)
{
	// halving
	inc_bbb_count(bbb_func_OP_2w_cast_to_1w);
	return mp_casth(_a, NUM_BITS*2);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_2w_sat_cast_to_1w(
    const tvector2w    _a)
{
	// halving with saturation
	inc_bbb_count(bbb_func_OP_2w_sat_cast_to_1w);
	return mp_scasth(_a, NUM_BITS*2);
}

/* clipping */

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_clip_asym(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_clip_asym);
	assert(_b>=0);
	return mp_limit(~_b, _a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_clipz(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_clipz);
	assert(_b>=0);
	return mp_limit(0, _a, _b, NUM_BITS*1);
}

/* division */
#ifdef HAS_div_unit
/* The operation count depends on the availability of a division unit in the core */
#define OP_1W_DIV_OP_CNT 1
#else
#ifdef ISP2400
#define OP_1W_DIV_OP_CNT 133 /* Emulated */
#else
#define OP_1W_DIV_OP_CNT 0
#endif
#endif

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_div(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_1w_div, OP_1W_DIV_OP_CNT);
	return mp_div(_a, _b, NUM_BITS*1);
}

#ifdef HAS_div_unit
/* The operation count depends on the availability of a division unit in the core */
#define OP_1W_QDIV_OP_CNT 1
#else
#define OP_1W_QDIV_OP_CNT 0
#endif

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_qdiv(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_1w_qdiv, OP_1W_QDIV_OP_CNT);
	return mp_qdiv(_a, _b, NUM_BITS*1);
}

#ifdef HAS_div_unit
/* The operation count depends on the availability of a division unit in the core */
#define OP_1W_MOD_OP_CNT 1
#else
#define OP_1W_MOD_OP_CNT 0
#endif

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_mod(
    const tvector1w     _a,
    const tvector1w     _b)
{
	tvector1w quotient;
	tvector1w temp;
	tvector1w res;
	inc_bbb_count_ext(bbb_func_OP_1w_mod, OP_1W_MOD_OP_CNT);
	quotient = OP_1w_div(_a,_b);
	temp     = OP_1w_mul(_b, quotient);
	res      = OP_1w_sub(_a, temp);
	return res;
}

/* Unsigned Integer square root */
#ifdef HAS_1w_sqrt_u_unit
/* The operation count depends on the availability of a division unit in the core */
#define OP_1W_SQRT_U_OP_CNT 1
#else
#define OP_1W_SQRT_U_OP_CNT 0
#endif
STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w_unsigned OP_1w_sqrt_u(
	const tvector1w_unsigned     _a)
{
	tvector1w_unsigned sq_out;

	inc_bbb_count_ext(bbb_func_OP_1w_sqrt_u, OP_1W_SQRT_U_OP_CNT);
	/* Square root */
	sq_out = mp_sqrt_u(_a, NUM_BITS*1);
	return sq_out;
}

/* Miscellaneous */

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_mux(
    const tvector1w     _a,
    const tvector1w     _b,
    const tflags           _c)
{
	inc_bbb_count(bbb_func_OP_1w_mux);
/*To Match with ISP implementation, the argument order is reversed
  from (_c, _a, _b) to (_c, _b, _a)*/
	return mp_mux(_c, _b, _a, NUM_BITS*1);
}


STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_avgrnd(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_avgrnd);
	return mp_addasr1(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_min(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_min);
	return mp_min(_a, _b, NUM_BITS*1);
}

STORAGE_CLASS_ISP_OP1W_FUNC_C tvector1w OP_1w_max(
    const tvector1w     _a,
    const tvector1w     _b)
{
	inc_bbb_count(bbb_func_OP_1w_max);
	return mp_max(_a, _b, NUM_BITS*1);
}

#endif /* ISP_OP1W_INLINED */
