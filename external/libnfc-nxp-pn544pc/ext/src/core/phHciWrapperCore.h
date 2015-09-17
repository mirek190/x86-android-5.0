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
 * \file phHciWrapperCore.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef PHHCINFCAPICORE_H
#define PHHCINFCAPICORE_H
#include "phNfcStatus.h"
#include "phNfcTypes.h"
#include "phHciNfc_Generic.h"

/**
 * @brief This API is used by LIBNFC for HCI Communication.
 */

/**
 * @defgroup groupHCIWrapper HCI Wrapper for LIBNFC (Internal)
 */

/**
 * @brief Init HCI Wrapper.
 * @ingroup groupHCIWrapper
 *
 * This function does not block.
 * Underlying implementation will call callbacks.
 *
 * @param [out]context Pointer to hold HCI Wrapper context. Allocated by wrapper.
 * @param [in]hciContext phHciNfc_sContext_t pointer.
 * @param [in]hwRef libnfc Hardware reference.
 * @return NFCSTATUS_PENDING If no error.
 **/
NFCSTATUS phHciWrapperCore_Init(void **context,void *hciContext,void *hwRef);

/**
 * @brief Release HCI Wrapper.
 * This function does not block.
 *
 * @ingroup groupHCIWrapper
 *
 * @param [in]context HCI Wrapper context allocated by phHciWrapperCore_Init().
 * @param [in]hciContext phHciNfc_sContext_t pointer. Sometimes LibNfc change it...
 * @param [in]hwRef libnfc Hardware reference. Sometimes LibNfc change it...
 * @return NFCSTATUS_SUCCESS if no error.
 **/
NFCSTATUS phHciWrapperCore_DeInit(void *context,void *hciContext,void *hwRef);
/**
 * @brief Reset HCI Wrapper.
 * Underlying implementation will call callbacks.
 *
 * @ingroup groupHCIWrapper
 *
 * @param [in]context HCI Wrapper context allocated by phHciWrapperCore_Init().
 * @param [in]hciContext phHciNfc_sContext_t pointer. Sometimes LibNfc change it...
 * @param [in]hwRef libnfc Hardware reference. Sometimes LibNfc change it...
 * @return NFCSTATUS_PENDING If no error.
 **/
NFCSTATUS phHciWrapperCore_Reset(void *context,void *hciContext,void *hwRef);

/**
 * @brief Release low layer communication layer.
 * Called only in case communication get blocked.
 *
 * @ingroup groupHCIWrapper
 *
 * @return void
 **/
void phHciWrapperCore_EmergencyRelease();

/**
 * @brief Send HCI Frame
 * HCI Fragmentation is handled by underlying layer.
 * Underlying implementation will call callbacks.
 *
 * @ingroup groupHCIWrapper
 *
 * @param [in]context HCI Wrapper context allocated by phHciWrapperCore_Init().
 * @param [in]data Data to send
 * @param [in]sz Size of data buffer to send.
 * @return NFCSTATUS_PENDING if no error.
 **/
NFCSTATUS phHciWrapperCore_SendHCI(void *context,uint8_t *data, size_t sz);
/**
 * @brief Read HCI Frame
 * HCI Fragmentation is handled by underlying layer.
 * Underlying implementation will call callbacks.
 *
 * @ingroup groupHCIWrapper
 *
 * @param [in]context HCI Wrapper context allocated by phHciWrapperCore_Init().
 * @param [in]data buffer to receive data. This buffer MUST be allocated by caller.
 * The buffer will be filled when callbacks get called.
 * @param [in]sz Expexted size (= size of the buffer)
 * @return NFCSTATUS_PENDING if no error.
 **/
NFCSTATUS phHciWrapperCore_ReceiveHCI(void *context,uint8_t *data, size_t sz);


/**
 * @brief Return HCI Context from LibNfc
 *
 * @return phHciNfc_sContext_t* pointer
 **/
phHciNfc_sContext_t *phHciWrapperCore_Internal_getHciContext();



#endif // ndef PHHCINFCAPICORE_H