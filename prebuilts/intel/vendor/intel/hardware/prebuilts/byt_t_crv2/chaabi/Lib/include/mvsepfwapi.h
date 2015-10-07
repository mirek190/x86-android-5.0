/**********************************************************************
 * Copyright 2009 - 2012 (c) Intel Corporation. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **********************************************************************/

#ifndef _MVSEPFWAPI_HDR_
#define _MVSEPFWAPI_HDR_


#include <inttypes.h>
#include "sepdrm-common.h"

#ifdef __cplusplus
extern "C"
{
#endif
int mvfw_sep_init();
sec_result_t mvsepfw_initialize();
sec_result_t mvsepfw_deinitialize();
sec_result_t mvsepfw_connect();
sec_result_t mvsepfw_disconnect();
sec_result_t mvsepfw_sendmessage( const uint8_t *pMsg, const uint32_t msgLen,
				  uint8_t *pRsp, uint32_t *pRspLen);

#ifdef __cplusplus
}
#endif

#endif
