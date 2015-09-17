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
 */

#ifndef PHDAL4NFC_DEFERREDCALL_H
#define PHDAL4NFC_DEFERREDCALL_H

#ifdef PH_NFC_CUSTOMINTEGRATION
#include <phNfcCustomInt.h>
#else

#ifdef _DAL_4_NFC_C
#define _ext_
#else
#define _ext_ extern
#endif

typedef pphLibNfc_DeferredCallback_t pphDal4Nfc_Deferred_Call_t;

typedef phLibNfc_DeferredCall_t phDal4Nfc_DeferredCall_Msg_t;

#ifndef WIN32

#ifdef USE_MQ_MESSAGE_QUEUE
#include <mqueue.h>
#define MQ_NAME_IDENTIFIER  "/nfc_queue"

_ext_ const struct mq_attr MQ_QUEUE_ATTRIBUTES
#ifdef _DAL_4_NFC_C
                                          = { 0,                               /* flags */
                                             10,                              /* max number of messages on queue */
                                             sizeof(phDal4Nfc_DeferredCall_Msg_t),   /* max message size in bytes */
                                             0                                /* number of messages currently in the queue */
                                           }
#endif
;
#endif

#endif

extern int nDeferedCallMessageQueueId;

void phDal4Nfc_DeferredCall(pphDal4Nfc_Deferred_Call_t func, void *param);
#endif
#endif
