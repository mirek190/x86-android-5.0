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

#ifndef __RESOURCE_PUBLIC_H_INCLUDED__
#define __RESOURCE_PUBLIC_H_INCLUDED__

typedef unsigned short			resource_h;

/*! Acquire RESOURCE[ID]
 
 \param	ID[in]				RESOURCE type identifier
 \param	reservation[in]		RESOURCE reservation type

 \decription
	- The resource manager maintains all resources
	  but this selection and hence "resource_type_ID_t"
	  is system dependent. resources are DMA-channels
	  buffers etc.
	- Resource reservations methods are also system
	  dependent and hence "resource_reservation_t"
	  reservations can be {permanent, persistent,
	  dedicated, shared}

 \return acquire(resource_pool[ID])
 */
STORAGE_CLASS_RESOURCE_H resource_h resource_acquire(
	const resource_type_ID_t		ID,
	const resource_reservation_t	reservation);

/*! Release RESOURCE[ID]
 
 \param	resource[in]		RESOURCE handle

 \return none, release(resource)
 */
STORAGE_CLASS_RESOURCE_H void resource_release(
	const resource_h				resource);

/*! Register RESOURCE[ID]
 
 \param	ID[in]				RESOURCE type identifier
 \param	resource[in]		RESOURCE handle

 \return none, add(resource_pool[ID], resource)
 */
STORAGE_CLASS_RESOURCE_H void resource_register(
	const resource_type_ID_t		ID,
	const resource_h				resource);

#endif /* __RESOURCE_PUBLIC_H_INCLUDED__ */
