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

#ifndef __VECTOR_OPS_PUBLIC_H_INCLUDED__
#define __VECTOR_OPS_PUBLIC_H_INCLUDED__

#include "slice_vector.isp.h"

STORAGE_CLASS_VECTOR_OPS_H tvector1x2 OP_vec_minmax (
    const tvector       _a,
    const tvector       _b);

STORAGE_CLASS_VECTOR_OPS_H tvector1x2 OP_vec_evenodd (
    const tvector       _a,
    const tvector       _b);

STORAGE_CLASS_VECTOR_OPS_H tvector1x2 OP_vec_mergelh (
    const tvector       _a,
    const tvector       _b);


STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_add (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_sub (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_subabs (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_addnextsat (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_nextsubsat (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_subnextsat (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_nextsubabs (
    s_slice_vector      a,
    s_slice_vector      b);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector isp_vecsl_subnextabs (
    s_slice_vector      a,
    s_slice_vector      b);


STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_addsat(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);

#if HAS_vecsl_addnextsat
STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_addnextsat(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);
#endif

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_subabs(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_subsat(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);

#if HAS_vecsl_subnextabs
STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_subnextabs(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);
#endif

#if HAS_vecsl_subnextsat
STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_subnextsat(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);
#endif

#if HAS_vecsl_nextsubabs
STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_nextsubabs(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);
#endif

#if HAS_vecsl_nextsubsat
STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vecsl_nextsubsat(
    const tslice1w		s0,
    const tvector1w		v0,
    const tslice1w		s1,
    const tvector1w		v1);
#endif

STORAGE_CLASS_VECTOR_OPS_H twidevector OP_vec_mul_c(
	const tvector1w		v0,
	const short			c1);

STORAGE_CLASS_VECTOR_OPS_H twidevector OP_widevec_mux_csel(
	const twidevector	v0,
	const twidevector	v1,
	const int			sel);

#if !HAS_vec_slice
STORAGE_CLASS_VECTOR_OPS_H tvector1w OP_vec_slice(
		const tslice1w		s,
		const tvector1w		v,
		const int		idx);
#endif

STORAGE_CLASS_VECTOR_OPS_H s_slice_vector OP_vec_slvec_select_high_pair(
		const tslice1w		s,
		const tvector1w		v,
		const int		idx);

#if HAS_vec_asrrnd_wsplit_c_rndin
STORAGE_CLASS_VECTOR_OPS_H tvector vec_asr_no_rnd (twidevector w, unsigned i);
#endif

#if HAS_vec_lsrrnd_wsplit_cu_rndin
STORAGE_CLASS_VECTOR_OPS_H tvector vec_lsr_no_rnd (twidevector w, unsigned i);
#endif

#if !HAS_vec_abssat
STORAGE_CLASS_VECTOR_OPS_H tvector OP_vec_abssat (tvector v);
#endif

#if !HAS_vec_rmux_c
STORAGE_CLASS_VECTOR_OPS_H tvector OP_vec_rmux_c (tvector v, unsigned c, tflags f);
#endif

#if !HAS_vec_gt_u
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_gt_u (tvectoru a, tvectoru b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>,  STORAGE_CLASS_VECTOR_OPS_H, OP_vec_gt_u,   tflags, tvectoru, tvectoru)
#endif

#if !HAS_vec_ge_u
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_ge_u (tvectoru a, tvectoru b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>=, STORAGE_CLASS_VECTOR_OPS_H, OP_vec_ge_u,   tflags, tvectoru, tvectoru)
#endif

#if !HAS_vec_gt_cu
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_gt_cu (tvectoru a, int b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>,  STORAGE_CLASS_VECTOR_OPS_H, OP_vec_gt_cu,   tflags, tvectoru, int)
#endif

#if !HAS_vec_ge_cu
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_ge_cu (tvectoru a, int b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>=, STORAGE_CLASS_VECTOR_OPS_H, OP_vec_ge_cu,   tflags, tvectoru, int)
#endif

#if !HAS_vec_gt
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_gt (tvectors a, tvectors b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>,  STORAGE_CLASS_VECTOR_OPS_H, OP_vec_gt,   tflags, tvectors, tvectors)
#endif

#if !HAS_vec_ge
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_ge (tvectors a, tvectors b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>=, STORAGE_CLASS_VECTOR_OPS_H, OP_vec_ge,   tflags, tvectors, tvectors)
#endif

#if !HAS_vec_gt_c
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_gt_c (tvectors a, int b);
// Pending compiler big: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>,  STORAGE_CLASS_VECTOR_OPS_H, OP_vec_gt_c,   tflags, tvectors, int)
#endif

#if !HAS_vec_ge_c
STORAGE_CLASS_VECTOR_OPS_H tflags OP_vec_ge_c (tvectors a, int b);
// Pending compiler bug: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (>=, STORAGE_CLASS_VECTOR_OPS_H, OP_vec_ge_c,   tflags, tvectors, int)
#endif

STORAGE_CLASS_VECTOR_OPS_H twidevector isp_vec_mul_ecu(tvectoru a, unsigned b);
// Pending compiler bug: cannot overload both signed and unsigned comparison.
//HIVE_BINARY_CALL (*,  STORAGE_CLASS_VECTOR_OPS_H, isp_vec_mul_ecu, twidevector, tvectoru, unsigned)

#endif /* __VECTOR_OPS_PUBLIC_H_INCLUDED__ */
