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

#ifndef __EVENT_FIFO_LOCAL_H
#define __EVENT_FIFO_LOCAL_H

/*
 * No global event information, all events come from
 * peer-to-peer connections not over a global IRQ
 *
 * Arguably the data here could also reside in "isp_local.h"
 */
//#include "event_global.h"

#include "isp.h"					/* FIFO ADRESSES */

typedef unsigned char	event_addr_t;

#define EVENT_DIRECT_MAPPING

#ifdef EVENT_DIRECT_MAPPING
/*
 * With direct mapping the ID is the address, to make sure
 * that there is no compiler overhead and that the input
 * to OP_std_snd(), OP_std_rcv() is const
 */
#ifdef HIVECC_PROPAGATES_CONST
typedef enum {
	INPUT_FORMATTER0_EVENT_ID = INPUT_FORMATTER0_FIFO_ADDRESS,
	INPUT_FORMATTER1_EVENT_ID = INPUT_FORMATTER1_FIFO_ADDRESS,
#ifdef USE_DMA_PROXY_EVENT
	DMA0_EVENT_ID = SP0_FIFO_ADDRESS,
#else  /* USE_DMA_PROXY_EVENT */
	DMA0_EVENT_ID = DMA0_FIFO_ADDRESS,
#endif /* USE_DMA_PROXY_EVENT */
	GDC0_EVENT_ID = GDC0_FIFO_ADDRESS,
	GDC1_EVENT_ID = GDC1_FIFO_ADDRESS,
	HOST0_EVENT_ID = HOST0_FIFO_ADDRESS,
	SP0_EVENT_ID = SP0_FIFO_ADDRESS,
	N_EVENT_ID = 7						/* Why does this look like a bad idea ?*/
} event_ID_t;
#else   /* HIVECC_PROPAGATES_CONST */
#define INPUT_FORMATTER0_EVENT_ID	INPUT_FORMATTER0_FIFO_ADDRESS
#define INPUT_FORMATTER1_EVENT_ID	INPUT_FORMATTER1_FIFO_ADDRESS
#ifdef USE_DMA_PROXY_EVENT
#define DMA0_EVENT_ID	SP0_FIFO_ADDRESS
#else  /* USE_DMA_PROXY_EVENT */
#define DMA0_EVENT_ID	DMA0_FIFO_ADDRESS
#endif /* USE_DMA_PROXY_EVENT */
#define GDC0_EVENT_ID	GDC0_FIFO_ADDRESS
#define GDC1_EVENT_ID	GDC1_FIFO_ADDRESS
#define HOST0_EVENT_ID	HOST0_FIFO_ADDRESS
#define SP0_EVENT_ID	SP0_FIFO_ADDRESS
#define N_EVENT_ID	7

#define event_wait_for(ID) (void)OP_std_rcv(ID)
#define cnd_event_wait_for(ID,cnd) \
	if (cnd) { \
		event_wait_for(ID); \
	}

#define event_receive_token(ID) OP_std_rcv(ID)
#define event_send_token(ID, token) OP_std_snd(ID, token)
#define cnd_event_send_token(ID, token, cnd) \
	if (cnd) { \
		event_send_token(ID, token); \
	}

typedef unsigned short	event_ID_t;
#endif /* HIVECC_PROPAGATES_CONST */

#else  /* DIRECT_MAPPING */

typedef enum {
	INPUT_FORMATTER0_EVENT_ID = 0,
	INPUT_FORMATTER1_EVENT_ID,
	DMA0_EVENT_ID,
	GDC0_EVENT_ID,
	GDC1_EVENT_ID,
	HOST0_EVENT_ID,
	SP0_EVENT_ID,
	N_EVENT_ID
} event_ID_t;

/* Events are read from FIFO */
static const event_addr_t event_source_addr[N_EVENT_ID] = {
	INPUT_FORMATTER0_FIFO_ADDRESS,
	INPUT_FORMATTER1_FIFO_ADDRESS,
#ifdef USE_DMA_PROXY_EVENT
	SP0_FIFO_ADDRESS,
#else  /* USE_DMA_PROXY_EVENT */
	DMA0_FIFO_ADDRESS,
#endif /* USE_DMA_PROXY_EVENT */
	GDC0_FIFO_ADDRESS,
	GDC1_FIFO_ADDRESS,
	HOST0_FIFO_ADDRESS,
	SP0_FIFO_ADDRESS};

/* Events are written to FIFO */
static const event_addr_t event_sink_addr[N_EVENT_ID] = {
	INPUT_FORMATTER0_FIFO_ADDRESS,
	INPUT_FORMATTER1_FIFO_ADDRESS,
#ifdef USE_DMA_PROXY_EVENT
	SP0_FIFO_ADDRESS,
#else  /* USE_DMA_PROXY_EVENT */
	DMA0_FIFO_ADDRESS,
#endif /* USE_DMA_PROXY_EVENT */
	GDC0_FIFO_ADDRESS,
	GDC1_FIFO_ADDRESS,
	HOST0_FIFO_ADDRESS,
	SP0_FIFO_ADDRESS};

#endif /* DIRECT_MAPPING */

#endif /* __EVENT_FIFO_LOCAL_H */
