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

#ifndef __GDC_PUBLIC_H_INCLUDED__
#define __GDC_PUBLIC_H_INCLUDED__

#ifndef __INLINE_GDC__
#error "gdc_public.h: Inlining required"
#endif

/*! Write to a control register of GDC[ID]

 \param	ID[in]				GDC identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, GDC[ID].ctrl[reg] = value
 */
STORAGE_CLASS_GDC_H void gdc_reg_store(
	const gdc_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value);

/*! Read from a control register of GDC[ID]

 \param	ID[in]				GDC identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return GDC[ID].ctrl[reg]
 */
STORAGE_CLASS_GDC_H hrt_data gdc_reg_load(
	const gdc_ID_t		ID,
	const unsigned int	reg);

/*! Configure the scaling function of GDC[ID]

 \param	ID[in]				GDC identifier
 \param	cfg[in]				GDC scale configuration

 \implementation dependent
	- The configuration parameter (structure) depends
	  on both algorithm and cell.
	- The block parameter structure must be defined
	  before using this function. Consequently this
	  function must be inlined

 \use
	- The run-time part "gdc_scale()" can be implemented
	  elsewhere. The user must ensure that configuration
	  has been done by "gdc_scale_cfg()"

 \return scale!=1.0, scale_cfg(cfg)
 */
STORAGE_CLASS_GDC_H bool gdc_scale_cfg(
	const gdc_ID_t			ID,
	gdc_scale_cfg_t			*cfg);

/*! Configure the warp function of GDC[ID]

 \param	ID[in]				GDC identifier

 \implementation dependent
	- The configuration parameters are compile time
	  defined and dependent of application specific
	  settings

\use
	- The run-time part "gdc_warp()" can be implemented
	  elsewhere. The user must ensure that configuration
	  has been done by "gdc_warp_cfg()"

 \return none, GDC[ID].ctrl[reg] = cfg
 */
STORAGE_CLASS_GDC_H void gdc_warp_cfg(
	const gdc_ID_t			ID);

#endif /* _GDC_PUBLIC_H_INCLUDED_ */
