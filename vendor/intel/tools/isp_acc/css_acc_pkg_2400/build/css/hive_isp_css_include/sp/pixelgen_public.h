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

#ifndef __PIXELGEN_PUBLIC_H_INCLUDED__
#define __PIXELGEN_PUBLIC_H_INCLUDED__

STORAGE_CLASS_PIXELGEN_H void pixelgen_tpg_config(
	pixelgen_ID_t id,
	stream2mmio_sid_ID_t sid_id,
	pixelgen_tpg_cfg_t *cfg);

STORAGE_CLASS_PIXELGEN_H void pixelgen_tpg_run(
	pixelgen_ID_t id);

STORAGE_CLASS_PIXELGEN_H bool pixelgen_tpg_is_done(
	pixelgen_ID_t id);

STORAGE_CLASS_PIXELGEN_H void pixelgen_tpg_stop(
	pixelgen_ID_t id);

STORAGE_CLASS_PIXELGEN_H void pixelgen_prbs_config(
	pixelgen_ID_t		id,
	stream2mmio_sid_ID_t sid_id,
	pixelgen_prbs_cfg_t	*cfg);

STORAGE_CLASS_PIXELGEN_H void pixelgen_prbs_run(
	pixelgen_ID_t id);

STORAGE_CLASS_PIXELGEN_H void pixelgen_prbs_stop(
	pixelgen_ID_t id);

STORAGE_CLASS_PIXELGEN_H void pixelgen_unit_test(
	void);

#endif /* __PIXELGEN_PUBLIC_H_INCLUDED__ */
