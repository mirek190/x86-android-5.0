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
 * \file phOsalNfc.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef PHMESSAGINGNFC_INTERFACE_H
#define PHMESSAGINGNFC_INTERFACE_H

#include "phNfcTypes.h"
#include "phOsalNfc.h"
#include "phNfcStatus.h"

#include "phDal4Nfc_DeferredCall.h"
#include "phDal4Nfc.h"
#include <phDal4Nfc_messageQueueLib.h>


typedef void   (*pphDal4Nfc_DeferFuncPointer_t) (void * );
/**
 * @brief Allocate a message.
 *
 * @param [out]message message structure.
 * @return void
 **/
void phMessagingNfc_Interface_AllocMessage(void ** message);
/**
 * @brief Free a message.
 *
 * @param [in]message message to release.
 * @return void
 **/
void phMessagingNfc_Interface_FreeMessage(void * message);

/**
 * @brief Post a function to be processed by Message queue.
 *
 * @param [in]message message structure allocated by phMessagingNfc_Interface_AllocMessage()
 * @param func Function to call.
 * @param ctx Context to call with this function.
 * @return NFCSTATUS NFCSTATUS_SUCCESS if all is ok.
 **/
NFCSTATUS phMessagingNfc_Interface_DeferredCall(void *message, pphDal4Nfc_DeferFuncPointer_t func, void *ctx);


void phMessagingNfc_Interface_CleanUp();

#endif // ndef PHMESSAGINGNFC_INTERFACE_H
