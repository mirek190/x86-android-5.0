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
 * \file phDownloadWrapperApi.h
 * $Author: Eff'Innov Technologies $
 */


#ifndef PHDOWNLOADWRAPPERAPI_H
#define PHDOWNLOADWRAPPERAPI_H

#include "phNfcTypes.h"
#include "phOsalNfc.h"
#include "phNfcStatus.h"

/**
 * @brief This API is used by phDownloadWrapperCore to download firmware on device.
 */

/**
 * @defgroup groupDownloadPorting Download Porting Layer
 */

/**
 * @defgroup groupDownloadCallBack Download CallBack API
 */

/**
 * @ingroup groupDownloadPorting
 * @brief Initialize download process. After this call the device must be in download mode.
 * Synchronous function.
 *
 * @param [out]stackContext private context of underlying download stack. This handle should be allocated by this function.
 * @param [in]coreContext Download core context. Should be used when calling phDownloadWrapper_SendComplete() & phDownloadWrapper_ReceiveComplete() callbacks.
 * @retval NFCSTATUS_SUCCESS if no error.
 **/
NFCSTATUS phDownloadWrapper_InitDownloadMode(void** stackContext,void *coreContext);
/**
 * @ingroup groupDownloadPorting
 * @brief Deinitialize download process. After this call the device is no more in download mode.
 *
 * @param [in]stackContext private context of underlying download stack. After this call the context will be freed.
 * @retval NFCSTATUS_SUCCESS if no error.
 **/
NFCSTATUS phDownloadWrapper_DeInitDownloadMode(void* stackContext);

/**
 * @ingroup groupDownloadPorting
 * @brief Read packet from device.
 * This function must not block. When reading is completed, call phDownloadWrapper_ReceiveComplete()
 * to notify LibNfc for asynchronous read.
 *
 * @param [in]stackContext private context of underlying download stack.
 * @param [in]data buffer that will contains read datas.
 * @param [in]size size of the buffer.
 * @retval NFCSTATUS_PENDING if no error, read is pending.
 * @retval NFCSTATUS_SUCCESS if no error, datas are availables.
 **/
NFCSTATUS phDownloadWrapper_ReadPacket(void* stackContext,uint8_t *data,size_t size);

/**
 * @ingroup groupDownloadPorting
 * @brief Write packet to device.
 * This function must not block. When writing is completed, call phDownloadWrapper_SendComplete()
 * to notify LibNfc for asynchronous send.
 *
 * @param [in]stackContext private context of underlying download stack.
 * @param [in]data buffer to write
 * @param [in]size size of the buffer.
 * @retval NFCSTATUS_PENDING if no error, send is pending.
 * @retval NFCSTATUS_SUCCESS if no error, data were sent.
 **/

NFCSTATUS phDownloadWrapper_WritePacket(void* stackContext,uint8_t *data,size_t size);

// TBD
// NFCSTATUS phDownloadWrapper_SetPinStatus();

/*void phDownloadWrapper_InitComplete(void* coreContext, NFCSTATUS errCode);
void phDownloadWrapper_DeinitComplete(void* coreContext, NFCSTATUS errCode);*/


/**
 * @ingroup groupDownloadCallBack
 * @brief Call this function when asynchronous send is done.
 *
 * @param [in]coreContext Download core context provided during phDownloadWrapper_InitDownloadMode() call.
 * @param [in]errorCode NFCSTATUS code.
 * @param [in]sz size of data written.
 * @return void
 **/
void phDownloadWrapper_SendComplete(void *coreContext,NFCSTATUS errorCode,size_t sz);

/**
 * @ingroup groupDownloadCallBack
 * @brief Call this function when asynchronous read is done.
 *
 * @param [in]coreContext Download core context provided during phDownloadWrapper_InitDownloadMode() call.
 * @param [in]errorCode NFCSTATUS code.
 * @param [in]dataDestPtr buffer containing read data.
 * @param [in]sz size of read data.
 * @return void
 **/
void phDownloadWrapper_ReceiveComplete(void *coreContext,NFCSTATUS errorCode, uint8_t* dataDestPtr,size_t sz);



#endif // ndef PHDOWNLOADWRAPPERAPI_H
