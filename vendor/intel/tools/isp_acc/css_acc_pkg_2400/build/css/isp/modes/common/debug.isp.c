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

/* some useful debugging functions */

// temporary function
static inline void
small_fix (void)
{
  tvector v_fix;

  v_fix = OP_vec_clone (1);
  v_fix = v_fix * 1;
}

static inline void
print_IF_BUFFER (unsigned int lines, unsigned int vectors_per_line)
{
  unsigned int j, i;

  for (j = 0; j < lines; j++) {
    for (i = 0; i < vectors_per_line; i++) {
      tvector vaux = input_buf[VECTORS_PER_LINE * j + i];

      info ("[line:%02d vector:%02d] ", j, i);
      OP___dump (__LINE__, vaux);
    }
  }
}

static inline void
print_DP_BUFFER (unsigned int lines, unsigned int vectors_per_line)
{
  unsigned int j, i;

  for (j = 0; j < lines; j++) {
    for (i = 0; i < vectors_per_line; i++) {
      tvector vaux = dp_buf[j][i];

      info ("[line:%02d vector:%02d] ", j, i);
      OP___dump (__LINE__, vaux);
    }
  }
}
