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

#ifndef __HMEM_PUBLIC_H_INCLUDED__
#define __HMEM_PUBLIC_H_INCLUDED__

#include <stddef.h>		/* size_t */

/*! Return the size of HMEM[ID]

 \param	ID[in]				HMEM identifier

 \Note: The size is the byte size of the area it occupies
		in the address map. I.e. disregarding internal structure

 \return sizeof(HMEM[ID])
 */
STORAGE_CLASS_HMEM_H size_t sizeof_hmem(
	const hmem_ID_t		ID);

/*! Read from (an ISP associated) HMEM[ID]

 \param	ID[in]				HMEM identifier
 \param	addr[in]			the address in HMEM
 \param data[in]			The virtual DDR address of data to be read
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function may depend on a DMA service
	- The DMA resource is aquired from a resource manager
	- This function may use an internal cache on the stack and
	  is hence not eligible for inlining

 \return none, data = HMEM[ID].hmem[addr...addr+size-1]
 */
STORAGE_CLASS_EXTERN void isp_hmem_load(
	const hmem_ID_t		ID,
	const unsigned int	addr,
	hrt_vaddress		data,
	const size_t		sizef);

/*! Zero initialise (an ISP associated) HMEM[ID]

 \param	ID[in]				HMEM identifier
 \param	addr[in]			the address in HMEM to clear
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function may depend on a DMA service
	- The DMA resource is aquired from a resource manager
	- This function may use an internal cache on the stack and
	  is hence not eligible for inlining

 \return none, HMEM[ID].hmem[addr...addr+size-1] = 0
 */
STORAGE_CLASS_EXTERN void isp_hmem_clear(
	const hmem_ID_t		ID,
	const unsigned int	addr,
	const size_t		size);

#endif /* __HMEM_PUBLIC_H_INCLUDED__ */
