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

#ifndef __ISP_PRIVATE_H_INCLUDED__
#define __ISP_PRIVATE_H_INCLUDED__

#include <stddef.h>
#include <stdbool.h>

#include "isp_public.h"

#ifndef __INLINE_RESOURCE__
#define __INLINE_RESOURCE__
#endif
#include "resource.h"
#ifndef __INLINE_EVENT__
#define __INLINE_EVENT__
#endif
#include "event_fifo.h"

#include "math_support.h"
#include "assert_support.h"

#include "dma.h"

#include "dma_proxy.isp.h"

#define _DMA_PROXY

STORAGE_CLASS_ISP_C void isp_dmem_store(
	const isp_ID_t		ID,
	unsigned int		addr,
	const hrt_vaddress	data,
	const size_t		size)
{
	unsigned int	channel_ID = resource_acquire(DMA_CHANNEL_RESOURCE_TYPE, SHARED_RESOURCE_RESERVATION);

OP___assert(ID < N_ISP_ID);
OP___assert((ISP_DMEM_BASE[ID] % HIVE_ISP_CTRL_DATA_BYTES) == 0);
OP___assert((addr % HIVE_ISP_CTRL_DATA_BYTES) == 0);
OP___assert((data % HIVE_ISP_DDR_WORD_BYTES) == 0);
/* System imposed burst size limit */
OP___assert(size <= (HIVE_ISP_MAX_BURST_LENGTH*HIVE_ISP_CTRL_DATA_BYTES));

#ifdef _DMA_PROXY
	dma_proxy_configure(channel_ID,(dma_connection)HIVE_DMA_BUS_DDR_CONN, (dma_extension)_DMA_ZERO_EXTEND, 1,
		0, HIVE_ISP_CTRL_DATA_BYTES, 0, ceil_div(size, HIVE_ISP_CTRL_DATA_BYTES),
		0, HIVE_ISP_DDR_WORD_BYTES, 0, ceil_div(size, HIVE_ISP_DDR_WORD_BYTES));

	dma_proxy_read_byte_addr(true, channel_ID,
		(volatile void *)(ISP_DMEM_BASE[ID] + addr),
		(volatile const void *)data);

	dma_proxy_ack(channel_ID);
#else /* end of _DMA_PROXY */
	hive_dma_configure(DMA0_FIFO_ADDRESS, channel_ID, HIVE_DMA_BUS_DDR_CONN, _DMA_ZERO_EXTEND, 1,
		0, HIVE_ISP_CTRL_DATA_BYTES, 0, ceil_div(size, HIVE_ISP_CTRL_DATA_BYTES),
		0, HIVE_ISP_DDR_WORD_BYTES, 0, ceil_div(size, HIVE_ISP_DDR_WORD_BYTES));
	hive_dma_move_b2a_data(DMA0_FIFO_ADDRESS, channel_ID, ISP_DMEM_BASE[ID] + addr, data, dma_true, dma_false);
	event_wait_for(DMA0_EVENT_ID);
#endif

	resource_release((resource_h)channel_ID);
return;
}

STORAGE_CLASS_ISP_C void isp_dmem_load(
	const isp_ID_t		ID,
	const unsigned int	addr,
	hrt_vaddress		data,
	const size_t		size)
{
	unsigned int	channel_ID = resource_acquire(DMA_CHANNEL_RESOURCE_TYPE, SHARED_RESOURCE_RESERVATION);

OP___assert(ID < N_ISP_ID);
OP___assert((ISP_DMEM_BASE[ID] % HIVE_ISP_CTRL_DATA_BYTES) == 0);
OP___assert((addr % HIVE_ISP_CTRL_DATA_BYTES) == 0);
OP___assert((data % HIVE_ISP_DDR_WORD_BYTES) == 0);
/* System imposed burst size limit */
OP___assert(size <= (HIVE_ISP_MAX_BURST_LENGTH*HIVE_ISP_CTRL_DATA_BYTES));

#ifdef _DMA_PROXY
	dma_proxy_configure(channel_ID,(dma_connection)HIVE_DMA_BUS_DDR_CONN, (dma_extension)_DMA_ZERO_EXTEND, 1,
		0, HIVE_ISP_CTRL_DATA_BYTES, 0, ceil_div(size, HIVE_ISP_CTRL_DATA_BYTES),
		0, HIVE_ISP_DDR_WORD_BYTES, 0, ceil_div(size, HIVE_ISP_DDR_WORD_BYTES));

	dma_proxy_write_byte_addr(true, channel_ID,
		(volatile const void *)(ISP_DMEM_BASE[ID] + addr),
		(volatile void *)data);

	dma_proxy_ack(channel_ID);
#else /* end of _DMA_PROXY */
	hive_dma_configure(DMA0_FIFO_ADDRESS, channel_ID, HIVE_DMA_BUS_DDR_CONN, _DMA_ZERO_EXTEND, 1,
		0, HIVE_ISP_CTRL_DATA_BYTES, 0, ceil_div(size, HIVE_ISP_CTRL_DATA_BYTES),
		0, HIVE_ISP_DDR_WORD_BYTES, 0, ceil_div(size, HIVE_ISP_DDR_WORD_BYTES));
	hive_dma_move_a2b_data(DMA0_FIFO_ADDRESS, channel_ID, ISP_DMEM_BASE[ID] + addr, data, dma_true, dma_false);
	event_wait_for(DMA0_EVENT_ID);
#endif

	resource_release((resource_h)channel_ID);
return;
}

STORAGE_CLASS_ISP_C void isp_dmem_clear(
	const isp_ID_t		ID,
	unsigned int		addr,
	const size_t		size)
{
	unsigned int	channel_ID = resource_acquire(DMA_CHANNEL_RESOURCE_TYPE, SHARED_RESOURCE_RESERVATION);

OP___assert(ID < N_ISP_ID);
OP___assert((ISP_DMEM_BASE[ID] % HIVE_ISP_CTRL_DATA_BYTES) == 0);
OP___assert((addr % HIVE_ISP_CTRL_DATA_BYTES) == 0);
OP___assert((size % HIVE_ISP_CTRL_DATA_BYTES) == 0);
/* System imposed burst size limit */
OP___assert(size <= (HIVE_ISP_MAX_BURST_LENGTH*HIVE_ISP_CTRL_DATA_BYTES));

#ifdef _DMA_PROXY
	dma_proxy_configure(channel_ID, (dma_connection)HIVE_DMA_BUS_DDR_CONN, (dma_extension)_DMA_ZERO_EXTEND, 1,
		0, HIVE_ISP_CTRL_DATA_BYTES, 0, ceil_div(size, HIVE_ISP_CTRL_DATA_BYTES),
		0, 0, 0, 0);

//	ia_css_dmaproxy_sp_init_isp_vector(channel_ID, (volatile void *)(SP_DMEM_BASE[ID] + addr), 0);

	dma_proxy_ack(channel_ID);
#else /* end of _DMA_PROXY */
	hive_dma_configure(DMA0_FIFO_ADDRESS, channel_ID, HIVE_DMA_BUS_DDR_CONN, _DMA_ZERO_EXTEND, 1,
		0, HIVE_ISP_CTRL_DATA_BYTES, 0, ceil_div(size, HIVE_ISP_CTRL_DATA_BYTES),
		0, 0, 0, 0);
	hive_dma_clear_data(DMA0_FIFO_ADDRESS, channel_ID, ISP_DMEM_BASE[ID] + addr, dma_true);
	event_wait_for(DMA0_EVENT_ID);
#endif

	resource_release(channel_ID);
return;
}

STORAGE_CLASS_ISP_C void isp_stream_get(
  bool cond,
	const isp_ID_t		ID,
	unsigned int		addr,
	unsigned int		stream_ID,
	const size_t		size)
{
OP___assert(ID < N_ISP_ID);

#ifdef _DMA_PROXY
(void)ID;
(void)size;
	dma_proxy_stream_get(cond, stream_ID, addr);
#else
#error "isp_private.h: isp_stream_get() only available through proxy"
#endif

return;
}

STORAGE_CLASS_ISP_C void isp_stream_put(
  bool cond,
	const isp_ID_t		ID,
	const unsigned int	addr,
	unsigned int		stream_ID,
	const size_t		size)
{
OP___assert(ID < N_ISP_ID);

#ifdef _DMA_PROXY
(void)ID;
(void)size;
	dma_proxy_stream_put(cond, stream_ID, addr);
#else
#error "isp_private.h: isp_stream_put() only available through proxy"
#endif

return;
}

STORAGE_CLASS_ISP_C void isp_vstream_get(
  bool cond,
	const bamem_ID_t	ID,
	unsigned int		addr,
	unsigned int		stream_ID,
	const size_t		size)
{
OP___assert(ID < N_BAMEM_ID);

#ifdef _DMA_PROXY
(void)ID;
(void)size;
	dma_proxy_vstream_get(cond, stream_ID, addr);
#else
#error "isp_private.h: isp_vstream_get() only available through proxy"
#endif

return;
}

STORAGE_CLASS_ISP_C void isp_vstream_put(
  bool cond,
	const bamem_ID_t	ID,
	const unsigned int	addr,
	unsigned int		stream_ID,
	const size_t		size)
{
OP___assert(ID < N_BAMEM_ID);

#ifdef _DMA_PROXY
(void)ID;
(void)size;
	dma_proxy_vstream_put(cond, stream_ID, addr);
#else
#error "isp_private.h: isp_vstream_put() only available through proxy"
#endif

return;
}

#endif /* __ISP_PRIVATE_H_INCLUDED__ */
