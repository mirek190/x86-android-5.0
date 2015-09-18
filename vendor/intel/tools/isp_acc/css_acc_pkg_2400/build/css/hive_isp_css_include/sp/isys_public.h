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

#ifndef __ISYS_PUBLIC_H_INCLUDED__
#define __ISYS_PUBLIC_H_INCLUDED__

#ifdef USE_INPUT_SYSTEM_VERSION_2401
STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_reset(
		void);

STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_channel_configure(
	input_system_channel_t		*me,
	input_system_channel_cfg_t	*cfg);

STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_channel_open(
	input_system_channel_t		*me);

STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_channel_transfer(
	input_system_channel_t		*me,
	uint32_t			dest_addr);

STORAGE_CLASS_INPUT_SYSTEM_H bool input_system_channel_sync(
	input_system_channel_t		*me);

STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_input_port_configure(
	input_system_input_port_t	*me,
	input_system_channel_t		*channel,
	input_system_channel_t		*md_channel,
	input_system_input_port_cfg_t	*cfg);

STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_input_port_open(
	input_system_input_port_t	*me,
	bool start_csi_rx_fe);

STORAGE_CLASS_INPUT_SYSTEM_H input_system_err_t input_system_input_port_close(
	input_system_input_port_t	*me,
	bool stop_csi_rx_fe);

#endif /* #ifdef USE_INPUT_SYSTEM_VERSION_2401 */

#endif /* __INPUT_SYSTEM_PUBLIC_H_INCLUDED__ */
