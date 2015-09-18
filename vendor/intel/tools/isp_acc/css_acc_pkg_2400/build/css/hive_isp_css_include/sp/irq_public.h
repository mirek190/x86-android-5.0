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

#ifndef __IRQ_PUBLIC_H_INCLUDED__
#define __IRQ_PUBLIC_H_INCLUDED__

#include "system_types.h"

/*#include "hive_isp_css_irq_types_hrt.h"*/	/* enum	hrt_isp_css_irq */

/*! Single field SW IRQ token
 *
 * It is visible on other cells
 */
extern hrt_data		irq_sw_interrupt_token;

/*! Write to a control register of IRQ[ID]
 
 \param	ID[in]				IRQ identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, IRQ[ID].ctrl[reg] = value
 */
STORAGE_CLASS_IRQ_H void irq_reg_store(
	const irq_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value);

/*! Read from a control register of IRQ[ID]
 
 \param	ID[in]				IRQ identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return IRQ[ID].ctrl[reg]
 */
STORAGE_CLASS_IRQ_H hrt_data irq_reg_load(
	const irq_ID_t		ID,
	const unsigned int	reg);

/*! Raise an interrupt on channel irq_id of device IRQ[ID]
 
 \param	ID[in]				IRQ (device) identifier
 \param	irq_id[in]			IRQ (channel) identifier
 
 \return none, signal(IRQ[ID].channel[irq_id])
 */
extern void irq_raise(
	const irq_ID_t				ID,
	const irq_sw_channel_id_t	irq_id);

/*! Set a token and raise an interrupt on channel irq_id of device IRQ[ID]
 
 \param	ID[in]				IRQ (device) identifier
 \param	irq_id[in]			IRQ (channel) identifier
 \param token[in]			Data token
 
 \Note: The tokens are not queued, only the last one is retained

 \return none, signal(IRQ[ID].channel[irq_id])
 */
extern void irq_raise_set_token(
	const irq_ID_t				ID,
	const irq_sw_channel_id_t	irq_id,
	const hrt_data				token);

#endif /* __IRQ_PUBLIC_H_INCLUDED__ */
