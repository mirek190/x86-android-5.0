/*++

   INTEL CONFIDENTIAL
   Copyright 2009 - 2012 Intel Corporation All Rights Reserved.
   The source code contained or described herein and all documents related
   to the source code ("Material") are owned by Intel Corporation or its
   suppliers or licensors. Title to the Material remains with Intel Corporation
   or its suppliers and licensors. The Material contains trade secrets and
   proprietary and confidential information of Intel or its suppliers and
   licensors. The Material is protected by worldwide copyright and trade secret
   laws and treaty provisions. No part of the Material may be used, copied,
   reproduced, modified, published, uploaded, posted, transmitted, distributed,
   or disclosed in any way without Intel's prior express written permission.
--*/

/*++

@file: HECILinux.c

--*/
#include "port/intel/HECI/HECIPorting.h"
#include "tools/core/framework_Interface.h"
#include "tools/core/framework_Allocator.h"

#include <memory.h>
#include <stdint.h>
#include <sys/types.h>
//FIX ME
#include "txei.h"
//for code error
#include <errno.h>
#include <string.h>
#include <stdio.h>


/* Set the Guid */
// NOTE : CG : On Windows Driver it's the same GUID, is it usefull to read it from a file ?
const GUID my_guid = {0x0bb17a78, 0x2a8e, 0x4c50, {0x94, 0xd4, 0x50, 0x26, 0x67, 0x23, 0x77, 0x5c}};

typedef enum _eReconnectionStatus
{
    RECONNECTION_NOT_STARTED,
    RECONNECTION_IN_PROG,
    RECONNECTION_ALREADY_IN_PROG,
    RECONNECTION_DONE,
    RECONNECTION_FAILED

} eReconnectionStatus;

/**
    Internal structure for HECI client context
 */
typedef struct sHECILinuxContext
{
    MEI_HANDLE          *hecihandle;         /* MEI Handle */
    void                *readingThreadId;    /* variable to get id of the reding thread to kill it if necessary during deinit */
    eReconnectionStatus  reconStatus;
    void                *reconLock;

} sHECILinuxContext_t;

static uint8_t  gHeciLinuxLogEnabled = 0;
static uint8_t  gAbortConnection = 0;     /* Set to 1 if IMEI connection aborted by HECI_Deinit() */
static void    *gPreviousContext = NULL;

void signal_handler(int signal_number)
{
    if (gHeciLinuxLogEnabled)
    {
        ALOGD("Signal received: %d\n", signal_number);
    }
}

static void HECILinux_CompleteReconnection(sHECILinuxContext_t *ctx, eReconnectionStatus result)
{
    framework_LockMutex(ctx->reconLock);
    // Check if other thread already in wait state.
    if (ctx->reconStatus == RECONNECTION_ALREADY_IN_PROG)
    {
        // It failed.
        ctx->reconStatus = result;
        // Notify other thread
        framework_NotifyMutex(ctx->reconLock, 0);
    } else
    {
        // It failed.
        ctx->reconStatus = result;
    }
    framework_UnlockMutex(ctx->reconLock);
}

/**
 * @brief Check if error is due to ME or IMEI reset.
 * If ME reset, we reconnect silently and return 1
 * to repeat current read or write operation.
 *
 * @param ctx sHECILinuxContext_t
 * @return uint8_t 1 indicate ME reset and read or write must be done again. 0 it's caused by HECI_Deinit().
 */
static uint8_t HECILinux_CheckAndHandleMeReset(sHECILinuxContext_t *ctx)
{
    if (gAbortConnection)
    {
        // Connection aborted by HECI_Deinit()
        gAbortConnection = 0;
        // No need to reconnect.
        return 0;
    } else
    {
        // NOTE : If this procedure not working, call directly phOsalNfc_RaiseException().

        // NOTE : Here only 2 threads can enter this function at the same time.
        framework_LockMutex(ctx->reconLock);
        if (ctx->reconStatus == RECONNECTION_IN_PROG)
        {
            // Other thread already in "Reconnection mode"
            ctx->reconStatus = RECONNECTION_ALREADY_IN_PROG;
            // We wait the completion of other reconnection thread.
            framework_WaitMutex(ctx->reconLock,0);
            // Check reconnection result.
            if (ctx->reconStatus == RECONNECTION_DONE)
            {
                framework_UnlockMutex(ctx->reconLock);
                return 1;
            }
            // Any other case.
            framework_UnlockMutex(ctx->reconLock);
            return 2;
        }

        ctx->reconStatus = RECONNECTION_IN_PROG;
        framework_UnlockMutex(ctx->reconLock);

        mei_disconnect(ctx->hecihandle);

        ctx->hecihandle = mei_connect(&my_guid);

        if (ctx->hecihandle == NULL)
        {
            HECILinux_CompleteReconnection(ctx, RECONNECTION_FAILED);

            framework_Error("%s : cannot connect to device\n", __FUNCTION__);
            return 2;
        }

        HECILinux_CompleteReconnection(ctx, RECONNECTION_DONE);

        return 1;
    }
}

/**
    Internal structure for HECI client context
*/
eHECICommStatus HECI_Init(void **context)
{
    eHECICommStatus status = HECICOMM_FAILED;
    sHECILinuxContext_t *newContext = NULL;

    // Release previous used context
    if(gPreviousContext != NULL)
    {
        framework_FreeMem(gPreviousContext);
        gPreviousContext = NULL;
    }

    *context =  NULL;

    newContext = (sHECILinuxContext_t*)framework_AllocMem(sizeof(sHECILinuxContext_t));

    if (newContext == NULL)
    {
        return HECICOMM_FAILED;
    }

    memset(newContext, 0, sizeof(sHECILinuxContext_t));

    newContext->hecihandle = mei_connect(&my_guid);

    if (newContext->hecihandle == NULL) {
        framework_Error("cannot connect to device\n");
        // Free context.
        framework_FreeMem(newContext);
        return HECICOMM_FAILED;
    }
    if (gHeciLinuxLogEnabled)
    {
        framework_Log("connection to device OK\n");
    }

    gAbortConnection = 0;

    framework_CreateMutex(&(newContext->reconLock));
    newContext->reconStatus = RECONNECTION_NOT_STARTED;

    // Complete initialization by supplying the handle to the user
    *context = newContext;

    return HECICOMM_SUCCESS;
}

eHECICommStatus HECI_Deinit(void *context)
{
    sHECILinuxContext_t* ctx = (sHECILinuxContext_t*)context;

    if (gHeciLinuxLogEnabled)
    {
        framework_Log("%s \n", __FUNCTION__);
    }

    if (ctx == NULL || ctx->hecihandle == NULL)
    {
        framework_Error("handle null in HECI_Deinit\n");
        return HECICOMM_INVALID_PARAM;
    }

    gAbortConnection = 1;

    framework_Log("Before mei_disconnect \n");
    mei_disconnect(ctx->hecihandle);
    framework_Log("After mei_disconnect \n");
    if (ctx->readingThreadId != NULL)
    {
        if (pthread_kill((int)ctx->readingThreadId, SIGUSR2) != 0) framework_Error("pthread_kill failed");
        ctx->readingThreadId= NULL;
    }
    else
    {
        if (gHeciLinuxLogEnabled)
        {
            framework_Log("readingThreadId == NULL");
        }
    }

    framework_DeleteMutex(ctx->reconLock);

    // Current context will be released in next init when reading thread is dead
    gPreviousContext = context;

    return HECICOMM_SUCCESS;
}


eHECICommStatus HECI_Read(void *context, uint8_t *buffer, uint32_t size, uint32_t *readSize)
{
    sHECILinuxContext_t *ctx             = context;
    uint8_t              readAgain       = 0;
    int32_t              rS              = 0;
    uint8_t              needToReconnect = 0;

    *readSize = 0;

    if (ctx == NULL || ctx->hecihandle == NULL)
    {
        framework_Error("handle null in HECI_Read\n");
        return HECICOMM_INVALID_PARAM;
    }

    if (buffer == NULL)
    {
        framework_Error("buffer null in HECI_Read\n");
        return HECICOMM_INVALID_PARAM;
    }

    if (size == 0)
    {
        framework_Warn("nb byte to read = 0\n");
    }

    if (gHeciLinuxLogEnabled)
    {
        framework_Log("%s %i\n", __FUNCTION__, __LINE__);
    }

    if (ctx->readingThreadId == NULL)
    {
         struct sigaction sa;
         memset(&sa, 0, sizeof(sa));
         sa.sa_handler = &signal_handler;
         sigaction(SIGUSR2, &sa, NULL);
         ctx->readingThreadId = framework_GetCurrentThreadId();
    }

    do
    {
        readAgain = 0;

        rS = mei_rcvmsg(ctx->hecihandle, buffer, size);

        switch (errno)
        {
            case EINVAL:
            {
                framework_Error("%s : Invalid argument passed to mei_rcvmsg()");
            }
            case ENOTTY:
            case EBUSY:
            {
                needToReconnect = 0;
                break;
            }
            case ENODEV:
            {
                needToReconnect = 1;
                break;
            }
            default:
            {
            }
        }

        framework_Log("%s %i\n",__FUNCTION__,__LINE__);

        if (rS < 0)
        {
            *readSize = 0;

            framework_Error("Error reading from HECI");
            framework_Error("error : %s\n", strerror(errno));

            if (needToReconnect)
            {
                // If we need to reconnect, read should be done again.
                readAgain = HECILinux_CheckAndHandleMeReset(ctx);

                if (readAgain != 1)
                {
                    return HECICOMM_FAILED;
                }
            } else
            {
                return HECICOMM_FAILED;
            }
        }
    } while (readAgain == 1);

    if (gHeciLinuxLogEnabled)
    {
        framework_Log("HECI readsize: %d \n",rS);
    }

    *readSize = rS;

    return HECICOMM_SUCCESS;
}



eHECICommStatus HECI_Write(void *context, const uint8_t *buffer, uint32_t size, uint32_t *writtenSize)
{
    sHECILinuxContext_t *ctx             = context;
    uint8_t              writeAgain      = 0;
    uint32_t             wS              = 0;
    uint8_t              needToReconnect = 0;

    *writtenSize = 0;

    if (ctx == NULL || ctx->hecihandle == NULL)
    {
        framework_Error("handle null in HECI_Write\n");
        return HECICOMM_INVALID_PARAM;
    }

    if (buffer == NULL)
    {
        framework_Error("buffer null in HECI_Write\n");
        return HECICOMM_INVALID_PARAM;
    }

    if (size == 0)
    {
        framework_Warn("nb byte to write = 0\n");
    }

    if (gHeciLinuxLogEnabled)
    {
        framework_Log("%s %i\n", __FUNCTION__, __LINE__);
    }

    do
    {
        writeAgain = 0;

        wS = mei_sndmsg(ctx->hecihandle, buffer, size);

        switch (errno)
        {
            case EINVAL:
            {
                framework_Error("%s : Invalid argument passed to mei_sndmsg()");
            }
            case ENOTTY:
            case EBUSY:
            {
                needToReconnect = 0;
                break;
            }
            case ENODEV:
            {
                needToReconnect = 1;
                break;
            }
            default:
            {
            }
        }

        if (gHeciLinuxLogEnabled)
        {
            framework_Log("%s %i\n", __FUNCTION__, __LINE__);
        }

        if (wS != size)
        {
            *writtenSize = 0;
            framework_Error("incorrect size sent %x instead of %x\n", wS,(unsigned int)size);
            framework_Error("error : %s\n",strerror(errno));

            if (needToReconnect)
            {
                // If we need to reconnect, write should be done again.
                writeAgain = HECILinux_CheckAndHandleMeReset(ctx);

                if (writeAgain != 1 )
                {
                    return HECICOMM_FAILED;
                }
            } else
            {
                return HECICOMM_FAILED;
            }
        }
    } while(writeAgain == 1);

    *writtenSize = wS;

    return HECICOMM_SUCCESS;
}


eHECICommStatus HECI_GetClient(void *context, uint32_t *maxMessageLength)
{
    sHECILinuxContext_t* ctx = context;

    if (ctx == NULL || ctx->hecihandle == NULL)
    {
        framework_Error("handle null in HECI_GetClient\n");
        return HECICOMM_INVALID_PARAM;
    }
    MEI_CLIENT heciclient = ctx->hecihandle->client_properties;
    *maxMessageLength = heciclient.MaxMessageLength;

    return HECICOMM_SUCCESS;
}


void HECI_EmergencyRelease(void *context)
{
    sHECILinuxContext_t* ctx = context;

    if (ctx == NULL || ctx->hecihandle == NULL)
    {
        framework_Error("handle null in HECI_EmergencyRelease\n");
        return;
    }
    // Force Close on HECI.
    mei_disconnect(ctx->hecihandle);
}

void HECI_SetLog(uint8_t value)
{
    gHeciLinuxLogEnabled = value;
}
