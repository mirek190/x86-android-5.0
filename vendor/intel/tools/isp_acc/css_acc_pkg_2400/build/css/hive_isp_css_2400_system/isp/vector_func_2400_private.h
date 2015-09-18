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

#ifndef __VECTOR_FUNC_PRIVATE_H_INCLUDED__
#define __VECTOR_FUNC_PRIVATE_H_INCLUDED__

#define PACK_INT(_a, _b)  ((_a&0x3FFF)|(_b<<ISP_VEC_ELEMBITS))

static inline void print_tvector2w(tvector2w data)
{
#ifndef C_RUN
	OP___dump(__LINE__, data.d0);
	OP___dump(__LINE__, data.d1);
#else
	for (unsigned int i = 0; i < ISP_NWAY; i++)
		printf("0x%08x ", PACK_INT(OP_vec_get(data.d0, i), OP_vec_get(data.d1, i)));
	printf("\n");
#endif
}

static inline void print_tvector1w(tvector1w data)
{
#ifndef C_RUN
	OP___dump(__LINE__, data);
#else
	for (unsigned int i = 0; i < ISP_NWAY; i++)
		printf("0x%08x ", OP_vec_get(data, i));
	printf("\n");
#endif
}

static inline void print_tflags(tflags data)
{
#ifndef C_RUN
	OP___dump(__LINE__, data);
#else
	for (unsigned int i = 0; i < ISP_NWAY; i++)
		printf("0x%08x ", (unsigned)((data>>i)&1));
	printf("\n");
#endif
}

#endif /* __VECTOR_FUNC_PRIVATE_H_INCLUDED__ */
