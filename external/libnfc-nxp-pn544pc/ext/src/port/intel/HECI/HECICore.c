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
 * \file HECICore.c
 * $Author: Eff'Innov Technologies $
 */

#include "port/intel/HECI/HECIApi.h"
#include "port/intel/HECI/HECIPorting.h"


#include "tools/core/framework_Allocator.h"
#include "tools/core/framework_Interface.h"
#include "tools/core/framework_Parcel.h"

#define LOG_HECI_EXCHANGE


typedef enum _eReadState
{
    READ_STATE_NONE,
    READ_STATE_WAIT,
    READ_STATE_HAVE_TO_READ,
    READ_STATE_HAVE_TO_EXIT
}eReadState;

/**
    Internal structure for HECI client context
*/
typedef struct _HECIData
{
    uint8_t               initialized;         /* true iff the HECI client is initialized successfuly */
    uint32_t              bufSize;             /* Size of the HECI maximum message size as recieved from FW during connection init */
    void                 *handle;              /* file handle to the HECI driver */
    void                 *receiveIfLock;       /* Lock over the Receive interface */
    uint8_t*              lastReceiveBuffer;   /* A pointer to the buffer that was the last buffer to be passed to the async Receive call */
    uint32_t              lastReceiveSize;     /* A pointer to the buffer that was the last buffer to be passed to the async Receive call */
    tHECIReceiveComplete  receiveCompleteCb;   /* Recieve Complete callback pointer */
    tHECISendComplete     sendCompleteCb;      /* Send Complete callback pointer */
    void                 *dispatchThread;      /* Dispatch thread context */
    void                 *userContext;         /* Caller context to be passed to callbacks */
    void                 *threadMutex;         /* Mutex to start async task */
    uint8_t               autoCleanUp;         /* Indicate if the CallDispatch thread have to clean up HECI Core.*/
    eReadState            currentReadState;    /*           */
} HECIData;

static void *autoCleanup_receiveIfLock  = NULL;
static void *autoCleanup_threadMutex    = NULL;
static void *autoCleanup_dispatchThread = NULL;

static uint8_t gHeciCoreLogEnabled = 0;

static void storePointerForCleanup(HECIData *context)
{
    autoCleanup_threadMutex    = context->threadMutex;
    context->threadMutex       = NULL;
    autoCleanup_receiveIfLock  = context->receiveIfLock;
    context->receiveIfLock     = NULL;
    autoCleanup_dispatchThread = context->dispatchThread;
    context->dispatchThread    = NULL;
}

static void checkAndReleasePreviousData()
{
    if (autoCleanup_dispatchThread)
    {
        // Wait thread maybe it does not complete !
        framework_JoinThread(autoCleanup_dispatchThread);
        framework_DeleteThread(autoCleanup_dispatchThread);
        autoCleanup_dispatchThread = NULL;
    }
    if (autoCleanup_receiveIfLock)
    {
        framework_DeleteMutex(autoCleanup_receiveIfLock);
        autoCleanup_receiveIfLock = NULL;
    }
    if (autoCleanup_threadMutex)
    {
        framework_DeleteMutex(autoCleanup_threadMutex);
        autoCleanup_threadMutex = NULL;
    }
}


// *************************************** Internal *************************************

static void* CallbackDispatchThread(void *lpThreadParameter)
{
    HECIData             *heciContext = (HECIData*)lpThreadParameter;
    tHECIReceiveComplete  receiveCompleteCb;
    uint8_t              *lastReceiveBuffer;
    uint32_t              receivedSize;
    eHECICommStatus       readStatus;

    if(gHeciCoreLogEnabled)
    {
        framework_Log("Starting thread %s",__FUNCTION__);
    }

    while (heciContext->initialized)
    {
        framework_LockMutex(heciContext->threadMutex);
        if (heciContext->currentReadState == READ_STATE_NONE)
        {
            heciContext->currentReadState = READ_STATE_WAIT;
            framework_WaitMutex(heciContext->threadMutex,0);
        }

        if (heciContext->currentReadState == READ_STATE_HAVE_TO_EXIT)
        {
            framework_UnlockMutex(heciContext->threadMutex);
            // Exit From loop.
            break;
        }else
        {
            heciContext->currentReadState = READ_STATE_NONE;
            framework_UnlockMutex(heciContext->threadMutex);
        }

        if (heciContext->handle != NULL)
        {
            readStatus = HECI_Read(heciContext->handle,heciContext->lastReceiveBuffer,heciContext->lastReceiveSize,&receivedSize);
        }
        // In case handle has been closed in the meanwhile, we don't notify.
        if (heciContext->handle != NULL)
        {
            framework_LockMutex(heciContext->receiveIfLock);
            // Keep callback pointer in local variable to avoid null pointer defererences
            // as the RegisterCallback method is not synchronized
            receiveCompleteCb = heciContext->receiveCompleteCb;

            // Get the last read buffer from context and free the context
            lastReceiveBuffer = (uint8_t*)heciContext->lastReceiveBuffer;

            // Clear context
            heciContext->lastReceiveBuffer = NULL;
            framework_UnlockMutex(heciContext->receiveIfLock);
            if (readStatus == HECICOMM_SUCCESS)
            {
#ifdef LOG_HECI_EXCHANGE
                framework_Log("****************** Receiving from HECI **********************");
                framework_HexDump(lastReceiveBuffer,receivedSize);
                framework_Log("****************** End Receiving from HECI **********************");
#endif // def LOG_HECI_EXCHANGE
                // Call the RecieveComplete callback
                if (receiveCompleteCb != NULL)
                {
                    receiveCompleteCb(heciContext->userContext,0 /* No error to report */,lastReceiveBuffer,(size_t)receivedSize);
                }
            }else
            {
                if (receiveCompleteCb != NULL)
                {
                    receiveCompleteCb(heciContext->userContext,1 ,lastReceiveBuffer,(size_t)receivedSize);
                }
            }
        }
    }

    if (gHeciCoreLogEnabled)
    {
        framework_Log("Exiting successfuly from %s",__FUNCTION__);
    }

    return 0;
}

static tHECIStatus HECIGetClient(HECIData* heciContext)
{
    eHECICommStatus result;

    result = HECI_GetClient(heciContext->handle,&(heciContext->bufSize));

    if (result == HECICOMM_SUCCESS)
    {
        return hsSuccess;
    }


    return hsCommunicationError;
}

// *************************************** HECI API Implementation *************************************

void HECIRegisterCallbacks(tHECIClientHandle hHECI,void* userContext,tHECIReceiveComplete receiveCompleteCb,tHECISendComplete sendCompleteCb)
{
    HECIData* heciContext = (HECIData*)hHECI;

    // Verify input parameters
    if ((heciContext == NULL) || (heciContext->initialized == 0))
    {
        return;
    }

    // Save to context
    heciContext->userContext       = userContext;
    heciContext->receiveCompleteCb = receiveCompleteCb;
    heciContext->sendCompleteCb    = sendCompleteCb;
}

tHECIStatus HECICreate(tHECIClientHandle* phHECI)
{
    HECIData    *heciContext;
    tHECIStatus  status = hsSuccess;
    uint8_t      unused = 0;

    // Free any leaked data from previous sessions.
    checkAndReleasePreviousData();

    // Verify output pointer
    if (phHECI == NULL)
    {
        return hsInvalidParams;
    }

    // Allocate memory for the new HECI client
    heciContext = (HECIData*)framework_AllocMem(sizeof(HECIData));
    if (heciContext == NULL)
    {
        return hsOutOfMemory;
    }

    // Reset memory allocation
    memset(heciContext, 0x00, sizeof(HECIData));

    do
    {
        // Initialize the different fields
        heciContext->initialized  = 0;
        heciContext->handle       = NULL;
        heciContext->currentReadState = READ_STATE_NONE;

        framework_CreateMutex(&heciContext->receiveIfLock);

        // Try and open a handle to the HECI driver
        status = HECI_Init(&(heciContext->handle));
        if (status != hsSuccess)
        {
            break;
        }

        // Try and connect to the HECI client
        status = HECIGetClient(heciContext);
        if (status != hsSuccess)
        {
            break;
        }

        framework_CreateMutex(&(heciContext->threadMutex));

        // Mark we have completed initialization before starting thread
        heciContext->initialized = 1;

        // Create the thread for reading from HECI
        framework_CreateThread(&(heciContext->dispatchThread),CallbackDispatchThread,heciContext);
        if (heciContext->dispatchThread == NULL)
        {
            heciContext->initialized = 0;
            status = hsOutOfResources;
            break;
        }
        // Mark we have completed initialization
        heciContext->initialized = 1;
    }while(unused);

    // Complete initialization by supplying the handle to the user
    if (status == hsSuccess)
    {
        *phHECI = (tHECIClientHandle)heciContext;
    }
    else
    {
        HECIDestroy(heciContext);
        // framework_FreeMem(heciContext);
        *phHECI = NULL;
    }

    return status;
}

tHECIStatus HECIReconnect(tHECIClientHandle hHECI)
{
    HECIData        *heciContext = (HECIData*)hHECI;
    tHECIStatus      status      = hsSuccess;
    void            *tempHandle  = NULL;
    uint8_t          unused      = 0;
    eHECICommStatus  result;

    framework_LockMutex(heciContext->threadMutex);

    do
    {
        if (heciContext->handle)
        {
            // Copy handle.
            tempHandle = heciContext->handle;
            // Set handle in context to NULL, the Dispatch thread will block in the wait...
            heciContext->handle = NULL;
            // Disconnect from HECI Driver.
            result = HECI_Deinit(tempHandle);
            if (result != HECICOMM_SUCCESS)
            {
                status = hsCommunicationError;
            }
        }
        // Reconnect to HECI Driver.
        result = HECI_Init(&(heciContext->handle));
        if (result != HECICOMM_SUCCESS)
        {
            status = hsCommunicationError;
            break;
        }

        // Get Info client.
        status = HECIGetClient(heciContext);
        if (status != hsSuccess)
        {
            break;
        }
    }while(unused);

    // If status == hsSuccess all communication should be fine !
    framework_UnlockMutex(heciContext->threadMutex);
    return status;
}


void HECIDestroy(tHECIClientHandle hHECI)
{
    HECIData* heciContext = (HECIData*)hHECI;

    // Verify input parameters
    if (heciContext == NULL)
    {
        return;
    }

    // Tell the thread we should stop.
    heciContext->initialized = 0;

    // If we are blocked in the wait ...
    if (heciContext->threadMutex)
    {
        // We change Current state !
        framework_LockMutex(heciContext->threadMutex);
        if (heciContext->currentReadState == READ_STATE_WAIT)
        {
            heciContext->currentReadState = READ_STATE_HAVE_TO_EXIT;
            framework_NotifyMutex(heciContext->threadMutex,0);
        }else
        {
            heciContext->currentReadState = READ_STATE_HAVE_TO_EXIT;
        }
        framework_UnlockMutex(heciContext->threadMutex);

        // Even if we are reading something, we will close the handle,
        // the thread will see that the current state is updated.
    }

    // Close the file handle
    if (heciContext->handle != NULL)
    {
        void *tempHandle = heciContext->handle;
        heciContext->handle = NULL;
        HECI_Deinit(tempHandle);
    }

    // Stop and clear the callback thread
    if (heciContext->dispatchThread != NULL)
    {
        // Check if we are not called from the dispatchThread
        if (framework_GetCurrentThreadId() == framework_GetThreadId(heciContext->dispatchThread))
        {
            if (heciContext->autoCleanUp == 0)
            {
                if (gHeciCoreLogEnabled)
                {
                    framework_Log("HECICore marked for cleanup.");
                }
                // We are in the same thread. Raise the flag first.
                heciContext->autoCleanUp = 1;
                storePointerForCleanup(heciContext);
                return;
            }else
            {
                framework_Warn("HECICore already marked for cleanup.");
                return;
            }
        }else
        {
            // Different thread we can clean up.
            framework_JoinThread(heciContext->dispatchThread);
            framework_DeleteThread(heciContext->dispatchThread);
            heciContext->dispatchThread = NULL;
        }
    }

    // Thread stopped, we can delete safely threadMutex
    if (heciContext->threadMutex)
    {
        framework_DeleteMutex(heciContext->threadMutex);
        heciContext->threadMutex = NULL;
    }

    // Reset values to initial values
    heciContext->bufSize = 0;

    // Delete Critical Section objects
    framework_DeleteMutex(heciContext->receiveIfLock);

    // Clear out the rest of the values in the HECI context
    heciContext->userContext       = NULL;
    heciContext->lastReceiveBuffer = NULL;
    heciContext->receiveCompleteCb = NULL;
    heciContext->sendCompleteCb    = NULL;
    // Free memory.
    framework_FreeMem(heciContext);
}

tHECIStatus HECIReceiveMessage( tHECIClientHandle hHECI, uint8_t* buffer, uint32_t iLength )
{
    HECIData        *heciContext = (HECIData*)hHECI;
    tHECIStatus      status    = hsSuccess;
    uint8_t          unused    = 0;

    // Verify input parameters
    if ((heciContext == NULL) || (heciContext->initialized == 0))
    {
        framework_Error("HECIReceiveMessage failed : hsInvalidHandle");
        return hsInvalidHandle;
    }

    if ((NULL == buffer) || (iLength < heciContext->bufSize))
    {
        framework_Error("HECIReceiveMessage failed : hsInvalidParams");
        return hsInvalidParams;
    }

    framework_LockMutex(heciContext->receiveIfLock);
    do
    {
        // Check we don't have Read in progress
        if (heciContext->lastReceiveBuffer != NULL)
        {
            status = hsReceiveAlreadyInProgress;
            break;
        }
        heciContext->lastReceiveBuffer = buffer;
        heciContext->lastReceiveSize   = (uint32_t)iLength;

        framework_LockMutex(heciContext->threadMutex);

        if (heciContext->currentReadState == READ_STATE_WAIT)
        {
            heciContext->currentReadState = READ_STATE_HAVE_TO_READ;
            framework_NotifyMutex(heciContext->threadMutex,0);
        }else
        {
            heciContext->currentReadState = READ_STATE_HAVE_TO_READ;
        }
        framework_UnlockMutex(heciContext->threadMutex);

        status = hsIoInProgress;

    }while (unused);

    framework_UnlockMutex(heciContext->receiveIfLock);

    return status;
}

tHECIStatus HECISendMessage(tHECIClientHandle hHECI,uint8_t* buffer,uint32_t iLength)
{
    HECIData    *heciContext = (HECIData*)hHECI;
    tHECIStatus  status      = hsSuccess;
    uint8_t      unused      = 0;

    // Verify input parameters
    if ((heciContext == NULL) || (heciContext->initialized == 0))
    {
        framework_Error("HECISendMessage failed : hsInvalidHandle");
        return hsInvalidHandle;
    }

    // Verify input parameters
    if ((NULL == buffer) ||(iLength > heciContext->bufSize) ||(iLength == 0))
    {
        framework_Error("HECISendMessage failed : hsInvalidParams");
        return hsInvalidParams;
    }
    do
    {
        eHECICommStatus  result;
        uint32_t         writtenSize = 0;
        // Issue a Write command to the radio.
#ifdef LOG_HECI_EXCHANGE
        framework_Log("****************** Sending to HECI **********************");
        framework_HexDump(buffer,(uint32_t)iLength);
        framework_Log("****************** End Sending to HECI **********************");
#endif // def LOG_HECI_EXCHANGE

        result = HECI_Write(heciContext->handle, buffer, iLength,&writtenSize);

        if (result != HECICOMM_SUCCESS)
        {
            status = hsCommunicationError;
        }else
        {
            status = hsIoInProgress;
        }

        if (heciContext->sendCompleteCb)
        {
            if (result != HECICOMM_SUCCESS)
            {

                heciContext->sendCompleteCb(heciContext->userContext, 1,0);
            }else
            {
                heciContext->sendCompleteCb(heciContext->userContext, 0,writtenSize);
            }
        }
    }
    while (unused);



    return status;
}

uint32_t HECIGetMaximumMessageSize (tHECIClientHandle hHECI)
{
    HECIData* heciContext = (HECIData*)hHECI;

    // Verify input parameters
    if ((heciContext == NULL) || (heciContext->initialized == 0))
    {
        return HECI_INVALID_MESSAGE_SIZE;
    }

    return heciContext->bufSize;
}


void HECIEmergencyRelease(tHECIClientHandle hHECI)
{
    HECIData* heciContext = (HECIData*)hHECI;

    if ((heciContext)&&(heciContext->handle))
    {
        HECI_EmergencyRelease(heciContext->handle);
    }
}

void HECISetLog(uint8_t value)
{
    gHeciCoreLogEnabled = value;
    HECI_SetLog(value);
}
