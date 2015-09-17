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
 * \file phDownloadWrapperCore.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef PHDOWNLOADWRAPPERCORE_H
#define PHDOWNLOADWRAPPERCORE_H

#include "phNfcStatus.h"
#include "phNfcTypes.h"
#include "phDal4Nfc.h"

/**
 * @brief This API is used by LIBNFC for Download.
 */

/**
 * @defgroup groupDownloadWrapper Download Wrapper for LIBNFC (Internal)
 */

/**
 * @ingroup groupDownloadWrapper
 * @brief Init download mode.
 * This function does not block.
 *
 * @param [out]context pointer to hold Download context wrapper. Allocated by this function.
 * @param [in]dalContext DAL Context. (Used for callback)
 * @param [in]hwdRef Hardware reference. (Used for callback)
 * @retval NFCSTATUS_SUCCESS if no error.
 **/
NFCSTATUS phDownloadWrapperCore_InitDownloadMode(void **context, phDal4Nfc_SContext_t* dalContext,void *hwdRef);

/**
 * @ingroup groupDownloadWrapper
 * @brief Disable download mode.
 *
 * @param [in]context Pointer on context allocated by phDownloadWrapperCore_InitDownloadMode(). After this call, context will no more be valid.
 * @retval NFCSTATUS_SUCCESS if no error.
 **/
NFCSTATUS phDownloadWrapperCore_DeInitDownloadMode(void *context);

/**
 * @ingroup groupDownloadWrapper
 * @brief Read data from device in download mode.
 *
 * @param [in]context Pointer on context allocated by phDownloadWrapperCore_InitDownloadMode().
 * @param [in]data Buffer that will hold read data.
 * @param [in]size size of the buffer.
 * @param [in]hwRef hwref passed by Dal read ...
 * @retval NFCSTATUS_PENDING if no error.
 **/
NFCSTATUS phDownloadWrapperCore_ReadPacket(void* context,uint8_t *data,size_t size,void *hwRef);

/**
 * @ingroup groupDownloadWrapper
 * @brief Write data to device in download mode.
 *
 * @param [in]context Pointer on context allocated by phDownloadWrapperCore_InitDownloadMode().
 * @param [in]data Buffer that contains data to write.
 * @param [in]size size of buffer.
 * @param [in]hwRef hwref passed by Dal Write ...
 * @retval NFCSTATUS_PENDING if no error.
 **/
NFCSTATUS phDownloadWrapperCore_WritePacket(void* context,uint8_t *data,size_t size,void *hwRef);



#endif // ndef PHDOWNLOADWRAPPERCORE_H