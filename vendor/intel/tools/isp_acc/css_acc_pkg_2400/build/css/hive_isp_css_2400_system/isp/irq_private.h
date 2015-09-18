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

#ifndef __IRQ_PRIVATE_H_INCLUDED__
#define __IRQ_PRIVATE_H_INCLUDED__

#include "irq_public.h"

#include "isp.h"

#ifndef __INLINE_EVENT__
#define __INLINE_EVENT__
#endif
#include "event_fifo.h"

#include "assert_support.h"

#include "dma_proxy_common.h"

STORAGE_CLASS_IRQ_C void irq_raise(
	const irq_ID_t				ID,
	const irq_sw_channel_id_t	irq_id)
{
OP___assert(ID == IRQ0_ID);
OP___assert(irq_id < N_IRQ_SW_CHANNEL_ID);
	event_send_token(SP0_EVENT_ID, _irq_proxy_create_cmd_token(IRQ_PROXY_RAISE_CMD));
return;
}

STORAGE_CLASS_IRQ_C void irq_raise_set_token(
	const irq_ID_t				ID,
	const irq_sw_channel_id_t	irq_id,
	const hrt_data				token)
{
OP___assert(ID == IRQ0_ID);
OP___assert(irq_id < N_IRQ_SW_CHANNEL_ID);
	event_send_token(SP0_EVENT_ID, _irq_proxy_create_cmd_token(IRQ_PROXY_RAISE_SET_TOKEN_CMD));
	event_send_token(SP0_EVENT_ID, token);
return;
}

#endif /* __IRQ_PRIVATE_H_INCLUDED__ */
