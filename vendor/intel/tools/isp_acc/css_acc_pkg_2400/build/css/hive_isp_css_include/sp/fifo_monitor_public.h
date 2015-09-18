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

#ifndef __FIFO_MONITOR_PUBLIC_H_INCLUDED__
#define __FIFO_MONITOR_PUBLIC_H_INCLUDED__

/*! Set a channel SWITCH[switch_id] of FIFO_MONITOR[ID]
 
 \param	ID[in]				FIFO_MONITOR identifier
 \param	switch_id[in]		SWITCH identifier
 \param sel[in]				Selector value

 \return none, FIFO_MONITOR[ID].switch[switch_id] = sel
 */
STORAGE_CLASS_FIFO_MONITOR_H void fifo_switch_set(
	const fifo_monitor_ID_t		ID,
	const fifo_switch_t			switch_id,
	const hrt_data				sel);

/*! Get a channel SWITCH[switch_id] of FIFO_MONITOR[ID]
 
 \param	ID[in]				FIFO_MONITOR identifier
 \param	switch_id[in]		SWITCH identifier

 \return return FIFO_MONITOR[ID].switch[switch_id]
 */
STORAGE_CLASS_FIFO_MONITOR_H hrt_data fifo_switch_get(
	const fifo_monitor_ID_t		ID,
	const fifo_switch_t			switch_id);


/*
 * The FIFO monitor is not used as an independent device on the SP
 * its resources are supporting "event.c"
 */

/*! Write to a control register of FIFO_MONITOR[ID]
 
 \param	ID[in]				FIFO_MONITOR identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, FIFO_MONITOR[ID].ctrl[reg] = value
 *
STORAGE_CLASS_FIFO_MONITOR_H void fifo_monitor_reg_store(
	const fifo_monitor_ID_t		ID,
	const unsigned int			reg,
	const hrt_data				value);
 */

/*! Read from a control register of FIFO_MONITOR[ID]
 
 \param	ID[in]				FIFO_MONITOR identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return FIFO_MONITOR[ID].ctrl[reg]
 *
STORAGE_CLASS_FIFO_MONITOR_H hrt_data fifo_monitor_reg_load(
	const fifo_monitor_ID_t		ID,
	const unsigned int			reg);
 */

#endif /* __FIFO_MONITOR_PUBLIC_H_INCLUDED__ */
