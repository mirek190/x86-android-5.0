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
 * \file phNfc_callback.c
 * $Author: Eff'Innov Technologies $
 */

#include "core/phMessagingNfc_Interface.h"

// **************************************** FRI Specific

// Use this structure to avoid memory allocation/deallocation.
typedef struct sDataPool
{
    phDal4Nfc_DeferredCall_Msg_t *data;
    struct sDataPool   *next;
}sDataPool_t;

typedef int BOOL;
#define TRUE 1
#define FALSE 0

static sDataPool_t *globalPool = NULL;

#define PH_DAL4NFC_MESSAGE_BASE  0x311

void phMessagingNfc_Interface_AllocMessage(void ** message)
{
    phDal4Nfc_DeferredCall_Msg_t *deferedCalldInfo  = NULL;

    // check if global pool is empty
    if (globalPool == NULL)
    {
        // Pool is empty create a new structure to hold our data.
        globalPool = phOsalNfc_GetMemory(sizeof(sDataPool_t));
        globalPool->data = NULL;
        globalPool->next = NULL;
    }

    if (!globalPool->data)
    {
        // Allocate phDal4Nfc_Message_Wrapper_t & phDal4Nfc_DeferMsg_t

        deferedCalldInfo = (phDal4Nfc_DeferredCall_Msg_t*)phOsalNfc_GetMemory(sizeof(phDal4Nfc_DeferredCall_Msg_t));
        memset(deferedCalldInfo, 0, sizeof(phDal4Nfc_DeferredCall_Msg_t));
        globalPool->data = deferedCalldInfo;
    }else
    {
        //nfcMessage = globalPool->data;
    }

    // Return first element of global pool.
    *message = globalPool;
    //Next element become the first one.
    globalPool = globalPool->next;
}

void phMessagingNfc_Interface_FreeMessage(void * message)
{
    sDataPool_t *poolData = (sDataPool_t*)message;
    sDataPool_t *oldData  = globalPool;

    // We will recycle the message for further use.

    // We push message to free as first element
    globalPool = poolData;
    // We put previous one as next.
    globalPool->next = oldData;
}


NFCSTATUS phMessagingNfc_Interface_DeferredCall(void *message, pphDal4Nfc_DeferFuncPointer_t func, void *ctx)
{
    int retvalue = 0;
    sDataPool_t                  *poolData    = (sDataPool_t*)message;
    phDal4Nfc_Message_Wrapper_t   nDeferedMessageWrapper;
    phDal4Nfc_DeferredCall_Msg_t *pDeferedMessage = poolData->data;

    if (!message)
    {
        // Synchronous CB
        func(ctx);
        return NFCSTATUS_SUCCESS;
    }

    #ifdef USE_MQ_MESSAGE_QUEUE
    //FA do not go there
        nDeferedMessage.eMsgType = PH_DAL4NFC_MESSAGE_BASE;
        nDeferedMessage.def_call = func;
        nDeferedMessage.params   = ctx;
        retvalue = (int)mq_send(nDeferedCallMessageQueueId, (char *)&nDeferedMessage, sizeof(phDal4Nfc_DeferredCall_Msg_t), 0);

    #else
        nDeferedMessageWrapper.mtype = 1;
        nDeferedMessageWrapper.msg.eMsgType = PH_DAL4NFC_MESSAGE_BASE;
        pDeferedMessage->pCallback = func;
        pDeferedMessage->pParameter = ctx;
        nDeferedMessageWrapper.msg.pMsgData = pDeferedMessage;
        nDeferedMessageWrapper.msg.Size = sizeof(phDal4Nfc_DeferredCall_Msg_t);
        retvalue = phDal4Nfc_msgsnd(nDeferedCallMessageQueueId,(struct msgbuf *)&nDeferedMessageWrapper, sizeof(phLibNfc_Message_t), 0);
    #endif

    if(retvalue <0)     return NFCSTATUS_FAILED;

    return NFCSTATUS_SUCCESS;
}

void phMessagingNfc_Interface_CleanUp()
{
    //empty for android
}
