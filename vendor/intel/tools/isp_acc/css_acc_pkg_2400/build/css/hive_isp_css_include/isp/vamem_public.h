/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2014 Intel Corporation.
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

#ifndef __VAMEM_PUBLIC_H_INCLUDED__
#define __VAMEM_PUBLIC_H_INCLUDED__

#include "vamem.h"
#include "isp.h"

/*! Align and clip data into an address vector for VAMEM

 \param	in0[in]			Data vector
 \param lutsize_log2[in]	(log2 of) the size of the VAMEM LUT

 \pre
	- The lutsize_log2 must be less than the ISP vector element
	  bitdepth

 \return out0 = clamp((in0>>lut_allign), 0, lut_range)
 */
STORAGE_CLASS_VAMEM_H tvector OP_vec_vmaddr_c (
	const tvector		in0,
	const unsigned short	lutsize_log2);

#endif /* __VAMEM_PUBLIC_H_INCLUDED__ */
