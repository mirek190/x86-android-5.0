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
or disclosed in any way without Intel�s prior express written permission.
--*/

/*++

@file: phHciWrapper_intel.c

--*/

#include "api/phHciWrapperApi.h"
#include "core/phHciWrapperCore.h"
#include "phHECICommunicationLayer.h"
#include "tools/core/framework_Interface.h"

typedef struct tNfcHECIContext
{
    void *heciStackContext;
    void *libNfcCtx;
} tNfcHECIContext_t;

extern uint8_t restartStack();
extern uint8_t stopStack();

typedef struct sResetThreadData
{
    void                    *resetThreadPtr;
    struct sResetThreadData *next;
}sResetThreadData_t;

static sResetThreadData_t *gResetThreadData = NULL;
static void               *gResetThreadDataLock = NULL;
static uint8_t             gIsCleanup = 0;


static void* stopThreadPtr = NULL;

static void* nfcHeciContext = NULL;

void* resetThread(void* ctx)
{
    //UNREFERENCED_PARAMETER(ctx);
    restartStack();
    return NULL;
}

void* stopThread(void* ctx)
{
    //UNREFERENCED_PARAMETER(ctx);
    framework_Log(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> stopThread");
    stopStack();
    return NULL;
}


void resetDriver(void* context)
{
    void *thread = NULL;

    //UNREFERENCED_PARAMETER(context);

    // Check if our Mutex is valid !
    if (gResetThreadDataLock == NULL)
    {
        framework_Error("%s : gResetThreadDataLock is NULL",__FUNCTION__);
        return;
    }

    // Lock the mutex to access global pool
    framework_LockMutex(gResetThreadDataLock);

    // We are cleaning up ignore the reset !
    if (gIsCleanup)
    {
        // NOTE : No need to unlock as the Mutex will be deleted !
        return;
    }

    // If thread pool is NULL
    if (gResetThreadData == NULL)
    {
        // Create a new empty one.
        gResetThreadData = phOsalNfc_GetMemory(sizeof(sResetThreadData_t));
        memset(gResetThreadData,0,sizeof(sResetThreadData_t));
    }

    // Create Thread for Async reset
    framework_CreateThread(&thread,resetThread,NULL);

    // Now save the thread object pointer for further delete
    if (gResetThreadData->resetThreadPtr == NULL)
    {
        gResetThreadData->resetThreadPtr = thread;
        gResetThreadData->next           = NULL;
    }else
    {
        sResetThreadData_t *newThreadData;

        newThreadData = phOsalNfc_GetMemory(sizeof(sResetThreadData_t));
        memset(newThreadData,0,sizeof(sResetThreadData_t));

        newThreadData->resetThreadPtr = thread;
        // Enqueue global data at the begining
        newThreadData->next = gResetThreadData;
        // Replace global with current one
        gResetThreadData = newThreadData;
    }

    framework_UnlockMutex(gResetThreadDataLock);
}

void stopDriver(void* context)
{
    //UNREFERENCED_PARAMETER(context);
    if (stopThreadPtr)
    {
        framework_JoinThread(stopThreadPtr);
        framework_DeleteThread(stopThreadPtr);
    }
    framework_CreateThread(&stopThreadPtr,stopThread,NULL);
}

void sendHciCallback(void* ctx,eHECIStatus status,size_t sz)
{
    tNfcHECIContext_t *context = (tNfcHECIContext_t*)ctx;
    if (status == HECI_SUCCESS)
    {
        phHciWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,sz);
    }else
    {
        phHciWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,sz);
    }
}

void receiveHciCallback(void* ctx,eHECIStatus status,uint8_t* buffer,size_t sz)
{
    tNfcHECIContext_t *context = (tNfcHECIContext_t*)ctx;

    if (status == HECI_SUCCESS)
    {
        phHciWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,buffer,sz);
    }else
    {
        phHciWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_FAILED,buffer,sz);
    }
}


/** See documentation in header file */
NFCSTATUS phHciWrapper_Init(void** stackContext,void*   libNfcCtx)
{
    tNfcHECIContext_t   *newContext = NULL;
    eHECIStatus          eResult    = HECI_FAILED;

    phHECI_SetLog(1);

    newContext = phOsalNfc_GetMemory(sizeof(tNfcHECIContext_t));
    memset(newContext,0,sizeof(tNfcHECIContext_t));

    newContext->libNfcCtx = libNfcCtx;

    if (gResetThreadDataLock == NULL)
    {
        framework_CreateMutex(&gResetThreadDataLock);
        gIsCleanup = 0;
    }


    // Set reset Driver function.
    phHECI_SetResetFunc(resetDriver,NULL);

    // Set stop Driver function.
    phHECI_SetStopFunc(stopDriver,NULL);

    // Connect to HECI.
    eResult = phHECI_ConnectNormalMode(&(newContext->heciStackContext),newContext,sendHciCallback,receiveHciCallback,1);

    if (eResult == HECI_SUCCESS)
    {
        if (stackContext != NULL)
        {
            // Reset Radio by toggle.
            eResult = phHECI_ChangePowerModeRadio(newContext->heciStackContext,0);
            if (eResult != HECI_SUCCESS)
            {
                // Change power radio not supported => use Soft reset !
                phHECI_RadioSoftwareReset(newContext->heciStackContext);
            }else
            {
                // Hardware reset supported => enable it !
                phHECI_ChangePowerModeRadio(newContext->heciStackContext,1);
            }

            // CGRFKILLONLY01
            // Disable RfKill
            // NOTE : RfKill will be automatically triggered when driver send autonomous command.
            //phHECI_RfKill(newContext....) should be commented in CGRFKILLONLY01
            phHECI_RfKill(newContext->heciStackContext,0);
            // END CGRFKILLONLY01
            *stackContext = newContext;
            nfcHeciContext = newContext;
        }
    }else
    {
        // Cleanup HECI connection
        phHciWrapper_Deinit(newContext);
        *stackContext = NULL;
        return NFCSTATUS_FAILED;
    }

    return NFCSTATUS_SUCCESS;
}

/** See documentation in header file */
NFCSTATUS phHciWrapper_Deinit(void* stackContext)
{
    tNfcHECIContext_t *context = (tNfcHECIContext_t*)stackContext;
    eHECIStatus        eResult = HECI_FAILED;

    if (context == NULL)
    {
        return NFCSTATUS_SUCCESS;
    }

    // CGRFKILLONLY01
    // We power off the radio.
    //FA those will have to be commented later for CGRFKILLONLY01
    eResult = phHECI_ChangePowerModeRadio(context->heciStackContext,0);

    if (eResult != HECI_SUCCESS)
    {
        framework_Warn("Cannot Power Off radio on this platform.");
    }
    // END CGRFKILLONLY01

    // NOTE : RF Kill done by disconnect.
    eResult = phHECI_Disconnect(context->heciStackContext);

    if (eResult != HECI_SUCCESS)
    {
        framework_Error("Failed to disconnect phHECI_Disconnect() return : %X",eResult);
        return NFCSTATUS_FAILED;
    }
    nfcHeciContext = NULL;
    phOsalNfc_FreeMemory(stackContext);

    return NFCSTATUS_SUCCESS;
}

/** See documentation in header file */
NFCSTATUS phHciWrapper_Reset(void* stackContext)
{
    // Avoid warning
    stackContext = NULL;
    // Do nothing ATM.
    return NFCSTATUS_SUCCESS;
}

/** See documentation in header file */
NFCSTATUS phHciWrapper_SendHCI(void* stackContext,uint8_t* data,size_t sz)
{
    tNfcHECIContext_t *context = (tNfcHECIContext_t*)stackContext;
    eHECIStatus        eResult = HECI_FAILED;

    eResult = phHECI_SendHCIFrame(context->heciStackContext,data,sz);

    if (eResult != HECI_PENDING)
    {
        return NFCSTATUS_BOARD_COMMUNICATION_ERROR;
    }
    return NFCSTATUS_PENDING;

}

/** See documentation in header file */
NFCSTATUS phHciWrapper_ReceiveHCI(void* stackContext,uint8_t* dataDestPtr,size_t sz)
{
    tNfcHECIContext_t *context = (tNfcHECIContext_t*)stackContext;
    eHECIStatus        eResult = HECI_FAILED;

    eResult = phHECI_ReceiveHCIFrame(context->heciStackContext,dataDestPtr,sz);

    if (eResult != HECI_PENDING)
    {
        return NFCSTATUS_BOARD_COMMUNICATION_ERROR;
    }

    return NFCSTATUS_PENDING;
}

void phHciWrapper_EmergencyRelease()
{
    phHECI_EmergencyRelease();
}


void phHciWrapper_DisableRfKill()
{
    if(nfcHeciContext)
    {
        tNfcHECIContext_t *context = (tNfcHECIContext_t*)nfcHeciContext;
        phHECI_RfKill(context->heciStackContext,0);
    }
}

void phHciWrapper_ClearResources()
{
    if (gResetThreadDataLock == NULL)
    {
        return;
    }

    // Lock the mutex to access global pool
    framework_LockMutex(gResetThreadDataLock);

    gIsCleanup = 1;

    // If thread pool is NULL
    if (gResetThreadData == NULL)
    {
        if (gResetThreadDataLock)
        {
            void *lock = gResetThreadDataLock;
            gResetThreadDataLock = NULL;
            framework_UnlockMutex(lock);
            framework_DeleteMutex(lock);
        }

        return;
    }

    // Clean up chained list of reset thread.
    while(gResetThreadData)
    {
        // Store next structure
        sResetThreadData_t *next = gResetThreadData->next;

        // Check if the Thread is valid
        if (gResetThreadData->resetThreadPtr)
        {
            // Wait for it to complete
            framework_JoinThread(gResetThreadData->resetThreadPtr);
            // Delete it !
            framework_DeleteThread(gResetThreadData->resetThreadPtr);
            gResetThreadData->resetThreadPtr = NULL;
        }

        phOsalNfc_FreeMemory(gResetThreadData);
        gResetThreadData = next;
    }

    framework_UnlockMutex(gResetThreadDataLock);

    framework_DeleteMutex(gResetThreadDataLock);
    gResetThreadDataLock = NULL;
}
