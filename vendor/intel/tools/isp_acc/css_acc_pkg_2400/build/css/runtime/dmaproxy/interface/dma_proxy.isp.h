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

#ifndef __DMA_PROXY_ISP_H
#define __DMA_PROXY_ISP_H

#include "hive_isp_css_isp_dma_api_modified.h"	/* HIVE_DMA_TR_ADDR() */

#include "dma.h"

#ifndef __INLINE_EVENT__
#define __INLINE_EVENT__
#endif
#include "event_fifo.h"

#include "assert_support.h"

/* Packing reduces the address range, this is an issue for C_RUN and the 2400 in general */
#include "dma_proxy_cmd.common.h"   /* DMA_USE_PACKED_CMD_DATA */

#include "isp_types.h"
#include "isp_sync.h"

extern sh_dma_cmd SYNC_WITH(DMA_PROXY_BUF) sh_dma_cmd_buffer[MAX_DMA_CMD_BUFFER];
extern unsigned     NO_SYNC sh_dma_cmd_buffer_idx;
extern unsigned     NO_SYNC sh_dma_cmd_buffer_cnt;
extern unsigned     NO_SYNC sh_dma_cmd_buffer_need_ack;
extern unsigned     NO_SYNC sh_dma_cmd_buffer_enabled;


/* Low level functions */
static inline void
_dma_proxy_snd_cmd_token(sh_dma_proxy_cmd cmd, sh_dma_proxy_chan channel)
{
	event_send_token(SP0_EVENT_ID, _dma_proxy_create_cmd_token(cmd, channel));
}

static inline void
_dma_proxy_snd_data_token(unsigned data)
{
	event_send_token(SP0_EVENT_ID, (hrt_data)data);
}

static inline void
_dma_proxy_snd_cmd_data_token(sh_dma_proxy_cmd cmd, sh_dma_proxy_chan channel, unsigned data)
{
#if defined(DMA_USE_PACKED_CMD_DATA)
	event_send_token(SP0_EVENT_ID, _dma_proxy_create_cmd_data_token(cmd, channel, data));
#else
	_dma_proxy_snd_cmd_token(cmd, channel);
	_dma_proxy_snd_data_token(data);
#endif /* defined(DMA_USE_PACKED_CMD_DATA) */
}

static inline unsigned
_dma_proxy_rcv_ack_token(void)
{
	return event_receive_token(SP0_FIFO_ADDRESS);
}


/*
  Basic DMA proxy API
*/
static inline void
dma_proxy_configure(
	sh_dma_proxy_chan channel, dma_connection connection, dma_extension extension,
	unsigned height,
	unsigned stride_A, unsigned elems_A, unsigned cropping_A, unsigned width_A,
	unsigned stride_B, unsigned elems_B, unsigned cropping_B, unsigned width_B)
{
	_dma_proxy_snd_cmd_token(DMA_PROXY_CONFIGURE_CMD, channel);
	_dma_proxy_snd_data_token(connection);
	_dma_proxy_snd_data_token(extension);
	_dma_proxy_snd_data_token(height);
	_dma_proxy_snd_data_token(stride_A);
	_dma_proxy_snd_data_token(elems_A);
	_dma_proxy_snd_data_token(cropping_A);
	_dma_proxy_snd_data_token(width_A);
	_dma_proxy_snd_data_token(stride_B);
	_dma_proxy_snd_data_token(elems_B);
	_dma_proxy_snd_data_token(cropping_B);
	_dma_proxy_snd_data_token(width_B);
}

#define dma_proxy_buffer_start()                       \
{ \
	/* we can send a new request only after the previous one is finished. */ \
	/* make sure that sh_dma_cmd_buffer_enabled is initialized to zero at declaration */ \
	OP___assert(sh_dma_cmd_buffer_enabled == 0);\
	sh_dma_cmd_buffer_cnt      = OP_std_imm(0) ; \
	sh_dma_cmd_buffer_need_ack = OP_std_imm(0) ; \
	sh_dma_cmd_buffer_enabled  = OP_std_imm(1) ; \
}

static inline unsigned
dma_proxy_buffer_inc_idx(unsigned idx, int inc)
{
	return (idx + inc) & (MAX_DMA_CMD_BUFFER-1);
}

static inline void
_dma_proxy_buffer_add_cmd(int cond, sh_dma_proxy_cmd cmd, unsigned ch, unsigned _to, unsigned _from)
{
	unsigned _idx  = sh_dma_cmd_buffer_idx;

	OP___assert(sh_dma_cmd_buffer_enabled);

#if defined(DMA_USE_PACKED_CMD_DATA)
	sh_dma_cmd_buffer[_idx].cmd_ch_data = (OP_std_pass(_dma_proxy_create_cmd_data_token(cmd, ch, _to)) NO_CSE);
#else  /* defined(DMA_USE_PACKED_CMD_DATA) */
	sh_dma_cmd_buffer[_idx].cmd_ch_data = (_dma_proxy_create_cmd_token(cmd, ch) NO_CSE);
	sh_dma_cmd_buffer[_idx].args.read_write_cmd.vmem_addr   = _to;
#endif /* defined(DMA_USE_PACKED_CMD_DATA) */
	sh_dma_cmd_buffer[_idx].args.read_write_cmd.xmem_addr   = (OP_std_pass(_from) NO_CSE);
	sh_dma_cmd_buffer_idx = dma_proxy_buffer_inc_idx (_idx, cond);
	sh_dma_cmd_buffer_cnt = sh_dma_cmd_buffer_cnt + (cond);
	if (DMA_PROXY_NEEDS_ACK(cmd)) sh_dma_cmd_buffer_need_ack = sh_dma_cmd_buffer_need_ack + (cond);
}

static inline void
_dma_proxy_buffer_add_configure(
	int cond, unsigned ch, dma_connection connection,
	dma_extension extension, unsigned height,
	unsigned stride_A, unsigned elems_A, unsigned cropping_A, unsigned width_A,
	unsigned stride_B, unsigned elems_B, unsigned cropping_B, unsigned width_B)
{
	sh_dma_proxy_cmd cmd = DMA_PROXY_CONFIGURE_CMD;
	unsigned _idx  = sh_dma_cmd_buffer_idx;
	/*
	 * UDS can set the initial height to 0 and modify it before use
	 *
	OP___assert(height > 0);
	 */

	sh_dma_cmd_buffer[_idx].cmd_ch_data   = (OP_std_pass(_dma_proxy_create_cmd_token(cmd, ch)) NO_CSE NO_HOIST);
	sh_dma_cmd_buffer[_idx].args.configure_cmd.connection    = (dma_connection)(OP_std_pass(connection)  NO_CSE NO_HOIST);
	sh_dma_cmd_buffer[_idx].args.configure_cmd.extension     = extension;
	sh_dma_cmd_buffer[_idx].args.configure_cmd.height        = height;
	sh_dma_cmd_buffer[_idx].args.configure_cmd.stride_A      = stride_A;
	sh_dma_cmd_buffer[_idx].args.configure_cmd.stride_B      = stride_B;
	sh_dma_cmd_buffer[_idx].args.configure_cmd.elems_A_B     = (elems_A    <<16 | elems_B NO_CSE NO_HOIST);
	sh_dma_cmd_buffer[_idx].args.configure_cmd.cropping_A_B  = cropping_A <<16 | cropping_B;
	sh_dma_cmd_buffer[_idx].args.configure_cmd.width_A       = (OP_std_pass(width_A) NO_CSE NO_HOIST);
	sh_dma_cmd_buffer[_idx].args.configure_cmd.width_B       = (OP_std_pass(width_B) NO_CSE NO_HOIST);
	sh_dma_cmd_buffer_idx = dma_proxy_buffer_inc_idx (OP_std_pass(_idx), cond);
	sh_dma_cmd_buffer_cnt = sh_dma_cmd_buffer_cnt + (cond);
}

#define dma_proxy_buffer_add_cmd(cond, cmd, ch, to_addr, from_addr)                       \
{                                                                                   \
	/*OP___dump(__LINE__, 0); */ \
	_dma_proxy_buffer_add_cmd(cond, cmd, ch, (unsigned)(to_addr), (unsigned)(from_addr)); \
}

static inline void
dma_proxy_reconfigure_mem(
	int cond, sh_dma_proxy_chan channel, volatile struct dma_channel_config *config)
{
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_CONFIGURE_MEM_CMD, channel, 0, config);
}

static inline void
dma_proxy_configure_stream(
	int cond, sh_dma_proxy_chan channel, volatile struct sh_proxy_stream_config *config)
{
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_CONFIGURE_STREAM_CMD, channel, 0, config);
}

static inline void
dma_proxy_height_exception(
	int cond, sh_dma_proxy_chan channel, volatile struct sh_proxy_height_exception *config)
{
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_HEIGHT_EXCEPTION_CMD, channel, 0, config);
}

static inline void
dma_proxy_width_exception(
	int cond, sh_dma_proxy_chan channel, volatile struct sh_proxy_width_exception *config)
{
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_WIDTH_EXCEPTION_CMD, channel, 0, config);
}

#define dma_proxy_reconfigure(cond, channel, connection, extension, height,	\
			      stride_A, elems_A, cropping_A, width_A,		\
			      stride_B, elems_B, cropping_B, width_B)		\
	_dma_proxy_buffer_add_configure(cond, channel, (connection NO_CSE), extension, height, \
				         stride_A, elems_A, cropping_A, width_A,		\
				         stride_B, elems_B, cropping_B, width_B);

#define dma_proxy_read(cond, channel, to_addr, from_addr)                                 \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_READ_CMD,  channel, HIVE_DMA_TR_ADDR(to_addr), from_addr)

#define dma_proxy_write(cond, channel, from_addr, to_addr)                                \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_WRITE_CMD, channel, HIVE_DMA_TR_ADDR(from_addr), to_addr)

#define dma_proxy_read_byte_addr(cond, channel, to_addr, from_addr)                       \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_READ_CMD,  channel, to_addr, from_addr)

#define dma_proxy_write_byte_addr(cond, channel, from_addr, to_addr)                      \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_WRITE_CMD, channel, from_addr, to_addr)

#define dma_proxy_set_width_b(cond, channel, width_b)                 \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_SET_WIDTH_B_CMD, channel, width_b, 0)

#define dma_proxy_seek(channel, offset, origin)                 \
{ \
	_dma_proxy_snd_cmd_token(DMA_PROXY_SEEK_B_CMD, channel); \
	_dma_proxy_snd_data_token(offset);							\
	_dma_proxy_snd_data_token(origin);							\
}

#define dma_proxy_set_width_AB(cond, channel, width_a, width_b)                      \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_SET_WIDTH_AB_CMD, channel, width_a, width_b)

#define dma_proxy_set_channel_elements_and_cropping_A(cond, channel, elements, crop)                      \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_SET_ELEMENTS_AND_CROPPING_A_CMD, channel, elements, crop)

#define dma_proxy_set_elem_cropping_b(cond, channel, elements, crop)                      \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_SET_ELEMENTS_AND_CROPPING_B_CMD, channel, elements, crop)

#ifdef _DMA_HEIGHT_PARAM
#define dma_proxy_set_height(cond, channel, height)                      \
	dma_proxy_buffer_add_cmd(cond, DMA_PROXY_SET_HEIGHT_CMD, channel, height, 0)
#endif

#ifdef __HIVECC
#define dma_proxy_stream_get(cond, channel, to_addr) \
	dma_proxy_read_byte_addr(cond, channel, ((char*)(to_addr)) + ISP0_DMEM_BASE, 0xffffffff)

#define dma_proxy_stream_put(cond, channel, from_addr) \
	dma_proxy_write_byte_addr(cond, channel, ((char*)(from_addr)) + ISP0_DMEM_BASE, 0xffffffff)
#else
#define dma_proxy_stream_get(cond, channel, to_addr) \
	dma_proxy_read_byte_addr(cond, channel, to_addr, 0xffffffff)

#define dma_proxy_stream_put(cond, channel, from_addr) \
	dma_proxy_write_byte_addr(cond, channel, from_addr, 0xffffffff)
#endif
#define dma_proxy_vstream_get(cond, channel, to_addr) \
	dma_proxy_read_byte_addr(cond, channel, HIVE_DMA_TR_ADDR(to_addr), 0xffffffff)

#define dma_proxy_vstream_put(cond, channel, from_addr) \
	dma_proxy_write_byte_addr(cond, channel, HIVE_DMA_TR_ADDR(from_addr), 0xffffffff)


static unsigned inline _dma_proxy_use_buffer_cmds(bool single_ack)
{
	int cnt = sh_dma_cmd_buffer_cnt;
	int idx = dma_proxy_buffer_inc_idx(sh_dma_cmd_buffer_idx, -cnt);
	int need_ack = sh_dma_cmd_buffer_need_ack;

	if (cnt) {
		unsigned token = (idx << 8) + cnt;
		token += need_ack << 16;
		token += single_ack << 24;

		_dma_proxy_snd_cmd_token(DMA_PROXY_USE_BUFFER_CMDS, 0);
		_dma_proxy_snd_data_token(token);
	}
	sh_dma_cmd_buffer_enabled = (OP_std_imm(0) NO_HOIST);
	return need_ack;
}

/* Send command buffer to proxy and request one ack per request */
static unsigned inline dma_proxy_use_buffer_cmds(void)
{
	return _dma_proxy_use_buffer_cmds(false);
}

/* Send command buffer to proxy and request a single ack for all requests */
static unsigned inline dma_proxy_use_buffer_cmds_single (void)
{
	return _dma_proxy_use_buffer_cmds(true);
}

#define dma_proxy_ack_n(channel, n, check, recv)						\
{											\
	_dma_proxy_snd_cmd_data_token(DMA_PROXY_ACK_CMD, channel, ((n)<<1) + check);	\
	if (recv)									\
		_dma_proxy_rcv_ack_token();						\
}

#define dma_proxy_ack(channel) dma_proxy_ack_n(channel, 1, 1, 1)

#endif /* __DMA_PROXY_ISP_H */
