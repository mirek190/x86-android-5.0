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

#ifndef __INPUT_FORMATTER_PUBLIC_H_INCLUDED__
#define __INPUT_FORMATTER_PUBLIC_H_INCLUDED__

#include <stdbool.h>
#include "system_types.h"

/*! Write to a control register of INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return none, INPUT_FORMATTER[ID].ctrl[reg] = value
 */
STORAGE_CLASS_INPUT_FORMATTER_H void input_formatter_reg_store(
	const input_formatter_ID_t		ID,
	const unsigned int				reg_addr,
	const hrt_data					value);

/*! Read from a control register of INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	reg_addr[in]		register byte address
 \param value[in]			The data to be written

 \return INPUT_FORMATTER[ID].ctrl[reg]
 */
STORAGE_CLASS_INPUT_FORMATTER_H hrt_data input_formatter_reg_load(
	const input_formatter_ID_t		ID,
	const unsigned int				reg_addr);

/*! Reset INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier

 \return none, reset(INPUT_FORMATTER[ID])
 */
STORAGE_CLASS_INPUT_FORMATTER_H void input_formatter_rst(
	const input_formatter_ID_t		ID);

/*! Conditionally reset INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	cnd[in]				predicate

 \return none, if(cnd) reset(INPUT_FORMATTER[ID])
 */

STORAGE_CLASS_INPUT_FORMATTER_H void cnd_input_formatter_rst(
	const input_formatter_ID_t		ID,
	const bool						cnd);

/*! Configure INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	cfg[in]				configuration parameters

 \return none, config(INPUT_FORMATTER[ID])
 */
STORAGE_CLASS_INPUT_FORMATTER_H void input_formatter_cfg(
	const input_formatter_ID_t		ID,
	const input_formatter_cfg_t		*cfg);

/*! Conditionally configure INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	cfg[in]				configuration parameters
 \param	cnd[in]				predicate

 \return none, if(cnd) config(INPUT_FORMATTER[ID])
 */
STORAGE_CLASS_INPUT_FORMATTER_H void cnd_input_formatter_cfg(
	const input_formatter_ID_t		ID,
	const input_formatter_cfg_t		*cfg,
	const bool						cnd);

/*! Subscribe the source identified by {channel_id, format_type} to INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	channel_id[in]		(logical) channel identifier
 \param	format_type[in]		(MIPI) format of the channel

 \NOTE: An input formatter can subscribe multiple times to different
  sources but a source only to a single input formatter. A source
  in this context is the unique combination {channel_id, format_type}

 \This function will replace any previous configuration of the
  source with the new one. The function will return "false" in case
  of "set" and "true" in case of "reset", i.e. when the source was
  previously connected to a source (possibly the same)

 \return rst, INPUT_FORMATTER[ID].switch_state[ch][type] = true
 */
STORAGE_CLASS_INPUT_FORMATTER_H bool input_formatter_switch_cfg(
	const input_formatter_ID_t		ID,
	const uint8_t					channel_id,
	const uint8_t					format_type);

/*! Unsubscribe the source identified by {channel_id, format_type} from INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier
 \param	channel_id[in]		(logical) channel identifier
 \param	format_type[in]		(MIPI) format of the channel

 \NOTE: As an input formatter can subscribe multiple times to different
  sources, but a source only to a single input formatter, this function will
  check whether the connection existed before removing it

 \This function will only clear the connection if it existed

 \return INPUT_FORMATTER[ID].switch_state[ch][type], INPUT_FORMATTER[ID].switch_state[ch][type] -> false
 */
STORAGE_CLASS_INPUT_FORMATTER_H bool input_formatter_switch_clear(
	const input_formatter_ID_t		ID,
	const uint8_t					channel_id,
	const uint8_t					format_type);

/*! Unsubscribe the source identified by {channel_id, format_type} from any sink it was connected to
 
 \param	channel_id[in]		(logical) channel identifier
 \param	format_type[in]		(MIPI) format of the channel

 \NOTE: Disconnect the source unconditionally. This function returns the ID of the sink
  to which the source was connected. In case there was no connection it returns an invalid ID.

 \return id; (INPUT_FORMATTER[id].switch_state[ch][type] == true), INPUT_FORMATTER[any].switch_state[ch][type] -> false
 */
STORAGE_CLASS_INPUT_FORMATTER_H input_formatter_ID_t input_formatter_switch_clear_source(
	const uint8_t					channel_id,
	const uint8_t					format_type);

/*! Unsubscribe all the sources subscribed to INPUT_FORMATTER[ID]
 
 \param	ID[in]				INPUT_FORMATTER identifier

 \NOTE: As an input formatter can subscribe multiple times to different
  sources, and/or a source to multiple input formatters. The sources
  have to be cleared, rather than the input formatter switch to be reset

 \return count, INPUT_FORMATTER[ID].switch_state[any][any] -> false
 */
STORAGE_CLASS_INPUT_FORMATTER_H uint8_t input_formatter_switch_clear_sink(
	const input_formatter_ID_t		ID);

#endif /* __INPUT_FORMATTER_PUBLIC_H_INCLUDED__ */
