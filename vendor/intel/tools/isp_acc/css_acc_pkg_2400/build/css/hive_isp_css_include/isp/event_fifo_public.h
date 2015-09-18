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

#ifndef __EVENT_FIFO_PUBLIC_H
#define __EVENT_FIFO_PUBLIC_H

#include <stdbool.h>
#include "system_types.h"

/*! Blocking wait for an event source EVENT[ID]
 
 \param	ID[in]				EVENT identifier

 \return none, dequeue(event_queue[ID])
 */
#ifdef HIVECC_PROPAGATES_CONST
STORAGE_CLASS_EVENT_H void event_wait_for(
	const event_ID_t		ID);
#endif

/*! Conditional blocking wait for an event source EVENT[ID]
 
 \param	ID[in]				EVENT identifier
 \param	cnd[in]				predicate

 \return none, if(cnd) dequeue(event_queue[ID])
 */
#ifdef HIVECC_PROPAGATES_CONST
STORAGE_CLASS_EVENT_H void cnd_event_wait_for(
	const event_ID_t		ID,
	const bool				cnd);
#endif

/*! Blocking read from an event source EVENT[ID]
 
 \param	ID[in]				EVENT identifier

 \return dequeue(event_queue[ID])
 */
#ifdef HIVECC_PROPAGATES_CONST
STORAGE_CLASS_EVENT_H hrt_data event_receive_token(
	const event_ID_t		ID);
#endif

/*! Blocking write to an event sink EVENT[ID]
 
 \param	ID[in]				EVENT identifier
 \param	token[in]			token to be written on the event

 \return none, enqueue(event_queue[ID])
 */
#ifdef HIVECC_PROPAGATES_CONST
STORAGE_CLASS_EVENT_H void event_send_token(
	const event_ID_t		ID,
	const hrt_data			token);
#endif

/*! Conditional blocking write to an event sink EVENT[ID]
 
 \param	ID[in]				EVENT identifier
 \param	token[in]			token to be written on the event
 \param	cnd[in]				predicate

 \return none,  if(cnd) enqueue(event_queue[ID])
 */
#ifdef HIVECC_PROPAGATES_CONST
STORAGE_CLASS_EVENT_H void cnd_event_send_token(
	const event_ID_t		ID,
	const hrt_data			token,
	const bool				cnd);
#endif

#endif /* __EVENT_FIFO_PUBLIC_H */
