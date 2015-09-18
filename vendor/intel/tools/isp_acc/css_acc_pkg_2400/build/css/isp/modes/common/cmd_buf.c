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

#include "assert_support.h"
#include "cmd_buf.h"

unsigned NO_SYNC io_requests = 0;
static unsigned NO_SYNC ack_request = 0;     /* Ack request has been sent */

static inline void
start_io_requests(void)
{
#if USE_DMA_CMD_BUFFER
	dma_proxy_buffer_start()
		SYNC (INPUT_BUF)    SYNC (OUTPUT_BUF)
		SYNC (VFOUT_Y_BUF)  SYNC (VFOUT_UV_BUF)
		SYNC (DMA_SYNC)     SYNC (DMA_PROXY_BUF);
#endif
}

static inline void
set_io_requests(void)
{
#if USE_DMA_CMD_BUFFER
	unsigned new_io_requests = (dma_proxy_use_buffer_cmds()
		SYNC (INPUT_BUF)    SYNC (OUTPUT_BUF)
		SYNC (VFOUT_Y_BUF)  SYNC (VFOUT_UV_BUF)
		SYNC (DMA_SYNC)     SYNC (DMA_PROXY_BUF));

	/* we can send a new request only after the previous one is finished. */
	OP___assert(new_io_requests == 0 || io_requests == 0);
	io_requests = new_io_requests;
#endif
}

/* Acknowledge all pending (input and) output requests in the command buffer */
static inline void
acknowledge_cmd_buffer(int ack)
{
#if USE_DMA_CMD_BUFFER
	unsigned requests = io_requests != 0;
	if (requests) {
		ISP_DMA_WAIT_FOR_ACK_N(0, 1, io_requests, ack)
			SYNC (INPUT_BUF)   SYNC (OUTPUT_BUF)
			SYNC (VFOUT_Y_BUF) SYNC (VFOUT_UV_BUF)
			SYNC (S3ATBL)      SYNC (DMA_SYNC)
			SYNC (DMA_PROXY_BUF);
	}
	ack_request = OP_std_pass(requests) ? !ack : ack_request;
	io_requests = 0;
#endif
}

static inline void
pre_acknowledge_io(void)
{
	acknowledge_cmd_buffer(0);
}

