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

#ifndef __DMA_PLAIN_COMMANDS_H
#define __DMA_PLAIN_COMMANDS_H

#include "isp.h"

#define USE_SYNC_ON_FUNCTIONS 0

#ifdef __HIVECC
#define _DMA_GET_WORD_SIZE_HIVECC(word_addr) sizeof(*word_addr)
#define _DMA_GET_WORD_SIZE_CRUN(word_addr) 1
#else
#define _DMA_GET_WORD_SIZE_HIVECC(word_addr) 1
#define _DMA_GET_WORD_SIZE_CRUN(word_addr) sizeof(*word_addr)
#endif

#ifndef HIVE_dma_SRU
#define HIVE_dma_SRU HIVE_DMA_SRU
#endif

#ifndef HIVE_dma_FIFO
#define HIVE_dma_FIFO HIVE_DMA_FIFO
#endif

#define ISP_DMA_ISP_VAR_TO_DDR(cond, ch, var_addr, ddr_addr, num_xfers)          \
	if (cond) \
		isp_dma_isp_var_to_ddr(ch, var_addr, ddr_addr)

/* Use macro to enable sync */
#define ISP_DMA_DDR_TO_ISP_VAR(cond, ch, ddr_addr, var_addr, num_xfers)          \
	if (cond) \
		isp_dma_ddr_to_isp_var(ch, ddr_addr, var_addr)

#define _ISP_DMA_READ_DATA(ch, to_addr, from_addr, to_sizeof) \
	hive_dma_v1_read_data(DMA, ch, (((unsigned int)(to_addr)) * (to_sizeof)), from_addr, dma_true, dma_false)

#define _ISP_DMA_WRITE_DATA(ch, from_addr, to_addr, from_sizeof) \
	hive_dma_v1_write_data(DMA, ch, (((unsigned int)(from_addr)) * (from_sizeof)), to_addr, dma_true, dma_false)

#define ISP_DMA_SET_CHANNEL_WIDTH_AB(ch, width_A, width_B)                 \
	{ isp_dma_set_channel_width_A(ch, width_A);                              \
	  isp_dma_set_channel_width_B(ch, width_B);                              \
	}

#define ISP_DMA_SET_CHANNEL_ELEMENTS_AND_CROPPING_A(channel, elements, cropping) \
	isp_dma_set_channel_elements_and_cropping_A(channel, elements, cropping)

#ifdef isp_dma_set_height
#define ISP_DMA_SET_HEIGHT(ch, height)      \
	isp_dma_set_height(cond, ch, height)
#endif

void elements_in_dma_transaction_is_no_multiple_of_number_of_transfers(void);

#define ISP_DMA_CONFIGURE_CHANNEL(channel, connection, extension, height, \
				  stride_A, elems_A, cropping_A, width_A,                \
				  stride_B, elems_B, cropping_B, width_B,                \
				  num_sub_xfers)                                         \
{                                                                                        \
/* disabled because width is dynamic for output channel if (width_A % num_sub_xfers || width_B % num_sub_xfers) elements_in_dma_transaction_is_no_multiple_of_number_of_transfers();*/ \
	/* assert(width_A % num_sub_xfers == 0);*/                                                  \
	/* assert(width_B % num_sub_xfers == 0);*/                                                  \
	isp_dma_configure_channel(channel, connection, extension, /* dma_keep_element_order,*/ height,       \
			    stride_A, elems_A, cropping_A, ((width_A) / (num_sub_xfers)),    \
			    stride_B, elems_B, cropping_B, ((width_B) / (num_sub_xfers)));    \
}

#define ISP_DMA_SEEK_CHANNEL(cond, channel, offset, origin)                                   \
{                                                                                        \
	isp_dma_seek_channel(cond, channel, offset, origin);    \
}

#if USE_SYNC_ON_FUNCTIONS
static inline void
isp_read_data_xfers(const unsigned int channel, const unsigned int to_addr,
		     const unsigned int from_addr, const unsigned int width_A,
		     const unsigned int width_B,
		     const unsigned int to_word_size_hivecc,
		     const unsigned int to_word_size_crun,
		     const unsigned int num_sub_xfers)
{
	const unsigned int X_stride_A = (width_A / num_sub_xfers) * to_word_size_crun;
	const unsigned int X_stride_B =
	  (width_B / num_sub_xfers) * XMEM_POW2_BYTES_PER_WORD;
	unsigned int i;

	for (i = 0; i < num_sub_xfers; i++) {
	  _ISP_DMA_READ_DATA(channel, to_addr + i * X_stride_A,
			from_addr + i * X_stride_B, to_word_size_hivecc);
#pragma hivecc unroll
	}
}

static inline void
isp_write_data_xfers(const unsigned int channel, const unsigned int from_addr,
		      const unsigned int to_addr, const unsigned int width_A,
		      const unsigned int width_B,
		      const unsigned int from_word_size_hivecc,
		      const unsigned int from_word_size_crun,
		      const unsigned int num_sub_xfers)
{
	const unsigned int X_stride_A = (width_A / num_sub_xfers) * to_word_size_crun;
	const unsigned int X_stride_B =
	  (width_B / num_sub_xfers) * XMEM_POW2_BYTES_PER_WORD;
	unsigned int i;

	for (i = 0; i < num_sub_xfers; i++) {
	  _ISP_DMA_WRITE_DATA(channel, from_addr + i * X_stride_A,
			 to_addr + i * X_stride_B, from_word_size_hivecc);
#pragma hivecc unroll
	}
}
#else
#define CAT(a, b) _CAT(a, b)
#define _CAT(a, b) a##b

#define UNROLL(n, times) CAT(UNROLL, times)(n)

#define UNROLL1(n) (n)
#define UNROLL2(n) UNROLL1(n) UNROLL1(n)
#define UNROLL3(n) UNROLL2(n) UNROLL1(n)
#define UNROLL4(n) UNROLL3(n) UNROLL1(n)
#define UNROLL5(n) UNROLL4(n) UNROLL1(n)
#define UNROLL6(n) UNROLL5(n) UNROLL1(n)
#define UNROLL7(n) UNROLL6(n) UNROLL1(n)
#define UNROLL8(n) UNROLL7(n) UNROLL1(n)
#define UNROLL9(n) UNROLL8(n) UNROLL1(n)
#define UNROLL10(n) UNROLL9(n) UNROLL1(n)
#define UNROLL11(n) UNROLL10(n) UNROLL1(n)
#define UNROLL12(n) UNROLL11(n) UNROLL1(n)
#define UNROLL13(n) UNROLL12(n) UNROLL1(n)
#define UNROLL14(n) UNROLL13(n) UNROLL1(n)
#define UNROLL15(n) UNROLL14(n) UNROLL1(n)
#define UNROLL16(n) UNROLL15(n) UNROLL1(n)
#define UNROLL17(n) UNROLL16(n) UNROLL1(n)
#define UNROLL18(n) UNROLL17(n) UNROLL1(n)
#define UNROLL19(n) UNROLL18(n) UNROLL1(n)
#define UNROLL20(n) UNROLL19(n) UNROLL1(n)
#define UNROLL21(n) UNROLL20(n) UNROLL1(n)
#define UNROLL22(n) UNROLL21(n) UNROLL1(n)
#define UNROLL23(n) UNROLL22(n) UNROLL1(n)
#define UNROLL24(n) UNROLL23(n) UNROLL1(n)
#define UNROLL25(n) UNROLL24(n) UNROLL1(n)
#define UNROLL26(n) UNROLL25(n) UNROLL1(n)
#define UNROLL27(n) UNROLL26(n) UNROLL1(n)
#define UNROLL28(n) UNROLL27(n) UNROLL1(n)
#define UNROLL29(n) UNROLL28(n) UNROLL1(n)
#define UNROLL30(n) UNROLL29(n) UNROLL1(n)

#define isp_read_data_xfers(channel, to_addr, from_addr, width_A, width_B, to_word_size_hivecc, to_word_size_crun, num_sub_xfers) \
{ \
	const unsigned int X_stride_A = ((width_A) / (num_sub_xfers)) * (to_word_size_crun); \
	const unsigned int X_stride_B = ((width_B) / (num_sub_xfers)) * XMEM_POW2_BYTES_PER_WORD; \
	unsigned int _i = 0; \
	UNROLL(_ISP_DMA_READ_DATA(channel, (to_addr) + _i * (X_stride_A), (from_addr) + _i * (X_stride_B), to_word_size_hivecc); _i++; , num_sub_xfers) \
}

#define isp_write_data_xfers(channel, from_addr, to_addr, width_A, width_B, from_word_size_hivecc, from_word_size_crun, num_sub_xfers) \
{ \
	const unsigned int X_stride_A = ((width_A) / (num_sub_xfers)) * (from_word_size_crun); \
	const unsigned int X_stride_B = ((width_B) / (num_sub_xfers)) * XMEM_POW2_BYTES_PER_WORD; \
	unsigned int _i = 0; \
	UNROLL(_ISP_DMA_WRITE_DATA(channel, (from_addr) + _i * (X_stride_A), (to_addr) + _i * (X_stride_B), from_word_size_hivecc); _i++; , num_sub_xfers) \
}
#endif

#if defined(HAS_ISP_2400_MAMOIADA) || defined(HAS_ISP_2401_MAMOIADA)
#define ISP_DMA_READ_DATA(cond, channel, to_addr, from_addr, width_A, width_B, num_sub_xfers)  \
	if (cond) \
		isp_dma_read_data(channel, to_addr, from_addr)

#define ISP_DMA_WRITE_DATA(cond, channel, from_addr, to_addr, width_A, width_B, num_sub_xfers) \
	if (cond) \
		isp_dma_write_data(channel, from_addr, to_addr)
#else
#define ISP_DMA_READ_DATA(cond, channel, to_addr, from_addr, width_A, width_B, num_sub_xfers)  \
{                                                                                        \
	if (cond) \
		isp_read_data_xfers(channel, (unsigned int)(to_addr), (unsigned int)(from_addr),           \
		      width_A, width_B, _DMA_GET_WORD_SIZE_HIVECC(to_addr), _DMA_GET_WORD_SIZE_CRUN(to_addr), num_sub_xfers);     \
}

#define ISP_DMA_WRITE_DATA(cond, channel, from_addr, to_addr, width_A, width_B, num_sub_xfers) \
{                                                                                        \
	if (cond) \
		isp_write_data_xfers(channel, (unsigned int)(from_addr), (unsigned int)(to_addr),          \
		       width_A, width_B, _DMA_GET_WORD_SIZE_HIVECC(from_addr), _DMA_GET_WORD_SIZE_CRUN(from_addr), num_sub_xfers);  \
}
#endif

#if USE_SYNC_ON_FUNCTIONS
static inline void
ISP_DMA_WAIT_FOR_ACK(const unsigned int channel, const unsigned int num_sub_xfers)
{
	unsigned int i;

	(void)channel;
	for (i = 0; i < num_sub_xfers; i++) {
		isp_dma_wait_for_ack();
#pragma hivecc unroll
	}
}
#elif 0
#define ISP_DMA_WAIT_FOR_ACK(channel, num_sub_xfers)	\
{							\
	unsigned int _i;				\
	for (_i = 0; _i < num_sub_xfers; _i++) {	\
		isp_dma_wait_for_ack();			\
	}						\
}
#else
#define ISP_DMA_WAIT_FOR_ACK(channel, num_sub_xfers)	\
{							\
	UNROLL(isp_dma_wait_for_ack(); , num_sub_xfers)	\
}
#endif

#define ISP_DMA_WAIT_FOR_ACK_N(channel, num_sub_xfers, n, recv)	\
{								\
	int _i;							\
	for (_i = 0; _i < n; _i++) {				\
		ISP_DMA_WAIT_FOR_ACK(channel, num_sub_xfers);	\
	}							\
}

#endif /* __DMA_PLAIN_COMMANDS_H */
