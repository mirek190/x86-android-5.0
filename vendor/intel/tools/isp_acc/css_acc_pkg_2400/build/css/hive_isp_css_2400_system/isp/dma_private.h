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

#ifndef __DMA_PRIVATE_H_INCLUDED__
#define __DMA_PRIVATE_H_INCLUDED__

#include "assert_support.h"
#include "dma_public.h"

#include "isp.h"

STORAGE_CLASS_DMA_C void dma_reg_store(
	const dma_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value)
{
/*	unsigned char	fifo_addr = DMA0_FIFO_ADDRESS; */
	hrt_data		token = HRT_DMA_PACK(reg, value);
OP___assert(ID < N_DMA_ID);
	OP_std_snd(DMA0_FIFO_ADDRESS, token);
return;
}

STORAGE_CLASS_DMA_C void cnd_dma_reg_store(
	const dma_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value,
	const bool			cnd)
{
	if (cnd)
		dma_reg_store(ID, reg, value);
return;
}

STORAGE_CLASS_DMA_C hrt_data dma_reg_load(
	const dma_ID_t		ID,
	const unsigned int	reg)
{
	unsigned char	fifo_addr = DMA0_FIFO_ADDRESS;
/* Here we should need a read command or something alike */
	hrt_data		token = HRT_DMA_PACK(reg, 0);
OP___assert(ID < N_DMA_ID);
/*	OP_std_snd(fifo_addr, token); */
	token = OP_std_rcv(fifo_addr);
return HRT_DMA_UNPACK(reg, token);
}

#endif /* __DMA_PRIVATE_H_INCLUDED__ */
