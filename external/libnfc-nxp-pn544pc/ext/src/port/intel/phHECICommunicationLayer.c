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
 * \file phHECICommunicationLayer.c
 * $Author: Eff'Innov Technologies $
 */

#include "phHECICommunicationLayer.h"

#include "tools/core/framework_Interface.h"
#include "tools/core/framework_Parcel.h"
#include "tools/core/framework_Timer.h"
#include "tools/core/framework_Allocator.h"

#include "port/intel/HECI/HECIApi.h"
#include "port/intel/NfcMeTypes.h"
#include "port/intel/phOsEvent_Interface.h"
#include "port/intel/WaiterNotifier.h"

// #define FAKE_TIMEOUT
// #define FAKE_FAILED_HECI_CONNECT
// #define FAKE_ME_RESET

#define CONNECT_CONTEXT_MAGIC_KEYWORD ((void *)0x02488420)

/**
 * Vendor/Version specific values
 */
#define SW_IVN                               0x01
#define SW_VENDOR_CODE                       0x01

/**
 * Some constants to control the code behavior
 */
// "Connect" command timeout.
#define CONNECT_COMMAND_TIMEOUT  2000
// Timeout after a command is timed out
#define SEND_COMMAND_TIMEOUT     1000
// Timeout after a command is timed out
#define RF_KILL_DEINIT_TIMEOUT   160
// Timeout to wait "Connect" after FW Upgrade.
#define CONNECT_DWL_TIMEOUT     10000
// Maximum retry after connection failure
#define MAX_CONNECTION_RETRY        4
// Maximum retry after send command failure
#define MAX_RESEND_FRAME_COUNT      3
// NOT USED : If send failed severals time, it's possible to disconnect/reconnect MAX_RECONNECT_RETRY times.
// See handleReconnection() function code.
#define MAX_RECONNECT_RETRY         3
// When first connection failed, wait this delay and retry.
#define CONNECTION_RETRY_DELAY1  1000
// Same as previous, but for 2nd retry
#define CONNECTION_RETRY_DELAY2  2000
// Same as previous, but for 3rd retry
#define CONNECTION_RETRY_DELAY3  4000
// Same as previous, but for 4th retry
#define CONNECTION_RETRY_DELAY4  6000
// Max Get Radio Ready State retry
#define MAX_GET_RADIO_READY_STATE_RETRY 5


// ******************** DEBUG FLAGS *************************
// CG TEST : SC38942 + SC38447
// #define SIMULATE_HECI_OPEN_FAILED
// #define SIMULATE_HCI_TIMEOUT
// #define SIMULATE_HCI_SEND_FAILED_SYNC
// #define SIMULATE_HCI_SEND_FAILED_ASYNC


/**
 * @brief Internal IMEI / HECI connection status
 **/
typedef enum _eConnectionResult
{
    CONNECTION_SUCCESS,
    CONNECTION_HECI_MISSING,
    CONNECTION_HECI_VERSION_MISMATCH,
    CONNECTION_FAILED
}eConnectionResult;

/**
 * @brief Internal status of current request to IMEI/HECI
 **/
typedef enum _eCommResult
{
    COMM_SUCCESS,
    COMM_TIMEOUT,
    COMM_FAILED
}eCommResult;

/**
 * @brief Internal status of a request (RF Kill, Send HCI, Power mode ...)
 **/
typedef enum _eRequestStatus
{
    REQUEST_OFF,
    REQUEST_INPROG
}eRequestStatus;


/**
 * @brief Internal Request status of a request (Success / Failed)
 **/
typedef enum _eMeRequestStatus
{
    ME_REQUEST_SUCCESS,
    ME_REQUEST_FAIL
}eMeRequestStatus;

/**
 * @brief Internal state of download.
 **/
typedef enum _eDownloadMode
{
    eDownloadMode_Off,
    eDownloadMode_Deactivating,
    eDownloadMode_Activating,
    eDownloadMode_On
} eDownloadMode;

/**
 * @brief Internal request enums.
 **/
typedef enum _eRequest
{
    eRequestNone,
    eRequestQueryMeInfo,
    eRequestSwitchMode,
    eRequestRfKill,
    eRequestPowerState,
    eRequestGetRadioReadyState
} eRequest;

/**
 * @brief HECI Communication Layer Context.
 **/
typedef struct tNfcMeContext
{
    tHECIClientHandle        hHECI;                        // HECI connection handle
    uint8_t                  FwIvn;                        // ME Firmware IVN
    uint8_t                  NFCC_Vendor;                  // NFC Vendor ID
    uint16_t                 MeMajor;                      // ME Major version
    uint16_t                 MeMinor;                      // ME Minor version
    uint16_t                 MeHotfix;                     // ME HotFix version
    uint16_t                 MeBuild;                      // ME Build number
    uint16_t                 iRequestId;                   // Request ID (See specs for more details)
    uint16_t                 iLastRequestIdSent;           // Last Request ID used
    uint16_t                 iLastAckRequestId;            // Last ACK Request ID.
    uint32_t                 iLastReadBufferSize;          // Last Size to read requested by phHECI_ReceiveHCIFrame()
    uint8_t                 *sendParcel;                   // Last Sent frame to IMEI/HECI as Parcel.
    uint32_t                 sendRetry;                    // Retry counter.
    void                    *userContext;                  // User context to provide when calling callback.
    uint8_t                 *recvBuffer;                   // Internal Reception buffer
    uint32_t                 recvBufferSize;               // Internal Reception buffer
    uint8_t                 *initialRecvBuffer;            // Buffer provided by Receive function.
    uint32_t                 initialRecvBufferSize;        // Size of buffer provided by Receive function.
    void                    *temporaryRecvParcel;          // If no read is armed, we store data inside this buffer
    void                    *readBufferLock;               // Lock for read.
    void                    *hRecieveDoneEvent;            // Internal Mutex/Notifier
    eCommResult              status;                       // Status of current request.
    void                    *timer;                        // Timer for command send.
    void                    *requestLock;                  // Request mutex.
    eRequestStatus           requestStatus;                // Request status
    heciCallback_SendHci    *sendHciCb;                    // Send command callback. Set by upper layer.
    heciCallback_ReceiveHci *receiveHciCb;                 // Receive callback. Set by upper layer.
    tMeInfo_t               *meQueryInfo;                  // Internal structure that contains ME Informations.
    eMeRequestStatus         meRequestStatus;              // ME Request status.
    void                    *meRequestLock;                // ME Request mutex.
    void                    *meRequestTimeout;             // ME Request timeout.
    eRequest                 meRequest;                    // ME Request type.
    uint32_t                 heciNextConnectionRetryDelay; // Next delay to wait before reconnect.
    uint8_t                  heciConnectRetry;             // Connection retry counter
    uint8_t                  shouldSendConnect;            // Indicate if connect should be sent when opening IMEI/HECI connection.
    eDownloadMode            downloadMode;                 // Download mode status.
    eHECIMode                eRequestedMode;               // ME status (Normal, FW upgrade)
    void                    *eventSubscription;            // Event Handle (to catch ME Reset)
    void                    *timerConnect;                 // Timer for CONNECT command.
    void                    *hciExchangeLock;              // Mutex/Notifier for HCI raw exchange.
    eHECIStatus              hciExchangeStatus;            // HCI Raw Exchange status?
    uint8_t                 *hciExchangeBuffer;            // HCI Raw Exchange data buffer.
    void                    *hciExchangeTimer;             // HCI Raw Exchange timer
    size_t                   hciReceivedSize;
    uint32_t                 hciExchangeBufferSize;        // HCI Raw Exchange data buffer size.
    uint8_t                  hciExchangeNoAck;             // Indicate if we expect SEND ACK from ME or not.
    uint8_t                  secureElementPipeId;          // Contains secure element pipe id if available else 0.
    uint8_t                  readerAPipeId;                // Contains gate A id if available else 0.
    uint8_t                  isInitHciParam;               // Indicate if we have get all info from HCI. (Pipe Id ...)
    uint8_t                  meResetDetected;              // Indicate if ME reset is detected.
    uint8_t                  meGetRadioInProg;             // Indicate that a Get Radio Status is in progress.
    uint8_t                  meIsRadioReady;               // Indicate if radio is ready.
    uint8_t                  dirtyHciWorkaround;           // We may receive HCI during a specific sequence.
    uint8_t                  emergencyTriggered;           // This flag is raised when shutdown emergency is used.
} tNfcMeContext_t;

static tNfcMeContext_t         *gCurrentContext;    // a pointer on current running context
static heci_ResetDriver        *gResetDriver;       // Function set by upper layer. This function is called when a reset is required.
static void                    *gResetContext;      // User context to provide with gResetDriver function.
static heci_StopDriver         *gStopDriver;        // Function set by upper layer.
static void                    *gStopContext;       // User context to provide with gStopDriver function.
static uint8_t                  gLogEnabled = 0;    // Indicate if logs are enabled or not.
static heciCallback_Log        *gLogFunc    = NULL; // Global Data exchange log function.

// Forward declarations.
static void generateErrorEvent(tNfcMeContext_t* context,eFaultStatus status);
static void timerHandler_ConnectTimeout(void *context);
static void timerHandler_SendTimeout(void *context);
static void timerHandler_ConnectChangeModeTimeout(void *context);
static void timerHandler_MERequestTimeout(void *context);
static void ConnectResponseHandler(void* pCallerContext,uint32_t iErrorCode,uint8_t* pData,uint32_t DataSize);
static void NfcMeIfReceiveComplete(void* pCallerContext,uint32_t iErrorCode,uint8_t* pData,uint32_t DataSize);
static void NfcMeIfSendComplete(void* pCallerContext,uint32_t iErrorCode, uint32_t size);
static eConnectionResult heciOpenConnection(void *context,uint8_t sendConnect);
static eConnectionResult heciCloseConnection(void *context);
static eCommResult heciSendConnectToMe(tNfcMeContext_t *context);
static eCommResult heciGetRadioStateReady(tNfcMeContext_t *context);
static eCommResult heciConnectToMe(tNfcMeContext_t *context);
static eHECIStatus heciRfKill(tNfcMeContext_t *context,uint8_t onOff,uint8_t isDeinit);
static eHECIStatus heciPowerCommand(tNfcMeContext_t *context,uint8_t onOff);
static eHECIStatus heciIsRadioReady(tNfcMeContext_t *context,uint8_t *isReady);
static void increaseRequestIdLocked(tNfcMeContext_t *context);
static void buildMeHeaderLocked(tNfcMeContext_t *context,eNfcMeCommand command,void *destParcel,uint16_t size,uint8_t useReqId);
static void updateRequestIdInParcelLocked(tNfcMeContext_t *context,void *frameParcelToUpdate);
static void checkAndInitHCI(tNfcMeContext_t *context);
static void heciMeResetCallBack(void* context);

// ****************************************** DEBUG
#ifdef FAKE_ME_RESET

void *fakeResetThread(void* context)
{
    if (gLogEnabled)
    {
        framework_Log(">>>>>>>>>>>>>>>>>> Wait 15 secs before Restart Driver");
    }
    framework_MilliSleep(15000);

    if (gResetDriver)
    {
        if (gLogEnabled)
        {
            framework_Log(">>>>>>>>>>>>>>>>>> Restart Driver");
        }
        gResetDriver(gResetContext);
    }

}
#endif // def FAKE_ME_RESET


// ****************************************** INTERNAL HELPERS

static void storeRecvDataInTempParcelLocked(tNfcMeContext_t *context,const uint8_t *recvData,uint32_t size)
{
    void           *tempParcel;
    const uint8_t  *tempData;
    uint32_t        tempSize;

    framework_ParcelCreate(&tempParcel);

    tempData = framework_ParcelGetData(context->temporaryRecvParcel);
    tempSize = framework_ParcelGetSize(context->temporaryRecvParcel);

    // Save Previous temporary buffer
    if (tempSize)
    {
        framework_ParcelWriteRaw(tempParcel,tempData,tempSize);
    }

    // Concat new data
    framework_ParcelWriteRaw(tempParcel,recvData,size);
    // Rewind parcel
    framework_ParcelRewind(tempParcel);
    // Delete previous temp buffer.
    framework_ParcelDelete(context->temporaryRecvParcel);
    // Set new temporary buffer.
    context->temporaryRecvParcel = tempParcel;
}

static uint8_t isDataInRecvTempParcelLocked(tNfcMeContext_t *context)
{
    uint32_t tempSize;
    tempSize = framework_ParcelGetSize(context->temporaryRecvParcel);

    if (tempSize)
    {
        return 1;
    }
    return 0;
}

static void popDataInRecvTempParcelLocked(tNfcMeContext_t *context,uint8_t *outBuffer,uint32_t outBufferSize,uint32_t *outBufferPopedSize)
{
    const uint8_t     *tempData     = NULL;
    uint32_t           tempSize;

    tempData = framework_ParcelGetData(context->temporaryRecvParcel);
    tempSize = framework_ParcelGetSize(context->temporaryRecvParcel);

    if (outBufferSize<=tempSize)
    {
        void    *tempParcel;
        memcpy(outBuffer,tempData,outBufferSize);

        framework_ParcelForward(context->temporaryRecvParcel,(uint32_t)outBufferSize);

        tempData = framework_ParcelDataAtCurrentPosition(context->temporaryRecvParcel);
        tempSize = framework_ParcelGetRemainingDataSize(context->temporaryRecvParcel);

        // Create a new parcel
        framework_ParcelCreate(&tempParcel);
        // Copy remaining data to it
        framework_ParcelWriteRaw(tempParcel,tempData,tempSize);
        framework_ParcelRewind(tempParcel);

        // Delete temp parcel
        framework_ParcelDelete(context->temporaryRecvParcel);
        // Set new one !
        context->temporaryRecvParcel = tempParcel;

        *outBufferPopedSize = outBufferSize;
    }else
    {
        memcpy(outBuffer,tempData,tempSize);

        // No more data in Parcel clean up !
        framework_ParcelClear(context->temporaryRecvParcel);

        *outBufferPopedSize = tempSize;
    }
}

/**
 * @brief Notify when a request is completed
 *
 * @param context HECI Communication Layer context.
 * @param status Result to notify.
 * @return void
 **/
static void internalNotify(tNfcMeContext_t *context,eCommResult status)
{
    context->status = status;
    // framework_NotifyMutex(context->hRecieveDoneEvent,1);
    WaiterNotifier_Notify(context->hRecieveDoneEvent,1);
}

/**
 * @brief Allocate the internal Recv buffer automatically.
 *
 * @param context HECI Communication Layer context.
 * @return void
 **/
static void checkAndAllocateRecvBuffer(tNfcMeContext_t *context)
{
    uint32_t heciBufferSize = 0;

    // Retrieve HECI maximum buffer size.
    heciBufferSize = HECIGetMaximumMessageSize(context->hHECI);

    // Is return value is valid.
    if (heciBufferSize != HECI_INVALID_MESSAGE_SIZE)
    {
        // Check if current buffer is valid and if buffer is big enougth.
        if ((!context->recvBuffer) || (heciBufferSize > context->recvBufferSize))
        {
            // Free any previous recv buffer.
            if (context->recvBuffer)
            {
                // Release any previous buffer.
                framework_FreeMem(context->recvBuffer);
                context->recvBuffer = NULL;
            }

            // Allocate new recv buffer.
            context->recvBuffer     = framework_AllocMem(heciBufferSize);
            context->recvBufferSize = heciBufferSize;
            if (gLogEnabled)
            {
                framework_Log("RecvBuffer allocated ********************************");
            }
        }
    }
}

/**
 * @brief Internaly handle connection retry.
 *
 * @param context HECI Communication Layer context.
 * @return void
 **/
static void handleInternalRetryConnection(tNfcMeContext_t *context)
{
    framework_Warn("%s sleep for  : %i ms",__FUNCTION__,context->heciNextConnectionRetryDelay);
    framework_MilliSleep(context->heciNextConnectionRetryDelay);
    switch (context->heciNextConnectionRetryDelay)
    {
        case CONNECTION_RETRY_DELAY1:
        {
            context->heciNextConnectionRetryDelay = CONNECTION_RETRY_DELAY2;
            break;
        }
        case CONNECTION_RETRY_DELAY2:
        {
            context->heciNextConnectionRetryDelay = CONNECTION_RETRY_DELAY3;
            break;
        }
        case CONNECTION_RETRY_DELAY3:
        {
            context->heciNextConnectionRetryDelay = CONNECTION_RETRY_DELAY4;
            break;
        }
        case CONNECTION_RETRY_DELAY4:
        {
            // Last Attempt ...
            break;
        }
        default:
        {
        }
    }
}

/**
 * @brief Handle reconnection. Used when sending a command failed several times.
 *
 * @param context HECI Communication Layer context.
 * @return 1 if succeed 0 if failed.
 **/
static uint8_t handleReconnection(tNfcMeContext_t *context)
{
    context->heciNextConnectionRetryDelay = CONNECTION_RETRY_DELAY1;

    while((context->heciConnectRetry < MAX_RECONNECT_RETRY)&&(context->emergencyTriggered == 0))
    {
        tHECIStatus status;
        eCommResult result;

        if (gLogEnabled)
        {
            framework_Log("Try to Reconnect to HECI Driver : attempt = %i",context->heciConnectRetry);
        }

        context->heciConnectRetry++;

        status = HECIReconnect(context->hHECI);

        if (status != hsSuccess)
        {
            framework_Error("%s : failed to reconnect to HECI.",__FUNCTION__);
            handleInternalRetryConnection(context);
            continue;
        }

        // Connect again !
        result = heciConnectToMe(context);

        if (result != COMM_SUCCESS)
        {
            continue;
        }

        context->heciConnectRetry = 0;
        context->heciNextConnectionRetryDelay = CONNECTION_RETRY_DELAY1;
        return 1;
    }
    return 0;
}

/**
 * @brief This function retrieves secure element and reader A pipe id if available.
 *
 * @param context tNfcMeContext
 * @return eHECIStatus HECI_SUCCESS if no error.
 **/
static eHECIStatus heciGetPipeId(tNfcMeContext_t *context)
{
    const uint8_t reqPipeIdSecureElem[] = {0x81,0x10,0xA1,0x00,0xA1};
    const uint8_t ansPipeIdSecureElem[] = {0x81,0x80,0x00,0xA1,0x00,0xA1,0xFF};
    const uint8_t reqPipeIdGateA[]      = {0x81,0x10,0x13,0x00,0x13};
    const uint8_t ansPipeIdReaderA[]    = {0x81,0x80,0x00,0x13,0x00,0x13,0xFF};
    size_t        receivedSize          = 0;
    eHECIStatus   eStatus               = HECI_FAILED;
    uint8_t       buffer[32];

    // Get Secure element pipe ID
    eStatus = phHECI_ExchangeHCI(context,reqPipeIdSecureElem,sizeof(reqPipeIdSecureElem),buffer,32,&receivedSize);

    if (eStatus == HECI_SUCCESS)
    {
        // Check if size match !
        if (receivedSize == sizeof(ansPipeIdSecureElem))
        {
            // Check if content match !
            if (memcmp(buffer,ansPipeIdSecureElem,receivedSize-1) == 0) // Ignore last byte
            {
                // We got the right answer, gather information
                context->secureElementPipeId = buffer[receivedSize-1] & 0x0F;
            }else
            {
                // Answer content does not match
                eStatus = HECI_FAILED;
            }
        }else
        {
            // Answer size does not match
            eStatus = HECI_FAILED;
        }
    }

    if (eStatus != HECI_SUCCESS)
    {
        framework_Warn("Cannot get secure element pipe Id. Maybe there is no secure element on radio.");
        // Something failed no secure element !
        context->secureElementPipeId = 0x00;
    }

    // Get Gate A pipe ID
    eStatus = phHECI_ExchangeHCI(context,reqPipeIdGateA,sizeof(reqPipeIdGateA),buffer,32,&receivedSize);

    if (eStatus == HECI_SUCCESS)
    {
        // Check if size match !
        if (receivedSize == sizeof(ansPipeIdReaderA))
        {
            if (memcmp(buffer,ansPipeIdReaderA,receivedSize-1) == 0) // Ignore last byte
            {
                // We got the right answer, gather information
                context->readerAPipeId = buffer[receivedSize-1] & 0x0F;
            }else
            {
                // Answer content does not match
                eStatus = HECI_FAILED;
            }
        }else
        {
            // Answer size does not match
            eStatus = HECI_FAILED;
        }
    }

    if (eStatus != HECI_SUCCESS)
    {
        framework_Error("Cannot get Reader A pipe ID.");
        // Something failed no gate A pipe ID !
        context->readerAPipeId = 0x00;
    }

    return eStatus;
}

// ****************************************** TIMEOUT HANDLING HELPERS

/**
 * @brief Handle timeout mechanism. Should be called in timeout callbacks.
 * When a timeout is triggered, this function guarantee that no answer have been
 * received in the meanwhile. This function is thread safe.
 *
 * @param context HECI Communication Layer context.
 * @return 0 the timeout is valid no answer have been received. 1 an answer have been received the timeout callback shall do nothing.
 **/
static uint8_t handleAnswerInTimeoutCallback(tNfcMeContext_t *context)
{
    framework_LockMutex(context->requestLock);

    if (gLogEnabled)
    {
        framework_Log("%s : Timeout triggered faulty frame is : ",__FUNCTION__);
        framework_HexDump(framework_ParcelGetData(context->sendParcel),framework_ParcelGetSize(context->sendParcel));
    }

    switch (context->requestStatus)
    {
        case REQUEST_OFF:
        {
            // Request have succeed in the meanwhile ...
            framework_UnlockMutex(context->requestLock);
            if (gLogEnabled)
            {
                framework_Log("%s Timeout ok",__FUNCTION__);
            }
            return 1;
            break;
        }

        case REQUEST_INPROG:
        {
            context->requestStatus = REQUEST_OFF;
            break;
        }
        default:
        {
            framework_Error("%s %s l.%i :This case should never happened",__FILE__,__FUNCTION__,__LINE__);
        }
    }
    framework_UnlockMutex(context->requestLock);

    return 0;
}

/**
 * @brief Handle timeout mechanism. This function should be called when a Answer callback get called.
 * When an answer is received, this function should be called to check if timeout is not triggered in the meanwhile.
 * This function stop internal timer automatically.
 * This function is thread safe.
 *
 * @param context HECI Communication Layer context.
 * @return 0 the timeout was not yet triggered. 1 the timeout was triggered, the receive function shall do nothing.
 **/
static uint8_t handleTimeoutInCallBack(tNfcMeContext_t *context)
{
    // NOTE : Timer could be triggered in this meanwhile ...
    framework_LockMutex(context->requestLock);
    if (gLogEnabled)
    {
        framework_Log("%s : No Timeout",__FUNCTION__);
    }
    switch(context->requestStatus)
    {
        case REQUEST_OFF:
        {
            // Timeout already triggered ...
            framework_UnlockMutex(context->requestLock);
            return 1;
            break;
        }
        case REQUEST_INPROG:
        {
            // If timeout callback called in the meanwhile, we set status to OFF
            // That's mean it has been handled.
            context->requestStatus = REQUEST_OFF;
            break;
        }
        default:
        {
            framework_Error("%s %s l.%i :This case should never happened",__FILE__,__FUNCTION__,__LINE__);
            break;
        }
    }

    framework_UnlockMutex(context->requestLock);

    // Stop the timer
    framework_TimerStop(context->timer);
    return 0;
}

static uint8_t cancelTimerRequest(tNfcMeContext_t *context)
{
    // NOTE : Timer could be triggered in this meanwhile ...
    framework_LockMutex(context->requestLock);
    if (gLogEnabled)
    {
        framework_Log("%s : No Timeout",__FUNCTION__);
    }
    switch(context->requestStatus)
    {
        case REQUEST_OFF:
        {
            // Timeout already triggered ...
            framework_UnlockMutex(context->requestLock);
            return 1;
            break;
        }
        case REQUEST_INPROG:
        {
            // If timeout callback called in the meanwhile, we set status to OFF
            // That's mean it has been handled.
            context->requestStatus = REQUEST_OFF;
            break;
        }
        default:
        {
            framework_Error("%s %s l.%i :This case should never happened",__FILE__,__FUNCTION__,__LINE__);
            break;
        }
    }

    framework_UnlockMutex(context->requestLock);

    // Stop the timer
    framework_TimerStop(context->timer);
    return 0;
}

// ****************************************** MISC HELPERS

/**
 * @brief This function handles SEND retry mechanism.
 *
 * @param context HECI Communication Layer context.
 * @return 0 if failed, 1 if succeed.
 **/
static uint8_t resendFrame(void *context)
{
    tNfcMeContext_t *ctx = (tNfcMeContext_t*)context;
    tHECIStatus      status;
#ifdef FAKE_TIMEOUT
    uint8_t *tmp = NULL;
#endif // def FAKE_TIMEOUT

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
        framework_HexDump(framework_ParcelGetData(ctx->sendParcel), framework_ParcelGetSize(ctx->sendParcel));
    }
    if (ctx->sendRetry < MAX_RESEND_FRAME_COUNT)
    {
        // Setup request status ...
        framework_LockMutex(ctx->requestLock);
        if (ctx->requestStatus != REQUEST_OFF)
        {
            framework_Error("ERROR : phHECI_SendHCIFrame called but previous Send not ACK.");
        }
        ctx->requestStatus = REQUEST_INPROG;
        ctx->sendRetry++;

        increaseRequestIdLocked(ctx);
        updateRequestIdInParcelLocked(ctx,ctx->sendParcel);

        framework_UnlockMutex(ctx->requestLock);

        // Start a new timeout
        framework_TimerStart(ctx->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,ctx);
#ifdef FAKE_TIMEOUT
        tmp = (uint8_t*)framework_ParcelGetData(ctx->sendParcel);
        tmp[0] = NfcMeCommandSend;
#endif // def FAKE_TIMEOUT
        // Send the message again !
        status = HECISendMessage(ctx->hHECI, (uint8_t*)framework_ParcelGetData(ctx->sendParcel), framework_ParcelGetSize(ctx->sendParcel));

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return 1;
    }
    // No more retry !
    return 0;
}

static eCommResult heciGetRadioStateReady(tNfcMeContext_t *context)
{
    eCommResult result     = COMM_FAILED;
    eHECIStatus heciStatus = HECI_FAILED;
    uint32_t    retries    = 0;
    uint8_t     isReady    = 0;

    context->meGetRadioInProg = 1;

    // Polling Get radio Ready.
    while (isReady == 0)
    {
        heciStatus = heciIsRadioReady(context,&isReady);

        if (heciStatus != HECI_SUCCESS)
        {
            // NOTE : Here we should return an Error.
            framework_Warn("Failed to send Get Radio Ready State Command. Retrying ...");
        }

        if (retries == MAX_GET_RADIO_READY_STATE_RETRY)
        {
            // We reached the maximum retries count. Radio is in degraded mode.
            context->meGetRadioInProg = 0;
            return COMM_SUCCESS;
        }else
        {
            if (context->sendRetry == 0)
            {
                framework_MilliSleep(200);
            }else
            {
                // A timeout of 1 second occurs
                // Radio seems not ready, continue process. Firmware download should occurs later.
                context->meGetRadioInProg = 0;
                return COMM_SUCCESS;
            }
        }
        retries++;
    }

    context->meGetRadioInProg = 0;

    return COMM_SUCCESS;
}

static eCommResult heciSendConnectToMe(tNfcMeContext_t *context)
{
    void                  *parcel     = NULL;
    tNfcHeciMessageHeader  request    = {0} ;
    uint8_t                unused     = 0;
    eCommResult            result     = COMM_FAILED;
    eHECIStatus            status;

    framework_ParcelCreate(&parcel);
    // framework_CreateMutex(&(context->hRecieveDoneEvent));
    WaiterNotifier_Create(&(context->hRecieveDoneEvent));

    do
    {
        // We set receive callback to get the response from FW to Connect command
        HECIRegisterCallbacks(context->hHECI, context, ConnectResponseHandler, NULL);

        // Fill in request
        request.Command  = NfcMeCommandMaintenance;
        request.DataSize = 0x0003;

        // Put header in parcel.
        framework_ParcelWriteRaw(parcel,&request,sizeof(tNfcHeciMessageHeader));
        framework_ParcelWriteByte(parcel,NfcMeMaintenanceSubcommandConnect);
        framework_ParcelWriteByte(parcel,SW_IVN);
        framework_ParcelWriteByte(parcel,SW_VENDOR_CODE);

        // A request will be issued
        framework_LockMutex(context->requestLock);
        if (context->requestStatus != REQUEST_OFF)
        {
            framework_Error("ERROR : phHECI_SendHCIFrame called but previous Send not ACK.");
        }
        context->requestStatus = REQUEST_INPROG;

        // Copy parcel into sendParcel
        framework_ParcelSetData(context->sendParcel,framework_ParcelGetData(parcel),framework_ParcelGetSize(parcel));

        framework_UnlockMutex(context->requestLock);

        // Send the Connect command
        if (hsIoInProgress != HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(parcel), framework_ParcelGetSize(parcel)))
        {
            break;
        }

        checkAndAllocateRecvBuffer(context);

        // Start timer
        framework_TimerStart(context->timer,CONNECT_COMMAND_TIMEOUT,timerHandler_ConnectTimeout,context);

        // Start async read
        status = HECIReceiveMessage(context->hHECI, context->recvBuffer, context->recvBufferSize);
        if ((status != hsSuccess) && (status != hsIoInProgress) && (status != hsReceiveAlreadyInProgress))
        {
            break;
        }

        // Waiting for asynchronous answer or timeout ...
        // framework_WaitMutex(context->hRecieveDoneEvent,1);
        WaiterNotifier_Wait(context->hRecieveDoneEvent,1);

        result = context->status;

    }while(unused);

    // Cleanup temporary event
    if (context->hRecieveDoneEvent != NULL)
    {
        // framework_DeleteMutex(context->hRecieveDoneEvent);
        WaiterNotifier_Delete(context->hRecieveDoneEvent);
    }

    if (parcel)
    {
        framework_ParcelDelete(parcel);
    }

    HECIRegisterCallbacks(context->hHECI,context,NfcMeIfReceiveComplete,NfcMeIfSendComplete);

    return result;
}

static eCommResult heciConnectToMe(tNfcMeContext_t *context)
{
    eCommResult  result = COMM_FAILED;

    if (gLogEnabled)
    {
        framework_Log("%s : heciSendConnectToMe()",__FUNCTION__);
    }

    result = heciSendConnectToMe(context);

    if (gLogEnabled)
    {
        framework_Log("%s : heciSendConnectToMe() returns %X",__FUNCTION__,result);
    }

    if (result != COMM_SUCCESS)
    {
        return result;
    }

    if (gLogEnabled)
    {
        framework_Log("%s : heciGetRadioStateReady()",__FUNCTION__);
    }

    result = heciGetRadioStateReady(context);

    if (gLogEnabled)
    {
        framework_Log("%s : heciGetRadioStateReady() returns %X",__FUNCTION__,result);
    }

    if (result != COMM_SUCCESS)
    {
        return result;
    }

    if (gLogEnabled)
    {
        framework_Log("%s : heciSendConnectToMe()",__FUNCTION__);
    }

    result = heciSendConnectToMe(context);

    if (gLogEnabled)
    {
        framework_Log("%s : heciSendConnectToMe() returns %X",__FUNCTION__,result);
    }

    if (result != COMM_SUCCESS)
    {
        return result;
    }

    return result;
}

/**
 * @brief Open HECI connection.
 * This function handles automatically internal retries and co. This function blocks until success or failure.
 *
 * @param context HECI Communication Layer context.
 * @param sendConnect 1 if CONNECT should be sent, 0 if not.
 * @return eConnectionResult
 **/
static eConnectionResult heciOpenConnection(void *context,uint8_t sendConnect)
{
    tHECIClientHandle      hHECI      = NULL;
    tHECIStatus            HECIStatus = hsInvalidHandle;
    eConnectionResult      result     = CONNECTION_FAILED;
    uint32_t               retryCount = 0   ;
    tNfcMeContext_t       *ctx        = (tNfcMeContext_t*)context;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }

    ctx->heciNextConnectionRetryDelay = CONNECTION_RETRY_DELAY1;

    while((retryCount < MAX_CONNECTION_RETRY)&&(ctx->emergencyTriggered == 0))
    {
        // Initialize HECI client
#ifdef FAKE_FAILED_HECI_CONNECT
        HECIStatus = hsInvalidHandle;
#else
        HECIStatus = HECICreate(&hHECI);
#endif // def FAKE_FAILED_HECI_CONNECT

        if (hsDriverMissing == HECIStatus)
        {
            framework_Warn("HECI Missing.");
            retryCount++;
            // Wait until reconnection
            handleInternalRetryConnection(ctx);
        }else if (hsSuccess != HECIStatus)
        {
            framework_Warn("HECI Connection failed HECIStatus : %X.",HECIStatus);
            retryCount++;
            // Wait until reconnection
            handleInternalRetryConnection(ctx);
        }else
        {
            ctx->hHECI = hHECI;

            if (sendConnect)
            {
                eCommResult status;

                status = heciConnectToMe(context);

                switch (status)
                {
                    case COMM_SUCCESS:
                    {
                        // Connection succeed.
                        result = CONNECTION_SUCCESS;
                        break;
                    }
                    case COMM_TIMEOUT:
                    {
                        framework_Warn("Connection (%i) timeout retrying after %i ms",retryCount,ctx->heciNextConnectionRetryDelay);
                        // Connection failed or timeout
                        retryCount++;
                        // Wait until reconnection
                        handleInternalRetryConnection(ctx);
                        break;
                    }
                    case COMM_FAILED:
                    {
                        framework_Warn("Connection (%i) failed retrying after %i ms",retryCount,ctx->heciNextConnectionRetryDelay);
                        // Connection failed or timeout
                        retryCount++;
                        // Wait until reconnection
                        handleInternalRetryConnection(ctx);
                        break;
                    }
                    default:
                    {
                        framework_Error("%s %s l.%i :This case should never happened",__FILE__,__FUNCTION__,__LINE__);
                    }
                }

                if (status == COMM_SUCCESS)
                {
                    // If we're here - we're successful!
                    if (ctx->FwIvn != SW_IVN)
                    {
                        result = CONNECTION_HECI_VERSION_MISMATCH;
                        break;
                    }

                    if (ctx->NFCC_Vendor != SW_VENDOR_CODE)
                    {
                        result = CONNECTION_HECI_VERSION_MISMATCH;
                        break;
                    }
                    break;
                }else
                {
                    // General Failure !
                    result = CONNECTION_FAILED;
                }

                if (result != CONNECTION_SUCCESS)
                {
                    generateErrorEvent(ctx,FAULT_CONNECTION_FAILED);
                    heciCloseConnection(context);
                }
            }else
            {
                result = CONNECTION_SUCCESS;
                break;
            }
        }
        framework_Error("Connection with HECI failed after %i retries.",retryCount);
    }

#ifdef SIMULATE_HECI_OPEN_FAILED
    result = CONNECTION_FAILED;
#endif // def SIMULATE_HECI_OPEN_FAILED

    if (result == CONNECTION_SUCCESS)
    {
        // We got the maintenance command response - Set callback context
        HECIRegisterCallbacks(hHECI,ctx,NfcMeIfReceiveComplete,NfcMeIfSendComplete);
        ctx->hHECI = hHECI;
    }else
    {
        // Failure closing HECI connection.
        heciCloseConnection(context);
    }

    return result;
}

/**
 * @brief Close Connection with HECI.
 *
 * @param context HECI Communication Layer context.
 * @return eConnectionResult
 **/
static eConnectionResult heciCloseConnection(void *context)
{
    tNfcMeContext_t   *ctx = (tNfcMeContext_t*)context;
    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }

    // SC39529 : If in the meanwhile an answer arrives it fails !
    // Unregister callbacks
    // HECIRegisterCallbacks(ctx->hHECI, NULL, NULL, NULL);
    // END SC39529
    // Close HECI handle
    if (ctx->hHECI)
    {
        HECIDestroy(ctx->hHECI);
        ctx->hHECI = NULL;
    }

    return CONNECTION_SUCCESS;
}

/**
 * @brief Send a RF KILL command and wait for result.
 * This function blocks until success or failure.
 *
 * @param context HECI Communication Layer context.
 * @param onOff RF KILL status
 * @return eHECIStatus
 **/
static eHECIStatus heciRfKill( tNfcMeContext_t* context, uint8_t onOff, uint8_t isDeinit)
{
    tHECIStatus status;
    // Verify parameters
    if (context->hHECI == HECI_INVALID_CLIENT)
    {
        return HECI_INVALID_PARAM;
    }

    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : %s called but previous Send not ACK.",__FUNCTION__);
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    context->requestStatus = REQUEST_INPROG;
    context->sendRetry     = 0;

    buildMeHeaderLocked(context,NfcMeCommandMaintenance,context->sendParcel,0x0002,0);

    framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandRfKillMode);

    framework_UnlockMutex(context->requestLock);

    // framework_LockMutex(context->meRequestLock);
    WaiterNotifier_Lock(context->meRequestLock);

    context->meRequest = eRequestRfKill;

    if (onOff)
    {
        framework_ParcelWriteByte(context->sendParcel,1);
    }else
    {
        framework_ParcelWriteByte(context->sendParcel,0);
    }


    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);
    if (isDeinit)
    {
        framework_TimerStart(context->meRequestTimeout,RF_KILL_DEINIT_TIMEOUT,timerHandler_MERequestTimeout,context);
    }

    // Send the message
    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));
    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        context->meQueryInfo = NULL;
        context->meRequest   = eRequestNone;
        if (isDeinit)
        {
            framework_TimerStop(context->meRequestTimeout);
        }
        // framework_UnlockMutex(context->meRequestLock);
        WaiterNotifier_Unlock(context->meRequestLock);
        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    // Pending block until answer !
    // framework_WaitMutex(context->meRequestLock,0);
    WaiterNotifier_Wait(context->meRequestLock,0);
    context->meRequest   = eRequestNone;
    if (isDeinit)
    {
        framework_TimerStop(context->meRequestTimeout);
    }
    // framework_UnlockMutex(context->meRequestLock);
    WaiterNotifier_Unlock(context->meRequestLock);

    if(context->meRequestStatus == ME_REQUEST_SUCCESS)
    {
        return HECI_SUCCESS;
    }else
    {
        return HECI_FAILED;
    }
}

/**
 * @brief Send POWER COMMAND and wait for result.
 * This function blocks until success or failure.
 *
 * @param context HECI Communication Layer context.
 * @param onOff 1 ON, 0 Off
 * @return eHECIStatus
 **/
static eHECIStatus heciPowerCommand(tNfcMeContext_t *context,uint8_t onOff)
{
    tHECIStatus status;
    // Verify parameters
    if (context->hHECI == HECI_INVALID_CLIENT)
    {
        return HECI_INVALID_PARAM;
    }

    if (context->downloadMode != eDownloadMode_Off)
    {
        framework_Warn("heciPowerCommand called, but we are download/degraded mode. Ignore the request.");
        return HECI_FAILED;
    }

    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : heciPowerCommand called but previous Send not ACK.");
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    context->requestStatus = REQUEST_INPROG;
    context->sendRetry     = 0;

    buildMeHeaderLocked(context,NfcMeCommandMaintenance,context->sendParcel,0x002,0);

    framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandPowerMode);

    framework_UnlockMutex(context->requestLock);

    // framework_LockMutex(context->meRequestLock);
    WaiterNotifier_Lock(context->meRequestLock);

    context->meRequest = eRequestPowerState;

    if (onOff)
    {
        framework_ParcelWriteByte(context->sendParcel,1);
    }else
    {
        framework_ParcelWriteByte(context->sendParcel,0);
    }


    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);

    // Send the message
    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));
    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        context->meQueryInfo = NULL;
        context->meRequest   = eRequestNone;
        // framework_UnlockMutex(context->meRequestLock);
        WaiterNotifier_Unlock(context->meRequestLock);

        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    // Pending block until answer !
    // framework_WaitMutex(context->meRequestLock,0);
    WaiterNotifier_Wait(context->meRequestLock,0);

    context->meRequest   = eRequestNone;
    // framework_UnlockMutex(context->meRequestLock);
    WaiterNotifier_Unlock(context->meRequestLock);

    if(context->meRequestStatus == ME_REQUEST_SUCCESS)
    {
        // Need to reinit HCI as the radio has been power on/off.
        context->isInitHciParam = 0;
        return HECI_SUCCESS;
    }else
    {
        return HECI_FAILED;
    }
}

static eHECIStatus heciIsRadioReady(tNfcMeContext_t *context,uint8_t *isReady)
{
    tHECIStatus status;
    // Verify parameters
    if (context->hHECI == HECI_INVALID_CLIENT)
    {
        return HECI_INVALID_PARAM;
    }

    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : heciPowerCommand called but previous Send not ACK.");
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    if (isReady)
    {
        *isReady = 0;
    }

    context->requestStatus  = REQUEST_INPROG;
    context->sendRetry      = 0;
    context->meIsRadioReady = 0;

    buildMeHeaderLocked(context,NfcMeCommandMaintenance,context->sendParcel,0x001,0);

    framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandGetRadioReadyState);

    framework_UnlockMutex(context->requestLock);

    // framework_LockMutex(context->meRequestLock);
    WaiterNotifier_Lock(context->meRequestLock);

    context->meRequest = eRequestGetRadioReadyState;

    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);

    // Send the message
    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));
    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        context->meQueryInfo = NULL;
        context->meRequest   = eRequestNone;
        // framework_UnlockMutex(context->meRequestLock);
        WaiterNotifier_Unlock(context->meRequestLock);
        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    // Pending block until answer !
    // framework_WaitMutex(context->meRequestLock,0);
    WaiterNotifier_Wait(context->meRequestLock,0);

    context->meRequest   = eRequestNone;
    // framework_UnlockMutex(context->meRequestLock);
    WaiterNotifier_Unlock(context->meRequestLock);

    if(context->meRequestStatus == ME_REQUEST_SUCCESS)
    {
        if (isReady)
        {
            *isReady = context->meIsRadioReady;
        }
        return HECI_SUCCESS;
    }else
    {
        return HECI_FAILED;
    }
}


/**
 * @brief Increment Request ID securely.
 * This function is NOT thread safe.
 *
 * @param context HECI Communication Layer context.
 * @return void
 **/
static void increaseRequestIdLocked(tNfcMeContext_t *context)
{
    // NOTE : if iRequestId == 0 ACK of Send failed !
    context->iRequestId++;
    if (context->iRequestId == 0x0000)
    {
        context->iRequestId = 0x0001;
    }
    // Save the last request ID in context
    context->iLastRequestIdSent = context->iRequestId;
}

/**
 * @brief Build ME Request Header into a Parcel.
 * The given Parcel is cleared before building header.
 * This function is NOT thread safe.
 *
 * @param context HECI Communication Layer context.
 * @param command ME Command. (not Sub Command)
 * @param destParcel Destination Parcel.
 * @param size Size of the complete frame.
 * @param useReqId Indicate if Request ID should be used or not. Call increaseRequestIdLocked() before using request ID.
 * @return void
 **/
static void buildMeHeaderLocked(tNfcMeContext_t *context,eNfcMeCommand command,void *destParcel,uint16_t size,uint8_t useReqId)
{
    tNfcHeciMessageHeader  heciMessage;
    // Fill in the ME FW interface header
    memset(&heciMessage, 0x00, sizeof(tNfcHeciMessageHeader));
    heciMessage.Command   = (uint8_t)command;
    heciMessage.DataSize  = size;
    if (useReqId)
    {
        heciMessage.RequestId = context->iRequestId;
    }else
    {
        heciMessage.RequestId = 0;
    }

    framework_ParcelClear(destParcel);
    // Put HECI message header inside parcel
    framework_ParcelWriteRaw(context->sendParcel,&heciMessage,sizeof(tNfcHeciMessageHeader));
}

/**
 * @brief Update Request ID value from the given frame as Parcel.
 * This function is NOT thread safe.
 *
 * @param context HECI Communication Layer context.
 * @param frameParcelToUpdate Frame as Parcel.
 * @return void
 **/
static void updateRequestIdInParcelLocked(tNfcMeContext_t *context,void *frameParcelToUpdate)
{
    tNfcHeciMessageHeader *heciMessage;
    uint8_t               *rawParcel;
    const uint8_t         *origParcel;
    uint32_t               origSize;

    // Get Parcel content
    origParcel = framework_ParcelGetData(frameParcelToUpdate);
    origSize   = framework_ParcelGetSize(frameParcelToUpdate);

    // NOTE : Temporary buffer, we don't have the right to modify parcel pointed buffer directly !
    rawParcel  = framework_AllocMem(origSize);
    // Copy parcel into this buffer.
    memcpy(rawParcel,origParcel,origSize);

    // update request ID
    heciMessage = (tNfcHeciMessageHeader*) rawParcel;
    heciMessage->RequestId = context->iRequestId;

    // Update parcel.
    framework_ParcelSetData(frameParcelToUpdate,rawParcel,origSize);

    // Free temporary buffer.
    framework_FreeMem(rawParcel);
}

// ****************************************** TIMEOUT CALLBACKS

static void timerHandler_ConnectTimeout(void *context)
{
    tNfcMeContext_t *ctx       = (tNfcMeContext_t*)context;
    uint8_t          isSucceed = 0;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }
    // Check if an answer was not received in the meanwhile.
    isSucceed = handleAnswerInTimeoutCallback(context);

    if (!isSucceed)
    {
        // Notify that timeout occurs !
        internalNotify(ctx,COMM_TIMEOUT);
    }
}

static void timerHandler_SendTimeout(void *context)
{
    tNfcMeContext_t *ctx       = (tNfcMeContext_t*)context;
    uint8_t          isSucceed = 0;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }
    // Check if an answer was not received in the meanwhile.
    isSucceed = handleAnswerInTimeoutCallback(ctx);

    if (!isSucceed)
    {
        // Try to send frame again.
        uint8_t isResend = 0;

        // A timeout occurs, generate a Log Event
        generateErrorEvent(ctx,FAULT_SEND_TIMEOUT);

        isResend = resendFrame(context);

        if (!isResend)
        {
            // Failed to send data to HECI.
            generateErrorEvent(ctx,FAULT_SEND_FAILED);

            // Depending on request, we set the associated answer.
            switch (ctx->meRequest)
            {
                case eRequestQueryMeInfo:
                case eRequestRfKill:
                case eRequestSwitchMode:
                case eRequestPowerState:
                case eRequestGetRadioReadyState:
                {
                    // Query Mode
                    // RF Kill
                    // Power State
                    // Switch mode.
                    ctx->meRequestStatus = ME_REQUEST_FAIL; // Error !
                    // framework_NotifyMutex(ctx->meRequestLock,1);
                    WaiterNotifier_Notify(ctx->meRequestLock,1);
                    break;
                }
                case eRequestNone:
                {
                    if (ctx->sendHciCb)
                    {
                        // Standard mode
                        ctx->sendHciCb(ctx->userContext,HECI_FAILED,0);
                    }
                    break;
                }
                default:
                {
                    framework_Error("Unhandled switch case : %s %i ctx->meRequest : %X",__FUNCTION__,__LINE__,ctx->meRequest);
                    // Need to handle it anyway !
                    if (ctx->sendHciCb)
                    {
                        // Standard mode
                        ctx->sendHciCb(ctx->userContext,HECI_FAILED,0);
                    }
                }
            }
        }
    }
}

void timerHandler_ConnectChangeModeTimeout(void* context)
{
    tNfcMeContext_t *ctx = (tNfcMeContext_t*)context;

    // We do not receive connect in time => notify error.
    framework_TimerStop(ctx->timerConnect);

    ctx->meRequestStatus = ME_REQUEST_FAIL;

    // framework_NotifyMutex(ctx->meRequestLock,1);
    WaiterNotifier_Notify(ctx->meRequestLock,1);
}

static void timerHandler_MERequestTimeout(void *context)
{
    tNfcMeContext_t *ctx       = (tNfcMeContext_t*)context;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }

    handleTimeoutInCallBack(ctx);

    // framework_LockMutex(ctx->meRequestLock);
    WaiterNotifier_Lock(ctx->meRequestLock);
    framework_TimerStop(ctx->meRequestTimeout);

    // Depending on request, we set the associated answer.
    switch (ctx->meRequest)
    {
        case eRequestQueryMeInfo:
        case eRequestRfKill:
        case eRequestSwitchMode:
        case eRequestPowerState:
        case eRequestGetRadioReadyState:
        {
            // Query Mode
            // RF Kill
            // Power State
            // Switch mode.
            ctx->meRequestStatus = ME_REQUEST_FAIL; // Error !
            // framework_NotifyMutex(ctx->meRequestLock,0);
            WaiterNotifier_Notify(ctx->meRequestLock,0);
            break;
        }
        case eRequestNone:
        default:
        {
            framework_Error("Unhandled switch case : %s %i ctx->meRequest : %X",__FUNCTION__,__LINE__,ctx->meRequest);
        }
    }
    // framework_UnlockMutex(ctx->meRequestLock);
    WaiterNotifier_Unlock(ctx->meRequestLock);
}


// ****************************************** COMMUNICATION CALLBACKS

static void ConnectResponseHandler(void* pCallerContext,uint32_t iErrorCode,uint8_t* pData,uint32_t DataSize)
{
    tNfcHeciMessageHeader       *response         = (tNfcHeciMessageHeader*)pData;
    tNfcMeContext_t             *context          = (tNfcMeContext_t*)pCallerContext;
    uint8_t                      isTimeoutHandled = 0;
    void                        *parcel           = NULL;
    eNfcMeMaintenanceSubcommand  eSubCommand;
    uint8_t                      FwIVN;
    uint8_t                      NFCC_Vendor;
    uint16_t                     MeMajor;
    uint16_t                     MeMinor;
    uint16_t                     MeHotfix;
    uint16_t                     MeBuild;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }
    // Timeout already handled ?
    isTimeoutHandled = handleTimeoutInCallBack(context);

    if (isTimeoutHandled)
    {
        // Timeout was called prior to us.
        framework_Error("%s : Connect received but too late. Already concidered as timed out !",__FUNCTION__);
        return;
    }

    // is Transmition succeed ?
    if (iErrorCode != 0x00)
    {
        framework_Error("%s : iErrorCode != 0x00",__FUNCTION__);
        internalNotify(context,COMM_FAILED);
        return;
    }

    if ((DataSize < sizeof(tNfcHeciMessageHeader))||(response->DataSize!=0x0B))
    {
        // Received size corrupted !
        framework_Error("%s : Received invalid CONNECT answer from ME.",__FUNCTION__);
        internalNotify(context,COMM_FAILED);
        return;
    }

    // Is it the right answer ?
    if (response->Command != NfcMeCommandMaintenance)
    {
        framework_Error("%s : This is not a NfcMeCommandMaintenance received from ME.",__FUNCTION__);
        internalNotify(context,COMM_FAILED);
        return;
    }

    // What is the status ?
    switch(response->Status)
    {
        case NfcMeStatusNormalMode: // Connected in standard mode
        {
            context->downloadMode = eDownloadMode_Off;
            break;
        }
        case NfcMeStatusFirwareUpdateMode: // Connected in Download mode
        {
            context->downloadMode = eDownloadMode_On;
            break;
        }
        default:
        {
            // Invalid status
            framework_Error("%s : received invalid status from ME : %X",__FUNCTION__,response->Status);
            internalNotify(context,COMM_FAILED);
            return;
        }
    }

    // All seems ok, we will parse the message Payload.
    framework_ParcelCreate(&parcel);
    // Put data into Parcel.
    framework_ParcelSetData(parcel,(uint8_t*)response,response->DataSize+sizeof(tNfcHeciMessageHeader));
    // Ignore header.
    framework_ParcelForward(parcel,sizeof(tNfcHeciMessageHeader));

    // Reading subcommand
    eSubCommand = framework_ParcelReadByte(parcel);
    // uint8_t FwIVN;
    FwIVN       = framework_ParcelReadByte(parcel);
    // uint8_t  NFCC_Vendor;
    NFCC_Vendor = framework_ParcelReadByte(parcel);
    // uint16_t MeMajor;
    MeMajor     = framework_ParcelReadShort(parcel);
    // uint16_t MeMinor;
    MeMinor     = framework_ParcelReadShort(parcel);
    // uint16_t MeHotfix;
    MeHotfix    = framework_ParcelReadShort(parcel);
    // uint16_t MeBuild;
    MeBuild     = framework_ParcelReadShort(parcel);

    // Verify that subcommand is legal
    if (eSubCommand != NfcMeMaintenanceSubcommandConnect)
    {
        framework_Error("%s : This is not a NfcMeMaintenanceSubcommandConnect subcommand received from ME.",__FUNCTION__);
        internalNotify(context,COMM_FAILED);
        // Clean up parcel
        framework_ParcelDelete(parcel);
        return;
    }

    // Save interface versions
    context->FwIvn       = FwIVN;
    context->NFCC_Vendor = NFCC_Vendor;
    context->MeMajor     = MeMajor;
    context->MeMinor     = MeMinor;
    context->MeHotfix    = MeHotfix;
    context->MeBuild     = MeBuild;

    // Notify the Connect interface we're done!
    internalNotify(context,COMM_SUCCESS);

    if (gLogEnabled)
    {
        framework_Log("***************************************************");
        framework_Log("%s : Connect successfully received : ",__FUNCTION__);
        framework_Log("%s : IVN        : 0x%X",__FUNCTION__,context->FwIvn);
        framework_Log("%s : NFC Vendor : 0x%X",__FUNCTION__,context->NFCC_Vendor);
        framework_Log("%s : ME         : %i.%i.%i.%i",__FUNCTION__,context->MeMajor,context->MeMinor,context->MeHotfix,context->MeBuild);
        if (context->downloadMode == eDownloadMode_Off)
        {
            framework_Log("%s : Radio : Normal mode",__FUNCTION__);
        }else if (context->downloadMode == eDownloadMode_On)
        {
            framework_Log("%s : Radio : Download mode",__FUNCTION__);
        }
        framework_Log("***************************************************");
    }

    // Clean up parcel
    framework_ParcelDelete(parcel);
}

static void NfcMeIfSendComplete(void* pCallerContext,uint32_t iErrorCode, uint32_t size)
{
    tNfcMeContext_t   *context = (tNfcMeContext_t*)pCallerContext;
    tHECIStatus        status;
    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }

    if (!context)
    {
        framework_Error("%s Context is NULL",__FUNCTION__);
        return;
    }

    // Call the assumed implementation of the Tx Complete method
    if (iErrorCode == 0)
    {
        if (context->hciExchangeNoAck)
        {
            uint8_t isTimeoutHandled = 0;

            // NOTE : Timer could be triggered in this meanwhile ...
            isTimeoutHandled = handleTimeoutInCallBack(context);

            if (!isTimeoutHandled)
            {
                // Zero out last request ID send
                context->iLastRequestIdSent = 0;

                if (context->sendHciCb)
                {
                    // FIXME sent size is wrong ?
                    context->sendHciCb(context->userContext,HECI_SUCCESS,size);
                }
            }
        }

        // NOTE : Do not handle timeout here, we will handle it when the HCI send will be confirmed.
        // Send succeed wait for ME answer !
        checkAndAllocateRecvBuffer(context);
        status = HECIReceiveMessage(context->hHECI, context->recvBuffer, context->recvBufferSize);
        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }
    }else
    {
        uint8_t isTimeoutHandled = 0;
        uint8_t isReconnect      = 0;

        if (gLogEnabled)
        {
            framework_Log("%s :  An error occurs during send ",__FUNCTION__);
        }

        // NOTE : Timer could be triggered in this meanwhile ...
        isTimeoutHandled = handleTimeoutInCallBack(context);

        if (isTimeoutHandled)
        {
            // Timeout was called prior to us.
            return;
        }

        // Check if this error
        if (context->meResetDetected)
        {
            isReconnect = handleReconnection(context);

            if (isReconnect)
            {
                uint8_t sendAgainResult = 0;
                // We reconnect successfully.

                // try to send frame that fail again !
                sendAgainResult = resendFrame(context);

                if (sendAgainResult)
                {
                    return;
                }
                // Else it's a failure !
            }
        }else
        {
            // General failure.
            // We definitly disconnect from HECI.
            heciCloseConnection(context);
        }

        // Send failed !
        if (gLogEnabled)
        {
            framework_Log("NfcMeIfSendComplete : SEND ACK\n");
        }

        switch (context->meRequest)
        {
            case eRequestRfKill:
            case eRequestSwitchMode:
            case eRequestPowerState:
            case eRequestQueryMeInfo:
            case eRequestGetRadioReadyState:
            {
                // Query Mode
                // RF Kill
                // Power State
                // Switch mode.
                if (gLogEnabled)
                {
                    framework_Log("DEBUG S3S4 : context->meRequestStatus = ME_REQUEST_FAIL;");
                }
                context->meRequestStatus = ME_REQUEST_FAIL; // Error !
                // framework_NotifyMutex(context->meRequestLock,1);
                WaiterNotifier_Notify(context->meRequestLock,1);
                if (gLogEnabled)
                {
                    framework_Log("DEBUG S3S4 : Nofity complete");
                }

                break;
            }
            case eRequestNone:
            {
                if (context->sendHciCb)
                {
                    // Standard mode
                    context->sendHciCb(context->userContext,HECI_FAILED,0);
                }
                break;
            }
            default:
            {
                framework_Error("Unhandled switch case : %s %i",__FUNCTION__,__LINE__);
            }
        }
    }
}

static void heciHelperRearmRequest(tNfcMeContext_t *context)
{
    // FIXME : We should rearm the timer !
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : %s called but previous Send not ACK.",__FUNCTION__);
        framework_UnlockMutex(context->requestLock);
    }

    context->requestStatus = REQUEST_INPROG;
    framework_UnlockMutex(context->requestLock);
}

static void NfcMeIfReceiveComplete(void* pCallerContext,uint32_t iErrorCode,uint8_t* pData,uint32_t DataSize)
{
    tNfcHeciMessageHeader *response         = (tNfcHeciMessageHeader*)pData;
    uint8_t                bShouldRearmRead = 1; /* This method will also rearm */
    tNfcMeContext_t       *context          = (tNfcMeContext_t*)pCallerContext;

    if (iErrorCode != 0)
    {
        framework_Error("%s Read error.",__FUNCTION__);
        return;
    }

    if (!context)
    {
        framework_Error("%s Context is NULL",__FUNCTION__);
        return;
    }

    if (DataSize < sizeof(tNfcHeciMessageHeader))
    {
        framework_Error("Invalid received answer, this frame is drop. Content :");
        framework_HexDump(pData,(uint32_t)DataSize);

        // NOTE : Invalid frame content we shall not take any risk, we ignore it.
        return;
    }
    if (gLogEnabled)
    {
        framework_Log("%s : command : %X",__FUNCTION__,response->Command);
    }
    // Parse the header
    switch (response->Command)
    {
        case NfcMeCommandMaintenance:
        {
            void                        *parcel = NULL;
            eNfcMeMaintenanceSubcommand  eSubCommand;
            uint8_t                      isTimeoutHandled = 0;

            // All seems ok, we will parse the message Payload.
            framework_ParcelCreate(&parcel);
            // Put data into Parcel.
            framework_ParcelSetData(parcel,(uint8_t*)response,response->DataSize+sizeof(tNfcHeciMessageHeader));
            // Ignore Header
            framework_ParcelForward(parcel,sizeof(tNfcHeciMessageHeader));
            // Parsing Data.
            eSubCommand = framework_ParcelReadByte(parcel);



            // NOTE : Any other command is related to a send command except Connect.
            // That's why we ignore timeout when it's this command.
            if ((eSubCommand != NfcMeMaintenanceSubcommandConnect)&&(eSubCommand != NfcMeMaintenanceSubcommandFwUpdateReceive))
            {
                // NOTE : Timer could be triggered in this meanwhile ...
                isTimeoutHandled = handleTimeoutInCallBack(context);

                if (isTimeoutHandled)
                {
                    // Timeout was called prior to us.
                    framework_ParcelDelete(parcel);
                    break;
                }
            }

            if (gLogEnabled)
            {
                framework_Log("receiving Sub Command : %X\n",eSubCommand);
            }

            if (eSubCommand == NfcMeMaintenanceSubcommandConnect)
            {
                // We receive an asynchronous Connect, maybe we change operating mode.
                if (gLogEnabled)
                {
                    framework_Log("DOWNLOAD_DEBUG : Received NfcMeMaintenanceSubcommandConnect with status : 0x%X",response->Status);
                }
                switch(response->Status)
                {
                    case NfcMeStatusNormalMode: // Connected in standard mode
                    {
                        if (context->meRequest == eRequestSwitchMode)
                        {
                            if (gLogEnabled)
                            {
                                framework_Log("DOWNLOAD_DEBUG : if (context->meRequest == eRequestSwitchMode)");
                            }
                            // Download mode is disabled ...
                            context->downloadMode = eDownloadMode_Off;

                            if (context->eRequestedMode == HECI_NORMAL)
                            {
                                // ... and it's what we want.
                                context->meRequestStatus = ME_REQUEST_SUCCESS;
                                if (gLogEnabled)
                                {
                                    framework_Log("DOWNLOAD_DEBUG : context->meRequestStatus = ME_REQUEST_SUCCESS;");
                                }
                            }else
                            {
                                // ... failed we asked for download mode but we are in normal mode
                                context->meRequestStatus = ME_REQUEST_FAIL;
                                if (gLogEnabled)
                                {
                                    framework_Log("DOWNLOAD_DEBUG : context->meRequestStatus = ME_REQUEST_FAIL;");
                                }
                            }

                            // Stop timer for connect
                            framework_TimerStop(context->timerConnect);

                            // framework_NotifyMutex(context->meRequestLock,1);
                            WaiterNotifier_Notify(context->meRequestLock,1);
                        }else
                        {
                            if (gLogEnabled)
                            {
                                framework_Log("DOWNLOAD_DEBUG : context->meRequest != eRequestSwitchMode");
                            }
                            if (context->meGetRadioInProg == 0)
                            {
                                // We don't ask to switch mode.
                                if (context->downloadMode != eDownloadMode_Off)
                                {
                                    // We switch from another mode to download mode off.
                                    // FIXME : We reset the driver, is it correct ?
                                    heciMeResetCallBack(context);
                                }// else simple notification we do nothing
                            }else
                            {
                                framework_Log("NfcMeMaintenanceSubcommandConnect : Receiving Connect during Get Radio State procedure, ignore it.");
                            }
                        }
                        break;
                    }
                    case NfcMeStatusFirwareUpdateMode: // Connected in Download mode
                    {
                        if (gLogEnabled)
                        {
                            framework_Log("DOWNLOAD_DEBUG : NfcMeStatusFirwareUpdateMode");
                        }
                        if (context->meRequest == eRequestSwitchMode)
                        {
                            if (gLogEnabled)
                            {
                                framework_Log("DOWNLOAD_DEBUG : if (context->meRequest == eRequestSwitchMode)");
                            }
                            // Download mode is enabled ...
                            context->downloadMode = eDownloadMode_On;

                            if (context->eRequestedMode == HECI_DOWNLOAD)
                            {
                                // ... and it's what we want.
                                context->meRequestStatus = ME_REQUEST_SUCCESS;
                                if (gLogEnabled)
                                {
                                    framework_Log("DOWNLOAD_DEBUG : context->meRequestStatus = ME_REQUEST_SUCCESS;");
                                }
                            }else
                            {
                                // ... failed we asked for download mode but we are in normal mode
                                context->meRequestStatus = ME_REQUEST_FAIL;
                                if (gLogEnabled)
                                {
                                    framework_Log("DOWNLOAD_DEBUG : context->meRequestStatus = ME_REQUEST_FAIL;");
                                }
                            }

                            // Stop timer for connect
                            framework_TimerStop(context->timerConnect);

                            // framework_NotifyMutex(context->meRequestLock,1);
                            WaiterNotifier_Notify(context->meRequestLock,1);
                        }else
                        {
                            if (gLogEnabled)
                            {
                                framework_Log("DOWNLOAD_DEBUG : context->meRequest != eRequestSwitchMode");
                            }
                            if (context->meGetRadioInProg == 0)
                            {
                                // We don't ask to switch mode.
                                if (context->downloadMode != eDownloadMode_On)
                                {
                                    // We switch from another mode to download mode on.
                                    // FIXME : We reset the driver, is it correct ?
                                    heciMeResetCallBack(context);
                                }// else simple notification we do nothing
                            }else
                            {
                                framework_Log("NfcMeMaintenanceSubcommandConnect : Receiving Connect during Get Radio State procedure, ignore it.");
                            }
                        }
                        break;
                    }
                    default:
                    {
                        return;
                    }
                }
            }else if (eSubCommand == NfcMeMaintenanceSubcommandVersionQuery)
            {
                if (context->meRequest == eRequestQueryMeInfo)
                {
                    if (context->meQueryInfo)
                    {
                        // Status 1 byte
                        eNfcMeQueryStatus status = (eNfcMeQueryStatus)framework_ParcelReadByte(parcel);

                        context->meRequestStatus = ME_REQUEST_SUCCESS;
                        if (status == NfcMeQueryStatusSuccess)
                        {
                            // RadioVersionSW 3 bytes
                            framework_ParcelReadRaw(parcel,context->meQueryInfo->RadioVersionSW,3);
                        }

                        if (status == NfcMeQueryStatusFailedGetRadio)
                        {
                            framework_ParcelForward(parcel,3);
                            memset(context->meQueryInfo->RadioVersionSW,0,3);
                        }

                        // Reserved 3 bytes
                        framework_ParcelForward(parcel,3);
                        // RadioVersionHard 3 bytes
                        framework_ParcelReadRaw(parcel,context->meQueryInfo->RadioVersionHard,3);
                        // Slave Address 1 bytes
                        context->meQueryInfo->Slave_Address = framework_ParcelReadByte(parcel);
                        // FwIVN 1 byte
                        context->meQueryInfo->FwIVN = framework_ParcelReadByte(parcel);
                        // NFCC Vendor ID 1 byte
                        context->meQueryInfo->NFCC_Vendor_ID = framework_ParcelReadByte(parcel);
                        // RadioType 1 byte
                        context->meQueryInfo->RadioType = framework_ParcelReadByte(parcel);

                        // Notify for answer.
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }else
                    {
                        framework_Error("In %s : NfcMeMaintenanceSubcommandVersionQuery, context->meQueryInfo is NULL",__FUNCTION__);
                    }
                }else
                {
                    framework_Error("Unexpected NfcMeMaintenanceSubcommandVersionQuery command received");
                    heciHelperRearmRequest(context);
                }

            }else if (eSubCommand == NfcMeMaintenanceSubcommandFwUpdateMode)
            {
                if (gLogEnabled)
                {
                    framework_Log("DOWNLOAD_DEBUG : receiving NfcMeMaintenanceSubcommandFwUpdateMode");
                }
                if (context->meRequest == eRequestSwitchMode)
                {
                    eNfcMeConnectStatus state = (eNfcMeConnectStatus)framework_ParcelReadByte(parcel);
                    if (gLogEnabled)
                    {
                        framework_Log("DOWNLOAD_DEBUG : if (context->meRequest == eRequestSwitchMode) : state = 0x%X",state);
                    }

                    switch (state)
                    {
                        // ME indicate we asked to switch to download mode.
                        case NfcMeStatusFirwareUpdateMode:
                        {
                            // We ask to switch to download mode.
                            if (context->eRequestedMode == HECI_DOWNLOAD)
                            {
                                // Check if already in download mode
                                if (context->downloadMode == eDownloadMode_On)
                                {
                                    // We are already in download mode.
                                    // Don't expect a connect.
                                    context->meRequestStatus = ME_REQUEST_SUCCESS;
                                    // framework_NotifyMutex(context->meRequestLock,1);
                                    WaiterNotifier_Notify(context->meRequestLock,1);
                                }else
                                {
                                    // We asked for download mode ! Wait for CONNECT.
                                    context->downloadMode = eDownloadMode_Activating;
                                    if (gLogEnabled)
                                    {
                                        framework_Log("DOWNLOAD_DEBUG : context->downloadMode = eDownloadMode_Activating;");
                                    }
                                    framework_TimerStart(context->timerConnect,CONNECT_DWL_TIMEOUT,timerHandler_ConnectChangeModeTimeout,context);
                                }
                            }else
                            {
                                // Error, we ask for Normal mode, but ME think we asked for Download mode.
                                framework_Error("Inconsistant ME state : Line : %s %i",__FUNCTION__,__LINE__);
                                // A request as been issued, we need to unlock other thread.
                                context->meRequestStatus = ME_REQUEST_FAIL;
                                // framework_NotifyMutex(context->meRequestLock,1);
                                WaiterNotifier_Notify(context->meRequestLock,1);
                            }
                            break;
                        }
                        // ME indicate we asked to switch to normal mode
                        case NfcMeStatusNormalMode:
                        {
                            // We ask to switch to normal mode.
                            if (context->eRequestedMode == HECI_NORMAL)
                            {
                                // Check if already in normal mode
                                if (context->downloadMode == eDownloadMode_Off)
                                {
                                    // We are already in normal mode.
                                    // Don't expect a connect.
                                    context->meRequestStatus = ME_REQUEST_SUCCESS;
                                    // framework_NotifyMutex(context->meRequestLock,1);
                                    WaiterNotifier_Notify(context->meRequestLock,1);
                                }else
                                {
                                    // We asked for normal mode ! Wait for CONNECT.
                                    context->downloadMode = eDownloadMode_Deactivating;
                                    if (gLogEnabled)
                                    {
                                        framework_Log("DOWNLOAD_DEBUG : context->downloadMode = eDownloadMode_Deactivating;");
                                    }
                                    framework_TimerStart(context->timerConnect,CONNECT_DWL_TIMEOUT,timerHandler_ConnectChangeModeTimeout,context);
                                }
                            }else
                            {
                                // Error, we ask for Download mode, but ME think we asked for normal mode.
                                framework_Error("Inconsistant ME state : Line : %s %i",__FUNCTION__,__LINE__);
                                // A request as been issued, we need to unlock other thread.
                                context->meRequestStatus = ME_REQUEST_FAIL;
                                // framework_NotifyMutex(context->meRequestLock,1);
                                WaiterNotifier_Notify(context->meRequestLock,1);
                            }
                            break;
                        }
                        default:
                        {
                            framework_Error("Wrong answer from ME : NfcMeMaintenanceSubcommandFwUpdateMode Unhandled status.");
                        }
                    }
                    // Now we should wait for asynchronous connect answer.
                }else
                {
                    framework_Error("Unexpected NfcMeMaintenanceSubcommandFirmwareUpdateEnter command received");
                    heciHelperRearmRequest(context);
                }
            }else if (eSubCommand == NfcMeMaintenanceSubcommandFwUpdateSend)
            {
                uint8_t state = framework_ParcelReadByte(parcel);
                if (gLogEnabled)
                {
                    framework_Log("DOWNLOAD_DEBUG: NfcMeMaintenanceSubcommandFwUpdateSend state : %X",state);
                }
                switch (state)
                {
                    case 0x00 :
                    {
                        if (gLogEnabled)
                        {
                            framework_Log("DOWNLOAD_DEBUG: NfcMeMaintenanceSubcommandFwUpdateSend response->DataSize : %i",response->DataSize);
                        }
                        if (context->sendHciCb)
                        {
                            // FIXME : Invalid size
                            context->sendHciCb(context->userContext,HECI_SUCCESS,response->DataSize);
                        }

                        break;
                    }
                    case 0x01 : // Failed to send packet to radio
                    {
                        if (context->sendHciCb)
                        {
                            context->sendHciCb(context->userContext,HECI_FAILED,0);
                        }
                        break;
                    }
                    case 0x02 : // Invalid packet type
                    {
                        if (context->sendHciCb)
                        {
                            context->sendHciCb(context->userContext,HECI_FAILED,0);
                        }
                        break;
                    }
                    case 0xFF : // Not in FW update mode.
                    {
                        if (context->sendHciCb)
                        {
                            context->sendHciCb(context->userContext,HECI_FAILED,0);
                        }
                        break;
                    }
                    default:
                    {
                        framework_Error("Wrong answer from ME : NfcMeMaintenanceSubcommandFwUpdateSend Unhandled status.");
                    }
                }

            }else if (eSubCommand == NfcMeMaintenanceSubcommandFwUpdateReceive)
            {
                const uint8_t *receivedData = framework_ParcelDataAtCurrentPosition(parcel);
                uint32_t       receivedSize = framework_ParcelGetRemainingDataSize(parcel);

                if (gLogEnabled)
                {
                    framework_HexDump(receivedData,(uint32_t)receivedSize);
                }

                // Copy received data to buffer provided by Receive function :
                if (context->initialRecvBufferSize >= receivedSize)
                {
                    memcpy(context->initialRecvBuffer,receivedData,receivedSize);
                    if(context->receiveHciCb)
                    {
                        context->receiveHciCb(context->userContext,HECI_SUCCESS,context->initialRecvBuffer,receivedSize);
                    }
                }else
                {
                    // Provided buffer too small.
                    if (gLogEnabled)
                    {
                        framework_Log(">>> Buffer too small : context->initialRecvBufferSize : %i need %i",context->initialRecvBufferSize,response->DataSize);
                        framework_HexDump(receivedData,receivedSize);
                    }
                    memcpy(context->initialRecvBuffer,receivedData,context->initialRecvBufferSize);
                    // Provide the buffer but it will be incomplete, se
                    if (context->receiveHciCb)
                    {
                        context->receiveHciCb(context->userContext,HECI_FAILED,context->initialRecvBuffer,context->initialRecvBufferSize);
                    }
                }
            }else if (eSubCommand == NfcMeMaintenanceSubcommandRfKillMode)
            {
                if (context->meRequest == eRequestRfKill)
                {
                    if (response->Status == 0x00)
                    {
                        // Sucess
                        context->meRequestStatus = ME_REQUEST_SUCCESS;
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }else
                    {
                        // Failed
                        context->meRequestStatus = ME_REQUEST_FAIL;
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }
                }else
                {
                    framework_Error("Unexpected NfcMeMaintenanceSubcommandRfKillMode command received");
                    heciHelperRearmRequest(context);
                }
            }else if (eSubCommand == NfcMeMaintenanceSubcommandPowerMode)
            {
                if (context->meRequest == eRequestPowerState)
                {
                    if (response->Status == NfcMePowerStatusRequestSuccess)
                    {
                        // Sucess
                        context->meRequestStatus = ME_REQUEST_SUCCESS;
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }else
                    {
                        // Failed
                        context->meRequestStatus = ME_REQUEST_FAIL;
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }
                }else
                {
                    framework_Error("Unexpected NfcMeMaintenanceSubcommandPowerMode command received");
                    heciHelperRearmRequest(context);
                }
            }else if (eSubCommand == NfcMeMaintenanceSubcommandGetRadioReadyState)
            {
                if (context->meRequest == eRequestGetRadioReadyState)
                {
                    uint8_t state = framework_ParcelReadByte(parcel);

                    if (response->Status == 0x00) // SUCCESS
                    {
                        // Sucess

                        if (state == 0x00)
                        {
                            context->meIsRadioReady = 1;
                        }else
                        {
                            context->meIsRadioReady = 0;
                        }

                        context->meRequestStatus = ME_REQUEST_SUCCESS;
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }else
                    {
                        // Failed
                        context->meRequestStatus = ME_REQUEST_FAIL;
                        // framework_NotifyMutex(context->meRequestLock,1);
                        WaiterNotifier_Notify(context->meRequestLock,1);
                    }
                }else
                {
                    framework_Error("Unexpected NfcMeMaintenanceSubcommandGetRadioReadyState command received");
                    heciHelperRearmRequest(context);
                }
            }else
            {
                framework_Warn("Receiving Unkown maintenance sub-command : NfcMeCommandMaintenance : %X",eSubCommand);
                heciHelperRearmRequest(context);
            }

            if (parcel)
            {
                framework_ParcelDelete(parcel);
                parcel = NULL;
            }

            break;
        }

        case NfcMeCommandReceive:
        {
            uint8_t *receivedData = ((uint8_t*)response)+sizeof(tNfcHeciMessageHeader);

            if (gLogEnabled)
            {
                framework_HexDump(receivedData,(uint32_t)response->DataSize);
            }
            if (context->dirtyHciWorkaround)
            {
                if (gLogEnabled)
                {
                    framework_Log("DEBUG : receiving HCI at an unexpected stage. Ignore it.");
                }
                break;
            }

            if (response->DataSize > 1)
            {
                // NOTE :
                // The second byte of HCI message - the one that follows pipeId has 2 highest bits to distinguish between HCI message types.
                // 00 - command (to radio)
                // 01 - event (two ways)
                // 10 - answer (from radio)
                // 11 - RFU
                // Answer from radio ?
                if ((receivedData[1]>>6) == 0x02)
                {
                    framework_LockMutex(context->requestLock);
                    // We check if the last ACK request ID is the same as last send Request ID.
                    // If not that's mean we receive an answer from a supposed timeout
                    // request.
                    if ((context->iLastAckRequestId != context->iLastRequestIdSent)&&(context->iLastRequestIdSent != 0))
                    {
                        if (gLogEnabled)
                        {
                            framework_Log("DEBUG : Receive answer from a timed out request => Ignore.");
                        }
                        framework_UnlockMutex(context->requestLock);
                        break;
                    }
                    // Our last request have been ACK, reset value.
                    context->iLastAckRequestId = 0;
                    context->iLastRequestIdSent = 0;
                    framework_UnlockMutex(context->requestLock);
                }
            }

            //
            // Forward this incoming HCI message to the recieve handler
            //
#ifdef SIMULATE_HCI_SEND_FAILED_ASYNC
            iErrorCode = 1;
#endif // def SIMULATE_HCI_SEND_FAILED_ASYNC
            if (iErrorCode == 0)
            {
                // CG Try to fix SC38886
                uint8_t *bufferForCallback;
                framework_LockMutex(context->readBufferLock);

                if (response->DataSize == 3)
                {
                    if ((context->secureElementPipeId > 0x01) && (context->readerAPipeId > 0x01))
                    {
                        uint8_t securePipeId  = 0x80 | context->secureElementPipeId;
                        uint8_t readerAPipeId = 0x80 | context->readerAPipeId;
                        if ((receivedData[0] == securePipeId) &&
                            (receivedData[1] == 0x50) &&
                            (receivedData[2] == 0x00))
                        {
                            if (gLogEnabled)
                            {
                                framework_Log("Translate secure element pipe ID from 0x%X to 0x%X",securePipeId,readerAPipeId);
                            }
                            receivedData[0] = readerAPipeId;
                        }
                    }
                }

                if ((context->initialRecvBufferSize == 0) || (context->initialRecvBuffer == NULL))
                {
                    // No read was armed by upper layer.
                    storeRecvDataInTempParcelLocked(context,receivedData,response->DataSize);

                    // nobody is reading so we go out.
                    framework_UnlockMutex(context->readBufferLock);
                    break;
                }
                // CG Try to fix SC38886
                bufferForCallback = context->initialRecvBuffer;

                // Copy received data to buffer provided by Receive function :
                if (context->initialRecvBufferSize >= response->DataSize)
                {
                    memcpy(bufferForCallback,receivedData,response->DataSize);

                    if (gLogFunc)
                    {
                        uint8_t *buffer = bufferForCallback;
                        size_t   sz     = response->DataSize;
                        gLogFunc(HECI_LOG_RECV_HCI,buffer,sz);
                    }

                    framework_UnlockMutex(context->readBufferLock);

                    if(context->receiveHciCb)
                    {
                        context->receiveHciCb(context->userContext,HECI_SUCCESS,bufferForCallback,response->DataSize);
                    }
                }else
                {
                    // Provided buffer too small.
                    if (gLogEnabled)
                    {
                        framework_Error(">>> Buffer too small : context->initialRecvBufferSize : %i need %i",context->initialRecvBufferSize,response->DataSize);
                        framework_HexDump(receivedData,response->DataSize);
                    }
                    memcpy(bufferForCallback,receivedData,context->initialRecvBufferSize);

                    framework_UnlockMutex(context->readBufferLock);

                    // Provide the buffer but it will be incomplete, se
                    if (context->receiveHciCb)
                    {
                        context->receiveHciCb(context->userContext,HECI_FAILED,bufferForCallback,context->initialRecvBufferSize);
                    }
                }
            }else
            {
                if (context->receiveHciCb)
                {
                    if (gLogEnabled)
                    {
                        framework_Log("DEBUG : NfcMeCommandReceive callback with HECI_FAILED");
                    }
                    context->receiveHciCb(context->userContext,HECI_FAILED,NULL,0);
                }
            }

            // Since this is an HCI command, we expect the caller of phHciNfcApi_ReceiveComplete to
            // rearm the async Read operation
            bShouldRearmRead = 0;
            break;
        }

        case NfcMeCommandSend:
        {
            uint8_t isTimeoutHandled = 0;

            framework_LockMutex(context->requestLock);

            // We store last ACK request ID.
            context->iLastAckRequestId = response->RequestId;

            // If we receive an HCI answer to a request, we that has timeout (for instance)
            // we should be able to ignore it.
            if (context->iLastRequestIdSent != response->RequestId)
            {
                if (gLogEnabled)
                {
                    framework_Log("DEBUG : Wrong request ID ACK, ignore it !");
                }
                framework_UnlockMutex(context->requestLock);
                break;
            }

            framework_UnlockMutex(context->requestLock);

            // NOTE : Timer could be triggered in this meanwhile ...
            isTimeoutHandled = handleTimeoutInCallBack(context);

            if (isTimeoutHandled)
            {
                // Timeout was called prior to us.
                break;
            }

            // FIXME : to be removed if FIX HCI Command/Answer skewed works !
            // Zero out last request ID send
            // context->iLastRequestIdSent = 0;

            if (gLogEnabled)
            {
                framework_Log("NfcMeCommandSend calling call back");
            }
            switch(response->Status)
            {
                case NfcMeStatusSendSucceed : // SUCCESS
                {
                    if (gLogFunc)
                    {
                        const uint8_t *buffer = framework_ParcelGetData(context->sendParcel);
                        size_t   sz     = framework_ParcelGetSize(context->sendParcel);
                        // NOTE : skip HECI header
                        gLogFunc(HECI_LOG_SEND_HCI,(buffer+10),sz-10);
                    }

                    if (context->sendHciCb)
                    {
                        // FIXME : Invalid size
                        context->sendHciCb(context->userContext,HECI_SUCCESS,response->DataSize);
                    }
                    break;
                }
                case NfcMeStatusSendFailed : // I2C_FAILURE or SHDLC Error
                {
                    uint8_t isResend = 0;

                    framework_Error("NfcMeStatusSendFailed : I2C_FAILURE or SHDLC Error\n");

                    isResend = resendFrame(context);

                    if (!isResend)
                    {
                        if (context->sendHciCb)
                        {
                            context->sendHciCb(context->userContext,HECI_FAILED,0);
                        }
                    }
                    break;
                }
                case 0x10: // FIXME : temporary workaround.
                {
                    uint8_t isResend = 0;

                    framework_Error("NfcMeStatusSendFailed : case 0x10\n");
                    // NOTE : wait for 1 sec before retrying
                    framework_MilliSleep(1000);
                    isResend = resendFrame(context);

                    if (!isResend)
                    {
                        if (context->sendHciCb)
                        {
                            context->sendHciCb(context->userContext,HECI_FAILED,0);
                        }
                    }
                    break;
                }
                case NfcMeStatusSendFWMode : // FW MODE
                {
                    framework_Error("NfcMeStatusSendFWMode : We are in download/degraded mode.\n");

                    if (context->sendHciCb)
                    {
                        context->sendHciCb(context->userContext,HECI_FAILED,0);
                    }

                    break;
                }
                default:
                {
                    framework_Warn("Unhandled Status : %X ",response->Status);

                    switch (context->meRequest)
                    {
                        case eRequestRfKill:
                        case eRequestSwitchMode:
                        case eRequestPowerState:
                        case eRequestQueryMeInfo:
                        case eRequestGetRadioReadyState:
                        {
                            // Query Mode
                            // RF Kill
                            // Power State
                            // Switch mode.
                            context->meRequestStatus = ME_REQUEST_FAIL; // Error !
                            // framework_NotifyMutex(context->meRequestLock,1);
                            WaiterNotifier_Notify(context->meRequestLock,1);
                            break;
                        }
                        case eRequestNone:
                        {
                            if (context->sendHciCb)
                            {
                                // Standard mode
                                context->sendHciCb(context->userContext,HECI_FAILED,0);
                            }
                        }
                        default:
                        {
                            framework_Error("Unhandled switch case %s %i",__FUNCTION__,__LINE__);
                        }
                    }
                }
            }


            break;
        }
        default:
        {
            if (gLogEnabled)
            {
                framework_Warn("Unknown command received : %X",response->Command);
            }
            break;
        }
    }

    // Rearm Read if needed.
    if (bShouldRearmRead)
    {
        tHECIStatus status;

        // FIXME : Check code comments !
        if (context->iLastReadBufferSize == 0)
        {
            // If no read initiate, store into internal buffer.
            status = HECIReceiveMessage(context->hHECI, context->recvBuffer, context->recvBufferSize);
        }else
        {
            // Reuse same pointer as NfcMeIfReceiveComplete()
            status = HECIReceiveMessage(context->hHECI, pData, context->iLastReadBufferSize);
            if ((status != hsSuccess) && (status != hsIoInProgress) && (status != hsReceiveAlreadyInProgress))
            {
                // Why ????
                status = HECIReceiveMessage(context->hHECI, context->recvBuffer, context->recvBufferSize);
            }
        }

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        if ((status != hsSuccess) && (status != hsIoInProgress) && (status != hsReceiveAlreadyInProgress))
        {
            framework_Error("Communication Error with HECI : HECIReceiveMessage() failed : %X",status);
        }

    }
}

// ****************************************** HECI Specific

static void heciMeResetCallBack(void* context)
{
    tNfcMeContext_t   *ctx = (tNfcMeContext_t*)context;
    if (gLogEnabled)
    {
        framework_Log("DEBUG : heciMeResetCallBack ctx : %X",ctx);
    }
    ctx->meResetDetected = 0;
    if (gResetDriver)
    {
        if (gLogEnabled)
        {
            framework_Log(">>> Reset Driver");
        }
        gResetDriver(gResetContext);
    }else
    {
        if (gLogEnabled)
        {
            framework_Log(">>> gResetDriver is NULL");
        }
    }
}

static void heciSubScribeResetEvent(tNfcMeContext_t* context)
{
    eEventStatus status;
    if (context->eventSubscription)
    {
        phOsEvent_Deinit(context->eventSubscription);
        context->eventSubscription = NULL;
    }

    status = phOsEvent_Init(&(context->eventSubscription));

    if (status != EVENT_SUCCESS)
    {
        framework_Error("phOsEvent_Init failed with %X",status);
        return;
    }

    phOsEvent_RegisterMeReset(context->eventSubscription,heciMeResetCallBack,context);
}

static void heciUnsubScribeResetEvent(tNfcMeContext_t* context)
{
    if (context->eventSubscription)
    {
        phOsEvent_Deinit(context->eventSubscription);
        context->eventSubscription = NULL;
    }
}

static void generateErrorEvent(tNfcMeContext_t* context,eFaultStatus status)
{
    if (!context->eventSubscription)
    {
        heciSubScribeResetEvent(context);
    }

    if (context->eventSubscription)
    {
        phOsEvent_GenerateEvent(context->eventSubscription,status);
    }
}

static void releaseHeciContext(tNfcMeContext_t* context)
{
    // Clean up timer
    if (context->timer)
    {
        framework_TimerDelete(context->timer);
    }

    if (context->timerConnect)
    {
        framework_TimerDelete(context->timerConnect);
    }

    if (context->hciExchangeTimer)
    {
        framework_TimerDelete(context->hciExchangeTimer);
    }

    if (context->meRequestTimeout)
    {
        framework_TimerDelete(context->meRequestTimeout);
    }

    // Clean up recvBuffer if any
    if (context->recvBuffer)
    {
        framework_FreeMem(context->recvBuffer);
        context->recvBuffer = NULL;
    }

    if (context->sendParcel)
    {
        framework_ParcelDelete(context->sendParcel);
        context->sendParcel = NULL;
    }

    if (context->temporaryRecvParcel)
    {
        framework_ParcelDelete(context->temporaryRecvParcel);
    }

    if (context->readBufferLock)
    {
        framework_DeleteMutex(context->readBufferLock);
    }

    if (context->hciExchangeLock)
    {
        // framework_DeleteMutex(context->hciExchangeLock);
        WaiterNotifier_Delete(context->hciExchangeLock);
    }

    if (context->hciExchangeBuffer)
    {
        framework_FreeMem(context->hciExchangeBuffer);
    }

    // Clean up Mutex
    if (context->requestLock)
    {
        framework_DeleteMutex(context->requestLock);
    }

    if (context->meRequestLock)
    {
        framework_DeleteMutex(context->meRequestLock);
    }
    // Clean up context
    framework_FreeMem(context);
}

// ****************************************** API Implementation

eHECIStatus phHECI_ConnectNormalMode(void** stackContext, void* userContext,heciCallback_SendHci *sendHciCb,heciCallback_ReceiveHci *receiveHciCb,uint8_t sendConnect)
{
    tNfcMeContext_t   *newContext = NULL;
    eConnectionResult  eResult    = CONNECTION_FAILED;

    *stackContext = NULL;

    // Keep the HECI client handle for later use
    newContext = framework_AllocMem(sizeof(tNfcMeContext_t));
    memset(newContext,0,sizeof(tNfcMeContext_t));
    newContext->hRecieveDoneEvent = NULL;
    newContext->requestStatus     = REQUEST_OFF;
    newContext->status            = COMM_FAILED;
    newContext->sendRetry         = 0;
    newContext->sendHciCb         = sendHciCb;
    newContext->receiveHciCb      = receiveHciCb;
    newContext->heciConnectRetry  = 0;
    newContext->shouldSendConnect = sendConnect;
    newContext->downloadMode      = eDownloadMode_Off;
    newContext->meRequest         = eRequestNone;

    framework_ParcelCreate(&(newContext->sendParcel));

    WaiterNotifier_Create(&(newContext->meRequestLock));
    framework_TimerCreate(&(newContext->timer));
    framework_TimerCreate(&(newContext->timerConnect));
    framework_TimerCreate(&(newContext->meRequestTimeout));
    framework_TimerCreate(&(newContext->hciExchangeTimer));
    framework_CreateMutex(&(newContext->requestLock));

    framework_ParcelCreate(&(newContext->temporaryRecvParcel));
    framework_CreateMutex(&(newContext->readBufferLock));

#ifdef FAKE_ME_RESET
    {
        void *thread;
        if (gLogEnabled)
        {
            framework_Log(">>>>>>>>>>>>>>>>>> Start fakeResetThread");
        }
        framework_CreateThread(&thread,fakeResetThread,NULL);
    }
#endif // def FAKE_ME_RESET

    newContext->userContext = userContext;

    gCurrentContext = newContext;

    eResult = heciOpenConnection(newContext,sendConnect);

    if (eResult == CONNECTION_SUCCESS)
    {
        if (stackContext != NULL)
        {
            *stackContext = newContext;
        }
        heciSubScribeResetEvent(newContext);
    }else
    {
        generateErrorEvent(newContext,FAULT_CONNECTION_FAILED);
        heciUnsubScribeResetEvent(newContext);

        releaseHeciContext(newContext);

        // Cleanup HECI connection
        *stackContext = NULL;
        gCurrentContext = NULL;
        return HECI_FAILED;
    }

    return HECI_SUCCESS;
}


eHECIStatus phHECI_Disconnect(void* stackContext)
{
    tNfcMeContext_t   *context = (tNfcMeContext_t*)stackContext;
    eConnectionResult  eResult = CONNECTION_FAILED;

    if (stackContext == NULL)
    {
        return HECI_SUCCESS;
    }

    heciRfKill(context,1,1);

    eResult = heciCloseConnection(context);

    if (eResult != CONNECTION_SUCCESS)
    {
        framework_Warn("Cannot close HECI Connection");
    }

    gCurrentContext = NULL;


    heciUnsubScribeResetEvent(context);

    releaseHeciContext(context);

    // And... we're done! :)
    return HECI_SUCCESS;
}

eHECIStatus phHECI_SendHCIFrame(void* stackContext, const uint8_t* data, size_t sz)
{
    tNfcMeContext_t       *context     = (tNfcMeContext_t*)stackContext;
    tHECIStatus            status;
#ifdef FAKE_TIMEOUT
    static uint8_t fakeTimeout = 0;
    uint8_t shift = 0;
    fakeTimeout++;
    if (fakeTimeout % 2 == 0)
    {
        shift = 18;
        if (gLogEnabled)
        {
            framework_Log(">>>>>>>>>>> fakeTimeout : %i",fakeTimeout);
        }
    }
#endif // def FAKE_TIMEOUT

    // Verify parameters
    if ((context == NULL) || (data == NULL) || (sz == 0) || (context->hHECI == HECI_INVALID_CLIENT))
    {
        framework_Error("%s : Invalid parameter.",__FUNCTION__);
        return HECI_INVALID_PARAM;
    }

    // Verify that HCI meets maximum size limit
    if (sz > HCI_MAX_PAYLOAD_SIZE)
    {
        framework_Error("Max HCI payload size : %i bytes (max is %i bytes)",sz,HCI_MAX_PAYLOAD_SIZE);
        return HECI_INVALID_PARAM;
    }

#ifdef SIMULATE_HCI_SEND_FAILED_SYNC
    return HECI_FAILED;
#endif

#ifndef SIMULATE_HCI_TIMEOUT
    checkAndInitHCI(context);
#endif // ndef SIMULATE_HCI_TIMEOUT
    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
        framework_HexDump(data,(uint32_t)sz);
    }
#ifdef SIMULATE_HCI_TIMEOUT
    return HECI_PENDING;
#endif //def SIMULATE_HCI_TIMEOUT

    // FIXME : TO REMOVE
//  if ((sz >= 2)&&(data[0] == 0x83)&&(data[1] == 0x33))
//  {
//      framework_MilliSleep(2000);
//      framework_Log("DEBUG : caught 83 33 ");
//      // phHECI_EmergencyRelease(context);
//      if (context->sendHciCb)
//      {
//          context->sendHciCb(context->userContext,HECI_SUCCESS,sz);
//      }
//      return HECI_PENDING;
//  }
//  if ((sz == 2)&&(data[0] == 0x83)&&(data[1] == 0x75))
//  {
//      framework_MilliSleep(5000);
//      framework_Log("DEBUG : caught 83 75 pausing");
//      phHECI_EmergencyRelease(context);
//  }
    // NOTE : When Proximity Driver is disabled, it put PN in Autonomous mode.
    // We decided to trap it. And we will use a specific command to do the RF KILL.
    // CGRFKILLONLY01
    // FA : this will have to be commented for CGRFKILLONLY01
    if ((sz == 2)&&(data[1] == 0x41))
    {
        //FA because of change in implementation use phHECI_RfKill instead of heciRfKill
        //heciRfKill(context,1);
        phHECI_RfKill(context,1);
        //end FA
        if (context->sendHciCb)
        {
            context->sendHciCb(context->userContext,HECI_SUCCESS,sz);
        }
        return HECI_PENDING;
    }
    // END CGRFKILLONLY01

    // HACK : drop close pipe admin command.
    if ((sz == 2) && (data[0] == 0x81) && (data[1] == 0x04))
    {
        if (context->sendHciCb)
        {
            context->sendHciCb(context->userContext,HECI_SUCCESS,sz);
        }

        if(context->receiveHciCb)
        {
            uint8_t fakeAnswer[] = {0x81,0x80};
            if (context->initialRecvBuffer)
            {
                memcpy(context->initialRecvBuffer,fakeAnswer,2);
                context->receiveHciCb(context->userContext,HECI_SUCCESS,context->initialRecvBuffer,2);
            }else
            {
                framework_Error("Error in %s at %i : No Read HCI called cannot Close Admin pipe command.",__FUNCTION__,__LINE__);
            }

        }

        return HECI_PENDING;
    }

    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : phHECI_SendHCIFrame called but previous Send not ACK.");
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    // Increase RequestID
    increaseRequestIdLocked(context);

    context->requestStatus = REQUEST_INPROG;
    context->sendRetry     = 0;

#ifdef FAKE_TIMEOUT
    buildMeHeaderLocked(context,(eNfcMeCommand)(NfcMeCommandSend+shift),context->sendParcel,sz,1);
#else
    buildMeHeaderLocked(context,NfcMeCommandSend,context->sendParcel,(uint16_t)sz,1);
#endif // def FAKE_TIMEOUT

    // Copy the message to the new buffer
    framework_ParcelWriteRaw(context->sendParcel,data,(uint32_t)sz);

    framework_UnlockMutex(context->requestLock);

    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);

    // Send the message

    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));

    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    return HECI_PENDING;
}


eHECIStatus phHECI_ReceiveHCIFrame(void* stackContext, uint8_t* dataDestPtr, size_t sz)
{
    tHECIStatus        status       = hsSuccess;
    tNfcMeContext_t   *context      = (tNfcMeContext_t*)stackContext;
    uint8_t            isTempData   = 0;
    size_t             sz4CallBack  = 0;
    uint8_t            callCallBack = 0;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
    }
    if ((stackContext == NULL) || (dataDestPtr == NULL) || (sz == 0) || (context->hHECI == HECI_INVALID_CLIENT))
    {
        framework_Error("Receive Invalid parameters !");
        return HECI_INVALID_PARAM;
    }

    if (gLogEnabled)
    {
        framework_Log("Receiving ...");
    }

    // Check if Recv buffer is allocated and big enougth.
    checkAndAllocateRecvBuffer(context);

    // NOTE : Useless as before doing any HCI request
    // a Send will occurs.
    // checkAndInitHCI(context);

    // Secure access !
    framework_LockMutex(context->readBufferLock);

    // Save destination buffer pointer.
    context->initialRecvBuffer     = dataDestPtr;
    context->initialRecvBufferSize = (uint32_t)sz;

    // Change expected size to Recv buffer size.
    sz = context->recvBufferSize;
    context->iLastReadBufferSize = (uint32_t)sz;

    // Check if there is data already received (due to a previous HCI send.)

    isTempData = isDataInRecvTempParcelLocked(context);


    if (isTempData)
    {
        uint32_t size = 0;
        // There is Data
        callCallBack = 1;
        popDataInRecvTempParcelLocked(context,context->initialRecvBuffer,context->initialRecvBufferSize,&size);

        sz4CallBack = (size_t)size;
    }
    framework_UnlockMutex(context->readBufferLock);

    if (callCallBack)
    {
        if(context->receiveHciCb)
        {
            context->receiveHciCb(context->userContext,HECI_SUCCESS,context->initialRecvBuffer,sz4CallBack);
        }
        return HECI_PENDING;
    }


    // Start an Asynchronous read.
    status = HECIReceiveMessage(context->hHECI, context->recvBuffer, (uint32_t)sz);

    // If read in progress or already rearmed
    if ((hsIoInProgress == status) || (status == hsReceiveAlreadyInProgress))
    {
        // Ok
        return HECI_PENDING;
    }

    if (status == hsCommLost)
    {
        if (gStopDriver)
        {
            gStopDriver(gStopContext);
        }
    }

    if (gLogEnabled)
    {
        framework_Error("Receive Failed !");
    }
    return HECI_FAILED;
}




eHECIStatus phHECI_QueryInfo(void* stackContext,tMeInfo_t* info)
{
    tNfcMeContext_t       *context     = (tNfcMeContext_t*)stackContext;
    tHECIStatus            status;

    // Verify parameters
    if ((context == NULL) || (info == NULL) || (context->hHECI == HECI_INVALID_CLIENT))
    {
        return HECI_INVALID_PARAM;
    }

    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : phHECI_QueryInfo called but previous Send not ACK.");
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    context->requestStatus = REQUEST_INPROG;
    context->sendRetry     = 0;

    buildMeHeaderLocked(context,NfcMeCommandMaintenance,context->sendParcel,0x0001,0);

    framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandVersionQuery);

    framework_UnlockMutex(context->requestLock);

    // framework_LockMutex(context->meRequestLock);
    WaiterNotifier_Lock(context->meRequestLock);

    context->meQueryInfo   = info;
    context->meRequest     = eRequestQueryMeInfo;

    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);

    // Send the message
    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));
    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        context->meQueryInfo = NULL;
        context->meRequest   = eRequestNone;
        // framework_UnlockMutex(context->meRequestLock);
        WaiterNotifier_Unlock(context->meRequestLock);
        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    // Pending block until answer !
    // framework_WaitMutex(context->meRequestLock,0);
    WaiterNotifier_Wait(context->meRequestLock,0);

    context->meQueryInfo = NULL;
    context->meRequest   = eRequestNone;
    // framework_UnlockMutex(context->meRequestLock);
    WaiterNotifier_Unlock(context->meRequestLock);

    if(context->meRequestStatus == ME_REQUEST_SUCCESS)
    {
        return HECI_SUCCESS;
    }else
    {
        return HECI_FAILED;
    }
}


void phHECI_SetResetFunc(heci_ResetDriver* hook, void* userContext)
{
    gResetDriver  = hook;
    gResetContext = userContext;
}

void phHECI_SetStopFunc ( heci_StopDriver* hook, void* userContext )
{
    gStopDriver  = hook;
    gStopContext = userContext;
}



void phHECI_SetLog(uint8_t enabled)
{
    gLogEnabled = enabled;
    HECISetLog(enabled);
}



eHECIStatus phHECI_ChangeMode(void* stackContext, eHECIMode emode)
{
    tNfcMeContext_t       *context     = (tNfcMeContext_t*)stackContext;
    tHECIStatus            status;
    // Verify parameters
    if ((context == NULL) || (context->hHECI == HECI_INVALID_CLIENT))
    {
        return HECI_INVALID_PARAM;
    }

    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : phHECI_ChangeMode called but previous Send not ACK.");
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    context->requestStatus = REQUEST_INPROG;
    context->sendRetry     = 0;

    buildMeHeaderLocked(context,NfcMeCommandMaintenance,context->sendParcel,0x0002,0);

    // Firmware udpate subcommand
    framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandFwUpdateMode);
    if (emode == HECI_DOWNLOAD)
    {
        // Download mode
        framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandFirmwareUpdateEnter);
    }else
    {
        // Normal mode
        framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandFirmwareUpdateExit);
    }

    framework_UnlockMutex(context->requestLock);

    // framework_LockMutex(context->meRequestLock);
    WaiterNotifier_Lock(context->meRequestLock);

    context->meRequest      = eRequestSwitchMode;
    context->eRequestedMode = emode;

    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);
    if (gLogEnabled)
    {
        framework_Log("DOWNLOAD_DEBUG : phHECI_ChangeMode(%X)\n",emode);
    }
    // Send the message
    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));
    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        context->meQueryInfo = NULL;
        context->meRequest   = eRequestNone;
        // framework_UnlockMutex(context->meRequestLock);
        WaiterNotifier_Unlock(context->meRequestLock);

        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    // Pending block until answer !
    // framework_WaitMutex(context->meRequestLock,0);
    WaiterNotifier_Wait(context->meRequestLock,0);

    context->meRequest   = eRequestNone;

    // framework_UnlockMutex(context->meRequestLock);
    WaiterNotifier_Unlock(context->meRequestLock);

    if(context->meRequestStatus == ME_REQUEST_SUCCESS)
    {
        return HECI_SUCCESS;
    }else
    {
        return HECI_FAILED;
    }
}


eHECIStatus phHECI_FWDataReceive(void* stackContext, uint8_t* buffer, size_t size)
{
    // NOTE : Same implementation ...
    if (gLogEnabled)
    {
        framework_Log("DEBUG : %s %X %i",__FUNCTION__,buffer,size);
    }
    return phHECI_ReceiveHCIFrame(stackContext,buffer,size);
}


eHECIStatus phHECI_FWDataSend(void* stackContext, const uint8_t* buffer, size_t size)
{
    tNfcMeContext_t       *context      = (tNfcMeContext_t*)stackContext;
    uint8_t                isResetFrame = 0;
    tHECIStatus            status;

    if (gLogEnabled)
    {
        framework_Log("%s",__FUNCTION__);
        framework_HexDump(buffer,(uint32_t)size);
    }


    // Verify parameters
    if ((context == NULL) || (buffer == NULL) || (size == 0) || (context->hHECI == HECI_INVALID_CLIENT))
    {
        return HECI_INVALID_PARAM;
    }

    // Verify that HCI meets maximum size limit
    if (size > HCI_MAX_PAYLOAD_SIZE)
    {
        framework_Error("Max HCI payload size : %i bytes (max is %i bytes)",size,HCI_MAX_PAYLOAD_SIZE);
        return HECI_INVALID_PARAM;
    }

    if (size > 32)
    {
        return HECI_INVALID_PARAM;
    }

        // Hook Reset command
    if (size == 0x03)
    {
        if ((buffer[0] == 0x01) && (buffer[1] == 0x00) && (buffer[2] == 0x00))
        {
            isResetFrame = 1;
        }
    }


    // Setup request status ...
    framework_LockMutex(context->requestLock);
    if (context->requestStatus != REQUEST_OFF)
    {
        framework_Error("ERROR : phHECI_FWDataSend called but previous Send not ACK.");
        framework_UnlockMutex(context->requestLock);
        return HECI_ALREADY_IN_PROG;
    }

    // Increase RequestID
    increaseRequestIdLocked(context);

    context->requestStatus = REQUEST_INPROG;
    context->sendRetry     = 0;

    buildMeHeaderLocked(context,NfcMeCommandMaintenance,context->sendParcel,(uint16_t)(size+1),1);

    // Add Sub Command : Send I2C
    framework_ParcelWriteByte(context->sendParcel,NfcMeMaintenanceSubcommandFwUpdateSend);

    if (isResetFrame)
    {
        // Real reset !
        framework_ParcelWriteByte(context->sendParcel,0x42);
        // Copy remaining buffer
        framework_ParcelWriteRaw(context->sendParcel,buffer+1,(uint32_t)(size-1));

        if (gLogEnabled)
        {
            framework_Log("NOTE : Translated reset to 0x42 0x00 0x00");
        }
    }else
    {
        // Copy the message to the new buffer
        framework_ParcelWriteRaw(context->sendParcel,buffer,(uint32_t)size);
    }


    framework_UnlockMutex(context->requestLock);

    // Start a new timeout
    framework_TimerStart(context->timer,SEND_COMMAND_TIMEOUT,timerHandler_SendTimeout,context);

    // Send the message
    status = HECISendMessage(context->hHECI, (uint8_t*)framework_ParcelGetData(context->sendParcel), framework_ParcelGetSize(context->sendParcel));
    if (hsIoInProgress != status)
    {
        framework_Error("HECISendMessage failed in %s!",__FUNCTION__);
        cancelTimerRequest(context);

        if (status == hsCommLost)
        {
            if (gStopDriver)
            {
                gStopDriver(gStopContext);
            }
        }

        return HECI_FAILED;
    }

    return HECI_PENDING;
}


eHECIStatus phHECI_ChangePowerModeRadio( void* stackContext, uint8_t onOff )
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)stackContext;

    if (context == NULL)
    {
        return HECI_INVALID_PARAM;
    }

    if (onOff!=0)
    {
        return heciPowerCommand(context,1);
    }else
    {
        return heciPowerCommand(context,0);
    }
}

eHECIStatus phHECI_RfKill( void* stackContext, uint8_t onOff )
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)stackContext;

    if (context == NULL)
    {
        return HECI_INVALID_PARAM;
    }

    if (onOff!=0)
    {
        return heciRfKill(context,1,0);
    }else
    {
        return heciRfKill(context,0,0);
    }

}

void heciExchangeSendHciCallback(void* ctx,eHECIStatus status,size_t sz)
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)ctx;

    sz = 0 ;

    if (status == HECI_SUCCESS)
    {
        // framework_LockMutex(context->hciExchangeLock);
        WaiterNotifier_Lock(context->hciExchangeLock);
        context->hciExchangeStatus = HECI_SUCCESS;
        // framework_NotifyMutex(context->hciExchangeLock,0);
        WaiterNotifier_Notify(context->hciExchangeLock,0);
        // framework_UnlockMutex(context->hciExchangeLock);
        WaiterNotifier_Unlock(context->hciExchangeLock);
    }else
    {
        // framework_LockMutex(context->hciExchangeLock);
        WaiterNotifier_Lock(context->hciExchangeLock);
        context->hciExchangeStatus = HECI_FAILED;
        // framework_NotifyMutex(context->hciExchangeLock,0);
        WaiterNotifier_Notify(context->hciExchangeLock,0);
        // framework_UnlockMutex(context->hciExchangeLock);
        WaiterNotifier_Unlock(context->hciExchangeLock);
    }
}


void heciExchangeReceiveHciCallback(void* ctx,eHECIStatus status,uint8_t* buffer,size_t sz)
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)ctx;

    buffer = NULL;

    if (status == HECI_SUCCESS)
    {
        // framework_LockMutex(context->hciExchangeLock);
        WaiterNotifier_Lock(context->hciExchangeLock);
        context->hciExchangeStatus = HECI_SUCCESS;
        context->hciReceivedSize   = sz;
        // framework_NotifyMutex(context->hciExchangeLock,0);
        WaiterNotifier_Notify(context->hciExchangeLock,0);
        // framework_UnlockMutex(context->hciExchangeLock);
        WaiterNotifier_Unlock(context->hciExchangeLock);
    }else
    {
        // framework_LockMutex(context->hciExchangeLock);
        WaiterNotifier_Lock(context->hciExchangeLock);
        context->hciExchangeStatus = HECI_FAILED;
        context->hciReceivedSize   = 0;
        // framework_NotifyMutex(context->hciExchangeLock,0);
        WaiterNotifier_Notify(context->hciExchangeLock,0);
        // framework_UnlockMutex(context->hciExchangeLock);
        WaiterNotifier_Unlock(context->hciExchangeLock);
    }
}


static void timerHandler_HCITimeout(void *ctx)
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)ctx;
    // framework_LockMutex(context->hciExchangeLock);
    WaiterNotifier_Lock(context->hciExchangeLock);
    context->hciExchangeStatus = HECI_FAILED;
    context->hciReceivedSize   = 0;
    // framework_NotifyMutex(context->hciExchangeLock,0);
    WaiterNotifier_Notify(context->hciExchangeLock,0);
    // framework_UnlockMutex(context->hciExchangeLock);
    WaiterNotifier_Unlock(context->hciExchangeLock);
}

eHECIStatus heciExchangeHciFrame(tNfcMeContext_t *context,const uint8_t *buffer, size_t sz,uint8_t *expectedAnswer,size_t expectedSize,size_t *receivedSize)
{
    eHECIStatus              heciStatus = HECI_SUCCESS;
    heciCallback_SendHci    *sendHciCbBackup;
    heciCallback_ReceiveHci *receiveHciCbBackup;
    void                    *userContextBackup;
    uint8_t                 *oldInitialRecvBuffer   = NULL;
    uint32_t                 oldInitialRecvBufferSz = 0;

    framework_LockMutex(context->readBufferLock);
    // Save destination buffer pointer.
    oldInitialRecvBuffer   = context->initialRecvBuffer;
    oldInitialRecvBufferSz = context->initialRecvBufferSize;

    context->initialRecvBuffer     = NULL;
    context->initialRecvBufferSize = 0;

    framework_UnlockMutex(context->readBufferLock);

    // backup callbacks ...
    sendHciCbBackup    = context->sendHciCb;
    receiveHciCbBackup = context->receiveHciCb;
    userContextBackup  = context->userContext;
    // ... and put our hooks
    context->sendHciCb    = heciExchangeSendHciCallback;
    context->receiveHciCb = heciExchangeReceiveHciCallback;
    context->userContext  = context;

    context->hciExchangeStatus = HECI_SUCCESS;

    framework_TimerStart(context->hciExchangeTimer,SEND_COMMAND_TIMEOUT*4,timerHandler_HCITimeout,context);

    heciStatus = phHECI_SendHCIFrame(context,buffer,sz);

    if (heciStatus != HECI_PENDING)
    {
        if (heciStatus != HECI_SUCCESS)
        {

            // Failed
            framework_TimerStop(context->hciExchangeTimer);
            context->sendHciCb    = sendHciCbBackup;
            context->receiveHciCb = receiveHciCbBackup;
            context->userContext  = userContextBackup;

            framework_LockMutex(context->readBufferLock);

            // Restore destination buffer pointer.
            context->initialRecvBuffer     = oldInitialRecvBuffer;
            context->initialRecvBufferSize = (uint32_t)oldInitialRecvBufferSz;
            framework_UnlockMutex(context->readBufferLock);

            return heciStatus;
        }else
        {
            // SUCCESS
            // No wait

        }
    }else
    {

        // Pending
        // framework_WaitMutex(context->hciExchangeLock,1);
        WaiterNotifier_Wait(context->hciExchangeLock,1);

        if (context->hciExchangeStatus != HECI_SUCCESS)
        {

            // Failed
            framework_TimerStop(context->hciExchangeTimer);

            context->sendHciCb    = sendHciCbBackup;
            context->receiveHciCb = receiveHciCbBackup;
            context->userContext  = userContextBackup;
            framework_LockMutex(context->readBufferLock);

            // Restore destination buffer pointer.
            context->initialRecvBuffer     = oldInitialRecvBuffer;
            context->initialRecvBufferSize = (uint32_t)oldInitialRecvBufferSz;
            framework_UnlockMutex(context->readBufferLock);
            framework_TimerStop(context->hciExchangeTimer);
            return context->hciExchangeStatus;
        }
    }

    context->hciExchangeStatus = HECI_SUCCESS;

    if (expectedAnswer)
    {

        framework_TimerStart(context->hciExchangeTimer,200,timerHandler_HCITimeout,context);

        heciStatus = phHECI_ReceiveHCIFrame(context,expectedAnswer,expectedSize);

        if (heciStatus != HECI_PENDING)
        {

            framework_TimerStop(context->hciExchangeTimer);
            context->sendHciCb    = sendHciCbBackup;
            context->receiveHciCb = receiveHciCbBackup;
            context->userContext  = userContextBackup;

            framework_LockMutex(context->readBufferLock);

            // Restore destination buffer pointer.
            context->initialRecvBuffer     = oldInitialRecvBuffer;
            context->initialRecvBufferSize = (uint32_t)oldInitialRecvBufferSz;
            framework_UnlockMutex(context->readBufferLock);

            return HECI_FAILED;
        }

        // Pending
        // framework_WaitMutex(context->hciExchangeLock,1);
        WaiterNotifier_Wait(context->hciExchangeLock,1);

        if (context->hciExchangeStatus != HECI_SUCCESS)
        {

            framework_TimerStop(context->hciExchangeTimer);

            // Failed
            context->sendHciCb    = sendHciCbBackup;
            context->receiveHciCb = receiveHciCbBackup;
            context->userContext  = userContextBackup;
            framework_LockMutex(context->readBufferLock);

            // Restore destination buffer pointer.
            context->initialRecvBuffer     = oldInitialRecvBuffer;
            context->initialRecvBufferSize = (uint32_t)oldInitialRecvBufferSz;
            framework_UnlockMutex(context->readBufferLock);

            return context->hciExchangeStatus;
        }
        if (gLogEnabled)
        {
            framework_Log("DEBUG : SC38153 : Check 1");
        }
        framework_TimerStop(context->hciExchangeTimer);
        if (gLogEnabled)
        {
            framework_Log("DEBUG : SC38153 : Check 2");
        }
        *receivedSize = context->hciReceivedSize;
    }


    // Restore Hooks.
    context->sendHciCb    = sendHciCbBackup;
    context->receiveHciCb = receiveHciCbBackup;
    context->userContext  = userContextBackup;
    framework_LockMutex(context->readBufferLock);

    // Restore destination buffer pointer.
    context->initialRecvBuffer     = oldInitialRecvBuffer;
    context->initialRecvBufferSize = (uint32_t)oldInitialRecvBufferSz;
    framework_UnlockMutex(context->readBufferLock);

    return HECI_SUCCESS;
}

void checkAndExpandHciBuffer(tNfcMeContext_t *context,uint16_t size)
{
    // Lazy Instanciate !
    if (!context->hciExchangeLock)
    {
        // framework_CreateMutex(&(context->hciExchangeLock));
        WaiterNotifier_Create(&(context->hciExchangeLock));
    }

    if (!context->hciExchangeBuffer)
    {
        context->hciExchangeBuffer     = framework_AllocMem(size);
        context->hciExchangeBufferSize = size;
    }

    if (context->hciExchangeBufferSize < size)
    {
        framework_FreeMem(context->hciExchangeBuffer);

        context->hciExchangeBuffer     = framework_AllocMem(size);
        context->hciExchangeBufferSize = size;
    }

}

eHECIStatus phHECI_RadioSoftwareReset ( void* stackContext )
{
    tNfcMeContext_t *context        = (tNfcMeContext_t*)stackContext;
    const uint8_t    createPipe[]   = {0x81,0x10,0x90,0x00,0x90};
    const uint8_t    createPipeA[]  = {0x81,0x80,0x00,0x90,0x00,0x90}; // Admin
    const uint8_t reqPipeIdGateA[]  = {0x81,0x10,0x13,0x00,0x13};
    const uint8_t ansPipeIdReaderA[]    = {0x81,0x80,0x00,0x13,0x00,0x13,0xFF};
    uint8_t          evtStopPoll[]  = {0xFF,0x51};
    uint8_t          openPipe[]     = {0xFF,0x03};
    uint8_t          resetCommand[] = {0xFF,0x3F,0x00,0xF8,0x12,0x01};
    eHECIStatus      heciStatus     = HECI_SUCCESS;
    uint8_t          pipeID         = 0x00;
    size_t           receivedSize   = 0;
    tMeInfo_t        meInfo;
    uint8_t          readerAPipeId  = 0;
    uint8_t          buffer[32];

#ifdef SIMULATE_HCI_SEND_FAILED_ASYNC
    return HECI_FAILED;
#endif
#ifdef SIMULATE_HCI_SEND_FAILED_SYNC
    return HECI_FAILED;
#endif
#ifdef SIMULATE_HCI_TIMEOUT
    return HECI_SUCCESS;
#endif

    if (context == NULL)
    {
        return HECI_INVALID_PARAM;
    }

    if (context->downloadMode != eDownloadMode_Off)
    {
        framework_Warn("phHECI_RadioSoftwareReset called, but we are download/degraded mode. Ignore the request.");
        return HECI_FAILED;
    }

    context->dirtyHciWorkaround = 1;
    heciStatus = phHECI_QueryInfo(stackContext,&meInfo);
    context->dirtyHciWorkaround = 0;

    if (heciStatus != HECI_SUCCESS)
    {
        return HECI_FAILED;
    }

    checkAndExpandHciBuffer(context,32);

    // Ignore HCI Init as we reset the radio !
    context->isInitHciParam = 1;

    // SEND    > 81 10 90 00 90
    // RECEIVE < 81 80 00 90 00 90 0X
    heciStatus = heciExchangeHciFrame(context,createPipe,sizeof(createPipe),context->hciExchangeBuffer,context->hciExchangeBufferSize,&receivedSize);

    if (heciStatus == HECI_SUCCESS)
    {
        if (memcmp(context->hciExchangeBuffer,createPipeA,sizeof(createPipeA))!=0)
        {
            return HECI_FAILED;
        }
    }
    else
    {
        return HECI_FAILED;
    }

    pipeID = context->hciExchangeBuffer[6] &0x0F;

    // SEND    > 8X 03
    // RECEIVE < 8X 80
    // Update pipe ID :
    openPipe[0] = 0x80 | pipeID;
    // Open pipe
    heciStatus = heciExchangeHciFrame(context,openPipe,sizeof(openPipe),context->hciExchangeBuffer,context->hciExchangeBufferSize,&receivedSize);
    if (heciStatus == HECI_SUCCESS)
    {
        if ((context->hciExchangeBuffer[0] != openPipe[0]) || (context->hciExchangeBuffer[1] != 0x80))
        {
            return HECI_FAILED;
        }
    }
    else
    {
        return HECI_FAILED;
    }

    // Pipe Reader A
    heciStatus = heciExchangeHciFrame(context,reqPipeIdGateA,sizeof(reqPipeIdGateA),buffer,32,&receivedSize);

    if (heciStatus == HECI_SUCCESS)
    {
        // Check if size match !
        if (receivedSize == sizeof(ansPipeIdReaderA))
        {
            if (memcmp(buffer,ansPipeIdReaderA,receivedSize-1) == 0) // Ignore last byte
            {
                // We got the right answer, gather information
                readerAPipeId = 0x80 | (buffer[receivedSize-1] & 0x0F);
            }else
            {
                // Answer content does not match
                heciStatus = HECI_FAILED;
            }
        }else
        {
            // Answer size does not match
            heciStatus = HECI_FAILED;
        }
    }

    if (readerAPipeId != 0x00)
    {
        // Send stop polling
        evtStopPoll[0] = readerAPipeId;
        heciExchangeHciFrame(context,evtStopPoll,sizeof(evtStopPoll),NULL,0,&receivedSize);
    }


    // SEND > 8X 3F 00 F8 12 01
    // Update pipe ID :
    resetCommand[0] = 0x80 | pipeID;
    // Reset

    if ((meInfo.RadioVersionSW[0]>=2) && (meInfo.RadioVersionSW[1]>=4))
    {
        heciStatus = heciExchangeHciFrame(context,resetCommand,sizeof(resetCommand),context->hciExchangeBuffer,context->hciExchangeBufferSize,&receivedSize);
    }else
    {
        // NOTE : On PN544 Firmware lower than 2.4, the HCI command is not ACK, so no callback from HECI.
        context->hciExchangeNoAck = 1;
        heciStatus = heciExchangeHciFrame(context,resetCommand,sizeof(resetCommand),NULL,0,&receivedSize);
        context->hciExchangeNoAck = 0;
    }

    if (heciStatus == HECI_SUCCESS)
    {
        // The board has been reset, need to reinit HCI.
        context->isInitHciParam = 0;

        // FIXS4HP
        if (gLogEnabled)
        {
            framework_Log("DEBUG DELAY of 600 ms");
        }
        framework_MilliSleep(600);
        if (gLogEnabled)
        {
            framework_Log("DEBUG END of DELAY of 600 ms");
        }
        // END FIXS4HP
    }

    return heciStatus;
}

eHECIStatus phHECI_ExchangeHCI(void* stackContext, const uint8_t* buffer, size_t sz, uint8_t* expectedAnswer, size_t expectedSize, size_t* receivedSize)
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)stackContext;

    if (context == NULL)
    {
        return HECI_INVALID_PARAM;
    }

    // FIXME : I think it's useless.
    checkAndExpandHciBuffer(context,(uint16_t)expectedSize);

    return heciExchangeHciFrame(context,buffer,sz,expectedAnswer,expectedSize,receivedSize);
}

void phHECI_SetLogFunc(heciCallback_Log* function)
{
    gLogFunc = function;
}

void checkAndInitHCI(tNfcMeContext_t* context)
{
    // NOTE : If we are not in normal mode don't try to init HCI.
    if (context->downloadMode != eDownloadMode_Off)
    {
        if (gLogEnabled)
        {
            framework_Log("We are in download mode abort checkAndInitHCI().");
        }
        return;
    }

    if (context->isInitHciParam == 0)
    {
        context->isInitHciParam = 1;

        // Retrieves some pipes id.
        if (gLogEnabled)
        {
            framework_Log("checkAndInitHCI() -> heciGetPipeId()");
        }
        heciGetPipeId(context);
        if (gLogEnabled)
        {
            framework_Log("checkAndInitHCI() <- heciGetPipeId()");
        }
    }
}


void phHECI_EmergencyRelease()
{
    tNfcMeContext_t *context = (tNfcMeContext_t*)gCurrentContext;

    if (!gCurrentContext)
    {
        return;
    }

    context->emergencyTriggered = 1;

    // framework_LockMutex(context->meRequestLock);
    WaiterNotifier_Lock(context->meRequestLock);

    // Depending on request, we set the associated answer.
    switch (context->meRequest)
    {
        case eRequestQueryMeInfo:
        case eRequestRfKill:
        case eRequestSwitchMode:
        case eRequestPowerState:
        case eRequestGetRadioReadyState:
        {
            // Query Mode
            // RF Kill
            // Power State
            // Switch mode.
            context->meRequestStatus = ME_REQUEST_FAIL; // Error !
            // framework_NotifyMutex(context->meRequestLock,0);
            WaiterNotifier_Notify(context->meRequestLock,0);
            break;
        }
        case eRequestNone:
        default:
        {
            // Nothing to do.
        }
    }
    // framework_UnlockMutex(context->meRequestLock);
    WaiterNotifier_Unlock(context->meRequestLock);

    if (context->hHECI)
    {
        HECIEmergencyRelease(context->hHECI);
    }
}
