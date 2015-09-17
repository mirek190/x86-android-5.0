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
 * \file HECIApi.h
 * $Author: Eff'Innov Technologies $
 */

#ifndef HECIAPI_H
#define HECIAPI_H

#include "tools/core/framework_Interface.h"

#ifdef __cplusplus
extern "C"
{
#endif // def __cplusplus

/**
    A handle to a given HECI client
*/
typedef void* tHECIClientHandle;

/**
    Defines the invalid HECI client
*/
#define HECI_INVALID_CLIENT ((tHECIClientHandle)0x00000000)

/**
*/
#define HECI_INVALID_MESSAGE_SIZE ((uint32_t)0xFFFFFFFF)

/**
   Defines the types for SendComplete and ReceiveComplete methods
*/
typedef void (*tHECISendComplete)(void* userContext,uint32_t errorCode,uint32_t size);
typedef void (*tHECIReceiveComplete)(void* userContext,uint32_t errorCode,uint8_t* data,uint32_t dataSize);

/**
    Status codes that can be returned from the
    different methods
*/
typedef enum _tHECIStatus
{
    /* 0x00 */ hsSuccess = 0,
    /* 0x01 */ hsDriverMissing,
    /* 0x02 */ hsClientUnknown,
    /* 0x03 */ hsSendFailed,
    /* 0x04 */ hsReceiveFailed,
    /* 0x05 */ hsInvalidParams,
    /* 0x06 */ hsOutOfMemory,
    /* 0x07 */ hsOutOfResources,
    /* 0x08 */ hsInvalidHandle,
    /* 0x09 */ hsCommunicationError,
    /* 0x0a */ hsIoInProgress,         /* Other IO operation (read or write) is in progress */
    /* 0x0b */ hsSyncError,            /* Sync object was deleted or corrupted unexpectedly */
    /* 0x0c */ hsTimedOut,             /* Sync read/write operation timedout */
    /* 0x0d */ hsReceiveAlreadyInProgress,
    /* 0x0e */ hsSendAlreadyInProgress,
    /* 0x0f */ hsCommLost
} tHECIStatus;


/**
 * @brief This method will set the context for SendCompelte and RecieveComplete scenarios.
 * Note that after invoking HECICreate method, no indication about any IO will occur until HECIRegisterCallbacks is called
 *
 * @param [in]hHECI The handle to the HECI client as obtained by a successful callto HECICreate.
 * @param [in]userContext This parameter will be assigned to the given HECI client, andwill be passed back to the caller upon RecieveComplete and SendCompletecallbacks.
 * @param [in]receiveCompleteCb The callback to be called when a message isreceived from HECI.
 * @param [in]sendCompleteCb The callback to be called when a message sendprocess is complete.
 * @return void
 **/
void HECIRegisterCallbacks(tHECIClientHandle hHECI,void* userContext,tHECIReceiveComplete receiveCompleteCb,tHECISendComplete sendCompleteCb);

/**
 * @brief  Creates a HECI client interfacing the NFC Client in ME FW
 *
 * @param [out]phHECI phHECI A pointer to a tHECIClientHandle, which will be filled by the method.
 * @return hsDriverMissing MEI driver was not found in the system.
 * @return hsCommunicationError Some communication error with the driver occurred. This indicates problems with underlying ME FW. In this case all resources will be automatically de-allocated.
 * @return hsOutOfMemory Memory allocations from heap fails.
 * @return hsOutOfResources Allocation synchronization object fails.
 * @return hsSuccess HECI connected successfully.
 **/
tHECIStatus HECICreate(tHECIClientHandle* phHECI);


/**
 * @brief Reconnect to HECI.
 *
 * @param [in]phHECI ...
 * @return tHECIStatus
 **/
tHECIStatus HECIReconnect(tHECIClientHandle hHECI);

/**
 * @brief Destructs a given HECI client by closing connection and freeing up all related resources.
 *
 * @param [in]hHECI A handle to close the related HECI connection to.
 * @return void
 **/
void HECIDestroy(tHECIClientHandle hHECI);

/**
 * @brief Starts an Async read operation from HECI.
 * Once read operation is done, the tReceiveComplete callback that was previously registered by a callback
 * to HECIRegisterCallbacks will be called
 *
 * @param [in]hHECI Handle to the HECI connection to read the data from
 * @param [in]buffer A pointer to the buffer to fill the incoming data in. Note that this buffer must not be used by caller until Rx completing callback is called.
 * @param [in]iLength The length, in bytes, of the buffer to with to
 * @return hsInvalidHandle If the supplied handle is invalid (connection is not correctly initialized)
 * @return hsInvalidParams Either HECI handle is invalid, pBuffer pointer to a NULL buffer or size is smaller than maximum message size (to determine maximum message size see HECIGetMaximumMessageSize API)
 * @return hsReceiveAlreadyInProgress Asynchronous Read operation already in progress (HECIReceiveMessage already called and no incoming message was yet received)
 * @return hsCommunicationFailure IO error occurred
 * @return hsIoInProgress Asynchronous Read operation started successfully
 **/
tHECIStatus HECIReceiveMessage(tHECIClientHandle hHECI, uint8_t* buffer, uint32_t iLength);

/**
 * @brief Starts an Async write operation to HECI.
 * Once write operation is done, the tSendComplete callback that was previously registered by a call
 * to HECIRegisterCallbacks will be called.
 * @param [in]hHECI Handle to the HECI connection to send the data through
 * @param [in]buffer A pointer to the buffer to send
 * @param [in]iLength The length, in bytes, of the buffer to send
 * @return hsInvalidHandle If the supplied handle is invalid (connection is not correctly initialized)
 * @return hsInvalidParams Either HECI handle is invalid, pBuffer pointer to a NULL or size to send is either 0 or exceeds maximum message size to determine maximum message size see HECIGetMaximumMessageSize API)
 * @return hsReceiveAlreadyInProgress Asynchronous Send operation already in progress (HECISendMessage already called and was not yet completed)
 * @return hsCommunicationFailure IO error occurred
 * @return hsIoInProgress Asynchronous Send operation started successfully
 **/
tHECIStatus HECISendMessage( tHECIClientHandle hHECI, uint8_t* buffer, uint32_t iLength );

/**
 * @brief Returns the size, in bytes, of the maximum size of each HECI message supported by the connected client.
 * Note, that this size is negotiated between ME FW and SW during connection establishment, and enforces:
 * a. Each READ operation must supply a buffer of *at least* this size.
 * b. Each SEND operation must not exceed this number of bytes.
 * @param [in]hHECI Handle to the HECI connection to send the data through
 * @return The size, in bytes, of negotiated maximum message size, or HECI_INVALID_MESSAGE_SIZE if the handle is invalid.
 **/
uint32_t HECIGetMaximumMessageSize(tHECIClientHandle hHECI);

/**
 * @brief Release HECI Driver
 * This function release the HECI driver connection. This function is only called in case the HECI communication
 * layer must be released.
 *
 * @param [in]hHECI ...
 * @return void
 **/
void HECIEmergencyRelease(tHECIClientHandle hHECI);

/**
 * @brief Enable or disable logs from HECICore
 *
 * @param [in]value 1 = On ; 0 = Off
 * @return void
 **/
void HECISetLog(uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // ndef HECIAPI_H