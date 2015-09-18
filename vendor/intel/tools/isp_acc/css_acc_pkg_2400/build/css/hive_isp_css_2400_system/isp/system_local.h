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

#ifndef __SYSTEM_LOCAL_H_INCLUDED__
#define __SYSTEM_LOCAL_H_INCLUDED__

#include "system_global.h"

#define HRT_ADDRESS_WIDTH	32

#include <stdint.h>
#ifdef __HIVECC     /* Not for CRUN */
typedef uint32_t			hrt_address;
typedef uint32_t			hrt_vaddress;
typedef uint32_t			hrt_data;
#endif

#ifndef PIPE_GENERATION

#ifdef __HIVECC
#define C_MEMORY(offset) (offset)
#else
#define C_MEMORY(offset) (0)
#endif

#define ISP0_DMEM_BASE	C_MEMORY(0x00200000UL)

/* This is the address where the DMA sees the ISP MEM */
static const unsigned int ISP_DMEM_BASE[N_ISP_ID] = {
	C_MEMORY(0x00200000UL)};

static const unsigned int ISP_BAMEM_BASE[N_BAMEM_ID] = {
	C_MEMORY(0x00000000UL)};

/* Dummy function to avoid compiler warnings about unused static variables
 * (declared above). */
STORAGE_CLASS_INLINE int
dummy_use_base_arrays(void)
{
	return ISP_DMEM_BASE[0] + ISP_BAMEM_BASE[0];
}
#endif

#endif /* __SYSTEM_LOCAL_H_INCLUDED__ */
