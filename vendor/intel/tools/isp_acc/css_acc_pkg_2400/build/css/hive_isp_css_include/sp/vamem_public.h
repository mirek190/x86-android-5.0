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

#ifndef __VAMEM_PUBLIC_H_INCLUDED__
#define __VAMEM_PUBLIC_H_INCLUDED__

#include <stddef.h>		/* size_t */

/*! Write to (an ISP associated) VAMEM[ID]

 \param	ID[in]				VAMEM identifier
 \param	addr[in]			the address in VAMEM
 \param data[in]			The virtual DDR address of data to be written
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function may depend on a DMA service
	- The DMA resource is aquired from a resource manager
	- This function may use an internal cache on the stack and
	  is hence not eligible for inlining

 \return none, VAMEM[ID].vamem[addr...addr+size-1] = data
 */
STORAGE_CLASS_EXTERN void isp_vamem_store(
	const vamem_ID_t	ID,
	unsigned int		addr,
	const hrt_vaddress	data,
	const size_t		size);

#endif /* __VAMEM_PUBLIC_H_INCLUDED__ */
