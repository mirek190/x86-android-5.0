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

#ifndef __DMA_PROXY_CMD_COMMON_H
#define __DMA_PROXY_CMD_COMMON_H

#include "dma_proxy.common.h"

static inline unsigned
_dma_proxy_get_data_from_token (unsigned token);

/************** inline functions on sh_dma_cmd's *************/

static inline unsigned
VMEM_ADDR(
	unsigned token,
#ifdef __ISP
	sh_dma_cmd *buf)
#else
	volatile sh_dma_cmd MEM(SP_XMEM) *buf) /* ISP buffer */
#endif
{
#if defined(DMA_USE_PACKED_CMD_DATA)
	(void)buf;
	return _dma_proxy_get_data_from_token(token);
#else
	(void)token;
	return buf->args.read_write_cmd.vmem_addr;
#endif
}

static inline unsigned
VMEM_ADDR_SP(
	unsigned token,
	sh_dma_cmd *buf) /* SP buffer */
{
#if defined(DMA_USE_PACKED_CMD_DATA)
	(void)buf;
	return _dma_proxy_get_data_from_token(token);
#else
	(void)token;
	return buf->args.read_write_cmd.vmem_addr;
#endif
}

static inline unsigned
_dma_proxy_create_cmd_token(sh_dma_proxy_cmd cmd, sh_dma_proxy_chan channel)
{
	return (cmd << DMA_PROXY_TOKEN_CMD_LSB) +
	 (channel << DMA_PROXY_TOKEN_CHAN_LSB) +
	 (DMA_PROXY_TOKEN_DMA_KIND << DMA_PROXY_TOKEN_KIND_LSB);
}

static inline unsigned
_dma_proxy_create_cmd_data_token(sh_dma_proxy_cmd cmd, sh_dma_proxy_chan channel, unsigned data)
{
	return _dma_proxy_create_cmd_token(cmd, channel) +
	 ((unsigned)data << DMA_PROXY_TOKEN_DATA_LSB);
}

static inline void
store_cmd_data(
	sh_dma_proxy_cmd cmd, sh_dma_proxy_chan ch, unsigned data,
	sh_dma_cmd *buf)
{
#if defined(DMA_USE_PACKED_CMD_DATA)
	buf->cmd_ch_data = _dma_proxy_create_cmd_data_token(cmd, ch, data);
#else
	buf->cmd_ch_data = _dma_proxy_create_cmd_token(cmd, ch);
	buf->args.read_write_cmd.vmem_addr = (unsigned)data;
#endif
}

static inline bool
DMA_PROXY_NEEDS_ACK(sh_dma_proxy_cmd cmd)
{
	return cmd == DMA_PROXY_READ_CMD ||
	 cmd == DMA_PROXY_WRITE_CMD;
}

static inline sh_dma_proxy_chan
_dma_proxy_get_chan_from_token(unsigned token)
{
	return(sh_dma_proxy_chan)((token >> DMA_PROXY_TOKEN_CHAN_LSB) & DMA_PROXY_TOKEN_CHAN_MASK);
}

static inline sh_dma_proxy_cmd
_dma_proxy_get_cmd_from_token(unsigned token)
{
	return (sh_dma_proxy_cmd)((token >> DMA_PROXY_TOKEN_CMD_LSB)  & DMA_PROXY_TOKEN_CMD_MASK);
}

static inline sh_dma_proxy_token_kind
_dma_proxy_get_kind_from_token(unsigned token)
{
	return (sh_dma_proxy_token_kind)((token >> DMA_PROXY_TOKEN_KIND_LSB) & DMA_PROXY_TOKEN_KIND_MASK);
}

static inline unsigned
_dma_proxy_get_free_from_token(unsigned token)
{
	return (token >> DMA_PROXY_TOKEN_FREE_LSB) & DMA_PROXY_TOKEN_FREE_MASK;
}

#ifdef __ISP
static inline unsigned
_dma_proxy_rcv_data_token(void)
{
	return event_receive_token(SP0_FIFO_ADDRESS);
}
#endif

static inline unsigned
_dma_proxy_get_data_from_token (unsigned token)
{
#if defined(DMA_USE_PACKED_CMD_DATA)
	return (token >> DMA_PROXY_TOKEN_DATA_LSB) & DMA_PROXY_TOKEN_DATA_MASK;
#else
	(void)token;
	return _dma_proxy_rcv_data_token();
#endif
}

static inline unsigned
_irq_proxy_create_cmd_token(sh_irq_proxy_cmd cmd)
{
	return (cmd << DMA_PROXY_TOKEN_CMD_LSB) +
		(DMA_PROXY_TOKEN_IRQ_KIND << DMA_PROXY_TOKEN_KIND_LSB);
}

static inline sh_irq_proxy_cmd
_irq_proxy_get_cmd_from_token(unsigned token)
{
	return (sh_irq_proxy_cmd)((token >> DMA_PROXY_TOKEN_CMD_LSB) & DMA_PROXY_TOKEN_CMD_MASK);
}

#endif /* __DMA_PROXY_CMD_COMMON_H */
