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

#ifndef __INPUT_SYSTEM_PUBLIC_H_INCLUDED__
#define __INPUT_SYSTEM_PUBLIC_H_INCLUDED__

#ifdef USE_INPUT_SYSTEM_VERSION_2401
#include "isys_public.h"
#else

/*! Write to a control register of INPUT_SYSTEM[ID]
 
 \param	ID[in]				INPUT_SYSTEM identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, INPUT_SYSTEM[ID].ctrl[reg] = value
 */
STORAGE_CLASS_INPUT_SYSTEM_H void input_system_reg_store(
	const input_system_ID_t			ID,
	const hrt_address			reg,
	const hrt_data				value);

/*! Read from a control register of INPUT_SYSTEM[ID]
 
 \param	ID[in]				INPUT_SYSTEM identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return INPUT_SYSTEM[ID].ctrl[reg]
 */
STORAGE_CLASS_INPUT_SYSTEM_H hrt_data input_system_reg_load(
	const input_system_ID_t		ID,
	const hrt_address		reg);

/*! Run the data acquisition process on INPUT_SYSTEM[ID]
 
 \param	ID[in]				INPUT_SYSTEM identifier
 \param	source_id[in]			data source identifier
 \param is_2ppc[in]			double data rate operation

 \return none, run(ACQUISITION)
 */
STORAGE_CLASS_INPUT_SYSTEM_H void input_system_acquisition_run(
	const input_system_ID_t         ID,
	const input_system_source_t	source_id,
	const bool			is_2ppc);

/*! Stop the data acquisition process on INPUT_SYSTEM[ID]
 
 \param	ID[in]				INPUT_SYSTEM identifier
 \param	source_id[in]			data source identifier

 \return none, stop(ACQUISITION)
 */
STORAGE_CLASS_INPUT_SYSTEM_H void input_system_acquisition_stop(
	const input_system_ID_t         ID,
	const input_system_source_t	source_id);

/*! Conditionally configure the data acquisition process on INPUT_SYSTEM[ID]
 
 \param	ID[in]			INPUT_SYSTEM identifier
 \param	cfg[in]			configuration parameters
 \param ctrl[in]		control parameters
 \param cnd[in]			predicate

 \return none, if(cnd) cfg(ACQUISITION)
 */
extern void cnd_input_system_cfg(
	const input_system_ID_t         ID,
	const input_system_cfg_t        *cfg,
	const input_system_ctrl_t       *ctrl,
	bool                            cnd);

/*! Write to a control register of RECEIVER[ID]
 
 \param	ID[in]				RECEIVER identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, RECEIVER[ID].ctrl[reg] = value
 */
STORAGE_CLASS_INPUT_SYSTEM_H void receiver_reg_store(
	const rx_ID_t				ID,
	const hrt_address			reg,
	const hrt_data				value);

/*! Read from a control register of RECEIVER[ID]
 
 \param	ID[in]				RECEIVER identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return RECEIVER[ID].ctrl[reg]
 */
STORAGE_CLASS_INPUT_SYSTEM_H hrt_data receiver_reg_load(
	const rx_ID_t				ID,
	const hrt_address			reg);

/*! Write to a control register of PORT[port_ID] of RECEIVER[ID]
 
 \param	ID[in]				RECEIVER identifier
 \param	port_ID[in]			mipi PORT identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, RECEIVER[ID].PORT[port_ID].ctrl[reg] = value
 */
STORAGE_CLASS_INPUT_SYSTEM_H void receiver_port_reg_store(
	const rx_ID_t				ID,
	const mipi_port_ID_t			port_ID,
	const hrt_address			reg,
	const hrt_data				value);

/*! Read from a control register PORT[port_ID] of of RECEIVER[ID]
 
 \param	ID[in]				RECEIVER identifier
 \param	port_ID[in]			mipi PORT identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return RECEIVER[ID].PORT[port_ID].ctrl[reg]
 */
STORAGE_CLASS_INPUT_SYSTEM_H hrt_data receiver_port_reg_load(
	const rx_ID_t				ID,
	const mipi_port_ID_t			port_ID,
	const hrt_address			reg);

STORAGE_CLASS_INPUT_SYSTEM_H void input_system_cfg(
	const bool		rst,
	const bool		binary_copy,
	const bool		from_isp);
#endif /* #ifdef USE_INPUT_SYSTEM_VERSION_2401 */

#endif /* __INPUT_SYSTEM_PUBLIC_H_INCLUDED__ */
