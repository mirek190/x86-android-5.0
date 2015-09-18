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

#ifndef __CSI_RX_PUBLIC_H_INCLUDED__
#define __CSI_RX_PUBLIC_H_INCLUDED__


/*****************************************************
 *
 * Native command interface (NCI).
 *
 *****************************************************/
/**
 * @brief Reset the CSI receiver backend.
 *
 * Reset the CSI receiver backend as the follows:
 *	1. Disable all short-packet LUT registers.
 *
 * @param[in]	id	The global unique ID of the backend.
 */
STORAGE_CLASS_CSI_RX_H void  csi_rx_backend_rst(
	csi_rx_backend_ID_t	id);

/**
 * @brief Configure the CSI receiver backend.
 * Configure the CSI receiver backend on behalf of the SID.
 *
 * @param[in]	id		The global unique ID of the backend.
 * @param[in]	sid_id	The ID of the SID.
 * @param[in]	cfg		The configuration of the backend.
 */
STORAGE_CLASS_CSI_RX_H void csi_rx_backend_config(
	csi_rx_backend_ID_t id,
	stream2mmio_sid_ID_t sid_id,
	csi_rx_backend_cfg_t *cfg);

/**
 * @brief Configure the CSI receiver frontend.
 * Configure the CSI receiver frontend on behalf of the SID.
 *
 * @param[in]	id		The global unique ID of the frontend.
 * @param[in]	sid_id	The ID of the SID.
 * @param[in]	cfg		The configuration of the frontend.
 */
STORAGE_CLASS_CSI_RX_H void csi_rx_frontend_config(
	csi_rx_frontend_ID_t id,
	csi_rx_frontend_cfg_t *cfg);

/**
 * @brief Enable the CSI receiver backend.
 * Enable the CSI receiver backend. The backend can only be
 * enabled after all "per-SID" configuraions have been applied.
 *
 * @param[in]	id		The global unique ID of the backend.
 */
STORAGE_CLASS_CSI_RX_H void csi_rx_backend_run(
	csi_rx_backend_ID_t id);

/**
 * @brief Disable the CSI receiver backend.
 *
 * @param[in]	id		The global unique ID of the backend.
 */
STORAGE_CLASS_CSI_RX_H void csi_rx_backend_stop(
	csi_rx_backend_ID_t id);

STORAGE_CLASS_CSI_RX_H void csi_rx_backend_enable(
	csi_rx_backend_ID_t id);
STORAGE_CLASS_CSI_RX_H void csi_rx_backend_disable(
	csi_rx_backend_ID_t id);

/**
 * @brief Enalbe the CSI receiver frontkend.
 * Enable the CSI receiver frontend. The frontend can only be
 * enabled after all "per-SID" configuraions have been applied.
 *
 * @param[in]	id		The global unique ID of the frontend.
 */
STORAGE_CLASS_CSI_RX_H void csi_rx_frontend_run(
	csi_rx_frontend_ID_t id);

/**
 * @brief Disable the CSI receiver frontkend.
 *
 * @param[in]	id		The global unique ID of the frontend.
 */
STORAGE_CLASS_CSI_RX_H void csi_rx_frontend_stop(
	csi_rx_frontend_ID_t id);
/** end of native command interface */

#endif /* __CSI_RX_PUBLIC_H_INCLUDED__ */
