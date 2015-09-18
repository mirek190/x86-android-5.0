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

#ifndef __DMA_PROXY_COMMON_H
#define __DMA_PROXY_COMMON_H

#include "dma.h"


/*
  Utils
*/
#define _dma_proxy_width_mask(w)		  ((~(unsigned int)0) >> (8 * sizeof(unsigned int) - (w)))

/* Packing reduces the address/data range, this is an issue for C_RUN and the 2400 in general */
#if defined(__HIVECC) && !defined(IS_ISP_2400_SYSTEM)
#define DMA_USE_PACKED_CMD_DATA
#endif

/*
  Token
*/
typedef int sh_dma_proxy_token;
#define DMA_PROXY_TOKEN_BITS		      32
#if HIVE_ISP_NUM_DMA_CHANNELS == 8
#define DMA_PROXY_TOKEN_CHAN_BITS		  3
#elif HIVE_ISP_NUM_DMA_CHANNELS <= 16
#define DMA_PROXY_TOKEN_CHAN_BITS		  4
#else
#define DMA_PROXY_TOKEN_CHAN_BITS		  5
#endif
#define DMA_PROXY_TOKEN_CMD_BITS		   5
#define DMA_PROXY_TOKEN_KIND_BITS		  4
#define DMA_PROXY_TOKEN_FREE_BITS		 (DMA_PROXY_TOKEN_BITS - DMA_PROXY_TOKEN_CHAN_BITS - \
							DMA_PROXY_TOKEN_CMD_BITS - DMA_PROXY_TOKEN_KIND_BITS)
#define DMA_PROXY_TOKEN_DATA_BITS		 DMA_PROXY_TOKEN_FREE_BITS

#define DMA_PROXY_TOKEN_CHAN_LSB		   0
#define DMA_PROXY_TOKEN_CMD_LSB		   (DMA_PROXY_TOKEN_CHAN_LSB + DMA_PROXY_TOKEN_CHAN_BITS)
#define DMA_PROXY_TOKEN_KIND_LSB		  (DMA_PROXY_TOKEN_CMD_LSB  + DMA_PROXY_TOKEN_CMD_BITS)
#define DMA_PROXY_TOKEN_FREE_LSB		  (DMA_PROXY_TOKEN_KIND_LSB + DMA_PROXY_TOKEN_KIND_BITS)
#define DMA_PROXY_TOKEN_DATA_LSB		  DMA_PROXY_TOKEN_FREE_LSB

#define DMA_PROXY_TOKEN_CHAN_MASK		  _dma_proxy_width_mask(DMA_PROXY_TOKEN_CHAN_BITS)
#define DMA_PROXY_TOKEN_CMD_MASK		   _dma_proxy_width_mask(DMA_PROXY_TOKEN_CMD_BITS)
#define DMA_PROXY_TOKEN_KIND_MASK		  _dma_proxy_width_mask(DMA_PROXY_TOKEN_KIND_BITS)
#define DMA_PROXY_TOKEN_FREE_MASK		  _dma_proxy_width_mask(DMA_PROXY_TOKEN_FREE_BITS)
#define DMA_PROXY_TOKEN_DATA_MASK		  _dma_proxy_width_mask(DMA_PROXY_TOKEN_DATA_BITS)

typedef enum {
	DMA_PROXY_TOKEN_DMA_KIND,
	DMA_PROXY_TOKEN_IRQ_KIND,
	DMA_PROXY_TOKEN_UNKNOWN_KIND
} sh_dma_proxy_token_kind;

typedef enum {
	DMA_PROXY_RESP_DMA_ACK_KIND,
	DMA_PROXY_RESP_UNKNOWN_KIND
} sh_dma_proxy_resp_kind;

/*
  DMA commands
*/
typedef enum {
	DMA_PROXY_CONFIGURE_CMD,
	DMA_PROXY_CONFIGURE_MEM_CMD,
	DMA_PROXY_CONFIGURE_STREAM_CMD,
	DMA_PROXY_HEIGHT_EXCEPTION_CMD,
	DMA_PROXY_WIDTH_EXCEPTION_CMD,
	DMA_PROXY_READ_CMD,
	DMA_PROXY_WRITE_CMD,

	DMA_PROXY_SET_WIDTH_AB_CMD,
	DMA_PROXY_SET_ELEMENTS_AND_CROPPING_A_CMD,

	DMA_PROXY_ACK_CMD, /* TODO: remove this once streaming monitors work properly.
	                            Can simply poll for DMA ack without needing to send the command to the SP */
	DMA_PROXY_USE_BUFFER_CMDS,

	DMA_PROXY_SET_WIDTH_B_CMD,
	DMA_PROXY_SET_ELEMENTS_AND_CROPPING_B_CMD,
	DMA_PROXY_SET_HEIGHT_CMD,
	DMA_PROXY_SEEK_B_CMD,

	DMA_PROXY_UNKNOWN_CMD
} sh_dma_proxy_cmd;

/*
  IRQ commands
*/
typedef enum {
	DMA_PROXY_SEEK_SET,
	DMA_PROXY_SEEK_CUR, /* for completeness of interface, not implemented yet */
	DMA_PROXY_SEEK_END  /* for completeness of interface, not implemented yet */
} sh_dma_seek;

typedef enum {
	IRQ_PROXY_RAISE_CMD,
	IRQ_PROXY_RAISE_SET_TOKEN_CMD
} sh_irq_proxy_cmd;

typedef unsigned int sh_dma_proxy_chan;

typedef enum{
	DMA_PROXY_STATUS_IDLE,
	DMA_PROXY_STATUS_BUSY,
	DMA_PROXY_STATUS_ERROR,
	DMA_PROXY_STATUS_UNKNOWN
} sh_dma_proxy_status;

/* Command buffer in dmem of isp */

typedef struct {
	unsigned			 cmd_ch_data;
	union {
		struct {
#if !defined(DMA_USE_PACKED_CMD_DATA)
			unsigned	 vmem_addr;
#else
			/* Vmem address is packed in cmd_ch_data */
#endif
			unsigned	 xmem_addr;
		} read_write_cmd;
		struct {
			dma_connection		connection;
			dma_extension		 extension;
			unsigned		height;
			unsigned		stride_A;
			unsigned		stride_B;
			unsigned		elems_A_B;
			unsigned		cropping_A_B;
			unsigned		width_A;
			unsigned		width_B;
		} configure_cmd;
	} args;
	unsigned ack_to_isp;
} sh_dma_cmd;

struct sh_proxy_stream_config {
	int32_t inc_0;
	int32_t inc_1;
	int32_t max_0;
	int32_t max_1;
	int32_t addr_b;
};

#ifndef INT32_MAX
#define INT32_MAX ((1<<31)-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN (-(1<<31))
#endif

#define SH_PROXY_DEFAULT_STREAM_CONFIG \
{				\
	0,		/* inc_0 */	\
	0,		/* inc_1 */	\
	INT32_MAX,	/* max_0 */	\
	INT32_MAX,	/* max_1 */	\
	0            /* addr_b */ \
}

struct sh_proxy_height_exception {
	int32_t  it_id;
	uint16_t norm_height;
	uint16_t except_height;
};

struct sh_proxy_width_exception {
	int32_t  it_id;
	uint16_t norm_width_a;
	uint16_t except_width_a;
	uint16_t norm_width_b;
	uint16_t except_width_b;
};

/**********************/

#define MAX_DMA_CMD_BUFFER	64

/* SP and ISP both requires this */
extern sh_dma_cmd *sh_dma_cmd_ptr;

#endif /* __DMA_PROXY_COMMON_H */
