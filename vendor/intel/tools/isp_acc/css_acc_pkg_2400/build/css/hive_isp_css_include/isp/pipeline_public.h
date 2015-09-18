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

#ifndef __PIPELINE_PUBLIC_H_INCLUDED__
#define __PIPELINE_PUBLIC_H_INCLUDED__

/*
 * Optimisation may require the use of macros to circumvent address generation
 */

#if defined(__PARAM_BY_ADDRESS__)
/*! Get the handle for the CSC params from the pipeline param aggregate
 
 \param	pipeline_param[in]			Handle to the pipeline parameter aggregate
 \param	csc_kernel_param_set[in]	CS parameter set identifier

 \return CSC_params = PIPELINE_params(csc_kernel_param_set)
 */
STORAGE_CLASS_PIPELINE_H csc_kernel_param_h pipeline_get_csc_param_set(
	pipeline_param_h				pipeline_param,
	const csc_kernel_param_set_t	csc_kernel_param_set);
#endif

#endif /* __PIPELINE_PUBLIC_H_INCLUDED__ */
