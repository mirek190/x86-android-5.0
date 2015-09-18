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

#ifndef __dump_hive_c
#define __dump_hive_c

#define DUMP 0

#define KLINE (100000*KERNEL+__LINE__)

#define SC_KERNEL 	10
#define WB_KERNEL 	12
#define S3A_KERNEL	15
#define SDIS_KERNEL	17
#define SS_KERNEL 	18

#define DP_KERNEL 	20
#define DPA_KERNEL 	20
#define DPB_KERNEL	25

#define	DE_KERNEL	30
#define YNR_KERNEL	40
#define CNR_KERNEL	45
#define CSC_KERNEL	50
#define GC_KERNEL	60
#define CTC_KERNEL	62
#define GU_KERNEL	65
#define OUT_KERNEL	70
#define VF_KERNEL	80

#define SWP_KERNEL	90

static inline void
dump_int (int line, int b)
{
#if DUMP
  OP___dump (line, b);
#else
  (void)line; (void)b;
#endif
}

static inline void
dump_vector (int line, tvector b)
{
#if DUMP
  OP___dump (line, b);
#else
  (void)line; (void)b;
#endif
}

static inline void
dump_vectorslice (int line, tvectorslice b)
{
#if DUMP
  OP___dump (line, b);
#else
  (void)line; (void)b;
#endif
}

#endif /* __dump_hive_c */
