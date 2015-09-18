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

#ifndef __DEBUG_PUBLIC_H_INCLUDED__
#define __DEBUG_PUBLIC_H_INCLUDED__

#include "stdbool.h"

#include "system_types.h"

typedef struct debug_data_s		debug_data_t;
typedef struct debug_data_ddr_s	debug_data_ddr_t;

/* extern debug_data_t				*debug_data_ptr; */
extern hrt_vaddress				debug_buffer_ddr_address;

/* STORAGE_CLASS_DEBUG_H bool is_debug_buffer_full(void); */

STORAGE_CLASS_DEBUG_H bool is_isp_debug_buffer_full(void);

STORAGE_CLASS_DEBUG_H bool is_ddr_debug_buffer_full(void);

/* STORAGE_CLASS_DEBUG_H void debug_enqueue(
	const hrt_data		value); */

STORAGE_CLASS_DEBUG_H void debug_enqueue_isp(
	const hrt_data		value);

STORAGE_CLASS_DEBUG_H void debug_enqueue_ddr(void);

extern void debug_buffer_init(void);

extern void debug_buffer_init_isp(void);

extern void debug_buffer_set_ddr_addr(
	const hrt_vaddress ddr_addr);


#endif /* __DEBUG_PUBLIC_H_INCLUDED__ */
