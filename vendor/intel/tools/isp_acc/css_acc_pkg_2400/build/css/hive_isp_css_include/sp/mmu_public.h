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

#ifndef __MMU_PUBLIC_H_INCLUDED__
#define __MMU_PUBLIC_H_INCLUDED__

#include "system_types.h"

/*! Invalidate the page table cache of MMU[ID]
 
 \param	ID[in]				MMU identifier

 \return none
 */
STORAGE_CLASS_EXTERN void mmu_invalidate_cache(
	const mmu_ID_t		ID,
	hrt_data		val);


/*! Write to a control register of MMU[ID]
 
 \param	ID[in]				MMU identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, MMU[ID].ctrl[reg] = value
 */
STORAGE_CLASS_MMU_H void mmu_reg_store(
	const mmu_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value);

/*! Read from a control register of MMU[ID]
 
 \param	ID[in]				MMU identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return MMU[ID].ctrl[reg]
 */
STORAGE_CLASS_MMU_H hrt_data mmu_reg_load(
	const mmu_ID_t		ID,
	const unsigned int	reg);

#endif /* __MMU_PUBLIC_H_INCLUDED__ */
