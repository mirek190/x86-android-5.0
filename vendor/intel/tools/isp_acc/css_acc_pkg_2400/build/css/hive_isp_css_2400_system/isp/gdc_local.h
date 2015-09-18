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

#ifndef __GDC_LOCAL_H_INCLUDED__
#define __GDC_LOCAL_H_INCLUDED__

#include "gdc_global.h"

#define HRT_GDC_COORD_FRAC_BITS		(HRT_GDC_FRAC_BITS - HRT_GDC_COORD_SCALE_BITS)
#define HRT_GDC_COORD_ONE			(1 << HRT_GDC_COORD_FRAC_BITS)

/* NOTE: The packing commands do not mask the inputs, the user must check ranges */
#ifdef __HIVECC
#define HRT_GDC_PACK_TOKEN(cmd) (cmd)
#else
#define HRT_GDC_PACK_TOKEN(cmd) ((cmd) | (1 << HRT_GDC_CRUN_POS))
#endif

#define HRT_GDC_IXDIM_IDX		HRT_GDC_P0X_IDX
#define HRT_GDC_IYDIM_IDX		HRT_GDC_P0Y_IDX

/* "HRT_GDC_CONFIG_CMD" permits sending parameters to register addresses */
#define HRT_GDC_PACK(reg_id, value) \
	HRT_GDC_PACK_TOKEN((((unsigned int)(value)) << HRT_GDC_DATA_POS) | (((unsigned int)(reg_id)) << HRT_GDC_REG_ID_POS) | (HRT_GDC_CONFIG_CMD << HRT_GDC_CMD_POS))

#define HRT_GDC_UNPACK(reg_id, token) \
	0

/* "HRT_GDC_DATA_CMD" is the "commit" for all run modes, could have been coded by address */
#define HRT_GDC_PACK_SCALE(x_int, y_frac) \
	HRT_GDC_PACK_TOKEN( x_int | (y_frac << HRT_GDC_FRY_BIT_OFFSET) | (HRT_GDC_DATA_CMD << HRT_GDC_CMD_POS) )

#define HRT_GDC_PACK_WARP(p0x) \
	HRT_GDC_PACK_TOKEN( p0x | (HRT_GDC_DATA_CMD << HRT_GDC_CMD_POS) )

#endif /* __GDC_LOCAL_H_INCLUDED__ */
