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
 * \file phHciwrapperCore.c
 * $Author: Eff'Innov Technologies $
 */

#include "core/phHciWrapperCore.h"

#include "tools/core/framework_Interface.h"

#include "api/phHciWrapperApi.h"
#include "core/phMessagingNfc_Interface.h"


#define INTERNAL_RECVBUFFER 1024

typedef struct tHciWrapperCoreContext
{
    void    *hciWrapperContext; // Used to hold underlying implementation context.
    void    *hciContext;        // Used by phHciWrapperCore_Internal_DeferredCb(), provided during Init.
    void    *hwRef;             // Used by phHciWrapperCore_Internal_DeferredCb(), provided during Init.
    uint8_t  internalRecvBuffer[INTERNAL_RECVBUFFER];
    void    *mutex;
    int32_t  callbackPending;
    uint8_t  needCallDeinitComplete;
}tHciWrapperCoreContext_t;

static tHciWrapperCoreContext_t *gHciWrapperCoreContext = NULL;

#define CB_TYPE_RECEIVE_COMPLETED       0x01
#define CB_TYPE_SEND_COMPLETED          0x02

typedef struct tCallBackData
{
    phNfc_sTransactionInfo_t  info;                  // Use to hold fake transaction informations
    void                     *message;               // Message to send
    uint8_t                   type;                  // Type of callback (see CB_TYPE_XXXX)
    tHciWrapperCoreContext_t *hciWrapperCoreContext; // Context of this Wrapper.
}tCallBackData_t;

static void phHciWrapperCore_Internal_DeferredCb(void  *params)
{
    tCallBackData_t          *cbData = (tCallBackData_t*)params;
    tHciWrapperCoreContext_t *ctx    =  cbData->hciWrapperCoreContext;
    NFCSTATUS retCode;

    if( gHciWrapperCoreContext == ctx )
    {
        framework_LockMutex(ctx->mutex);
        if (ctx->needCallDeinitComplete == 1)
        {
            // Check if we are the last callback.
            if (ctx->callbackPending == 0)
            {
                framework_UnlockMutex(ctx->mutex);
                // Need to deinit we are the last remaining callback.
                phHciWrapper_DeinitComplete(ctx,NFCSTATUS_SUCCESS);
                // Free allocated message.
                phMessagingNfc_Interface_FreeMessage(cbData->message);
                //FA  Free copy of received hci
                if(cbData->info.buffer != NULL)
                {
                    phOsalNfc_FreeMemory(cbData->info.buffer);
                }
                phOsalNfc_FreeMemory(cbData);
                // Free Cleanup message queue
                phMessagingNfc_Interface_CleanUp();
            }
            else
            {
                // Next pending callback will do the job.
                ctx->callbackPending--;
                framework_UnlockMutex(ctx->mutex);
            }

            // Not calling any pending read.
            return;
        }
        framework_UnlockMutex(ctx->mutex);

        // Processing the right callback.
        if (cbData->type == CB_TYPE_SEND_COMPLETED)
        {
            phHciNfc_Send_Complete(ctx->hciContext,ctx->hwRef,&(cbData->info));
        }
        if (cbData->type == CB_TYPE_RECEIVE_COMPLETED)
        {
            phHciNfc_Receive_Complete(ctx->hciContext,ctx->hwRef,&(cbData->info));
        }

        framework_LockMutex(ctx->mutex);


        ctx->callbackPending--;

        if (ctx->needCallDeinitComplete == 0)
        {
            framework_UnlockMutex(ctx->mutex);
            // Auto rearm a read.
            retCode = phHciWrapper_ReceiveHCI(ctx->hciWrapperContext,ctx->internalRecvBuffer,INTERNAL_RECVBUFFER);

            if ((retCode != NFCSTATUS_PENDING)&&(retCode != NFCSTATUS_SUCCESS))
            {
                framework_Error(">>> Rearm read failed\n");
            }
        }
        else
        {
            if (ctx->callbackPending == 0)
            {
                framework_UnlockMutex(ctx->mutex);
                // Async deinit !
                phHciWrapper_DeinitComplete(ctx,NFCSTATUS_SUCCESS);
                // Free allocated message.
                phMessagingNfc_Interface_FreeMessage(cbData->message);
                //FA  Free copy of received hci
                if(cbData->info.buffer != NULL)
                {
                    phOsalNfc_FreeMemory(cbData->info.buffer);
                }
                phOsalNfc_FreeMemory(cbData);
                // Free Cleanup message queue
                phMessagingNfc_Interface_CleanUp();
                return;
            }
            else
            {
                framework_UnlockMutex(ctx->mutex);
            }
        }

        // Free allocated message.
        phMessagingNfc_Interface_FreeMessage(cbData->message);
        //FA  Free copy of received hci
        if(cbData->info.buffer != NULL)
        {
            phOsalNfc_FreeMemory(cbData->info.buffer);
        }
        // Free callback data.
        // NOTE: the callback function exits here after freeing the cb data structure.
        // In certain situations the message queue still holds a last pending request
        // with same cbData ptr value. The callback is then called immediately again
        // with now invalid cb data ptr which was just freed. In the end it obviously
        // generates a crash when deferencing cbData at callback function entry. To
        // overcome this situation, we ignore the last request with invalid pointer.
        phOsalNfc_FreeMemory(cbData);
    } else {
        // Ignore the last request with invalid callback data ptr which was already
        // freed at callback exit just above.
        framework_Log("%s: ignoring outdated deferred callback\n", __FUNCTION__);
    }
}


NFCSTATUS phHciWrapperCore_Init(void** context,void *hciContext,void *hwRef)
{
    NFCSTATUS                 status;
    tHciWrapperCoreContext_t *newContext = NULL;
    framework_Log("in phHciWrapperCore_Init");
    if (!context)
    {
        framework_Log("%s NFCSTATUS_INVALID_PARAMETER",__FUNCTION__);
        return NFCSTATUS_INVALID_PARAMETER;
    }

    // Allocating our context.
    newContext = (tHciWrapperCoreContext_t*)phOsalNfc_GetMemory(sizeof(tHciWrapperCoreContext_t));

    // Setup structure
    newContext->hciWrapperContext      = NULL;
    newContext->hciContext             = hciContext;
    newContext->hwRef                  = hwRef;
    newContext->callbackPending        = 0;
    newContext->needCallDeinitComplete = 0;
    newContext->mutex                  = NULL;

    framework_CreateMutex(&(newContext->mutex));

    // We initialize underlying implementation layer.
    status = phHciWrapper_Init(&(newContext->hciWrapperContext),newContext);
    // If all is ok ...
    if ((status != NFCSTATUS_SUCCESS) && (status != NFCSTATUS_PENDING))
    {
        // ... abort !
        *context = NULL;
        return status;
    }
    // ... newContext->hciWrapperContext will contain underlying context.
    // We should hold it.
    // Now our context is ready, we can provide it to upper layer.
    *context = gHciWrapperCoreContext = newContext;




    // Is it a synchronous or asynchronous Init ?
    if (status == NFCSTATUS_SUCCESS)
    {
        // Simulate async behavior.
        status = NFCSTATUS_PENDING;
        framework_Log("%s : Notifi init completed.\n", __FUNCTION__);
        phHciWrapper_InitComplete(newContext,NFCSTATUS_SUCCESS);
    }

    return status;
}
NFCSTATUS phHciWrapperCore_DeInit(void* context,void *hciContext,void *hwRef)
{
    NFCSTATUS                 status;
    tHciWrapperCoreContext_t *ctx = (tHciWrapperCoreContext_t*)context;
    framework_Log("phHciWrapperCore_DeInit \n");
    if (ctx == NULL)
    {
        // Nothing to deinit ...
        return NFCSTATUS_SUCCESS;
    }
    // We need to update these values ... may change ...
    ctx->hciContext = hciContext;
    ctx->hwRef      = hwRef;

    // Deinitialize underlying implementation.
    status = phHciWrapper_Deinit(ctx->hciWrapperContext);

    // Is status is a valid one ? Success or Pending ?
    if ((status != NFCSTATUS_SUCCESS) && (status != NFCSTATUS_PENDING))
    {
        return status;
    }

    // Is it synchronous
    if (status == NFCSTATUS_SUCCESS)
    {
        int32_t callbackPending = 0;

        framework_LockMutex(ctx->mutex);
        // NOTE : If there are pending callbacks this value will be greater than 1
        // so callbacks will deal with deinit.
        callbackPending = ctx->callbackPending;

        if (callbackPending > 0)
        {
            ctx->needCallDeinitComplete = 1;
        }

        framework_UnlockMutex(ctx->mutex);
        // NOTE : underlying layer cannot post anymore callback. So it is safe.
        if (callbackPending == 0)
        {
            // Simulate asynchronous.
            phHciWrapper_DeinitComplete(ctx,NFCSTATUS_SUCCESS);
            phMessagingNfc_Interface_CleanUp();
        }
    }// else if pending it will be deallocated asynchronously.

    gHciWrapperCoreContext = NULL;

    framework_Log("phHciWrapperCore_DeInit Done\n");
    // NOTE : Must return NFCSTATUS_SUCCESS
    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phHciWrapperCore_Reset(void* context, void* hciContext, void* hwRef)
{
    NFCSTATUS                 status;
    tHciWrapperCoreContext_t *ctx = (tHciWrapperCoreContext_t*)context;

    if (ctx == NULL)
    {
        framework_Log("%s NFCSTATUS_INVALID_PARAMETER",__FUNCTION__);
        // Invalid parameter
        return NFCSTATUS_INVALID_PARAMETER;
    }

    // We need to update these values ... may change ...
    ctx->hciContext = hciContext;
    ctx->hwRef      = hwRef;

    // Call underlying reset.
    status = phHciWrapper_Reset(ctx->hciWrapperContext);

    // Is it a synchronous or asynchronous Init ?
    if (status == NFCSTATUS_SUCCESS)
    {
        // Simulate async behavior.
        status = NFCSTATUS_PENDING;
        phHciWrapper_InitComplete(context,NFCSTATUS_SUCCESS);
    }
    return status;
}


NFCSTATUS phHciWrapperCore_SendHCI(void* context, uint8_t* data, size_t sz)
{
    tHciWrapperCoreContext_t *ctx = (tHciWrapperCoreContext_t*)context;
    NFCSTATUS                 status;

    if (ctx == NULL)
    {
        // Invalid parameter
        return NFCSTATUS_INVALID_PARAMETER;
    }

    // Call underlying SendHCI implementation.
    status = phHciWrapper_SendHCI(ctx->hciWrapperContext,data,sz);

    // Is it synchronous ?
    if (status == NFCSTATUS_SUCCESS)
    {
        // Emulate asynchronous.
        status = NFCSTATUS_PENDING;
        phHciWrapper_SendComplete(ctx,NFCSTATUS_SUCCESS,sz);
    }
    // Should return NFCSTATUS_PENDING
    return status;
}

NFCSTATUS phHciWrapperCore_ReceiveHCI(void* context, uint8_t* data, size_t sz)
{
    tHciWrapperCoreContext_t *ctx = (tHciWrapperCoreContext_t*)context;
    NFCSTATUS                 status;

    if (ctx == NULL)
    {
        // Invalid parameter
        return NFCSTATUS_INVALID_PARAMETER;
    }

    // Call underlying ReceiveHCI
    //FA workaround too small buffer error obtain when reading tag felica/desfire bug
    if(sz > 29)
    {
        //end FA
        status = phHciWrapper_ReceiveHCI(ctx->hciWrapperContext,data,sz);
    //FA
    }
    else
    {
        status = phHciWrapper_ReceiveHCI(ctx->hciWrapperContext,ctx->internalRecvBuffer,INTERNAL_RECVBUFFER);
    //end FA
    }

    // Is it synchronous ?
    if (status == NFCSTATUS_SUCCESS)
    {
        // Emulate asynchronous.
        status = NFCSTATUS_PENDING;
        phHciWrapper_ReceiveComplete(ctx,NFCSTATUS_SUCCESS,data,sz);
    }
    // Should return NFCSTATUS_PENDING
    return status;
}


void phHciWrapper_InitComplete(void* libNfcCtx, NFCSTATUS errCode)
{
    // Init is supposed to be asynchronous.
    tHciWrapperCoreContext_t *ctx        = (tHciWrapperCoreContext_t*)libNfcCtx;
    phHciNfc_sContext_t      *hciContext = NULL;
    phNfc_sCompletionInfo_t   fakeInfo;

    // Fill fake information to meet LibNfc requirement.
    fakeInfo.status = NFCSTATUS_SUCCESS;
    fakeInfo.type   = NFC_NOTIFY_INIT_COMPLETED;
    fakeInfo.info   = NULL;
    // Retrieve LibNfc HCI Context structure
    hciContext      = phHciWrapperCore_Internal_getHciContext(libNfcCtx);

    // Notify upper layer that init procedure is completed.
    phHciNfc_Notify_Event(hciContext,ctx->hwRef,NFC_NOTIFY_INIT_COMPLETED,&fakeInfo);
}

void phHciWrapper_DeinitComplete(void* libNfcCtx, NFCSTATUS errCode)
{
    tHciWrapperCoreContext_t *ctx = (tHciWrapperCoreContext_t*)libNfcCtx;

    framework_DeleteMutex(ctx->mutex);
    // Simply free context
    phOsalNfc_FreeMemory(ctx);
}

void phHciWrapper_SendComplete(void *context,NFCSTATUS errorCode,size_t sz)
{
    tHciWrapperCoreContext_t *ctx    = (tHciWrapperCoreContext_t*)context;
    tCallBackData_t          *cbData = NULL;

    // Allocating callback struture.
    cbData = phOsalNfc_GetMemory(sizeof(tCallBackData_t));
    // Setup callback structure informations
    cbData->info.status           = errorCode;
    cbData->info.length           = (uint16_t)sz;
    cbData->info.buffer           = NULL;
    cbData->type                  = CB_TYPE_SEND_COMPLETED;
    cbData->hciWrapperCoreContext = ctx;
    // Allocating message underlying message structure.
    phMessagingNfc_Interface_AllocMessage(&cbData->message);
    // Post message with callback structure as context.
    framework_LockMutex(ctx->mutex);
    ctx->callbackPending++;
    framework_UnlockMutex(ctx->mutex);
    phMessagingNfc_Interface_DeferredCall(cbData->message,phHciWrapperCore_Internal_DeferredCb,cbData);
}

void phHciWrapper_ReceiveComplete(void *context,NFCSTATUS errorCode, uint8_t* dataDestPtr,size_t sz)
{
    tHciWrapperCoreContext_t *ctx    = (tHciWrapperCoreContext_t*)context;
    tCallBackData_t          *cbData = NULL;
    framework_Log("in phHciWrapper_receiveComplete");
    // Allocating callback struture.
    cbData = phOsalNfc_GetMemory(sizeof(tCallBackData_t));

    //FA bug T4 => buffer is partially erase when a second frame is received too fast
    //duplicate the buffer
    uint8_t * datacopy;
    datacopy = phOsalNfc_GetMemory(sz*sizeof(uint8_t));
    memcpy(datacopy, dataDestPtr, sz);

    // Setup callback structure informations
    cbData->info.status           = errorCode;
    cbData->info.length           = (uint16_t)sz;
    //FA bug T4 = get the copied data instead
    //cbData->info.buffer           = dataDestPtr;
    cbData->info.buffer           = datacopy;
    cbData->type                  = CB_TYPE_RECEIVE_COMPLETED;
    cbData->hciWrapperCoreContext = ctx;
    // Allocating message underlying message structure.
    phMessagingNfc_Interface_AllocMessage(&cbData->message);
    // Post message with callback structure as context.
    framework_Log("%s : Receive Complete before mutex\n", __FUNCTION__);
    framework_LockMutex(ctx->mutex);
    ctx->callbackPending++;
    framework_UnlockMutex(ctx->mutex);
    phMessagingNfc_Interface_DeferredCall(cbData->message,phHciWrapperCore_Internal_DeferredCb,cbData);
}

phHciNfc_sContext_t* phHciWrapperCore_Internal_getHciContext (void* context)
{
    tHciWrapperCoreContext_t *ctx = (tHciWrapperCoreContext_t*)context;
    // Retrieve LibNfc HCI Context.
    return (phHciNfc_sContext_t*)ctx->hciContext;
}


void phHciWrapperCore_EmergencyRelease ()
{
    phHciWrapper_EmergencyRelease();
}
