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

#ifndef __ISYS_STREAM2MMIO_PUBLIC_H_INCLUDED__
#define __ISYS_STREAM2MMIO_PUBLIC_H_INCLUDED__


/*****************************************************
 *
 * Native command interface (NCI).
 *
 *****************************************************/
/**
 * @brief Configure the Stream2MMIO.
 * Configure the Stream2MMIO on behalf of a SID.
 *
 * @paran[in]	id		The global unique ID of the Stream2MMIO.
 * @param[in]	sid_id	The ID of the SID.
 * @param[in]	cfg		The configuration of the Stream2MMIO.
 */
STORAGE_CLASS_STREAM2MMIO_H void stream2mmio_config(
	stream2mmio_ID_t id,
	stream2mmio_sid_ID_t sid_id,
	stream2mmio_cfg_t *cfg);

STORAGE_CLASS_STREAM2MMIO_H void stream2mmio_send_command(
	stream2mmio_ID_t id,
	stream2mmio_sid_ID_t sid_id,
	uint32_t cmd);
/** end of native command interface */

#endif /* __ISYS_STREAM2MMIO_PUBLIC_H_INCLUDED__ */
