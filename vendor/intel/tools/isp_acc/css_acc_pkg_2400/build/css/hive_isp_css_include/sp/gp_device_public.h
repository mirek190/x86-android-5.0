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

#ifndef __GP_DEVICE_PUBLIC_H_INCLUDED__
#define __GP_DEVICE_PUBLIC_H_INCLUDED__

#include <stdbool.h>
#include "system_types.h"

/*! Write to a control register of GP_DEVICE[ID]

 \param	ID[in]				GP_DEVICE identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return none, GP_DEVICE[ID].ctrl[reg] = value
 */
STORAGE_CLASS_GP_DEVICE_H void gp_device_reg_store(
	const gp_device_ID_t	ID,
	const unsigned int		reg_addr,
	const hrt_data			value);

/*! Conditionally write to a control register of GP_DEVICE[ID]

 \param	ID[in]				GP_DEVICE identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written
 \param cnd[in]				The predicate

 \return none, if (cnd) GP_DEVICE[ID].ctrl[reg] = value
 */
STORAGE_CLASS_GP_DEVICE_H void cnd_gp_device_reg_store(
	const gp_device_ID_t	ID,
	const unsigned int		reg_addr,
	const hrt_data			value,
	const bool				cnd);

/*! Read from a control register of GP_DEVICE[ID]
 
 \param	ID[in]				GP_DEVICE identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return GP_DEVICE[ID].ctrl[reg]
 */
STORAGE_CLASS_GP_DEVICE_H hrt_data gp_device_reg_load(
	const gp_device_ID_t	ID,
	const hrt_address	reg_addr);

#endif /* __GP_DEVICE_PUBLIC_H_INCLUDED__ */
