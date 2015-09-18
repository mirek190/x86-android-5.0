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

#include <stdbool.h>

/*! Write to a control register of GDC[ID].channel[channel_ID]

 \param	ID[in]				GDC identifier
 \param	channel[in]			GDC channel identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none,  GDC[ID].channel[channel_ID].ctrl[reg] = value
 */
STORAGE_CLASS_GDC_H void gdc_reg_store(
	const gdc_ID_t				ID,
	const gdc_channel_ID_t		channel,
	const unsigned int			reg,
	const hrt_data				value);

/*! Conditional write to a control register of  GDC[ID].channel[channel_ID]
 
 \param	ID[in]				GDC identifier
 \param	channel[in]			GDC channel identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written
 \param	cnd[in]				predicate

 \return none, if(cnd)  GDC[ID].channel[channel_ID].ctrl[reg] = value
 */
STORAGE_CLASS_GDC_H void cnd_gdc_reg_store(
	const gdc_ID_t				ID,
	const gdc_channel_ID_t		channel,
	const unsigned int			reg,
	const hrt_data				value,
	const bool					cnd);

/*! Read from a control register of  GDC[ID].channel[channel_ID]
 
 \param	ID[in]				GDC identifier
 \param	channel[in]			GDC channel identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return  GDC[ID].channel[channel_ID].ctrl[reg]
 */
STORAGE_CLASS_GDC_H hrt_data gdc_reg_load(
	const gdc_ID_t				ID,
	const gdc_channel_ID_t		channel,
	const unsigned int			reg);

/*! Translate GDC warp coordinates
 
 \param	param[in]			GDC warp parameters

 \implementation dependent
	- The warp parameter ranges depend on the cell
	- The warp parameter structure must be defined
	  before using this function. Consequently this
	  function must be inlined

 \use
	- Compute the translation of the coordinates
	  before applying the coordinates in the warp
	  function

 \return param = translate(param)
 */
STORAGE_CLASS_GDC_H gdc_warp_param_t gdc_warp_translate(
	gdc_warp_param_t		param);

/*! Perform warping of a data block using GDC[ID]
 
 \param	ID[in]				GDC identifier
 \param	param[in]			GDC warp parameters
 \param	in[in]				Input buffer address
 \param	out[out]			Output buffer address

 \implementation dependent
	- The warp parameter (structure) depends on
	  both algorithm and cell.
	- The warp parameter structure must be defined
	  before using this function. Consequently this
	  function must be inlined
	- The GDC computes from relative coordinates
	  hence the warp parameters are relative to
	  the origin of the buffer, not the frame
	- The function "gdc_warp_translate()" computes
	  the required translation

 \use
	- The configuration-time part "gdc_warp_cfg()" can be
	  implemented elsewhere. The user must ensure that
	  configuration has been done before calling "gdc_warp()"

 \return none, out = warp(in,param)
 */
/*
STORAGE_CLASS_GDC_H void gdc_warp(
	const gdc_ID_t			ID,
	gdc_warp_param_t		param,
	const gdc_data_ptr_t	*in,
	gdc_data_ptr_t			*out);
*/

/*! Perform scaling of a data block using GDC[ID]
 
 \param	ID[in]				GDC identifier
 \param	param[in]			GDC scale parameter descriptor
 \param	in[in]				Input buffer address "&X[0][0] of X[N][M]"
 \param	N[in]				The number of rows N of the input buffer
 \param	M[in]				The number of columns M of the input buffer
 \param	y_int[in]			The start row for scaling
 \param	y_frac[in]			The fractional part of the start row
 \param	out[out]			Output buffer address

 \decription
	- The input buffer is a rectangular matrix X[N][M]
	  of vectors, not elements.
	- The start offest "x_int, x_frac" are encoded on the
	  sclae parameter descriptor

 \implementation dependent
	- The scale parameter (structure) depends on
	  both algorithm and cell.
	- The scale parameter structure must be defined
	  before using this function. Consequently this
	  function must be inlined
	- The GDC supports processing from a circular
	  input buffer. The wrap around addresses are
	  compute from the rows "N" and parameters in "param"
	- The GDC computes from relative coordinates
	  hence the scale parameters are relative to
	  the origin or offset of the input buffer,
	  not the frame.
	- The start pointer to the GDC must be vector aligned,
	  the integer offset parameter is limited to the
	  element offset with a vector
	- Translation of the coordinates, i.e. to compute the
	  required offsets, must be part of "gdc_scale_cfg()" 

 \use
	- The configuration-time part "gdc_scale_cfg()" can be
	  implemented elsewhere. The user must ensure that
	  configuration has been done before calling "gdc_scale()"

 \return none, out = scale(in,param)
 */
/*
STORAGE_CLASS_GDC_H void gdc_scale(
	const gdc_ID_t			ID,
	gdc_scale_param_t		param,
	const gdc_data_ptr_t	*in,
	const unsigned short	N,
	const unsigned short	M,
	const unsigned short	y_int,
	const unsigned short	y_frac,
	gdc_data_ptr_t			*out);
*/
/*! Conditional scaling of a data block using GDC[ID]
 
 \param	ID[in]				GDC identifier
 \param	param[in]			GDC scale parameter descriptor
 \param	in[in]				Input buffer address "&X[0][0] of X[N][M]"
 \param	N[in]				The number of rows N of the input buffer
 \param	M[in]				The number of colums M of the input buffer
 \param	y_int[in]			The start row for scaling
 \param	y_frac[in]			The fractional part of the start row
 \param	out[out]			Output buffer address
 \param	cnd[in]				predicate

 \return none, if(cnd) out = scale(in,param)
 */
/*
STORAGE_CLASS_GDC_H void cnd_gdc_scale(
	const gdc_ID_t			ID,
	const gdc_scale_param_t	param,
	const gdc_data_ptr_t	*in,
	const unsigned short	N,
	const unsigned short	M,
	const unsigned short	y_int,
	const unsigned short	y_frac,
	gdc_data_ptr_t			*out,
	const bool				cnd);
*/

/*! Configure the warp function of GDC[ID]
 
 \param	ID[in]				GDC identifier
 \param	channel[in]			GDC channel identifier
 \param	cfg[in]				Configuration parameters

 \implementation dependent
	- The configuration parameters are compile time
	  defined and dependent of application specific
	  settings

\use
	- The run-time part "gdc_warp()" can be implemented
	  elsewhere. The user must ensure that configuration
	  has been done by "gdc_warp_cfg()"

 \note
	- This function is deprecated. There will be no ISP
	  side configuration

 \return none, GDC[ID].channel[channel_ID].ctrl[reg] = cfg
 */
STORAGE_CLASS_GDC_H void gdc_warp_cfg(
	const gdc_ID_t				ID,
	const gdc_channel_ID_t		channel,
	const gdc_warp_cfg_t		*cfg);

#endif /* __GDC_PUBLIC_H_INCLUDED__ */
