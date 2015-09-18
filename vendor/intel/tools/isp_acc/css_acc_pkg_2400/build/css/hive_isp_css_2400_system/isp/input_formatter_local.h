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

#ifndef __INPUT_FORMATTER_LOCAL_H_INCLUDED__
#define __INPUT_FORMATTER_LOCAL_H_INCLUDED__

#include "input_formatter_global.h"

#include "isp.h"					/* FIFO ADRESSES */

#define IF_DIRECT_MAPPING

#ifdef IF_DIRECT_MAPPING
/*
 * With direct mapping the ID is the address, to make sure
 * that there is no compiler overhead and that the input
 * to OP_std_snd(), OP_std_rcv() is const
 */
#ifdef HIVECC_PROPAGATES_CONST
#error "input_formatter_local.h: No inline functions that propagate const ID's supported"
#else   /* HIVECC_PROPAGATES_CONST */
/* CID = Channel ID, to separate it from the regular (address map) ID */
#define INPUT_FORMATTER0_CID	INPUT_FORMATTER0_FIFO_ADDRESS
#define INPUT_FORMATTER1_CID	INPUT_FORMATTER1_FIFO_ADDRESS

#define input_formatter_send_token(CID, token) OP_std_snd(CID, token)
#define cnd_input_formatter_send_token(CID, token, cnd) \
	if (cnd) { \
		input_formatter_send_token(CID, token); \
	}

typedef unsigned short	input_formatter_CID_t;
#endif /* HIVECC_PROPAGATES_CONST */

#else  /* IF_DIRECT_MAPPING */
#error "input_formatter_local.h: Only direct mapping of ID's supported"
#endif /* IF_DIRECT_MAPPING */

#endif /* __INPUT_FORMATTER_LOCAL_H_INCLUDED__ */
