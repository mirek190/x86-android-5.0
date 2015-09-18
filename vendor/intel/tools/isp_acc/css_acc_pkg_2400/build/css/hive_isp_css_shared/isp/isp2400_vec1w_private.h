/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2014 Intel Corporation.
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

#ifndef __ISP2400_VEC1W_PRIVATE_H_INCLUDED__
#define __ISP2400_VEC1W_PRIVATE_H_INCLUDED__

/*
 * This file is part of the Multi-precision vector operations exstension package of the HiveFlex isp2400 series.
 * The accompanying manaul describes the data-types, multi-precision operations and multi-precision overloading possibilities.
 */
 
/* 
 * Single-precision vector operations
 * Version 0.2
 */
 
/*  
 * Prerequisites:
 *  - Overloading is enabled for CRUN simulation through enabling HIVE_OVERLOADING=1
 *  - The standard instruction set of the HiveFlex isp2400
 *
 */
 
 
/* The vector native types definition */ 
#include "isp2400_vecNw_native_types.h"

/* C_RUN support casts */
#ifdef __HIVECC
#   define CASTVEC1W(x)         x
#   define CASTVECSLICE1W(x)    x
#   define CASTFLG(x)           x
#else
#   define CASTVEC1W(x)         tvector(x)
#   define CASTVECSLICE1W(x)    tvectorslice(x)
#   define CASTFLG(x)           tflags(x)
#endif





/*
 * Single-precision data type specification
 */

typedef tvector             tvector1w;
typedef mp_fragment_t       tscalar1w;
typedef tvectorslice        tslice1w;

typedef tvector             mvector1w;
typedef mp_fragment_t       mscalar1w;
typedef tvectorslice        mslice1w;

typedef __register struct {
  tvector       d0;
  tvector       d1;
} tvector2w;

typedef __register struct {
  tvector       d;
  tflags        f;
} tvector1w_tflags;

typedef __register struct {
  tvector       d;
  tvectorslice  s;
} tvector1w_tslice1w;





/*
 * Single-precision prototype specification
 */

/* Arithmetic */

/* Use standard operations */
#define   OP_vec1w_and      OP_vec_and
#define   OP_vec1w_and_c    OP_vec_and_c    
#define   OP_vec1w_or       OP_vec_or
#define   OP_vec1w_or_c     OP_vec_or_c
#define   OP_vec1w_xor      OP_vec_xor
#define   OP_vec1w_xor_c    OP_vec_xor_c
#define   OP_vec1w_inv      OP_vec_inv


/* Additive */

static inline tvector1w OP_vec1w_add(
    const tvector1w     _a,
    const tvector1w     _b);

static inline tvector1w OP_vec1w_add_c(
    const tvector1w     _a,
    const tscalar1w     _b);

static inline tvector1w OP_vec1w_sub(
    const tvector1w     _a,
    const tvector1w     _b);

static inline tvector1w OP_vec1w_sub_c(
    const tvector1w     _a,
    const tscalar1w     _b);
    
#define   OP_vec1w_addsat   OP_vec_addsat  
#define   OP_vec1w_addsat_c OP_vec_addsat_c
#define   OP_vec1w_subsat   OP_vec_subsat  
#define   OP_vec1w_subsat_c OP_vec_subsat_c  


/* Multiplicative */

static inline tvector2w OP_vec1w_muld(
    const tvector1w     _a,
    const tvector1w     _b);

static inline tvector2w OP_vec1w_muld_c(
    const tvector1w     _a,
    const tscalar1w     _c);


/* Comparative */

/* Use standard operations */
#define   OP_vec1w_eq       OP_vec_eq
#define   OP_vec1w_eq_c     OP_vec_eq_c
#define   OP_vec1w_neq      OP_vec_neq
#define   OP_vec1w_neq_c    OP_vec_neq_c
#define   OP_vec1w_le       OP_vec_le
#define   OP_vec1w_le_c     OP_vec_le_c
#define   OP_vec1w_lt       OP_vec_lt
#define   OP_vec1w_lt_c     OP_vec_lt_c
#define   OP_vec1w_ge       OP_vec_ge
#define   OP_vec1w_ge_c     OP_vec_ge_c
#define   OP_vec1w_gt       OP_vec_gt
#define   OP_vec1w_gt_c     OP_vec_gt_c

/* Shift */

static inline tvector1w OP_vec1w_asr(
    const tvector1w     _a,
    const tvector       _b); 

static inline tvector1w OP_vec1w_asrrnd(
    const tvector1w     _a,
    const tvector       _b);
    
static inline tvector1w OP_vec1w_lsl(
    const tvector1w     _a,
    const tvector       _b);
    
static inline tvector1w OP_vec1w_lslsat(
    const tvector1w     _a,
    const tvector       _b);
    
static inline tvector1w OP_vec1w_lsr(
    const tvector1w     _a,
    const tvector1w     _b);
    
static inline tvector1w OP_vec1w_lsrrnd(
    const tvector1w     _a,
    const tvector1w     _b);
    
static inline tvector1w OP_vec1w_lsrrnd_c(
    const tvector1w     _a,
    const sp_fragment_t _c);
    
#define   OP_vec1w_asr_c    OP_vec_asr_c
#define   OP_vec1w_asrrnd_c OP_vec_asrrnd_c 
#define   OP_vec1w_lsl_c    OP_vec_lsl_c
#define   OP_vec1w_lslsat_c OP_vec_lslsat_c
#define   OP_vec1w_lsr_c    OP_vec_lsr_c
   
 
/* Cast */

static inline tscalar1w OP_int_cast_scalar1w (
    const int           _a);
    
static inline int OP_scalar1w_cast_int (
    const tscalar1w      _a);
    
static inline tvector2w OP_vec1w_cast_vec2w (
    const tvector1w     _a);

static inline tvector1w OP_vec2w_cast_vec1w (
    const tvector2w    _a);


/* Vector-slice */

static inline tvector1w_tslice1w OP_vec1w_slice_select_low(
    const tslice1w          _a,
    const tvector1w         _b,
    const sp_fragment_s_t   _c);
    
static inline tvector1w_tslice1w OP_vec1w_slice_select_high(
    const tslice1w          _a,
    const tvector1w         _b,
    const sp_fragment_s_t   _c);
    
#define   OP_vec1w_slice        OP_vec_slice
#define   OP_vec1w_select_low   OP_vec_select_low
#define   OP_vec1w_select_high  OP_vec_select_high

    
/* Vector and vector-slice */

static inline tvector1w_tslice1w OP_vecsl1w_addsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);

static inline tvector1w_tslice1w OP_vecsl1w_addnextsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);
    
static inline tvector1w_tslice1w OP_vecsl1w_subabs(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);
    
static inline tvector1w_tslice1w OP_vecsl1w_subsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);
    
static inline tvector1w_tslice1w OP_vecsl1w_subnextabs(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);

static inline tvector1w_tslice1w OP_vecsl1w_subnextsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);
    
static inline tvector1w_tslice1w OP_vecsl1w_nextsubabs(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);
    
static inline tvector1w_tslice1w OP_vecsl1w_nextsubsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d);
    
           
/* Miscellaneous */

/* Use standard operations */
#define   OP_vec1w_clone    OP_vec_clone
#define   OP_vec1w_mux      OP_vec_mux
#define   OP_vec1w_mux_c    OP_vec_mux_c
#define   OP_vec1w_mux_csel OP_vec_mux_csel
#define   OP_vec1w_max      OP_vec_max
#define   OP_vec1w_min      OP_vec_min

static inline tvector1w_tflags OP_vec1w_max_gt(
    const tvector1w     _a,
    const tvector1w     _b);

static inline tvector1w_tflags OP_vec1w_max_gt_c(
    const tvector1w     _a,
    const tscalar1w     _c);

static inline tvector1w_tflags OP_vec1w_min_le(
    const tvector1w     _a,
    const tvector1w     _b);

static inline tvector1w_tflags OP_vec1w_min_le_c(
    const tvector1w     _a,
    const tscalar1w     _c);

/* Use standard operations */
#define   OP_vec1w_imax             OP_vec_imax
#define   OP_vec1w_imin             OP_vec_imin
#define   OP_vec1w_even             OP_vec_even
#define   OP_vec1w_odd              OP_vec_odd
#define   OP_vec1w_clipz            OP_vec_clipz
#define   OP_vec1w_clipz_c          OP_vec_clipz_c
#define   OP_vec1w_clip_asym        OP_vec_clip_asym
#define   OP_vec1w_clip_asym_c      OP_vec_clip_asym_c
#define   OP_vec1w_get              OP_vec_get
#define   OP_vec1w_set              OP_vec_set
#define   OP_vec1w_neg              OP_vec_neg
#define   OP_vec1w_abs              OP_vec_abs
#define   OP_vec1w_subabs           OP_vec_subabs
#define   OP_vec1w_mergeh           OP_vec_mergeh
#define   OP_vec1w_mergel           OP_vec_mergel
#define   OP_vec1w_avgrnd           OP_vec_avgrnd
#define   OP_vec1w_avgrnd_c         OP_vec_avgrnd_c
#define   OP_vec1w_subhalfrnd       OP_vec_subhalfrnd
#define   OP_vec1w_subhalfrnd_c     OP_vec_subhalfrnd_c
#define   OP_vec1w_deint3           OP_vec_deint3
#define   OP_vec1w_groupshuffle8    OP_vec_groupshuffle8
#define   OP_vec1w_groupshuffle16   OP_vec_groupshuffle16
#define   OP_vec1w_shuffle16        OP_vec_shuffle16


/* Supporting functions */

static inline tflags OP_vec1w_comparator(
    const tvector1w     _a,
    const tvector1w     _b,
    const tflags        eq);

static inline tflags OP_vec1w_comparator_lt(
    const tvector1w     _a,
    const tvector1w     _b);

static inline tflags OP_vec1w_comparator_le(
    const tvector1w     _a,
    const tvector1w     _b);





/*
 * Single-precision overloading is specified according to the accompying processor manual.
 */
 



/*
 * Single-precision operations implementation.
 */


/* Load/Store */

static inline mvector1w OP_vec1w_saldo(
    const addr_t        _addr)
{
    addr_t  incr = 0;
    mvector1w _r;
    vec_saldoi_s(Any, _addr, incr, _r, incr);
    return _r;
}


static inline void OP_vec1w_sasto(
    const addr_t        _addr,
    const tvector1w     _a)
{ 
    addr_t    incr = 0;  
    vec_sastoi_s(Any, _addr ,incr, CASTVEC1W(_a), incr); 
    return; 
}


static inline tscalar1w OP_scalar1w_saldo(
    const addr_t        _addr)
{ 
#   if MP_FRAGMENT_BYTES==2
        return OP_std_ld16o(_addr, 0);
#   elif MP_FRAGMENT_BYTES==4
        return OP_std_ld32o(_addr, 0);
#   endif
}


static inline void OP_scalar1w_sasto(
    const addr_t        _addr,
    const mscalar1w     _a)
{
#   if MP_FRAGMENT_BYTES==2
        OP_std_st16o(_addr, 0,   _a);
#   elif MP_FRAGMENT_BYTES==4
        OP_std_st32o(_addr, 0,   _a);
#   endif
    return; 
}


static inline mvector1w OP_vec1w_saldo_crun(
    const mvector1w MEM(VMEM)   *_a)
{
    mvector1w _r =  *_a;
    return _r;
}


static inline mscalar1w OP_scalar1w_saldo_crun(
    const mscalar1w MEM(DMEM)   *_a)
{ 
    mscalar1w _r;
    _r = *_a;
    return _r;
}


static inline void OP_vec1w_sasto_crun(
          mvector1w MEM(VMEM)  *_a,
    const mvector1w             _c)
{ 
    *_a = _c;
    return; 
}


static inline void OP_scalar1w_sasto_crun(
          mscalar1w MEM(DMEM) * _a,
    const mscalar1w             _c)
{
    *_a = _c;
    return; 
}


/* Arithmetic */



/* Additive */

static inline tvector1w OP_vec1w_add(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector     out;
    tflags      carry;
    vec_add_cout (Any, CASTVEC1W(_a), CASTVEC1W(_b), out, carry);
    return out;
}


static inline tvector1w OP_vec1w_add_c(
    const tvector1w     _a,
    const tscalar1w     _b)
{
    tvector     out;
    tflags      carry;
    vec_add_cout (Any, CASTVEC1W(_a), OP_vec_clone(_b), out, carry);
    return out;
}


static inline tvector1w OP_vec1w_sub(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector     out;
    tflags      carry = OP_flag_clone(1);
    vec_add_cincout (Any, CASTVEC1W(_a), ~_b, carry, out, carry);
    return out;
}


static inline tvector1w OP_vec1w_sub_c(
    const tvector1w     _a,
    const tscalar1w     _b)
{
    tvector     out;
    tflags      carry = OP_flag_clone(1);
    vec_add_cincout (Any, CASTVEC1W(_a), OP_vec_clone(~_b), carry, out, carry);
    return out;
}


/* Multiplicative */

static inline tvector2w OP_vec1w_muld(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector2w  out;
    vec_mul (Any, CASTVEC1W(_a), CASTVEC1W(_b), out.d1, out.d0);
    return out;
}

static inline tvector2w OP_vec1w_muld_c(
    const tvector1w     _a,
    const tscalar1w     _c)
{
    tvector2w  out;
    vec_mul_c (Any, CASTVEC1W(_a), _c, out.d1, out.d0);
    return out;
}


/* Comperative */
//--

/* Shift */

static inline tvector1w OP_vec1w_asr(
    const tvector1w     _a,
    const tvector1w     _b)
{
    int i;
    tvector out = OP_vec_clone(0);
    for (i=0;i<ISP_VEC_NELEMS;i++)
    {
        sp_fragment_t   shft = OP_vec_get(CASTVEC1W(_b),i);
        tvector         tmp  = OP_vec_asr_c(CASTVEC1W(_a),shft);
        sp_fragment_t   outi = OP_vec_get(tmp,i);
        out = OP_vec_set(out,i,outi);
    }
    return out;
}


static inline tvector1w OP_vec1w_asrrnd(
    const tvector1w     _a,
    const tvector1w     _b)
{
    int i;
    tvector out = OP_vec_clone(0);
    for (i=0;i<ISP_VEC_NELEMS;i++)
    {
        sp_fragment_t   shft = OP_vec_get(CASTVEC1W(_b),i);
        tvector         tmp  = OP_vec_asrrnd_c(CASTVEC1W(_a),shft);
        sp_fragment_t   outi = OP_vec_get(tmp,i);
        out = OP_vec_set(out,i,outi);
    }
    return out;
}


static inline tvector1w OP_vec1w_lsl(
    const tvector1w     _a,
    const tvector1w     _b)
{
    int i;
    tvector out = OP_vec_clone(0);
    for (i=0;i<ISP_VEC_NELEMS;i++)
    {
        sp_fragment_t   shft = OP_vec_get(CASTVEC1W(_b),i);
        tvector         tmp  = OP_vec_lsl_c(CASTVEC1W(_a),shft);
        sp_fragment_t   outi = OP_vec_get(tmp,i);
        out = OP_vec_set(out,i,outi);
    }
    return out;
}


static inline tvector1w OP_vec1w_lslsat(
    const tvector1w     _a,
    const tvector       _b)
{
    int  i;
    tvector out = OP_vec_clone(0);
    for (i=0;i<ISP_VEC_NELEMS;i++)
    {
        sp_fragment_t   shft = OP_vec_get(CASTVEC1W(_b),i);
        tvector         tmp  = OP_vec_lslsat_c(CASTVEC1W(_a),shft);
        sp_fragment_t   outi = OP_vec_get(tmp,i);
        out = OP_vec_set(out,i,outi);
    }
    return out;
}


static inline tvector1w OP_vec1w_lsr(
    const tvector1w     _a,
    const tvector1w     _b)
{
    int  i;
    tvector out = OP_vec_clone(0);
    for (i=0;i<ISP_VEC_NELEMS;i++)
    {
        sp_fragment_t   shft = OP_vec_get(CASTVEC1W(_b),i);
        tvector         tmp  = OP_vec_lsr_c(CASTVEC1W(_a),shft);
        sp_fragment_t   outi = OP_vec_get(tmp,i);
        out = OP_vec_set(out,i,outi);
    }
    return out;
}

    
static inline tvector1w OP_vec1w_lsrrnd(
    const tvector1w     _a,
    const tvector1w     _b)
{
    int  i;
    tvector out = OP_vec_clone(0);
    for (i=0;i<ISP_VEC_NELEMS;i++)
    {
        sp_fragment_t   shft = OP_vec_get(CASTVEC1W(_b),i);
        tvector         tmp  = OP_vec1w_lsrrnd_c(CASTVEC1W(_a),shft);
        sp_fragment_t   outi = OP_vec_get(tmp,i);
        out = OP_vec_set(out,i,outi);
    }
    return out;
}    


static inline tvector1w OP_vec1w_lsrrnd_c(
    const tvector1w     _a,
    const sp_fragment_t _c)
{
    tflags carry;
    tvector out = OP_vec_clone(0);
    vec_lsrrnd_wsplit_cu_rndout(Any, OP_vec_sgnext(CASTVEC1W(_a)), CASTVEC1W(_a), _c, out, carry);
    return out;
}   
 
    
/* Cast */

static inline tscalar1w OP_int_cast_scalar1w (
    const int           _a)
{
    tscalar1w _r = (tscalar1w) _a; 
    return _r;
} 

   
static inline int OP_scalar1w_cast_int (
    const tscalar1w      _a)
{
    int _r = (int) _a; 
    return _r;
}

    
static inline tvector2w OP_vec1w_cast_vec2w (
    const tvector1w     _a)
{
    tvector2w _r = {_a,OP_vec_sgnext(CASTVEC1W(_a))};
    return _r;
}


static inline tvector1w OP_vec2w_cast_vec1w (
    const tvector2w    _a)
{
    return _a.d0;
}


/* Vector-slice */

static inline tvector1w_tslice1w OP_vec1w_slice_select_low(
    const tslice1w          _a,
    const tvector1w         _b,
    const sp_fragment_s_t   _c)
{
    tvector1w_tslice1w  out;
    vec_slice_select_low (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), _c, out.d, out.s);
    return out;
}


static inline tvector1w_tslice1w OP_vec1w_slice_select_high(
    const tslice1w          _a,
    const tvector1w         _b,
    const sp_fragment_s_t   _c)
{   
    tvector1w_tslice1w  out;
    vec_slice_select_high (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), _c, out.d, out.s);
    return out;
}


/* Vector and vector-slice */

static inline tvector1w_tslice1w OP_vecsl1w_addsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_addsat (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}

static inline tvector1w_tslice1w OP_vecsl1w_addnextsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_addnextsat (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


static inline tvector1w_tslice1w OP_vecsl1w_subabs(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_subabs (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


static inline tvector1w_tslice1w OP_vecsl1w_subsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_subsat (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


static inline tvector1w_tslice1w OP_vecsl1w_subnextabs(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_subnextabs (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


static inline tvector1w_tslice1w OP_vecsl1w_subnextsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_subnextsat (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


static inline tvector1w_tslice1w OP_vecsl1w_nextsubabs(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_nextsubabs (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


static inline tvector1w_tslice1w OP_vecsl1w_nextsubsat(
    const tslice1w      _a,
    const tvector1w     _b,
    const tslice1w      _c,
    const tvector1w     _d)
{
    tvector1w_tslice1w  out;
    vecsl_nextsubsat (Any, CASTVECSLICE1W(_a), CASTVEC1W(_b), CASTVECSLICE1W(_c), CASTVEC1W(_d), out.s, out.d);
    return out;
}


/* Miscellaneous */

static inline tvector1w_tflags OP_vec1w_max_gt(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector1w_tflags    out;
    vec_maxgt_o2 (Any, CASTVEC1W(_a), CASTVEC1W(_b), out.d, out.f);
    return out;
}


static inline tvector1w_tflags OP_vec1w_max_gt_c(
    const tvector1w     _a,
    const tscalar1w     _c)
{
    tvector1w_tflags    out;
    vec_maxgt_co2 (Any, CASTVEC1W(_a), _c, out.d, out.f);
    return out;
}


static inline tvector1w_tflags OP_vec1w_min_le(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector1w_tflags    out;
    vec_minle_o2 (Any, CASTVEC1W(_a), CASTVEC1W(_b), out.d, out.f);
    return out;
}


static inline tvector1w_tflags OP_vec1w_min_le_c(
    const tvector1w     _a,
    const tscalar1w     _c)
{
    tvector1w_tflags    out;
    vec_minle_co2 (Any, CASTVEC1W(_a), _c, out.d, out.f);
    return out;
}


/* Supporting functions */

static inline tflags OP_vec1w_comparator(
    const tvector1w     _a,
    const tvector1w     _b,
    const tflags        eq)
{
    tvector dum;
    tflags  sa = (_a<0);
    tflags  sb = (_b<0);
    tflags  carry = ~(CASTFLG(eq));
    vec_add_cincout (Any, CASTVEC1W(_a), ~_b, carry, dum, carry);
    return (sa^(~sb)^carry);
}


static inline tflags OP_vec1w_comparator_lt(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector dum;
    tflags  sa = (_a<0);
    tflags  sb = (_b<0);
    tflags  carry = OP_flag_clone(1);
    vec_add_cincout (Any, CASTVEC1W(_a), ~_b, carry, dum, carry);
    return (sa^(~sb)^carry);
}


static inline tflags OP_vec1w_comparator_le(
    const tvector1w     _a,
    const tvector1w     _b)
{
    tvector dum;
    tflags  sa = (_a<0);
    tflags  sb = (_b<0);
    tflags  carry = OP_flag_clone(0);
    vec_add_cincout (Any, CASTVEC1W(_a), ~_b, carry, dum, carry);
    return (sa^(~sb)^carry);
}

#endif /* __ISP2400_VEC1W_H_INCLUDED__ */
