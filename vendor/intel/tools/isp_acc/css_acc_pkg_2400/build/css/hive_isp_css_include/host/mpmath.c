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

 /* self */
#include "mpmath.h"

#include "isp_config.h" /* for rounding mode */

/*
 * Reference implementation for Multi Precision (MP) functions at
 * present the reference assumes that the MP word can be represented
 * in an C99 64-bit integer and that the (implementations) of the
 * C99 operations are trusted.
 *
 * Future improvements can consider the use of a pre-validated MP
 * library under this interface.
 *
 * The shift functions assume the common, but not C99 standard
 * behaviour that right shifts are signed.
 */


/*
 * Implementation limits
 */
#define MIN_SIGNED_VAL          ((mpsdata_t)1<<(MAX_BITDEPTH-1))
#define MAX_SIGNED_VAL          (~MIN_SIGNED_VAL)
#define MIN_UNSIGNED_VAL        ((mpsdata_t)0)
#define MAX_UNSIGNED_VAL        (~MIN_UNSIGNED_VAL)
#define ONE                     1
#define ZERO                    0

/*
 * Application limits
 */
#define MP_MIN_BITDEPTH            2
#define MP_MAX_BITDEPTH            16

#if ((MAX_BITDEPTH&1)!=0)
#error "MAX_BITDEPTH must be even"
#endif


#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>

/* when inlined the C file is empty */
#ifndef MPMATH_INLINED

#define STORAGE_CLASS_MPMATH_FUNC_LOCAL_C STORAGE_CLASS_INLINE
#define STORAGE_CLASS_MPMATH_DATA_LOCAL_C STORAGE_CLASS_INLINE_DATA

/*
 * Private method declarations
 */
STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t inAlignMpsdata(
    const mpsdata_t             data,
    const bitdepth_t            bitdepth);

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpudata_t inAlignMpudata(
    const mpudata_t             data,
    const bitdepth_t            bitdepth);

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t outAlignMpsdata(
    const mpsdata_t             data,
    const bitdepth_t            bitdepth);

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpudata_t outAlignMpudata(
    const mpudata_t             data,
    const bitdepth_t            bitdepth);

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpudata_t mp_csub (
    const mpudata_t             UL_var1,
    const mpudata_t             UL_var2,
    mpudata_t                  *out_state);

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t qdiv (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth,
    const bitdepth_t            outbitdepth,
    const bool                  dorounding);

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t intdiv (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth,
    const bitdepth_t            outbitdepth,
    const bool                  dorounding);

/*
 * Reguires aligned inputs
 */
STORAGE_CLASS_MPMATH_FUNC_LOCAL_C bool signMpsdata(
    const mpsdata_t             data);

/*
* Unsigned Integer square root
Following examples show the behaviour of unsigned integer square root

1) mp_sqrt_u(0x400)  = 32
2) mp_sqrt_u(0x9000) = 192

*/
STORAGE_CLASS_MPMATH_FUNC_C mpudata_t mp_sqrt_u(
	const mpudata_t	in0,  /* input */
	const bitdepth_t	bitdepth /* input bit-precision */
	)
{
mpudata_t one = (mpudata_t)1 << ((MAX_BITDEPTH-1)&~1); /* highest power of four */
mpudata_t val = in0;
mpudata_t res = 0;

/* no need to align to MSB because we don't have saturation issues with sqrt */
assert(isValidMpudata(in0, bitdepth));

/* find highest power of 4 that fits the input. */
while (one > in0) {
	one >>= 2;
}

while (one != 0) {
	if (val >= res + one) {
		val -= res + one;
		res += one << 1;
	}
	res >>= 1;
	one >>= 2;
}

return res;
}

/*
 * Halving and doubling casts (shift variants)
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_castd (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
bitdepth_t  doublingbitdept = bitdepth<<1;
assert(doublingbitdept<=MAX_BITDEPTH);
/* Storage format is signed extended */
return in0;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_casth (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
bitdepth_t  halvingbitdept = bitdepth>>1;
mpsdata_t   out = mp_lsl(in0,halvingbitdept,bitdepth);
return out>>halvingbitdept;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_scasth (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
bitdepth_t  halvingbitdept = bitdepth>>1;
mpsdata_t   out = mp_asl(in0,halvingbitdept,bitdepth);
return out>>halvingbitdept;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qcastd (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
bitdepth_t  doublingbitdept = bitdepth<<1;
assert(doublingbitdept<=MAX_BITDEPTH);
return mp_asl(in0,bitdepth,doublingbitdept);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qcasth (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
bitdepth_t  halvingbitdept = bitdepth>>1;
assert((halvingbitdept<<1)==bitdepth);
return mp_asr(in0,halvingbitdept,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qrcasth (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
bitdepth_t  halvingbitdept = bitdepth>>1;
assert((halvingbitdept<<1)==bitdepth);
return mp_rasr(in0,halvingbitdept,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_abs (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
bool        s0 = signMpsdata(in0_loc);
mpsdata_t   out = in0_loc;
if (s0)
    out = ((in0_loc==MIN_SIGNED_VAL)?MAX_SIGNED_VAL:-in0_loc);

return outAlignMpsdata(out,bitdepth);
}

/*
 * Limiter
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_limit (
    const mpsdata_t             bnd_low,
    const mpsdata_t             in0,
    const mpsdata_t             bnd_high,
    const bitdepth_t            bitdepth)
{
mpsdata_t   out = mp_max(in0,bnd_low,bitdepth);

return mp_min(out,bnd_high,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_max (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = ((in0_loc<in1_loc)?in1_loc:in0_loc);

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_min (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = ((in0_loc>in1_loc)?in1_loc:in0_loc);

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_mux (
    const spudata_t             sel,
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = ((sel==0)?in0_loc:in1_loc);

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_rmux (
    const spudata_t             sel,
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
return mp_mux(!sel,in0,in1,bitdepth);
}

/*
 * Non-saturating (wrapping) addition
 *  - Valid for both signed and unsigned inputs
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_add (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = in0_loc + in1_loc;

return outAlignMpsdata(out,bitdepth);
}

/*
 * Non-saturating (wrapping) subtraction
 *  - Valid for both signed and unsigned inputs
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_sub (
    const mpsdata_t              in0,
    const mpsdata_t              in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = in0_loc - in1_loc;

return outAlignMpsdata(out,bitdepth);
}

/*
 * Saturating addition
 *  - Defined only for signed inputs, for unsigned
 *    bias the inputs
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_sadd (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
bool        s0 = signMpsdata(in0_loc);
bool        s1 = signMpsdata(in1_loc);
mpsdata_t   out = in0_loc + in1_loc;
bool        sout = signMpsdata(out);
/*
 * Can only overflow for equal signed inputs
 * and overflow only happened when the sign of
 * the sum is unequal to either one of the inputs
 */
if ( (s0==s1) && (sout!=s1) )
	{
	out = (mpsdata_t)(sout?MAX_SIGNED_VAL:MIN_SIGNED_VAL);
    }

return outAlignMpsdata(out, bitdepth);
}

/*
 * Saturating subtraction
 *  - Defined only for signed inputs, for unsigned
 *    bias the inputs
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_ssub (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
bool        s0 = signMpsdata(in0_loc);
bool        s1 = signMpsdata(in1_loc);
mpsdata_t   out = in0_loc - in1_loc;
bool        sout = signMpsdata(out);
/*
 * Can only overflow for differently signed inputs
 * and overflow only happened when the sign of
 * the sum equals the sign of the second input
 */
if ( (s0!=s1) && (sout==s1) )
	{
	out = (mpsdata_t)(sout?MAX_SIGNED_VAL:MIN_SIGNED_VAL);
    }

return outAlignMpsdata(out, bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_addasr1 (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
bool        s0 = signMpsdata(in0_loc);
bool        s1 = signMpsdata(in1_loc);
mpsdata_t   sum = in0_loc + in1_loc;
bool        sout= signMpsdata(sum);
mpsdata_t   out = sum>>1;
mpudata_t   out_frac = ((mpudata_t)out) << bitdepth;
bool        inc = false;
/*
 * Can only overflow for equal signed inputs
 * and overflow only happened when the sign of
 * the sum is unequal to either one of the inputs
 */
if ( (s0==s1) && (sout!=s1) )
	{
	out ^= MIN_SIGNED_VAL;
    }

#if (ROUNDMODE==ROUND_NEAREST)
    inc = (out_frac>=(mpudata_t)MIN_SIGNED_VAL);
#elif (ROUNDMODE==ROUND_NEAREST_EVEN)
    inc = ((out_frac>(mpudata_t)MIN_SIGNED_VAL) || ((out_frac==(mpudata_t)MIN_SIGNED_VAL) && (((out<<(bitdepth-1))&MIN_SIGNED_VAL)!=0)));
#else
#error "ROUNDMODE must be on of {ROUND_NEAREST,ROUND_NEAREST_EVEN}"
#endif
/* No overflow possible */
return outAlignMpsdata(out, bitdepth) + inc;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_subasr1 (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
bool        s0 = signMpsdata(in0_loc);
bool        s1 = signMpsdata(in1_loc);
mpsdata_t   sum = in0_loc - in1_loc;
bool        sout= signMpsdata(sum);
mpsdata_t   out = sum>>1;
mpudata_t   out_frac = ((mpudata_t)out) << bitdepth;
bool        inc = false;
/*
 * Can only overflow for differently signed inputs
 * and overflow only happened when the sign of
 * the sum equals the sign of the second input
 */
if ( (s0!=s1) && (sout==s1) )
	{
	out ^= MIN_SIGNED_VAL;
    }

#if (ROUNDMODE==ROUND_NEAREST)
    inc = (out_frac>=(mpudata_t)MIN_SIGNED_VAL);
#elif (ROUNDMODE==ROUND_NEAREST_EVEN)
    inc = ((out_frac>(mpudata_t)MIN_SIGNED_VAL) || ((out_frac==(mpudata_t)MIN_SIGNED_VAL) && (((out<<(bitdepth-1))&MIN_SIGNED_VAL)!=0)));
#else
#error "ROUNDMODE must be on of {ROUND_NEAREST,ROUND_NEAREST_EVEN}"
#endif
/* No overflow possible */
return outAlignMpsdata(out, bitdepth) + inc;
}

/*
 * Logical shift right
 *  - Defined only for unsigned inputs
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_lsr (
    const mpsdata_t             in0,
    const spsdata_t             shft,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   out;
/*
 * Handle all exceptional shift values
 */
if (shft<(mpsdata_t)0)
	{
	spsdata_t	nshft = ((shft<(-MAX_BITDEPTH))?MAX_BITDEPTH:(-shft));
	out = mp_lsl(in0, nshft,bitdepth);
	}
else if (shft>(MAX_BITDEPTH-1))
	out = 0;
else
	out = (mpsdata_t)(((mpudata_t)in0_loc)>>shft);

return outAlignMpsdata(out, bitdepth);
}

/*
 * Arithmetic shift right
 *  - Truncates, sign extends
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_asr (
    const mpsdata_t             in0,
    const spsdata_t             shft,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   out;
/*
 * Handle all exceptional shift values
 */
if (shft<0)
	{
	spsdata_t	nshft = ((shft<(-MAX_BITDEPTH))?MAX_BITDEPTH:(-shft));
	out = mp_asl(in0, nshft, bitdepth);
	}
else if (shft>(MAX_BITDEPTH-1))
	out = (signMpsdata(in0_loc)?MAX_UNSIGNED_VAL:MIN_UNSIGNED_VAL);
else
	out = (in0_loc >> shft);

return outAlignMpsdata(out, bitdepth);
}

/*
 * Rounding arithmetic shift right
 *  - Rounds, sign extends
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_rasr (
    const mpsdata_t             in0,
    const spsdata_t             shft,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpudata_t   in0_frac;
mpsdata_t   out;
bool        inc = false;
/*
 * Handle all exceptional shift values
 */
if (shft<0)
	{
	spsdata_t	nshft = ((shft<(-MAX_BITDEPTH))?MAX_BITDEPTH:(-shft));
	out = mp_asl(in0, nshft, bitdepth);
	}
else if (shft>=bitdepth)
    {
/* For both round modes */
	out = MIN_UNSIGNED_VAL;
    }
else
    {
	out = in0_loc >> shft;
	in0_frac = in0_loc << (bitdepth-shft);

#if (ROUNDMODE==ROUND_NEAREST)
    inc = (in0_frac>=(mpudata_t)MIN_SIGNED_VAL);
#elif (ROUNDMODE==ROUND_NEAREST_EVEN)
    inc = ((in0_frac>(mpudata_t)MIN_SIGNED_VAL) || ((in0_frac==(mpudata_t)MIN_SIGNED_VAL) && (((out<<(bitdepth-1))&MIN_SIGNED_VAL)!=0)));
#else
#error "ROUNDMODE must be on of {ROUND_NEAREST,ROUND_NEAREST_EVEN}"
#endif
    }
/* No overflow possible */
return outAlignMpsdata(out, bitdepth) + inc;
}

/*
 * Rounding arithmetic shift right
 *  - Rounds, no sign extension
 */
STORAGE_CLASS_MPMATH_FUNC_C mpudata_t mp_rasr_u (
    const mpudata_t             in0,
    const spsdata_t             shft,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpudata(in0,bitdepth) ^ MIN_SIGNED_VAL;
mpsdata_t   tmp = mp_rasr(outAlignMpsdata(in0_loc,bitdepth),shft,bitdepth);
mpsdata_t   out = inAlignMpsdata(tmp,bitdepth) ^ MIN_SIGNED_VAL;

return outAlignMpudata(out, bitdepth);
}

/*
 * Logical shift left
 *  - Defined only for unsigned inputs
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_lsl (
    const mpsdata_t             in0,
    const spsdata_t             shft,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   out;
/*
 * Handle all exceptional shift values
 */
if (shft<0)
	{
	spsdata_t	nshft = ((shft<(-MAX_BITDEPTH))?MAX_BITDEPTH:(-shft));
	out = mp_lsr(in0, nshft,bitdepth);
	}
else if (shft>(MAX_BITDEPTH-1))
	out = 0;
else
	out = in0_loc<<shft;

return outAlignMpsdata(out, bitdepth);
}

/*
 * Arithmetic shift left
 *  - Saturates
 */
STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_asl (
    const mpsdata_t             in0,
    const spsdata_t             shft,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t	oflow = (signMpsdata(in0_loc)?MIN_SIGNED_VAL:MAX_SIGNED_VAL);
mpsdata_t   out;
/*
 * Handle all exceptional shift values
 */
if (shft<0)
	{
	spsdata_t	nshft = ((shft<(-MAX_BITDEPTH))?MAX_BITDEPTH:(-shft));
	out = mp_asr(in0, nshft,bitdepth);
	}
else if (shft>(MAX_BITDEPTH-1))
	{
	out = ((in0_loc==0)?(mpsdata_t)0:oflow);
	}
else
	{
    mpsdata_t   noflow= in0_loc<<shft;
	out = (((noflow>>shft)==in0_loc)?noflow:oflow);
	}

return outAlignMpsdata(out, bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_muld (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
bitdepth_t  doublingbitdept = bitdepth<<1;
/* For the moment do not make a reference doubling multiplier */
assert(bitdepth<=(MAX_BITDEPTH/2));
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth) >> (MAX_BITDEPTH/2);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth) >> (MAX_BITDEPTH/2);
mpsdata_t   out = in0_loc * in1_loc;

return outAlignMpsdata(out,doublingbitdept);
}
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_mul (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = (inAlignMpsdata(in1,bitdepth) >> (MAX_BITDEPTH - bitdepth));
mpsdata_t   out = in0_loc * in1_loc;

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qmul (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
	mpsdata_t   mul     = mp_muld(in0, in1, bitdepth);
	mpsdata_t   shifted = mp_asl(mul, 1, 2*bitdepth);
	mpsdata_t   out     = mp_qcasth(shifted, 2*bitdepth);

	return out;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qrmul (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
	mpsdata_t   mul     = mp_muld(in0, in1, bitdepth);
	mpsdata_t   shifted = mp_asl(mul, 1, 2*bitdepth);
	mpsdata_t   out     = mp_qrcasth(shifted, 2*bitdepth);

	return out;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qdiv (
	const mpsdata_t 						in0,
	const mpsdata_t 						in1,
	const bitdepth_t						bitdepth)
{
	return qdiv(in0, in1, bitdepth, bitdepth, false);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_qdivh (
	const mpsdata_t 						in0,
	const mpsdata_t 						in1,
	const bitdepth_t						bitdepth)
{
	return qdiv(in0, in1, bitdepth, bitdepth>>1, false);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_div (
	const mpsdata_t 						in0,
	const mpsdata_t 						in1,
	const bitdepth_t						bitdepth)
{
	return intdiv(in0, in1, bitdepth, bitdepth, false);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_divh (
	const mpsdata_t 						in0,
	const mpsdata_t 						in1,
	const bitdepth_t						bitdepth)
{
	mpsdata_t out = intdiv(in0, in1, bitdepth, bitdepth, false);
	return mp_scasth(out, bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_and (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = in0_loc & in1_loc;

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_compl (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   out = ~in0_loc;

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_or (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = in0_loc | in1_loc;

return outAlignMpsdata(out,bitdepth);
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_xor (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
mpsdata_t   out = in0_loc ^ in1_loc;

return outAlignMpsdata(out,bitdepth);
}


STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isEQ (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
spudata_t   out = (in0_loc==in1_loc);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isNE (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
spudata_t   out = (in0_loc!=in1_loc);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isGT (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
spudata_t   out = (in0_loc>in1_loc);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isGE (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
spudata_t   out = (in0_loc>=in1_loc);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isLT (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
spudata_t   out = (in0_loc<in1_loc);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isLE (
    const mpsdata_t             in0,
    const mpsdata_t             in1,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
mpsdata_t   in1_loc = inAlignMpsdata(in1,bitdepth);
spudata_t   out = (in0_loc<=in1_loc);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isEQZ (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
spudata_t   out = (in0_loc==ZERO);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isNEZ (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
spudata_t   out = (in0_loc!=ZERO);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isGTZ (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
spudata_t   out = (in0_loc>ZERO);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isGEZ (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
spudata_t   out = (in0_loc>=ZERO);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isLTZ (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
spudata_t   out = (in0_loc<ZERO);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C spudata_t mp_isLEZ (
    const mpsdata_t             in0,
    const bitdepth_t            bitdepth)
{
mpsdata_t   in0_loc = inAlignMpsdata(in0,bitdepth);
spudata_t   out = (in0_loc<=ZERO);

return out;
}

STORAGE_CLASS_MPMATH_FUNC_C mpsdata_t mp_const (
    const mp_const_ID_t         ID,
    const bitdepth_t            bitdepth)
{
mpsdata_t   out = ZERO;
switch (ID)
    {
    case mp_one_ID:
        out = ONE;
    break;
    case mp_mone_ID:
        out = -ONE;
    break;
    case mp_smin_ID:
        out = MIN_SIGNED_VAL;
    break;
    case mp_smax_ID:
        out = MAX_SIGNED_VAL;
    break;
    case mp_zero_ID: /* Fallthrough*/
    case mp_umin_ID:
        out = MIN_UNSIGNED_VAL;
    break;
    case mp_umax_ID:
        out = MAX_UNSIGNED_VAL;
    break;
    case N_mp_const_ID:
        assert(0);
    break;
    }

return outAlignMpsdata(out,bitdepth);;
}

/*
 * Private methods
 */
STORAGE_CLASS_MPMATH_FUNC_LOCAL_C bool signMpsdata(
    const mpsdata_t              data)
{
return (bool)(data>>(MAX_BITDEPTH-1));
}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t inAlignMpsdata(
    const mpsdata_t             data,
    const bitdepth_t            bitdepth)
{
assert(isValidMpsdata(data,bitdepth));
return (data<<(MAX_BITDEPTH-bitdepth));
}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpudata_t inAlignMpudata(
    const mpudata_t             data,
    const bitdepth_t            bitdepth)
{
assert(isValidMpudata(data,bitdepth));
return (data<<(MAX_BITDEPTH-bitdepth));
}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t outAlignMpsdata(
    const mpsdata_t             data,
    const bitdepth_t            bitdepth)
{
return (data>>(MAX_BITDEPTH-bitdepth));
}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpudata_t outAlignMpudata(
    const mpudata_t             data,
    const bitdepth_t            bitdepth)
{
return (data>>(MAX_BITDEPTH-bitdepth));
}

STORAGE_CLASS_MPMATH_FUNC_C bool isValidMpsdata(
    const mpsdata_t             data,
    const bitdepth_t            bitdepth)
{
bool    cnd = ((bitdepth>=MIN_BITDEPTH)&&(bitdepth<=MAX_BITDEPTH));
if (cnd)
    {
    mpsdata_t   minval = (~(mpudata_t)ZERO)<<(bitdepth-1);
    mpsdata_t   maxval = (~(mpudata_t)ZERO)>>(MAX_BITDEPTH-bitdepth+1);
    cnd = ((data>=minval)&&(data<=maxval));
    }
return cnd;
}

STORAGE_CLASS_MPMATH_FUNC_C bool isValidMpudata(
    const mpudata_t             data,
    const bitdepth_t            bitdepth)
{
bool    cnd = ((bitdepth>=MIN_BITDEPTH)&&(bitdepth<=MAX_BITDEPTH));
if (cnd)
    {
    mpudata_t minval = ZERO;
    mpudata_t maxval = (~(mpudata_t)ZERO)>>(MAX_BITDEPTH-bitdepth);
    cnd = ((data>=minval)&&(data<=maxval));
    }
return cnd;
}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpudata_t mp_csub (
	const mpudata_t		UL_var1,
	const mpudata_t		UL_var2,
	mpudata_t			*out_state)
{
	bool ov = (UL_var1>=MAX_SIGNED_VAL);
	mpudata_t UL_tmp = UL_var1<<1;

*out_state <<= 1;
/* Test on MSB prior to shifting only for full 32-bit division */
	if ((UL_tmp >= UL_var2) || ov)
	{
	UL_tmp -= UL_var2;
	*out_state |= (mpudata_t)0x00000001;
	}

	return UL_tmp;
}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t qdiv (
	const mpsdata_t 						in0,
	const mpsdata_t 						in1,
	const bitdepth_t						bitdepth,
	const bitdepth_t						outbitdepth,
	const bool      						dorounding)
{
	mpsdata_t	tmp;
	mpsdata_t L_sign = ((in0 < 0) ^ (in1 < 0));
	mpsdata_t in0_abs = inAlignMpsdata(mp_abs(in0, bitdepth), bitdepth);
	mpsdata_t in1_abs = inAlignMpsdata(mp_abs(in1, bitdepth), bitdepth);
#if DEBUG_MPMATH
printf("in0: %d, in1: %d \n", (int)in0, (int)in1);
printf("in0: %f, in1: %f \n", (float)in0, (float)in1);
printf("floatres frac: %f\n",((float)in0 * (1<<15))/ (float)(in1));
#endif
	if (in0_abs >= in1_abs)
	{
		tmp = ((in0_abs!=0)?MAX_SIGNED_VAL:0);
	}
	else
	{
		int i;
		mpudata_t UL_tmp = 0;
		for (i=0;i<outbitdepth-1;i++){
			in0_abs = mp_csub(in0_abs,in1_abs,&UL_tmp);
		}
		if (dorounding) {
			/* in case of rounding we need one extra iteration */
			in0_abs = mp_csub(in0_abs,in1_abs,&UL_tmp);
			UL_tmp = (UL_tmp+1)>>1;
		}
		tmp = (mpsdata_t)UL_tmp;
		/* realigning to MSB aligned */
		tmp = tmp << (MAX_BITDEPTH - outbitdepth);
	}

	tmp = ((L_sign==0)?tmp:-tmp);
#if DEBUG_MPMATH
	printf("out %d\n",outAlignMpsdata(tmp,outbitdepth));
#endif
	return outAlignMpsdata(tmp,outbitdepth);

}

STORAGE_CLASS_MPMATH_FUNC_LOCAL_C mpsdata_t intdiv (
	const mpsdata_t 						in0,
	const mpsdata_t 						in1,
	const bitdepth_t						bitdepth,
	const bitdepth_t						outbitdepth,
	const bool      						dorounding)
{
	mpsdata_t	tmp;
	mpsdata_t L_sign = ((in0 < 0) ^ (in1 < 0));
	mpsdata_t in0_abs = inAlignMpsdata(mp_abs(in0, bitdepth), bitdepth) >> (bitdepth-1);
	mpsdata_t in1_abs = inAlignMpsdata(mp_abs(in1, bitdepth), bitdepth);
#if DEBUG_MPMATH
printf("in0: %d, in1: %d \n", (int)in0, (int)in1);
printf("in0: %f, in1: %f \n", (float)in0, (float)in1);
printf("floatres int: %f\n",(float)in0 / (float)(in1));
#endif
	if (in0_abs >= in1_abs)
	{
		tmp = ((in0_abs!=0)?MAX_SIGNED_VAL:0);
	}
	else
	{
		int i;
		mpudata_t UL_tmp = 0;
		for (i=0;i<outbitdepth-1;i++){
			in0_abs = mp_csub(in0_abs,in1_abs,&UL_tmp);
		}
		if (dorounding) {
			/* in case of rounding we need one extra iteration */
			in0_abs = mp_csub(in0_abs,in1_abs,&UL_tmp);
			UL_tmp = (UL_tmp+1)>>1;
		}
		tmp = (mpsdata_t)UL_tmp;
		/* realigning to MSB aligned */
		tmp = tmp << (MAX_BITDEPTH - outbitdepth);
	}

	tmp = ((L_sign==0)?tmp:-tmp);
#if DEBUG_MPMATH
	printf("out %d\n",outAlignMpsdata(tmp,outbitdepth));
#endif
	return outAlignMpsdata(tmp,outbitdepth);
}

#endif /* MPMATH_INLINED */

