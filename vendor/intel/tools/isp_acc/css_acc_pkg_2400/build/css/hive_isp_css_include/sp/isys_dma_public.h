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

#ifndef __ISYS_DMA_PUBLIC_H_INCLUDED__
#define __ISYS_DMA_PUBLIC_H_INCLUDED__


/*****************************************************
 *
 * Native command interface (NCI).
 *
 *****************************************************/
/**
 * @breif Configure the DMA in the legacy way.
 * Configure the DMA device and the DMA ports in the
 * same API.
 *
 * This way of configuration started since the Medfield
 * platform and will be over in the Broxton platform.
 *
 * @param[in]	id		The global unique ID of a DMA device.
 * @param[in]	cfg		The configuration of the DMA device.
 * @param[in]	src_port_cfg	The configuration of the source port.
 * @param[in]	dest_port_cfg	The configuration of the destination port.
 */
STORAGE_CLASS_ISYS2401_DMA_H void isys2401_dma_config_legacy(
	isys2401_dma_ID_t id,
	isys2401_dma_cfg_t *cfg,
	isys2401_dma_port_cfg_t *src_port_cfg,
	isys2401_dma_port_cfg_t *dest_port_cfg);
/** end of native command interface */

STORAGE_CLASS_ISYS2401_DMA_H void isys2401_dma_unit_test(void);

#endif /* __ISYS_DMA_PUBLIC_H_INCLUDED__ */
