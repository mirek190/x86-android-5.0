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

#ifndef __IBUF_CTRL_PUBLIC_H_INCLUDED__
#define __IBUF_CTRL_PUBLIC_H_INCLUDED__


#include "stdbool.h"	/* bool */

/*****************************************************
 *
 * Native command interface (NCI).
 *
 *****************************************************/
/**
 * @brief Configure the input-buffer controller.
 * Configure the input-buffer controller on behalf of the SID.
 *
 * @param[in]	id	The global unique ID of the input-buffer controller.
 * @param[in]	sid_id	The global unique ID of the SID.
 * @param[in]	cfg	The configuration of the input-buffer controller.
 */
STORAGE_CLASS_IBUF_CTRL_H void ibuf_ctrl_config(
		ibuf_ctrl_ID_t id,
		stream2mmio_sid_ID_t sid_id,
		ibuf_ctrl_cfg_t *cfg);

/**
 * @brief Activate the input-buffer controller.
 * Activate the input-buffer controller on behalf of the SID. Must always
 * call "ibuf_ctrl_sync" once afterwards.
 *
 * @param[in]	id	The global unique ID of the input-buffer controller.
 * @param[in]	sid_id	The global unique ID of the SID.
 */
STORAGE_CLASS_IBUF_CTRL_H void ibuf_ctrl_run(
		ibuf_ctrl_ID_t id,
		stream2mmio_sid_ID_t sid_id);

/**
 * @brief Make the input-buffer controller transfer a frame to the external buffer.
 * The input-buffer controller controls the SID and the DMA to write the sensor
 * data to the external buffer. Must always call "ibuf_ctrl_sync" once afterwards.
 *
 * @param[in]	id		The global unique ID of the input-buffer controller.
 * @param[in]	sid_id		The global unique ID of the SID.
 * @param[in]	dest_addr	The start address of the external buffer (the destination).
 */
STORAGE_CLASS_IBUF_CTRL_H void ibuf_ctrl_transfer(
		ibuf_ctrl_ID_t id,
		stream2mmio_sid_ID_t sid_id,
		uint32_t dest_addr);

/** end of native command interface */

#endif /* __IBUF_CTRL_PUBLIC_H_INCLUDED__ */
