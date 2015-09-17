/*
 * Copyright (C) 2010-2013 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * Developped by Eff'Innov Technologies : contact@effinnov.com
 */


/*!
 * \file phHciWrapperApi.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef PHHCINFCAPI_H
#define PHHCINFCAPI_H

#include "phNfcTypes.h"
#include "phOsalNfc.h"
#include "phNfcStatus.h"

/**
 * @brief This API is used by phHciWrapperCore to send and receive HCI packet.
 */

/**
 * @defgroup groupHCIPorting HCI Porting Layer
 */

/**
 * @defgroup groupHCICallBack HCI CallBack API
 */

/**
 * @brief Initialize the underlying HCI stack.
 * @ingroup groupHCIPorting
 *
 * @param [out]stackContext private context of underlying HCI stack.
 * This handle should be allocated by this function. This function must not block.
 * When initialzation is completed, you must call phHciWrapper_InitComplete() only if
 * NFCSTATUS_PENDING is returned.
 *
 * @param [in]libNfcCtx LibNfc context, this context should be passed for calling read/write/init/deinit callbacks.
 * @retval NFCSTATUS_PENDING if no error, phHciWrapper_InitComplete() will be called later by implementation.
 * @retval NFCSTATUS_SUCCESS if no error. Implementation does not need to call phHciWrapper_InitComplete().
 **/
NFCSTATUS phHciWrapper_Init(void** stackContext,void *libNfcCtx);

/**
 * @brief Release the underlying HCI stack.
 * @ingroup groupHCIPorting
 *
 * @param [in]stackContext context of underlying stack. This function should not block.
 * When deinitialzation is completed, you must call phHciWrapper_DeinitComplete(), or
 * return NFCSTATUS_SUCCESS instead of NFCSTATUS_PENDING.
 * @retval NFCSTATUS_PENDING if no error. phHciWrapper_DeinitComplete() will be called later by implementation.
 * @retval NFCSTATUS_SUCCESS if no error.
 **/
NFCSTATUS phHciWrapper_Deinit(void* stackContext);

/**
 * @brief Reset the underlying HCI stack.
 * @ingroup groupHCIPorting
 *
 * @param [in]stackContext context of underlying stack.
 * This function must not block.
 * When initialzation is completed, you must call phHciWrapper_InitComplete();
 * @retval NFCSTATUS_PENDING if no error, phHciWrapper_InitComplete() will be called later by implementation.
 * @retval NFCSTATUS_SUCCESS if no error. Implementation does not need to call phHciWrapper_InitComplete().
 **/
NFCSTATUS phHciWrapper_Reset(void* stackContext);


void phHciWrapper_EmergencyRelease();

/**
 * @brief Send HCI data.
 * This function must not block. When sending is completed, call phHciWrapper_SendComplete()
 * to notify LibNfc for asynchronous send.
 * @ingroup groupHCIPorting
 *
 * @param [in]stackContext context of underlying HCI stack.
 * @param data Data to send. This buffer remains valid until phHciWrapper_SendComplete() get called.
 * @param sz Size of data to send.
 * @retval NFCSTATUS_PENDING if no error. phHciWrapper_SendComplete() will be called later by implementation.
 * @retval NFCSTATUS_SUCCESS if no error. Implementation does not need to call phHciWrapper_SendComplete().
 **/

NFCSTATUS phHciWrapper_SendHCI(void* stackContext,uint8_t* data,size_t sz);

/**
 * @brief Receive HCI data.
 * This function must not block. When expected datas are received, call
 * phHciWrapper_ReceiveComplete() to notify LibNfc.
 * @ingroup groupHCIPorting
 *
 * @param [in]stackContext context of underlying HCI stack
 * @param [in]dataDestPtr Buffer that will contain received data. The Buffer is allocated by caller and remains
 * valid until phHciWrapper_ReceiveComplete() get called.
 * @param [in]sz size of Buffer/expected Data size.
 * @retval NFCSTATUS_PENDING if no error. phHciWrapper_ReceiveComplete() will be called later by implementation.
 * @retval NFCSTATUS_PENDING if no error. Implementation does not need to call phHciWrapper_ReceiveComplete().
 **/
NFCSTATUS phHciWrapper_ReceiveHCI(void* stackContext,uint8_t* dataDestPtr,size_t sz);


/**
 * @brief Call this function when Init or Reset is completed.
 * @ingroup groupHCICallBack
 *
 * @param [in]libNfcCtx LibNfc context provided during init.
 * @param [in]errCode NFCSTATUS_SUCCESS if no error.
 * @return void
 **/
void phHciWrapper_InitComplete(void* libNfcCtx, NFCSTATUS errCode);

/**
 * @brief Call this function when Deinit is completed.
 * @ingroup groupHCICallBack
 *
 * @param [in]libNfcCtx LibNfc context provided during init.
 * @param [in]errCode NFCSTATUS_SUCCESS if no error.
 * @return void
 **/
void phHciWrapper_DeinitComplete(void* libNfcCtx, NFCSTATUS errCode);

/**
 * @brief Call this function when HCI datas are sent.
 * @ingroup groupHCICallBack
 *
 * @param [in]libNfcCtx LibNfc context provided during init.
 * @param [in]errorCode NFCSTATUS_SUCCESS if no error.
 * @param [in]sz number of bytes sent.
 * @return void
 **/
void phHciWrapper_SendComplete(void *libNfcCtx,NFCSTATUS errorCode,size_t sz);

/**
 * @brief Call this function when HCI datas are received.
 * @ingroup groupHCICallBack
 *
 * @param [in]libNfcCtx LibNfc context provided during init.
 * @param [in]errorCode NFCSTATUS_SUCCESS if no error.
 * @param [in]dataDestPtr Buffer that will contains data. Must be equal to buffer provided by phHciWrapper_ReceiveHCI().
 * @param [in]sz size of received datas.
 * @return void
 **/
void phHciWrapper_ReceiveComplete(void *libNfcCtx,NFCSTATUS errorCode, uint8_t* dataDestPtr,size_t sz);


#endif // ndef PHHCINFCAPI_H
