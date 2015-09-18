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

#ifndef __TIMED_CTRL_PUBLIC_H_INCLUDED__
#define __TIMED_CTRL_PUBLIC_H_INCLUDED__

#include "system_types.h"

/*! Write to a control register of TIMED_CTRL[ID]

 \param	ID[in]				TIMED_CTRL identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return none, TIMED_CTRL[ID].ctrl[reg] = value
 */
STORAGE_CLASS_TIMED_CTRL_H void timed_ctrl_reg_store(
	const timed_ctrl_ID_t	ID,
	const unsigned int		reg_addr,
	const hrt_data			value);

STORAGE_CLASS_TIMED_CTRL_H void timed_ctrl_snd_commnd(
	const timed_ctrl_ID_t	ID,
	hrt_data				mask,
	hrt_data				condition,
	hrt_data				counter,
	hrt_address				addr,
	hrt_data				value);

STORAGE_CLASS_TIMED_CTRL_H void timed_ctrl_snd_sp_commnd(
	const timed_ctrl_ID_t	ID,
	hrt_data				mask,
	hrt_data				condition,
	hrt_data				counter,
	const sp_ID_t			SP_ID,
	hrt_address				offset,
	hrt_data				value);

#if !defined(HAS_NO_GPIO)
STORAGE_CLASS_TIMED_CTRL_H void timed_ctrl_snd_gpio_commnd(
	const timed_ctrl_ID_t	ID,
	hrt_data				mask,
	hrt_data				condition,
	hrt_data				counter,
	const gpio_ID_t			GPIO_ID,
	hrt_address				offset,
	hrt_data				value);
#endif


#endif /* __GPIO_PUBLIC_H_INCLUDED__ */
