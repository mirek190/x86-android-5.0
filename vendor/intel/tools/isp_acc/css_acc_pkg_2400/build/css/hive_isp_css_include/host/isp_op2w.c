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



#include "isp_op2w.h"
#include "isp_config.h"
#include "isp_op_count.h"
#include <assert_support.h>

/* when inlined the C file is empty because it is already included inside the H file */
#ifndef ISP_OP2W_INLINED

/* Arithmetic */

#define OP_2W_AND_OP_CNT 2

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_and(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_and, OP_2W_AND_OP_CNT);
	return mp_and(_a, _b, NUM_BITS*2);
}

#define OP_2W_OR_OP_CNT 2

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_or(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_or, OP_2W_OR_OP_CNT);
	return mp_or(_a, _b, NUM_BITS*2);
}

#define OP_2W_XOR_OP_CNT 2

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_xor(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_xor, OP_2W_XOR_OP_CNT);
	return mp_xor(_a, _b, NUM_BITS*2);
}

#define OP_2W_INV_OP_CNT 2

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_inv(
    const tvector2w     _a)
{
	inc_bbb_count_ext(bbb_func_OP_2w_inv, OP_2W_INV_OP_CNT);
	return mp_compl(_a, NUM_BITS*2);
}

/* Additive */

#define OP_2W_ADD_OP_CNT 2

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_add(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	inc_bbb_count_ext(bbb_func_OP_2w_add, OP_2W_ADD_OP_CNT);
	out = mp_add(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_SUB_OP_CNT 4

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_sub(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	inc_bbb_count_ext(bbb_func_OP_2w_sub, OP_2W_SUB_OP_CNT);
	out = mp_sub(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_ADDSAT_OP_CNT 4

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_addsat(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	inc_bbb_count_ext(bbb_func_OP_2w_addsat, OP_2W_ADDSAT_OP_CNT);
	out = mp_sadd(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_SUBSAT_OP_CNT 6

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_subsat(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	inc_bbb_count_ext(bbb_func_OP_2w_subsat, OP_2W_SUBSAT_OP_CNT);
	out = mp_ssub(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_SUBASR1_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_subasr1(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_subasr1, OP_2W_SUBASR1_OP_CNT);
	return mp_subasr1(_a, _b, NUM_BITS*2);
}

#define OP_2W_ABS_OP_CNT 10

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_abs(
    const tvector2w     _a)
{
	inc_bbb_count_ext(bbb_func_OP_2w_abs, OP_2W_ABS_OP_CNT);
	return mp_abs(_a, NUM_BITS*2);
}

#define OP_2W_SUBABSSAT_OP_CNT 16

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_subabssat(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_subabssat, OP_2W_SUBABSSAT_OP_CNT);
	return mp_abs(mp_ssub(_a, _b, NUM_BITS*2), NUM_BITS*2);
}


/* Multiplicative */

#define OP_2W_MUL_OP_CNT 5

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_mul(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_mul, OP_2W_MUL_OP_CNT);
	return mp_mul(_a, _b, NUM_BITS*2);
}

#define OP_2W_QMUL_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_qmul(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_qmul, OP_2W_QMUL_OP_CNT);
	return mp_qmul(_a, _b, NUM_BITS*2);
}

#define OP_2W_QRMUL_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_qrmul(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_qrmul, OP_2W_QRMUL_OP_CNT);
	return mp_qrmul(_a, _b, NUM_BITS*2);
}

/* Comparative */

#define OP_2W_EQ_OP_CNT 3

STORAGE_CLASS_ISP_OP2W_FUNC_C tflags OP_2w_eq(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tflags out;
	inc_bbb_count_ext(bbb_func_OP_2w_eq, OP_2W_EQ_OP_CNT);
	out = mp_isEQ(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_NE_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tflags OP_2w_ne(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tflags out;
	inc_bbb_count_ext(bbb_func_OP_2w_ne, OP_2W_NE_OP_CNT);
	out = mp_isNE(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_LE_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tflags OP_2w_le(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tflags out;
	inc_bbb_count_ext(bbb_func_OP_2w_le, OP_2W_LE_OP_CNT);
	out = mp_isLE(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_LT_OP_CNT 5

STORAGE_CLASS_ISP_OP2W_FUNC_C tflags OP_2w_lt(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tflags out;
	inc_bbb_count_ext(bbb_func_OP_2w_lt, OP_2W_LT_OP_CNT);
	out = mp_isLT(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_GE_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tflags OP_2w_ge(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tflags out;
	inc_bbb_count_ext(bbb_func_OP_2w_ge, OP_2W_GE_OP_CNT);
	out = mp_isGE(_a, _b, NUM_BITS*2);
	return out;
}

#define OP_2W_GT_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tflags OP_2w_gt(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tflags out;
	inc_bbb_count_ext(bbb_func_OP_2w_gt, OP_2W_GT_OP_CNT);
	out = mp_isGT(_a, _b, NUM_BITS*2);
	return out;
}

/* Shift */

#define OP_2W_ASR_OP_CNT 6

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_asr(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_2W);
	inc_bbb_count_ext(bbb_func_OP_2w_asr, OP_2W_ASR_OP_CNT);
	out = mp_asr(_a, b, NUM_BITS*2);
	return out;
}

#define OP_2W_ASRRND_OP_CNT 4

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_asrrnd(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_2W);
	inc_bbb_count_ext(bbb_func_OP_2w_asrrnd, OP_2W_ASRRND_OP_CNT);
	out = mp_rasr(_a, b, NUM_BITS*2);
	return out;
}

#define OP_2W_ASL_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_asl(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_2W);
	inc_bbb_count_ext(bbb_func_OP_2w_asl, OP_2W_ASL_OP_CNT);
	out = mp_asl(_a, b, NUM_BITS*2);
	return out;
}

#define OP_2W_ASLSAT_OP_CNT 8

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_aslsat(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_2W);
	inc_bbb_count_ext(bbb_func_OP_2w_aslsat, OP_2W_ASLSAT_OP_CNT);
	out = mp_asl(_a, b, NUM_BITS*2);
	return out;
}

#define OP_2W_LSL_OP_CNT 5

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_lsl(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_2W);
	inc_bbb_count_ext(bbb_func_OP_2w_lsl, OP_2W_LSL_OP_CNT);
	out = mp_lsl(_a, b, NUM_BITS*2);
	return out;
}

#define OP_2W_LSR_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_lsr(
    const tvector2w     _a,
    const tvector2w     _b)
{
	tvector2w out;
	mpsdata_t b = _b;
	assert(_b >= 0);
	assert(_b <= MAX_SHIFT_2W);
	inc_bbb_count_ext(bbb_func_OP_2w_lsr, OP_2W_LSR_OP_CNT);
	out = mp_lsr(_a, b, NUM_BITS*2);
	return out;
}

/* clipping */

#define OP_2W_CLIP_ASYM_OP_CNT 17

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_clip_asym(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_clip_asym, OP_2W_CLIP_ASYM_OP_CNT);
	assert(_b >= 0);
	return mp_limit(~_b, _a, _b, NUM_BITS*2);
}

#define OP_2W_CLIPZ_OP_CNT 11

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_clipz(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_clipz, OP_2W_CLIPZ_OP_CNT);
	assert(_b >= 0);
	return mp_limit(0, _a, _b, NUM_BITS*2);
}

/* division */

#ifdef HAS_div_unit
/* The operation count depends on the availability of a 1w or 2w division unit in the core */
#define OP_2W_DIV_OP_CNT 0
#else
#ifdef ISP2400
#define OP_2W_DIV_OP_CNT 736	/* Emulation */
#else
#define OP_2W_DIV_OP_CNT 0
#endif
#endif

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_div(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_div, OP_2W_DIV_OP_CNT);
	return mp_div(_a, _b, NUM_BITS*2);
}

#ifdef HAS_div_unit
/* The operation count depends on the availability of a 1w or 2w division unit in the core */
#define OP_2W_DIVH_OP_CNT 0
#else
#ifdef ISP2400
#define OP_2W_DIVH_OP_CNT 740	/* Emulation */
#else
#define OP_2W_DIVH_OP_CNT 0
#endif
#endif

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector1w OP_2w_divh(
    const tvector2w     _a,
    const tvector1w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_divh, OP_2W_DIVH_OP_CNT);
	return mp_divh(_a, _b, NUM_BITS*2);
}

#define OP_2W_MOD_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_mod(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_mod, OP_2W_MOD_OP_CNT);
	/* not yet implemented*/
	assert(0); 
	(void) _a;
	(void) _b;
	return 0;
}

#ifdef HAS_2w_sqrt_u_unit
/* The operation count depends on the availability of a 2w unsigned square root unit in the core */
#define OP_2W_SQRT_U_OP_CNT 1
#else
#define OP_2W_SQRT_U_OP_CNT 0
#endif

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector1w_unsigned OP_2w_sqrt_u(
	const tvector2w_unsigned     _a)
{
	tvector1w_unsigned sq_out;

	inc_bbb_count_ext(bbb_func_OP_2w_sqrt_u, OP_2W_SQRT_U_OP_CNT);

	/* Square root */
	sq_out = mp_sqrt_u(_a, NUM_BITS*2);
	return sq_out;
}

/* Miscellaneous */

#define OP_2W_MUX_OP_CNT 2

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_mux(
    const tvector2w     _a,
    const tvector2w     _b,
    const tflags           _c)
{
/*To Match with ISP implementation, the argument order is reversed
  from (_c, _a, _b) to (_c, _b, _a)*/
	inc_bbb_count_ext(bbb_func_OP_2w_mux, OP_2W_MUX_OP_CNT);
	return mp_mux(_c, _b, _a, NUM_BITS*2);
}

#define OP_2W_AVGRND_OP_CNT 0

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_avgrnd(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_avgrnd, OP_2W_AVGRND_OP_CNT);
	return mp_addasr1(_a, _b, NUM_BITS*2);
}

#define OP_2W_MIN_OP_CNT 5

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_min(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_min, OP_2W_MIN_OP_CNT);
	return mp_min(_a, _b, NUM_BITS*2);
}

#define OP_2W_MAX_OP_CNT 5

STORAGE_CLASS_ISP_OP2W_FUNC_C tvector2w OP_2w_max(
    const tvector2w     _a,
    const tvector2w     _b)
{
	inc_bbb_count_ext(bbb_func_OP_2w_max, OP_2W_MAX_OP_CNT);
	return mp_max(_a, _b, NUM_BITS*2);
}

#endif /* ISP_OP2W_INLINED */

