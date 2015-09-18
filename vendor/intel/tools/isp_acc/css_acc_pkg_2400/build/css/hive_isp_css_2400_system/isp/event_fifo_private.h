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

#ifndef __EVENT_FIFO_PRIVATE_H
#define __EVENT_FIFO_PRIVATE_H

#include "event_fifo_public.h"

#include "isp.h"

#if !defined(IS_ISP_2400_MAMOIADA) && !defined(IS_ISP_2401_MAMOIADA)
#error "event_fifo_private.h for ISP_2400_MAMOIADA only"
#endif

#include "assert_support.h"

#ifdef HIVECC_PROPAGATES_CONST
STORAGE_CLASS_EVENT_C void event_wait_for(
	const event_ID_t		ID)
{
#ifdef EVENT_DIRECT_MAPPING
	(void)OP_std_rcv(ID);
#else
OP___assert(ID < N_EVENT_ID);
OP___assert(event_source_addr[ID]!=((event_addr_t)-1));
	(void)OP_std_rcv(event_source_addr[ID]);
#endif
return;
}

STORAGE_CLASS_EVENT_C void cnd_event_wait_for(
	const event_ID_t		ID,
	const bool				cnd)
{
	if (cnd) {
		event_wait_for(ID);
	}
return;
}

STORAGE_CLASS_EVENT_C hrt_data event_receive_token(
	const event_ID_t		ID)
{
#ifdef EVENT_DIRECT_MAPPING
return OP_std_rcv(ID);
#else
OP___assert(ID < N_EVENT_ID);
OP___assert(event_source_addr[ID] != ((event_addr_t)-1));
return OP_std_rcv(event_source_addr[ID]);
#endif
}

STORAGE_CLASS_EVENT_C void event_send_token(
	const event_ID_t		ID,
	const hrt_data			token)
{
#ifdef EVENT_DIRECT_MAPPING
	OP_std_snd(ID,token);
#else
OP___assert(ID < N_EVENT_ID);
OP___assert(event_sink_addr[ID] != ((event_addr_t)-1));
	OP_std_snd(event_sink_addr[ID], token);
#endif
return;
}

STORAGE_CLASS_EVENT_C void cnd_event_send_token(
	const event_ID_t		ID,
	const hrt_data			token,
	const bool				cnd)
{
	if (cnd) {
		event_send_token(ID, token);
	}
return;
}

#endif  /* HIVECC_PROPAGATES_CONST */

#endif /* __EVENT_FIFO_PRIVATE_H */
