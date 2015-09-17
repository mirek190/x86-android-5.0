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
 * \file phOsEvent_linux.c
 * $Author: Eff'Innov Technologies $
 */

#include "port/intel/phOsEvent_Interface.h"

#include "tools/core/framework_Interface.h"
#include "tools/core/framework_Allocator.h"


//#include <conio.h>
#include <stdio.h>

// #include <evntprov.h>
//#include "Messages.h" // Generated from Messages.mc

typedef struct tEventContext
{
    phOsEvent_MeResetCallBack *meResetCallback;
    void                      *meResetUserContext;
}tEventContext_t;





eEventStatus phOsEvent_Init(void **context)
{
    tEventContext_t *ctx;
    if (context == NULL)
    {
        return EVENT_INVALID_PARAM;
    }

    ctx = (tEventContext_t*)framework_AllocMem(sizeof(tEventContext_t));

    if (!ctx)
    {
        return EVENT_FAILED;
    }

    ctx->meResetCallback    = NULL;
    ctx->meResetUserContext = NULL;

    //registerMeResetEvent(ctx);

    *context = ctx;

    return EVENT_SUCCESS;
}

eEventStatus phOsEvent_Deinit(void *context)
{
    tEventContext_t *ctx = (tEventContext_t*)context;

    if (ctx == NULL)
    {
        return EVENT_INVALID_PARAM;
    }

    //unregisterMeResetEvent(ctx);
    return EVENT_SUCCESS;
}

eEventStatus phOsEvent_RegisterMeReset(void* context, phOsEvent_MeResetCallBack* cb, void* userContext )
{
    tEventContext_t *ctx = (tEventContext_t*)context;

    if (ctx == NULL)
    {
        return EVENT_INVALID_PARAM;
    }

    ctx->meResetCallback = cb;
    ctx->meResetUserContext = userContext;

    return EVENT_SUCCESS;
}

eEventStatus phOsEvent_UnregisterMeReset(void *context)
{
    tEventContext_t *ctx = (tEventContext_t*)context;

    if (ctx == NULL)
    {
        return EVENT_INVALID_PARAM;
    }

    ctx->meResetCallback = NULL;
    ctx->meResetUserContext = NULL;

    return EVENT_SUCCESS;
}

eEventStatus phOsEvent_GenerateEvent(void *context,eFaultStatus status)
{
    tEventContext_t *ctx = (tEventContext_t*)context;

    if (ctx == NULL)
    {
        return EVENT_INVALID_PARAM;
    }

    //generateWindowsErrorEvent(ctx,status);

    return EVENT_SUCCESS;
}
