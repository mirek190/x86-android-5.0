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

#ifndef __XMEM_PRIVATE_H_INCLUDED__
#define __XMEM_PRIVATE_H_INCLUDED__

/* Implementation of DDR buffer access from a cell 
 * uses dma to transfer via vmem into vectors.
 */

#include "xmem_public.h"
#include <event_fifo.h>
#include <hive_isp_css_isp_dma_api_modified.h>
#include "math_support.h"

#include "isp.h"

#include "isp_support.h"

/* No xmem lsu available.
 * xmem pointers really are hrt_vaddresses.
 * However, to guarantee consistency with e.g. 2600 system with a master port,
 * xmem is mapped to dmem.
 */
#define xmem base_dmem

static inline tvector vec_load_xmem(const short MEM(xmem) * va_in);
static inline void vec_store_xmem(short MEM(xmem) * va_out, tvector data);

#define DDR_DMEM_MEMCPY_DMA_CH      0
#define DDR_VMEM_MEMCPY_DMA_CH      1
#define SYNC_INPUT_DMA  0x1001
#define SYNC_OUTPUT_DMA 0x1002

static inline void ddr_vmem_dma_init(uint32_t nr_elems)
{
	uint32_t height = 1;          /* load all elements together */
	uint32_t ddr_bits_per_element = 16;
	uint32_t ddr_elems_per_word = ceil_div(HIVE_ISP_DDR_WORD_BITS, ddr_bits_per_element);
	uint32_t ddr_words_per_line = ceil_div(nr_elems, ddr_elems_per_word);
	uint32_t vmem_elems_per_word = ISP_VEC_NELEMS;
	uint32_t vmem_words_per_line = ceil_div(nr_elems, vmem_elems_per_word);

	hive_dma_configure(DMA0_FIFO_ADDRESS, DDR_VMEM_MEMCPY_DMA_CH, dma_isp_to_ddr_connection, _DMA_SIGN_EXTEND,
			   height,              /* height */
			   0,                   /* VMEM stride */
			   vmem_elems_per_word, /* VMEM elems per word */
			   0,                   /* VMEM cropping */
			   vmem_words_per_line, /* VMEM words per line */
			   0,                   /* DDR stride */
			   ddr_elems_per_word,  /* DDR elems per word*/
			   0,                   /* DDR cropping */
			   ddr_words_per_line   /* DDR words per line */
			  ) SYNC(SYNC_INPUT_DMA) SYNC(SYNC_OUTPUT_DMA);
}

static inline void ddr2vmem_memcpy(hrt_vaddress p_ddr_from, tvector *p_vmem_to, uint32_t nr_elems)
{
	uint32_t vmem_address = ISP_BAMEM_BASE[ISP0_ID] + HIVE_DMA_TR_ADDR((uint32_t)p_vmem_to);
	ddr_vmem_dma_init(nr_elems);
	hive_dma_move_b2a_data(DMA0_FIFO_ADDRESS, DDR_VMEM_MEMCPY_DMA_CH, vmem_address, p_ddr_from, dma_true, dma_false) SYNC(SYNC_INPUT_DMA);
	event_wait_for(DMA0_EVENT_ID) SYNC(SYNC_INPUT_DMA);
}

static inline void vmem2ddr_memcpy(const tvector *p_vmem_from, hrt_vaddress p_ddr_to, uint32_t nr_elems)
{
	uint32_t vmem_address = ISP_BAMEM_BASE[ISP0_ID] + HIVE_DMA_TR_ADDR((uint32_t)p_vmem_from);
	ddr_vmem_dma_init(nr_elems);
	hive_dma_move_a2b_data(DMA0_FIFO_ADDRESS, DDR_VMEM_MEMCPY_DMA_CH, vmem_address, p_ddr_to, dma_true, dma_false) SYNC(SYNC_OUTPUT_DMA);
	event_wait_for(DMA0_EVENT_ID) SYNC(SYNC_OUTPUT_DMA);
}

/*
 * ISP_2400_SYSTEM specific xmem-based implementations of the access functions.
 * Uses DMA.
 * */
#define SYNC_IO_VMEM  0x1003

static inline tvector elems_load_xmem(const short MEM(xmem) * va_in, uint32_t nr_elems)
{
	tvector SYNC_WITH(SYNC_IO_VMEM) in;

	hrt_vaddress ld_ptr = (hrt_vaddress)va_in;
	ddr2vmem_memcpy(ld_ptr, &in, nr_elems) SYNC(SYNC_IO_VMEM);

	return in;
}

static inline void elems_store_xmem(short MEM(xmem) * va_out, tvector data, uint32_t nr_elems)
{
	tvector SYNC_WITH(SYNC_IO_VMEM) out;
	hrt_vaddress st_ptr = (hrt_vaddress)va_out;
	out = data;
	vmem2ddr_memcpy(&out, st_ptr, nr_elems) SYNC(SYNC_IO_VMEM);
}

static inline tvector vec_load_xmem(const short MEM(xmem) * va_in)
{
	return elems_load_xmem(va_in, ISP_VEC_NELEMS);
}

static inline void vec_store_xmem(short MEM(xmem) * va_out, tvector data)
{
	elems_store_xmem(va_out, data, ISP_VEC_NELEMS);
}

static inline void ddr_dmem_dma_init(uint32_t data_size_in_bytes)
{
	uint32_t height = 1;     /* load all elements together */
	uint32_t data_size = data_size_in_bytes*8; /* data_size in bits */
	uint32_t bits_per_element = 8;
	uint32_t width = ceil_div(data_size, bits_per_element); /* width in terms of #elements */

	uint32_t bus_word_bits  = 32;
	uint32_t bus_word_bytes = bus_word_bits/8;
	uint32_t isp_elems_per_word = ceil_div(bus_word_bits, bits_per_element);
	uint32_t ddr_elems_per_word = ceil_div(HIVE_ISP_DDR_WORD_BITS, bits_per_element);
	uint32_t isp_words_per_line = ceil_div(width, isp_elems_per_word);
	uint32_t ddr_words_per_line = ceil_div(width, ddr_elems_per_word);

	hive_dma_configure(DMA0_FIFO_ADDRESS, DDR_DMEM_MEMCPY_DMA_CH, dma_bus_to_ddr_connection, _DMA_ZERO_EXTEND,
			   height,                               /* height */
			   isp_words_per_line * bus_word_bytes,  /* DMEM stride */
			   isp_elems_per_word,                   /* DMEM elems per word */
			   0,                                    /* DMEM cropping */
			   isp_words_per_line,                   /* DMEM words per line */
			   ddr_words_per_line * HIVE_ISP_DDR_WORD_BYTES, /* DDR stride */
			   ddr_elems_per_word,                  /* DDR elems per word*/
			   0,                                   /* DDR cropping */
			   ddr_words_per_line                   /* DDR words per line */
			  ) SYNC(SYNC_INPUT_DMA) SYNC(SYNC_OUTPUT_DMA);
}

static inline void ddr2dmem_memcpy(hrt_vaddress p_ddr_from, void *p_dmem_to, uint32_t data_size_in_bytes)
{
	uint32_t dmem_address = ISP_DMEM_BASE[ISP0_ID] + (uint32_t)p_dmem_to;
	OP___assert(data_size_in_bytes % 4 == 0);
	ddr_dmem_dma_init(data_size_in_bytes);
	hive_dma_move_b2a_data(DMA0_FIFO_ADDRESS, DDR_DMEM_MEMCPY_DMA_CH, dmem_address, p_ddr_from, dma_true, dma_false) SYNC(SYNC_INPUT_DMA);
	event_wait_for(DMA0_EVENT_ID) SYNC(SYNC_INPUT_DMA);
}

static inline void dmem2ddr_memcpy(const void *p_dmem_from, hrt_vaddress p_ddr_to, uint32_t data_size_in_bytes)
{
	uint32_t dmem_address = ISP_DMEM_BASE[ISP0_ID] + (uint32_t)p_dmem_from;
	OP___assert(data_size_in_bytes % 4 == 0);
	ddr_dmem_dma_init(data_size_in_bytes);
	hive_dma_move_a2b_data(DMA0_FIFO_ADDRESS, DDR_DMEM_MEMCPY_DMA_CH, dmem_address, p_ddr_to, dma_true, dma_false) SYNC(SYNC_OUTPUT_DMA);
	event_wait_for(DMA0_EVENT_ID) SYNC(SYNC_OUTPUT_DMA);
}

#endif /* __XMEM_PRIVATE_H_INCLUDED__ */
