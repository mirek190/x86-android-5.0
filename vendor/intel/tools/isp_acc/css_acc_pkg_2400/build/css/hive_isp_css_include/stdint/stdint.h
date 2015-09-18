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

#ifndef __STDINT_H_INCLUDED__
#define __STDINT_H_INCLUDED__

/* stdint.h for ISP/SP code */
typedef unsigned char		uint8_t;
typedef signed char			int8_t;

typedef unsigned short		uint16_t;
typedef signed short		int16_t;

typedef unsigned int		uint32_t;
typedef signed int			int32_t;

typedef unsigned long long	uint64_t;
typedef signed long long	int64_t;

#define UINT8_MAX			0xff
#define UINT16_MAX			0xffff
#define UINT32_MAX			0xffffffffUL
#define UINT64_MAX			0xffffffffffffffffULL

#define INT8_MAX			0x7f
#define INT16_MAX			0x7fff
#define INT32_MAX			0x7fffffffL
#define INT64_MAX			0x7fffffffffffffffLL

#define INT8_MIN			0x80
#define INT16_MIN			0x8000
#define INT32_MIN			0x80000000L
#define INT64_MIN			0x8000000000000000LL

#endif /* __STDINT_H_INCLUDED__ */
