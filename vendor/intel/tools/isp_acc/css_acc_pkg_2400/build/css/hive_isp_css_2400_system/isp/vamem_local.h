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

#ifndef __VAMEM_LOCAL_H_INCLUDED__
#define __VAMEM_LOCAL_H_INCLUDED__

#include "vamem_global.h"

#include "isp.h"

/* Numbering in core should start from 0 */
#define VAMEM0 simd_vamem1
#define VAMEM1 simd_vamem2
#define VAMEM2 simd_vamem3

#define vec_vmldo_lookup(VAMEM_ID, offset, in, out) vec_vmldo(VAMEM_ID, offset, (in<<VAMEM_INTERP_STEP_LOG2), out)

#endif /* __VAMEM_LOCAL_H_INCLUDED__ */
