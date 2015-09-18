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

#ifndef __GPIO_PUBLIC_H_INCLUDED__
#define __GPIO_PUBLIC_H_INCLUDED__

#include "system_types.h"

/*! Write to a control register of GP_DEVICE[ID]

 \param	ID[in]				GP_DEVICE identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return none, GP_DEVICE[ID].ctrl[reg] = value
 */
STORAGE_CLASS_GPIO_H void gpio_reg_store(
	const gpio_ID_t	ID,
	const unsigned int		reg_addr,
	const hrt_data			value);

/*! Read from a control register of GP_DEVICE[ID]
 
 \param	ID[in]				GP_DEVICE identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return GP_DEVICE[ID].ctrl[reg]
 */
STORAGE_CLASS_GPIO_H hrt_data gpio_reg_load(
	const gpio_ID_t	ID,
	const unsigned int		reg_addr);

#endif /* __GPIO_PUBLIC_H_INCLUDED__ */
