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

#ifndef __VMEM_PRIVATE_H_INCLUDED__
#define __VMEM_PRIVATE_H_INCLUDED__

#include <assert_support.h>

#include "vmem_public.h"
#include <event_fifo.h>
#include <hive_isp_css_isp_dma_api_modified.h>

#include "isp.h"

#include "isp_support.h"

STORAGE_CLASS_VMEM_C tvector vmem_unaligned_load(
	tmemvectoru MEM(VMEM) *p,
	unsigned pix_offset)
{
#if defined(__HIVECC) && HAS_asp_bmldrow_s
	/* Use unaligned load */
	return OP_asp_bmldrow_s(0, (unsigned)p*ISP_VEC_NELEMS + pix_offset);
#else /* Can be optimized using shuffles */
	unsigned j, k;

	/* Load 2 adjacent vectors */
	tvector v0 = p[pix_offset / ISP_VEC_NELEMS];
	tvector v1 = p[pix_offset / ISP_VEC_NELEMS + 1];
	tvector v = OP_vec_clone(0);

	OP___assert((int)pix_offset >= 0);
	/* Copy upper pixels from v0 */
	for (k = 0, j = pix_offset % ISP_VEC_NELEMS; j < ISP_VEC_NELEMS; j++, k++) {
		v = OP_vec_set(v, k, OP_vec_get(v0, j));
	}
	/* Copy lower pixels from v1 */
	for (j = 0; j < pix_offset % ISP_VEC_NELEMS; j++, k++) {
		v = OP_vec_set(v, k, OP_vec_get(v1, j));
	}
	return v;
#endif
}

STORAGE_CLASS_VMEM_C void vmem_unaligned_store(
	tmemvectoru MEM(VMEM) *p,
	unsigned pix_offset,
	tvector v)
{
#if defined(__HIVECC) && HAS_asp_bmstrow_s
	/* Use unaligned store */
	OP_asp_bmstrow_s(0, (unsigned)p*ISP_VEC_NELEMS + pix_offset, v);
#else /* Can be optimized using shuffles */
	unsigned j, k;

	/* Load 2 adjacent vectors */
	tvector v0 = p[pix_offset / ISP_VEC_NELEMS];
	tvector v1 = p[pix_offset / ISP_VEC_NELEMS + 1];

	OP___assert((int)pix_offset >= 0);
	/* Copy upper pixels into v0 */
	for (k = 0, j = pix_offset % ISP_VEC_NELEMS; j < ISP_VEC_NELEMS; j++, k++) {
		v0 = OP_vec_set(v0, j, OP_vec_get(v, k));
	}
	/* Copy lower pixels into v1 */
	for (j = 0; j < pix_offset % ISP_VEC_NELEMS; j++, k++) {
		v1 = OP_vec_set(v1, j, OP_vec_get(v, k));
	}
	p[pix_offset / ISP_VEC_NELEMS]     = v0;
	p[pix_offset / ISP_VEC_NELEMS + 1] = v1;
#endif
}

#endif /* __VMEM_PRIVATE_H_INCLUDED__ */
