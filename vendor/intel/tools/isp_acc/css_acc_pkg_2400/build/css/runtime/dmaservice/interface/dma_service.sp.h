/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _DMA_SERVICE_SP_
#define _DMA_SERVICE_SP_

#include "type_support.h"
#include <stdbool.h>
/* Note on include "sp.h".(rvanimme)
 * Without the following include, CRUN gave a segmentation fault in hivesim when using
 * an event_fifo function that uses a master_port_load/store (e.g. is_event_pending).
 * I could not fully analyze/understand the issue but it turned out that the macro
 * master_port_load/store expanded to the incorrect function (see master_port.sim.h)
 * This was based on an “#ifdef HRT_MT_INT_LSU” flag. Without further detailed
 * investigation, it turned out that including “sp.h” made sure that this flag was defined
 * and that the correct master_port_load/store expansion happened.
 */
#include "sp.h"
#include "dma.h"
#include "hive_isp_css_mm_hrt.h"
#include "event_fifo.h"

#define DMA_CHANNEL_INVALID ((uint32_t)-1) /* This channel number is never used by dma-service */

enum dma_memory_device {
	DMA_VMEM = 0,
	DMA_BAMEM = 1,      /* explicit assignement to avoid any confusion in mapping
			   connection and bus. Each assignment is representing index of array for
			   mapping.*/
	DMA_DMEM_SP0 = 2,   /* DONT CHANGE THE VALUES - it will affect the mapping */
	DMA_DMEM_SP1 = 3,
	DMA_DMEM_ISP = 4,
	DMA_MMIO = 5,
	DMA_OSYS = 6,
	DMA_DDR = 7,
	MAX_NR_DMA_MEM_DEV  /* this should remain as last mem device */
};

typedef void (*dma_ack_callback) (void *context, uint32_t channel_id);

void dma_init(void);

/**
 * dma_acquire() - To acquire the channel in the DMA Service.
 *
 * This function acquires the channel available in the DMA service.
 *
 * Return: Channel_id
 */
uint32_t dma_acquire_channel(void);

/**
 * dma_release_channel() - To release channel in DMA Service.
 * @arg1:	Channel ID.
 *
 * This function releases the channel.
 *
 * Return: N.A.
 */
void dma_release_channel(uint32_t channel_id);

/**
 * dma_any_ack_pending() - To check ack status in DMA Service.
 * @arg1:	Channel ID.
 *
 * This function checks whether DMA Service is expecting any ack from the channels.
 *	It should be called when there is need to know whether all requests to DMA
 * service have been handled.
 *
 * Return: boolean result: True - If any ack pending, otherwise False
 */
bool dma_any_ack_pending(void);

/**
 * dma_configure_channel() - To configure the dma channel.
 * @arg1:	Channel ID.
 * @arg2:	Memory Device - "From" where data is being read.
 * @arg3:	Memory Device - "To" where data is being written.
 * @arg4:	bits_per_elem - bits packing in Memory Device.
 * @arg5:	Zero/Sign dma extension.
 * @arg6:	Cropping of the data (from the src).
 * @arg7:	Width of the data (num. of elems).
 * @arg8:	Height of the data in the memory.
 * @arg9:	Stride src (bytes)- in Memory Device(From).
 * @arg10:	Stride dest (bytes)- in Memory Device(To).
 *
 * This function configures the dma channel with the values provided
 * in the arguments.
 *
 * Return: N.A.
 */
void dma_configure_channel(uint32_t channel_id,
			   enum dma_memory_device from, enum dma_memory_device to,
			   uint32_t bits_per_elem, dma_extension ext,
			   uint32_t cropping, uint32_t width,
			   uint32_t height, uint32_t stride_src, uint32_t stride_dest);

/**
 * dma_request_transfer() - To request the data transfer to DMA.
 * @arg1:	Channel ID.
 * @arg2:	Source address (From).
 * @arg3:	Destination address (To).
 * @arg4:	Acknowledgement handler.
 * @arg5:	Argument to the Acknowledge handler.
 *
 * This function request the data transfer on the channel on the given
 * addresses. The acknowledgement of the request will be handled by given
 * acknowledge handler with arguments. In case, there is no need of an
 * acknowledgement, arg4 and arg5 should be given the value 'NULL'.
 *
 * Return: N.A.
 */
void dma_request_transfer(uint32_t channel_id,
			  hmm_ptr src_addr,
			  hmm_ptr dst_addr,
			  dma_ack_callback dma_ack_handler,
			  void *ack_arg);

/**
 * dma_ack_handle() - To handle the dma acknowledgement.
 *
 * This function handles all the dma acknowledgements coming on the Fifo of the SP.
 * It pops out and invokes the ack_handler of the request for which ack has arrrived.
 *	It should be called for any DMA acknowledgement event on the fifo of SP.
 *
 * Return: N.A.
 */
void dma_ack_handler(void);

/**
 * dma_set_height() - To set the height of the data.
 * @arg1:	Channel ID.
 * @arg2:	Height (new value).
 *
 * This function sets the new value of the height of the data.
 *
 * Return: N.A.
 */
void dma_set_height(uint32_t channel_id, uint32_t height);

/**
 * dma_set_width() - To set the width of the data.
 * @arg1:	Channel ID.
 * @arg2:	Width (Num of elems).
 *
 * This function sets the new value of the Width of the data in the Memory
 * device.
 *
 * Return: N.A.
 */
void dma_set_width(uint32_t channel_id, uint32_t width);

#endif

