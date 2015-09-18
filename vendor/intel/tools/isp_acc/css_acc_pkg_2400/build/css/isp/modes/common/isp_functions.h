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

#ifndef _ISP_FUNCTIONS_H
#define _ISP_FUNCTIONS_H

#include "types.h"
#include "assert_support.h"

#ifndef __INLINE_VECTOR_OPS__
#define __INLINE_VECTOR_OPS__
#endif
#include "vector_ops.h"

// These functions should be removed.
// Instead functions from  hive_isp_css_shared/isp/vector_ops_*  should be used.

#if 0
#define P(x) (OP___udump(__LINE__, x), x)
#else
#define P(x) x
#endif

#ifndef reset_int
#define reset_int(s) (0)
#endif

#ifndef reset_unsigned
#define reset_unsigned(s) (0)
#endif

#ifndef POISON_STAGE_VALUE
#define POISON_STAGE_VALUE 0
#endif

static inline tvector
reset_tvector (tvector v)
{
#ifndef RESET_UNINIT
  v = OP_vec_clone(POISON_STAGE_VALUE);
#endif
  return v;
}

static inline tvector
vector_mux (tvector a, tvector b, int c)
{
#if HAS_vec_mux_csel
  return OP_vec_mux_csel (a, b, c);
#else
  return OP_vec_mux (a, b, OP_vec_neq_c (OP_vec_clone(c), 0));
#endif
}

static inline twidevector
widevector_mux (twidevector a, twidevector b, int c)
{
  twidevector w = { vector_mux (a.hi, b.hi, c), vector_mux (a.lo, b.lo, c) };
  return w;
}

static inline s_2_vectors
isp_vec_stretch (tvector v)
{
  s_2_vectors p;
  tvector _v = OP_vec_slice (OP_vec_select_high(v), v, 1);
  tvector va = OP_vec_avgrnd (v, _v);
  p.v0 = OP_vec_mergel (v, va);
  p.v1 = OP_vec_mergeh (v, va);
  return p;
}

static inline twidevector
mk_wide (tvector hi, tvector lo)
{
  twidevector w = { hi, lo };
  return w;
}

static inline twidevector
isp_wvec_even (twidevector a, twidevector b)
{
  twidevector w = { OP_vec_even(a.hi,b.hi), OP_vec_even(a.lo,b.lo) };
  return w;
}

static inline twidevector
isp_wvec_odd (twidevector a, twidevector b)
{
  twidevector w = { OP_vec_odd(a.hi,b.hi), OP_vec_odd(a.lo,b.lo) };
  return w;
}

static inline twidevector
isp_wvec_mux (twidevector a, twidevector b, tflags c)
{
  twidevector w = { OP_vec_mux (a.hi, b.hi, c), OP_vec_mux (a.lo, b.lo, c) };
  return w;
}

static inline twidevector
isp_wvec_mux_csel (twidevector a, twidevector b, int c)
{
#if HAS_vec_mux_csel
  twidevector w = { OP_vec_mux_csel (a.hi, b.hi, c), OP_vec_mux_csel (a.lo, b.lo, c) };
#else
  twidevector w = isp_wvec_mux (a, b, OP_vec_neq_c (OP_vec_clone(c), 0));
#endif
  return w;
}

static inline twidevector
isp_wvec_mux_c (twidevector a, int b, tflags c)
{
  twidevector w = { OP_vec_mux_c (a.hi, b, c), OP_vec_mux_c (a.lo, b, c) };
  return w;
}

static inline twidevector
isp_wpass (twidevector a)
{
#if HAS_std_pass_v
  twidevector w = { (OP_std_pass_v(a.hi) NO_CSE),
		            (OP_std_pass_v(a.lo) NO_CSE) };
#else
  twidevector w = { (OP_vector_pass(a.hi) NO_CSE),
		            (OP_vector_pass(a.lo) NO_CSE) };
#endif
  return w;
}

#if 0
static inline v_pair vec_minmax (tvector a, tvector b)
{
  v_pair c;
#if HAS_vec_minmax_o2
  vec_minmax_o2 (Any, a, b, c.v0, c.v1);
#else
  c.v0 = OP_vec_min (a, b);
  c.v1 = OP_vec_max (a, b);
#endif
  return c;
}
#endif

static inline void
isp_memcpy (void *dst, const void *src, size_t size)
{
  unsigned i;
  int *to = (int*)dst;
  const int *from = (int*)src;

  OP___assert((unsigned)dst  % sizeof(int) == 0);
  OP___assert((unsigned)src  % sizeof(int) == 0);
  OP___assert((unsigned)size % sizeof(int) == 0);

  for (i = 0; i < size / sizeof(int); i++) {
    to[i] = from[i];
  }
}
/******************/

#include "dump.isp.c"

#endif /* _ISP_FUNCTIONS_H */
