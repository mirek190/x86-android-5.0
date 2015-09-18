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

#ifndef __SP_PUBLIC_H_INCLUDED__
#define __SP_PUBLIC_H_INCLUDED__

#include <stddef.h>	/* size_t */

/*! Write to DMEM of SP[ID]
 
 \param	ID[in]				SP identifier
 \param	addr[in]			the address in DMEM (written to)
 \param data[in]			The virtual DDR address of data to be written
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM

 \return none, SP[ID].dmem[addr...addr+size-1] = data
 */
STORAGE_CLASS_SP_H void sp_dmem_store(
	const sp_ID_t		ID,
	unsigned int		addr,
	const hrt_vaddress	data,
	const size_t		size);

/*! Read from to DMEM of SP[ID]
 
 \param	ID[in]				SP identifier
 \param	addr[in]			the address in DMEM (read from)
 \param data[in]			The virtual DDR address of data read to
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM

 \return none, data = SP[ID].dmem[addr...addr+size-1]
 */
STORAGE_CLASS_SP_H void sp_dmem_load(
	const sp_ID_t		ID,
	const unsigned int	addr,
	hrt_vaddress		data,
	const size_t		size);

/*! Clear DMEM of SP[ID]
 
 \param	ID[in]				SP identifier
 \param	addr[in]			the address in DMEM (to be cleared)
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM

 \return none, SP[ID].dmem[addr...addr+size-1] = 0
 */
STORAGE_CLASS_SP_H void sp_dmem_clear(
	const sp_ID_t		ID,
	unsigned int		addr,
	const size_t		size);

#endif /* __SP_PUBLIC_H_INCLUDED__ */
