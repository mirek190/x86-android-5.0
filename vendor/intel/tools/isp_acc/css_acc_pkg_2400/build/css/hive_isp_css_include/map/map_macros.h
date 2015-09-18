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

#ifndef __MAP_MACROS_H_INCLUDED__
#define __MAP_MACROS_H_INCLUDED__

/*
 * Macros to get information out of the hivecc generated
 * mapfiles. Their use is deprecated, the information
 * should be obtained from the firmware handle
 */

/* Return the string that labels the memory store where the variable is mapped */
#define	MAP_MEMORY_OF(var) (HIVE_MEM_ ## var)
/* Return the address of the variable in the memory store */
#define	MAP_ADDRESS_OF(var) (HIVE_ADDR_ ## var)
/* Return the total size of the variable in the memory store */
#define	MAP_SIZE_OF(var) (HIVE_SIZE_ ## var)
/* Return the type of the (elements) of the variable (sorry, does not exist) */
#define	MAP_TYPE_OF(var) (HIVE_TYPE_ ## var)

#endif /* __MAP_MACROS_H_INCLUDED__ */
