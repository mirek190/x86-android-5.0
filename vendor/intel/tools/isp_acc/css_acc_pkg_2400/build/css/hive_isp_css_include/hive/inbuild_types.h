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

#ifndef __INBUILD_TYPES_H_INCLUDED__
#define __INBUILD_TYPES_H_INCLUDED__

/*
 * The DLI includes ISP headers that depend on HIVECC inbuild types
 * The impipe tool (to generate the pipeline) sees those types but
 * cannot obtain a definition. Choosing the CRUN path for the pipeline
 * generation is no option as it depends on system headers
 *
 * The fw generator can create a SP dmem debug file, which also requires
 * inbuild types
 *
 * For the above reasons this file gives (bogus) definitions to the
 * inbuild types
 */

#define _CoreElement	int
#define __int12			int
#define tvectors		int
#define tvectoru		int

#define __int14			short	/* 2300 */
#define __int17			int		/* 2400 */
#define __int18			int		/* 2401 */

#define Any				0
#define s1_sru			0

/*
 * or,...
 *
 * Why bother, just prevent inclusion of the headers that cause problems
 */
#define _customops_h
#define _CELL_SUPPORT_H

#endif /* __INBUILD_TYPES_H_INCLUDED__ */
