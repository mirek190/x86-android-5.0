/*++

   INTEL CONFIDENTIAL
   Copyright (c) 2007 - 2008 Intel Corporation. All Rights Reserved.
   The source code contained or described herein and all documents related
   to the source code ("Material") are owned by Intel Corporation or its
   suppliers or licensors. Title to the Material remains with Intel Corporation
   or its suppliers and licensors. The Material contains trade secrets and
   proprietary and confidential information of Intel or its suppliers and l
   icensors. The Material is protected by worldwide copyright and trade secret
   laws and treaty provisions. No part of the Material may be used, copied,
   reproduced, modified, published, uploaded, posted, transmitted, distributed,
   or disclosed in any way without Intel's prior express written permission.

   No license under any patent, copyright, trade secret or other intellectual
   property right is granted to or conferred upon you by disclosure or delivery
   of the Materials, either expressly, by implication, inducement, estoppel or
   otherwise. Any license under such intellectual property rights must be
   express and approved by Intel in writing.
*/

#ifndef _SEPMVDRM_HDR_
#define _SEPMVDRM_HDR_



#include "sepdrm-common.h"

#define IV_SIZE 16


#ifdef __cplusplus
extern "C"
{
#endif
sec_result_t sep_ext_drm_allocate_decrypt_buffer( const uint32_t sessionID,
						  uint32_t* IMRBufferSize,
						  uint32_t* IMRMemoryOffset );

sec_result_t sep_ext_drm_free_decrypt_buffer( uint32_t sessionID );

sec_result_t sep_ext_drm_mv_decrypt_slice( const uint32_t sessionID,
					   const uint8_t IV[IV_SIZE],
					   const uint32_t IMROffset,
					   const uint8_t* pCleartextBuffer,
					   const uint32_t cleartextSize,
					   const uint8_t* pCiphertextBuffer,
					   const uint32_t ciphertextSize);
#ifdef __cplusplus
}
#endif
#endif
