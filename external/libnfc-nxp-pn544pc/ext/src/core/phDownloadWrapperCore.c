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
 * \file phDownloadWrapperCore.c
 * $Author: Eff'Innov Technologies $
 */

#include "core/phDownloadWrapperCore.h"
#include "core/phMessagingNfc_Interface.h"
#include "api/phDownloadWrapperApi.h"

#include "tools/core/framework_Interface.h"

typedef struct tDownloadWrapperCoreContext
{
    void                 *downloadWrapperContext; // Used to hold underlying implementation context.
    phDal4Nfc_SContext_t *dalContext;             // Hold DAL context.
    void                 *hwdRef;                 // hardware ref.
}tDownloadWrapperCoreContext_t;


#define CB_TYPE_RECEIVE_COMPLETED       0x01
#define CB_TYPE_SEND_COMPLETED          0x02

typedef struct tCallBackData
{
    phNfc_sTransactionInfo_t       info;                  // Use to hold fake transaction informations
    void                          *message;               // Message to send
    uint8_t                        type;                  // Type of callback (see CB_TYPE_XXXX)
    tDownloadWrapperCoreContext_t *downloadWrapperCoreContext; // Context of this Wrapper.
}tCallBackData_t;


static tDownloadWrapperCoreContext_t * g_downloadWrapperCoreContext = NULL;

static void phDownloadWrapperCore_Internal_DeferredCb(void  *params)
{
    tCallBackData_t               *cbData = (tCallBackData_t*)params;
    tDownloadWrapperCoreContext_t *ctx    =  cbData->downloadWrapperCoreContext;

    framework_Log("--> %s",__FUNCTION__);

    if (g_downloadWrapperCoreContext == ctx)
    {
        // Processing the right callback.
        if (cbData->type == CB_TYPE_SEND_COMPLETED)
        {
            // Send cb
            if (ctx->dalContext->cb_if.send_complete)
            {
                ctx->dalContext->cb_if.send_complete(ctx->dalContext->cb_if.pif_ctxt, ctx->hwdRef, &(cbData->info));
            }else
            {
                framework_Error("ctx->dalContext->cb_if.send_complete is NULL !\n");
            }
        }

        if (cbData->type == CB_TYPE_RECEIVE_COMPLETED)
        {
            // Receive cb
            if (ctx->dalContext->cb_if.receive_complete)
            {
                ctx->dalContext->cb_if.receive_complete(ctx->dalContext->cb_if.pif_ctxt, ctx->hwdRef, &(cbData->info));
            }else
            {
                framework_Error("ctx->dalContext->cb_if.receive_complete is NULL !\n");
            }
        }
    } else {
        framework_Log("Ignoring outdated deffered callback (g_downloadWrapperCoreContext != ctx) \n");
    }
    // Free allocated message.
    phMessagingNfc_Interface_FreeMessage(cbData->message);
    // Free callback data.
    phOsalNfc_FreeMemory(cbData);
}


NFCSTATUS phDownloadWrapperCore_InitDownloadMode(void** context, phDal4Nfc_SContext_t* dalContext,void *hwdRef)
{
    NFCSTATUS                      retCode    = NFCSTATUS_SUCCESS;
    tDownloadWrapperCoreContext_t *newContext = NULL;

    framework_Log("--> %s",__FUNCTION__);

    if (context == NULL)
    {
        framework_Log("<-- %s NFCSTATUS_INVALID_PARAMETER",__FUNCTION__);
        return NFCSTATUS_INVALID_PARAMETER;
    }

    newContext = phOsalNfc_GetMemory(sizeof(tDownloadWrapperCoreContext_t));

    retCode = phDownloadWrapper_InitDownloadMode(&(newContext->downloadWrapperContext),newContext);

    if (retCode != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_FreeMemory(newContext);
        framework_Log("<-- %s retCode != NFCSTATUS_SUCCESS",__FUNCTION__);
        return retCode;
    }

    newContext->dalContext = dalContext;
    newContext->hwdRef     = hwdRef;

    *context = g_downloadWrapperCoreContext = newContext;
    framework_Log("<-- %s",__FUNCTION__);
    return retCode;
}


NFCSTATUS phDownloadWrapperCore_DeInitDownloadMode(void* context)
{
    tDownloadWrapperCoreContext_t *ctx        = (tDownloadWrapperCoreContext_t*)context;
    NFCSTATUS                      retCode    = NFCSTATUS_SUCCESS;

    if (ctx == NULL)
    {
        // Nothing to deinit...
        return NFCSTATUS_SUCCESS;
    }

    retCode = phDownloadWrapper_DeInitDownloadMode(ctx->downloadWrapperContext);

    g_downloadWrapperCoreContext = NULL;
    phOsalNfc_FreeMemory(ctx);

    phMessagingNfc_Interface_CleanUp();

    return retCode;
}


NFCSTATUS phDownloadWrapperCore_ReadPacket(void* context, uint8_t* data, size_t size,void *hwRef)
{
    tDownloadWrapperCoreContext_t *ctx     = (tDownloadWrapperCoreContext_t*)context;
    NFCSTATUS                      retCode = NFCSTATUS_SUCCESS;

    if (ctx == NULL)
    {
        // Invalid parameter
        return NFCSTATUS_INVALID_PARAMETER;
    }

    ctx->hwdRef = hwRef;

    retCode = phDownloadWrapper_ReadPacket(ctx->downloadWrapperContext,data,size);

    if (retCode == NFCSTATUS_SUCCESS)
    {
        retCode = NFCSTATUS_PENDING;
        phDownloadWrapper_ReceiveComplete(ctx,NFCSTATUS_SUCCESS,data,size);
    }


    return retCode;
}


NFCSTATUS phDownloadWrapperCore_WritePacket(void* context, uint8_t* data, size_t size,void *hwRef)
{
    tDownloadWrapperCoreContext_t *ctx     = (tDownloadWrapperCoreContext_t*)context;
    NFCSTATUS                      retCode = NFCSTATUS_SUCCESS;

    if (ctx == NULL)
    {
        // Invalid parameter
        return NFCSTATUS_INVALID_PARAMETER;
    }

    ctx->hwdRef = hwRef;

    retCode = phDownloadWrapper_WritePacket(ctx->downloadWrapperContext,data,size);

    if (retCode == NFCSTATUS_SUCCESS)
    {
        retCode = NFCSTATUS_PENDING;
        phDownloadWrapper_SendComplete(ctx,NFCSTATUS_SUCCESS,size);
    }

    return retCode;
}


void phDownloadWrapper_SendComplete(void *coreContext,NFCSTATUS errorCode,size_t sz)
{
    tDownloadWrapperCoreContext_t *ctx    = (tDownloadWrapperCoreContext_t*)coreContext;
    tCallBackData_t               *cbData = NULL;

    // Allocating callback struture.
    cbData = phOsalNfc_GetMemory(sizeof(tCallBackData_t));
    // Setup callback structure informations
    cbData->info.status                = errorCode;
    cbData->info.length                = (uint16_t)sz;
    cbData->info.buffer                = NULL;
    cbData->type                       = CB_TYPE_SEND_COMPLETED;
    cbData->downloadWrapperCoreContext = ctx;
    // Allocating message underlying message structure.
    phMessagingNfc_Interface_AllocMessage(&cbData->message);
    // Post message with callback structure as context.
    phMessagingNfc_Interface_DeferredCall(cbData->message,phDownloadWrapperCore_Internal_DeferredCb,cbData);
}
void phDownloadWrapper_ReceiveComplete(void *coreContext,NFCSTATUS errorCode, uint8_t* dataDestPtr,size_t sz)
{
    tDownloadWrapperCoreContext_t *ctx    = (tDownloadWrapperCoreContext_t*)coreContext;
    tCallBackData_t               *cbData = NULL;

    // Allocating callback struture.
    cbData = phOsalNfc_GetMemory(sizeof(tCallBackData_t));
    // Setup callback structure informations
    cbData->info.status                = errorCode;
    cbData->info.length                = (uint16_t)sz;
    cbData->info.buffer                = dataDestPtr;
    cbData->type                       = CB_TYPE_RECEIVE_COMPLETED;
    cbData->downloadWrapperCoreContext = ctx;
    // Allocating message underlying message structure.
    phMessagingNfc_Interface_AllocMessage(&cbData->message);
    // Post message with callback structure as context.
    phMessagingNfc_Interface_DeferredCall(cbData->message,phDownloadWrapperCore_Internal_DeferredCb,cbData);
}
