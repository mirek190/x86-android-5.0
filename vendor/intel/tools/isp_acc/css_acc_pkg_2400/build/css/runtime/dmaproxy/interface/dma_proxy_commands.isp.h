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

#ifndef __DMA_PROXY_COMMANDS_H
#define __DMA_PROXY_COMMANDS_H

#include "dma_proxy.isp.h"

#define ISP_DMA_CONFIGURE_CHANNEL(channel, connection, extension, height,	\
					stride_A, elems_A, cropping_A, width_A,	\
					stride_B, elems_B, cropping_B, width_B,	\
					num_sub_xfers)				\
	dma_proxy_configure(channel, connection, extension, height,		\
			    stride_A, elems_A, cropping_A, width_A,		\
			    stride_B, elems_B, cropping_B, width_B)

#define ISP_DMA_SEEK_CHANNEL(channel, offset, origin )				\
	dma_proxy_seek(channel, offset, origin )

#define ISP_DMA_RECONFIGURE_CHANNEL(cond, channel, connection, extension, height,	\
					    stride_A, elems_A, cropping_A, width_A,	\
					    stride_B, elems_B, cropping_B, width_B,	\
					    num_sub_xfers)				\
	dma_proxy_reconfigure(cond, channel, connection, extension, height,		\
				stride_A, elems_A, cropping_A, width_A,			\
				stride_B, elems_B, cropping_B, width_B)

#define ISP_DMA_RECONFIGURE_MEM(cond, channel, config)					\
	dma_proxy_reconfigure_mem(cond, channel, config)

#define ISP_DMA_CONFIGURE_STREAM(cond, channel, config)					\
	dma_proxy_configure_stream(cond, channel, config)

#define ISP_DMA_HEIGHT_EXCEPTION(cond, channel, config)					\
	dma_proxy_height_exception(cond, channel, config)

#define ISP_DMA_WIDTH_EXCEPTION(cond, channel, config)					\
	dma_proxy_width_exception(cond, channel, config)

#define ISP_DMA_READ_DATA(cond, channel, to_addr, from_addr, width_A, width_B, num_sub_xfers)\
	dma_proxy_read(cond, channel, to_addr, from_addr)

#define ISP_DMA_WRITE_DATA(cond, channel, from_addr, to_addr, width_A, width_B, num_sub_xfers)\
	dma_proxy_write(cond, channel, from_addr, to_addr)

#define ISP_DMA_WAIT_FOR_ACK(channel, num_sub_xfers)						\
	dma_proxy_ack(channel)

#define ISP_DMA_WAIT_FOR_ACK_N(channel, num_sub_xfers, n, recv)					\
	dma_proxy_ack_n(channel, n, 0, recv)

#ifdef __HIVECC
#define ISP_DMA_ISP_VAR_TO_DDR(cond, ch, var_addr, ddr_addr, num_xfers)	\
	dma_proxy_write_byte_addr(cond, ch, ((char *)(var_addr)) + ISP0_DMEM_BASE, (void *)(ddr_addr))

#define ISP_DMA_DDR_TO_ISP_VAR(cond, ch, ddr_addr, var_addr, num_xfers)	\
	dma_proxy_read_byte_addr(cond, ch, ((char *)(var_addr)) + ISP0_DMEM_BASE, (void *)(ddr_addr))
#else
#define ISP_DMA_ISP_VAR_TO_DDR(cond, ch, var_addr, ddr_addr, num_xfers)	\
	dma_proxy_write_byte_addr(cond, ch, (void *)(var_addr), (void *)(ddr_addr))

#define ISP_DMA_DDR_TO_ISP_VAR(cond, ch, ddr_addr, var_addr, num_xfers)	\
	dma_proxy_read_byte_addr(cond, ch, (void *)(var_addr), (void *)(ddr_addr))
#endif

#define ISP_DMA_SET_CHANNEL_WIDTH_AB(cond, ch, width_A, width_B)	\
	dma_proxy_set_width_AB(cond, ch, width_A, width_B)

#define ISP_DMA_SET_CHANNEL_ELEMENTS_AND_CROPPING_A(cond, channel, elements, cropping)	\
	dma_proxy_set_channel_elements_and_cropping_A(cond, channel, elements, cropping)

#define ISP_DMA_SET_CHANNEL_ELEMENTS_AND_CROPPING_B(cond, channel, elements, cropping)	\
	dma_proxy_set_elem_cropping_b(cond, channel, elements, cropping)

#ifdef dma_proxy_set_height
#define ISP_DMA_SET_HEIGHT(cond, ch, height)	\
	dma_proxy_set_height(cond, ch, height)
#endif

#endif /* __DMA_PROXY_COMMANDS_H */
