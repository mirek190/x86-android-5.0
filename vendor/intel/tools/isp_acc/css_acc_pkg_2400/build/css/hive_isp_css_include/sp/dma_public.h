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

#ifndef __DMA_PUBLIC_H_INCLUDED__
#define __DMA_PUBLIC_H_INCLUDED__

#include <stdbool.h>

/*! Write to a control register of DMA[ID]

 \param	ID[in]				DMA identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, DMA[ID].ctrl[reg] = value
 */
STORAGE_CLASS_DMA_H void dma_reg_store(
	const dma_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value);

/*! Conditional write to a control register of DMA[ID]
 
 \param	ID[in]				DMA identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written
 \param	cnd[in]				predicate

 \return none, if(cnd) DMA[ID].ctrl[reg] = value
 */
STORAGE_CLASS_DMA_H void cnd_dma_reg_store(
	const dma_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value,
	const bool			cnd);

/*! Read from a control register of DMA[ID]
 
 \param	ID[in]				DMA identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return DMA[ID].ctrl[reg]
 */
STORAGE_CLASS_DMA_C hrt_data dma_reg_load(
	const dma_ID_t		ID,
	const unsigned int	reg);

#endif /* __DMA_PUBLIC_H_INCLUDED__ */
