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
 * \file WaiterNotifier.c
 * $Author: Eff'Innov Technologies $
 */

#include "port/intel/WaiterNotifier.h"

#include "tools/core/framework_Allocator.h"
#include "tools/core/framework_Interface.h"

typedef enum _eWaitMutexStatus
{
    WAITMUTEX_NONE,
    WAITMUTEX_WAITING,
    WAITMUTEX_NOTIFIED
}eWaitMutexStatus;


typedef struct tWaiterNotifierContext
{
    void             *mutex;      // Mutex object
    eWaitMutexStatus  status;     // Status of object.
}tWaiterNotifierContext_t;

eWaiterNotifierStatus WaiterNotifier_Create(void** context)
{
    tWaiterNotifierContext_t *newContext;
    eResult result;

    // Check if parameter is valid.
    if (context == NULL)
    {
        return WAITERNOTIFIER_INVALID_PARAMETER;
    }

    // Allocate memory for context.
    newContext = framework_AllocMem(sizeof(tWaiterNotifierContext_t));

    // Create the mutex object.
    result = framework_CreateMutex(&(newContext->mutex));

    if (result != FRAMEWORK_SUCCESS)
    {
        framework_FreeMem(newContext);
        return WAITERNOTIFIER_FAILED;
    }

    newContext->status = WAITMUTEX_NONE;

    *context = newContext;

    return WAITERNOTIFIER_SUCCESS;
}

eWaiterNotifierStatus WaiterNotifier_Delete(void* context)
{
    tWaiterNotifierContext_t *ctx = (tWaiterNotifierContext_t *) context;

    // Check if parameter valid
    if (!ctx)
    {
        return WAITERNOTIFIER_INVALID_PARAMETER;
    }

    framework_LockMutex(ctx->mutex);

    // Check if the mutex is in wait position.
    if (ctx->status == WAITMUTEX_WAITING)
    {
        // Force any blocked wait to unblock.
        ctx->status = WAITMUTEX_NOTIFIED;
        framework_NotifyMutex(ctx->mutex,0);
        // NOTE :
        framework_Warn("%s : Join the thread that is in Wait state before deleting this WaiterNotifier. It might crash !\n",__FUNCTION__);

    }// else not in wait state.

    framework_UnlockMutex(ctx->mutex);

    framework_DeleteMutex(ctx->mutex);

    framework_FreeMem(ctx);

    return WAITERNOTIFIER_SUCCESS;
}

eWaiterNotifierStatus WaiterNotifier_Lock(void* context)
{
    tWaiterNotifierContext_t *ctx = (tWaiterNotifierContext_t *) context;

    // Check Parameter
    if (!ctx)
    {
        return WAITERNOTIFIER_INVALID_PARAMETER;
    }
    // Lock the mutex
    framework_LockMutex(ctx->mutex);
    return WAITERNOTIFIER_SUCCESS;
}

eWaiterNotifierStatus WaiterNotifier_Unlock(void* context)
{
    tWaiterNotifierContext_t *ctx = (tWaiterNotifierContext_t *) context;

    // Check Parameter
    if (!ctx)
    {
        return WAITERNOTIFIER_INVALID_PARAMETER;
    }
    // Unlock the mutex
    framework_UnlockMutex(ctx->mutex);
    return WAITERNOTIFIER_SUCCESS;
}

eWaiterNotifierStatus WaiterNotifier_Wait(void* context, uint8_t needLock)
{
    tWaiterNotifierContext_t *ctx = (tWaiterNotifierContext_t *) context;

    // Check Parameter
    if (!ctx)
    {
        return WAITERNOTIFIER_INVALID_PARAMETER;
    }

    if (needLock)
    {
        // Lock the mutex
        WaiterNotifier_Lock(ctx);
    }

    // Check if we have been notified prior to this function call
    if (ctx->status == WAITMUTEX_NONE)
    {
        // Nobody notify ! Go in Wait state.
        ctx->status = WAITMUTEX_WAITING;
        framework_WaitMutex(ctx->mutex,0);
        // NOTE : WaitMutex will reacquire the Mutex so we are still in safe section !
    }else if (ctx->status == WAITMUTEX_NOTIFIED)
    {
        // No wait go !
    }else
    {
        // Unsupported state => should never happened
        framework_Error("%s invalid state :%X",ctx->status);
    }

    // We reset the state to NONE before releasing the mutex (if requested).
    ctx->status = WAITMUTEX_NONE;

    if (needLock)
    {
        // Unlock the mutex
        WaiterNotifier_Unlock(ctx);
    }
    return WAITERNOTIFIER_SUCCESS;
}

eWaiterNotifierStatus WaiterNotifier_Notify(void* context, uint8_t needLock)
{
    tWaiterNotifierContext_t *ctx = (tWaiterNotifierContext_t *) context;

    // Check Parameter
    if (!ctx)
    {
        return WAITERNOTIFIER_INVALID_PARAMETER;
    }

    if (needLock)
    {
        // Lock the mutex
        WaiterNotifier_Lock(ctx);
    }

    // Check if the mutex is in WAIT state ...
    if (ctx->status == WAITMUTEX_WAITING)
    {
        // Need to notify
        ctx->status = WAITMUTEX_NOTIFIED;
        framework_NotifyMutex(ctx->mutex,0);
    }else
    {
        // It is not in wait state no need to notify.
        ctx->status = WAITMUTEX_NOTIFIED;
    }

    if (needLock)
    {
        // Unlock the mutex
        WaiterNotifier_Unlock(ctx);
    }
    return WAITERNOTIFIER_SUCCESS;
}
