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

#ifndef __VAMEM_PRIVATE_H_INCLUDED__
#define __VAMEM_PRIVATE_H_INCLUDED__

#include "vamem_public.h"

#include "isp.h"

#if !defined(IS_ISP_2400_MAMOIADA) && !defined(IS_ISP_2401_MAMOIADA)
#error "vamem_private.h for ISP_2400_MAMOIADA only"
#endif

#include "assert_support.h"

#include "isp_support.h"

STORAGE_CLASS_VAMEM_C tvector OP_vec_vmaddr_c (
	const tvector			in0,
	const unsigned short	lutsize_log2)
{
	tvector			out0;
	unsigned short  addrrange= (1U<<(VAMEM_INTERP_STEP_LOG2+lutsize_log2))-1;
	unsigned short	addrshft = ISP_VMEM_ELEMBITS-VAMEM_INTERP_STEP_LOG2-1-lutsize_log2;
OP___assert(addrshft<ISP_VMEM_ELEMBITS);
OP___assert(addrrange<(VAMEM_TABLE_UNIT_SIZE<<VAMEM_INTERP_STEP_LOG2));
	out0 = OP_vec_asr_c (CASTVEC1W(in0), addrshft);
	out0 = OP_vec_clipz_c (out0, addrrange);
return out0;
}

/*
STORAGE_CLASS_VAMEM_C tvector OP_vec_vmldo_lookup (
	_CoreElement			VAMEM_ID,
	const unsigned short	offset,
	const tvector			in0)
{
	tvector	out0;
	vec_vmldo(VAMEM_ID, offset, (CASTVEC1W(in0)<<VAMEM_INTERP_STEP_LOG2), out0);
return out0;
}
*/

#endif /* __VAMEM_PRIVATE_H_INCLUDED__ */
