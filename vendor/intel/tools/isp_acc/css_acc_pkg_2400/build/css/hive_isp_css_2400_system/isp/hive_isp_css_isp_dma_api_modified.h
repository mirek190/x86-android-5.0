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

#ifndef _hive_isp_css_isp_dma_api_h_
#define _hive_isp_css_isp_dma_api_h_

#include "dma.h"

#include <hive/cell_support.h>
#include <dma_v2_api.h>

/* This ISP specific api adds 2 things on top of the dma_v2 API:
   - LSU and FIFO insertion
   - translate word addresses to byte address

   Important note: we assume A is the ISP and B is the remote device.
                   This is needed to translate word addresses into byte
                   addresses.
                   A side effect of this is that we can only initialize
                   local data, not remove data.
   */

#ifdef __HIVECC
#define HIVE_DMA_TR_ADDR(word_addr) (((unsigned int)(word_addr)) * ISP_VEC_ALIGN)
#else
#define HIVE_DMA_TR_ADDR(word_addr) (word_addr)
#endif

/* We cannot include the ids_hrt file here because it conflicts
   with existing device identifiers (such as GDC) */
/*
#include <isp2400_support.h>

#define _dma_bus_master HRTCAT(HRTCAT(DMA, _), HIVE_DMA_BUS_MASTER)

#define _dma_to_isp_mem_addr(mem) \
  (hrt_master_to_slave_address(_dma_bus_master, _hrt_cell_mem_slave_port(ISP, mem)) + hrt_mem_slave_port_address(ISP, mem))

#define _dma_to_isp_dmem_addr \
  _dma_to_isp_mem_addr(hrt_isp_dmem(ISP))
*/
/*
typedef enum {
  dma_zero_extension = dma_v2_zero_extension,
  dma_sign_extension = dma_v2_sign_extension
} dma_extension;
*/

/* default DMA has 3 connections */
/*
#ifndef HIVE_ISP_NUM_DMA_CONNS
#define HIVE_ISP_NUM_DMA_CONNS 3
#endif
*/
/*
typedef enum {
#if (HIVE_ISP_NUM_DMA_CONNS==2)
  dma_isp_to_ddr_connection = HIVE_DMA_ISP_DDR_CONN,
  dma_bus_to_ddr_connection = HIVE_DMA_BUS_DDR_CONN,
#else
  dma_isp_to_bus_connection = HIVE_DMA_ISP_BUS_CONN,
  dma_isp_to_ddr_connection = HIVE_DMA_ISP_DDR_CONN,
  dma_bus_to_ddr_connection = HIVE_DMA_BUS_DDR_CONN,
#endif
} dma_connection;

typedef unsigned int dma_channel;
*/
/*
static inline void
isp_dma_wait_for_ack(void)
{
  hive_dma_v2_wait_for_ack(dma);
}

static inline void
isp_dma_configure_channel(dma_channel ch, dma_connection conn, dma_extension ext, unsigned int height,
                          unsigned int stride_A, unsigned int elems_A, unsigned int cropping_A, unsigned int width_A,
                          unsigned int stride_B, unsigned int elems_B, unsigned int cropping_B, unsigned int width_B)
{
  hive_dma_v2_configure_channel(dma, ch, conn, ext, height,
                                stride_A, elems_A, cropping_A, width_A,
                                stride_B, elems_B, cropping_B, width_B);
}

static inline void
isp_dma_configure_dmem_ddr(dma_channel channel, unsigned int width, unsigned int height, dma_extension ext, unsigned int bits_per_element)
{
  unsigned int bus_word_bits      = 32,
               bus_word_bytes     = bus_word_bits/8,
               isp_elems_per_word = _hrt_ceil_div(bus_word_bits, bits_per_element),
               ddr_elems_per_word = _hrt_ceil_div(HIVE_ISP_DDR_WORD_BITS, bits_per_element),
               isp_words_per_line = _hrt_ceil_div(width, isp_elems_per_word),
               ddr_words_per_line = _hrt_ceil_div(width, ddr_elems_per_word);

  isp_dma_configure_channel (channel,
                             dma_bus_to_ddr_connection,
                             ext,
                             height,
                             isp_words_per_line * bus_word_bytes, 
                             isp_elems_per_word, 
                             0,
                             isp_words_per_line,
                             ddr_words_per_line * HIVE_ISP_DDR_WORD_BYTES,
                             ddr_elems_per_word,
                             0,
                             ddr_words_per_line);
}

static inline void
isp_dma_configure_vmem_ddr(dma_channel channel, unsigned int width, unsigned int height, dma_extension ext, unsigned int bits_per_element)
{
  unsigned int ddr_elems_per_word = HIVE_ISP_DDR_WORD_BITS/bits_per_element,
               isp_words_per_line = _hrt_ceil_div(width, ISP_VEC_NELEMS),
               ddr_words_per_line = _hrt_ceil_div(width, ddr_elems_per_word);

  isp_dma_configure_channel (channel,
                             dma_isp_to_ddr_connection,
                             ext,
                             height,
                             isp_words_per_line * ISP_VEC_ALIGN,
                             ISP_VEC_NELEMS,
                             0,
                             isp_words_per_line,
                             ddr_words_per_line * HIVE_ISP_DDR_WORD_BYTES,
                             ddr_elems_per_word,
                             0,
                             ddr_words_per_line);
}

static inline void
isp_dma_configure_dmem_ddr_unsigned(dma_channel channel, unsigned int width, unsigned int height, unsigned int bits_per_ddr_element)
{
  isp_dma_configure_dmem_ddr(channel, width, height, dma_zero_extension, bits_per_ddr_element);
}

static inline void
isp_dma_configure_dmem_ddr_signed(dma_channel channel, unsigned int width, unsigned int height, unsigned int bits_per_ddr_element)
{
  isp_dma_configure_dmem_ddr(channel, width, height, dma_sign_extension, bits_per_ddr_element);
}

static inline void
isp_dma_configure_vmem_ddr_unsigned(dma_channel channel, unsigned int width, unsigned int height, unsigned int bits_per_ddr_element)
{
  isp_dma_configure_vmem_ddr(channel, width, height, dma_zero_extension, bits_per_ddr_element);
}

static inline void
isp_dma_configure_vmem_ddr_signed(dma_channel channel, unsigned int width, unsigned int height, unsigned int bits_per_ddr_element)
{
  isp_dma_configure_vmem_ddr(channel, width, height, dma_sign_extension, bits_per_ddr_element);
}

#define isp_dma_write_data(ch, from_addr, to_addr) \
  hive_dma_v2_move_a2b_data(dma, ch, HIVE_DMA_TR_ADDR(from_addr), to_addr, dma_true, dma_false)

#define isp_dma_read_data(ch, to_addr, from_addr) \
  hive_dma_v2_move_b2a_data(dma, ch, HIVE_DMA_TR_ADDR(to_addr), from_addr, dma_true, dma_false)
*/
/*
 * Some regression application compiled with HIVECC have an issue with this interface
 *
static inline void
isp_dma_write_data(
    dma_channel     ch,
    volatile void   *from_addr,
    void            *to_addr)
{
hive_dma_v2_move_a2b_data(dma, ch, HIVE_DMA_TR_ADDR(from_addr), to_addr, dma_true, dma_false);
return;
}

static inline void 
isp_dma_read_data(
    dma_channel     ch,
    volatile void   *to_addr,
    void            *from_addr)
{
hive_dma_v2_move_b2a_data(dma, ch, HIVE_DMA_TR_ADDR(to_addr), from_addr, dma_true, dma_false);
return;
}
*/
/*
#define isp_dma_init_data(ch, addr, val) \
  hive_dma_v2_init_a_data(dma, ch, HIVE_DMA_TR_ADDR(addr), val, dma_true)
  
#define isp_dma_init_xmem(ch, addr, val) \
  hive_dma_v2_init_b_data(dma, ch, addr, val, dma_true)
*/
/* Functions to transfer between ddr and ISP variables */
/*
static inline void
isp_dma_isp_var_to_ddr(dma_channel ch, const void *var_addr, void *ddr_addr)
{
#ifdef __HIVECC
  hive_dma_v2_move_a2b_data(dma, ch, ((char*)var_addr) + _dma_to_isp_dmem_addr, ddr_addr, dma_true, dma_false);
#else
  hive_dma_v2_move_a2b_data(dma, ch, var_addr, ddr_addr, dma_true, dma_false);
#endif
}

static inline void
isp_dma_ddr_to_isp_var(dma_channel ch, const void *ddr_addr, void *var_addr)
{
#ifdef __HIVECC
  hive_dma_v2_move_b2a_data(dma, ch, ((char*)var_addr) + _dma_to_isp_dmem_addr, ddr_addr, dma_true, dma_false);
#else
  hive_dma_v2_move_b2a_data(dma, ch, var_addr, ddr_addr, dma_true, dma_false);
#endif
}

static inline void
isp_dma_init_isp_var(dma_channel ch, void *var_addr, int val)
{
#ifdef __HIVECC
  hive_dma_v2_init_a_data(dma, ch, ((char*)var_addr) + _dma_to_isp_dmem_addr, val, dma_true);
#else
  hive_dma_v2_init_a_data(dma, ch, var_addr, val, dma_true);
#endif
}

static inline void
isp_dma_set_channel_packing(dma_channel ch, dma_connection conn, dma_extension ext)
{
  hive_dma_v2_set_channel_packing(dma, ch, conn, ext);
}

static inline void
isp_dma_set_channel_stride_A(dma_channel channel, unsigned int stride)
{
  hive_dma_v2_set_channel_stride_A(dma, channel, stride);
}

static inline void
isp_dma_set_channel_elements_and_cropping_A(dma_channel channel, unsigned int elements, unsigned int cropping)
{
  hive_dma_v2_set_channel_elements_and_cropping_A(dma, channel, elements, cropping);
}
      
static inline void
isp_dma_set_channel_width_A(dma_channel channel, unsigned int width)
{
  hive_dma_v2_set_channel_width_A(dma, channel, width);
}
      
static inline void
isp_dma_set_channel_stride_B(dma_channel channel, unsigned int stride)
{
  hive_dma_v2_set_channel_stride_B(dma, channel, stride);
}

static inline void
isp_dma_set_channel_elements_and_cropping_B(dma_channel channel, unsigned int elements, unsigned int cropping)
{
  hive_dma_v2_set_channel_elements_and_cropping_B(dma, channel, elements, cropping);
}

static inline void
isp_dma_set_channel_width_B(dma_channel channel, unsigned int width)
{
  hive_dma_v2_set_channel_width_B(dma, channel, width);
}
      
static inline void
isp_dma_set_channel_height(dma_channel channel, unsigned int height)
{
  hive_dma_v2_set_channel_height(dma, channel, height);
}
*/
/*
static inline void
isp_dma_reset(void)
{
  hive_dma_v2_reset(dma);
}
*/
#ifndef HIVE_ISP_NO_SP

/* Functions to transfer between SP variables and ISP vectors */
/*
#include <sp_hrt.h>

#ifdef __HIVECC
#define _dma_sp_var_addr(var) \
  ((void*)(hrt_master_to_slave_address(_dma_bus_master, _hrt_cell_mem_slave_port(SP, hrt_sp_dmem(SP))) + \
           hrt_mem_slave_port_address(SP, hrt_sp_dmem(SP)) + \
          ((char*)(HRTCAT(HIVE_ADDR_,var)))))
#else
#define _dma_sp_var_addr(var)              ((void*)(var))
#endif

#define _dma_isp_vec_addr(vec,idx)         HIVE_DMA_TR_ADDR(&vec[idx])

#define isp_dma_isp_vector_to_sp_var(ch, vec, vec_idx, var) \
  hive_dma_v2_move_a2b_data(dma, ch, _dma_isp_vec_addr(vec, vec_idx), _dma_sp_var_addr(var), dma_true, dma_true)

#define isp_dma_sp_var_to_isp_vector(ch, var, vec, vec_idx) \
  hive_dma_v2_move_b2a_data(dma, ch, _dma_isp_vec_addr(vec, vec_idx), _dma_sp_var_addr(var), dma_true, dma_true)

#define isp_dma_init_sp_var(ch, var, val) \
  _isp_dma_init_sp_var(ch, _dma_sp_var_addr(var), val)
#define isp_dma_sp_var_to_ddr(ch, var, ddr_addr) \
  _isp_dma_sp_var_to_ddr(ch, _dma_sp_var_addr(var), ddr_addr) 
#define isp_dma_ddr_to_sp_var(ch, ddr_addr, var) \
  _isp_dma_ddr_to_sp_var(ch, ddr_addr, _dma_sp_var_addr(var))

static inline void
_isp_dma_init_sp_var(dma_channel ch, void *var_addr, int val)
{
  hive_dma_v2_init_a_data(dma, ch, var_addr, val, dma_true);
}
*/
/* Functions to transfer between ddr and SP variables */
/*
static inline void
_isp_dma_sp_var_to_ddr(dma_channel ch, const volatile void *var, void *ddr_addr)
{
  hive_dma_v2_move_a2b_data(dma, ch, var, ddr_addr, dma_true, dma_false);
}

static inline void
_isp_dma_ddr_to_sp_var(dma_channel ch, const void *ddr_addr, volatile void *var)
{
  hive_dma_v2_move_b2a_data(dma, ch, var, ddr_addr, dma_true, dma_false);
}
*/
#endif /* HIVE_ISP_NO_SP */

#endif /* _hive_isp_css_isp_dma_api_h_ */
