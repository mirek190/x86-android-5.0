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
 * \file phDownloadWrapper_intel.c
 * $Author: Eff'Innov Technologies $
 */

#include "tools/core/framework_Interface.h"

#include "api/phDownloadWrapperApi.h"
#include "port/intel/phHECICommunicationLayer.h"

#include "tools/core/framework_Interface.h"
#include "tools/core/framework_Parcel.h"
#include "tools/core/framework_Allocator.h"


typedef enum _eWriteStatus
{
    WRITESTATUS_NONE,                   // No frame to check sent.
    WRITESTATUS_INPROG,                 // A (chunked) frame is being sent
    WRITESTATUS_FRAMEBUILT,             // The frame has been sent
    WRITESTATUS_SAVE_LAST_WRITE_ANSWER, // Save last written answer.
    WRITESTATUS_READBACK,               // We read back written data from sent frame.
    WRITESTATUS_ACK_LAST_WRITE,         // Read back data matches with written data. ACK last write from upper layer.
    WRITESTATUS_NACK_LAST_WRITE         // Read back data mismatches with written data. NACK last write from upper layer.
}eWriteStatus;

typedef enum _eDownloadResult
{
    DOWNLOAD_SUCCESS,                 // Operation succeed
    DOWNLOAD_SUCCESS_NOACK,           // Operation succeed, do not ACK upper layer
    DOWNLOAD_SUCCESS_NOACK_SAVE_SIZE, // Operation succeed, do not ACK upper layer, save ACK size from HECI.
    DOWNLOAD_FAILED                   // Something goes wrong, NACK upper layer.
}eDownloadResult;

typedef struct tDownloadWrapperIntelContext
{
    void         *heciContext;         // HECI Context
    void         *libNfcCtx;           // Upper layer context
    void         *parcelSendFrame;     // This parcel object will contains the 0x0C frame that upper layer send.
    void         *lockSendFrame;       // General Mutex
    void         *parcelReadBackFrame; // This parcel object will contains data written in firmware (read back data). This data will be compared with payload from frame "parcelSendFrame"
    eWriteStatus  writeStatus;         // State of writing process.
    uint32_t      frame0xCSize;        // used only with chunked frame. Contain the size of chunked 0x0C frame.
    uint8_t       addrBuffer[3];       // Buffer to store address for read back.
    uint16_t      sizeToReadBack;      // Size of buffer we have to read back.
    uint8_t       internalBuffer[32];  // Use to read back data.
    size_t        lastWrittenSize;     // Used by last write to ACK final check.
    void         *parcelLastWriteAnswer;// Used by last write to ACK final check.
}tDownloadWrapperIntelContext_t;

// This macro enable read back mechanism for firmware download. It allows to check that written frames are valid in PN544 memory.
// #define USE_READBACK
// This macro allows to test a firmware download failure => Readback content mismatch with written frame.
// #define TEST_READBACK_FAILURE

/**
 * @brief This function read a short from a Parcel in PN544 byte order.
 * For instance, 0x1234 while be read as 0x12 0x34. Normal parcel is 0x34 0x12.
 *
 * @param parcel Parcel to read from.
 * @return uint16_t read value
 **/
static uint16_t readShortFromParcel(void *parcel)
{
    uint16_t b1,b2;
    // NOTE : as framework_ParcelReadShort() byte order is not guarantee, we do it manually
    b1 = framework_ParcelReadByte(parcel);
    b2 = framework_ParcelReadByte(parcel);

    return (b1 << 8) | b2;
}

/**
 * @brief This function write a short into a Parcel in PN544 byte order.
 * For instance, 0x1234 while be read as 0x12 0x34. Normal parcel is 0x34 0x12.
 *
 * @param parcel Parcel to write to.
 * @param val Value to write
 * @return void
 **/
static void writeShortToParcel(void *parcel,uint16_t val)
{
    uint8_t b1,b2;
    // NOTE : as framework_ParcelWriteShort() byte order is not guarantee, we do it manually
    b1 = (val >> 8)&0xFF;
    b2 = (val & 0xFF);

    framework_ParcelWriteByte(parcel,b1);
    framework_ParcelWriteByte(parcel,b2);

    return;
}

/**
 * @brief Reset read back process status.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error
 **/
static eDownloadResult resetReadBackProcess(tDownloadWrapperIntelContext_t *context)
{
    framework_LockMutex(context->lockSendFrame);
    // Simply reset status. All necessary variables will be cleanup during process begining.
    context->writeStatus = WRITESTATUS_NONE;
    framework_UnlockMutex(context->lockSendFrame);

    return DOWNLOAD_SUCCESS;
}

/**
 * @brief Parse a chunked frame(0x0D command) from the given Parcel.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @param workingParcel Parcel object containing the frame.
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error
 **/
static eDownloadResult parse0x0DFrameLocked(tDownloadWrapperIntelContext_t *context,void *workingParcel)
{
    uint8_t        lastReadByte  = 0;
    uint16_t       lastReadShort = 0;
    uint16_t       frameLenght   = 0;
    const uint8_t *frameContent  = NULL;

    // Chunked frame are as following :
    // [0x0D] [Length on 2 bytes] [Payload]

    // Check again if first byte is 0x0D
    lastReadByte = framework_ParcelReadByte(workingParcel);

    if (lastReadByte != 0x0D)
    {
        framework_Error("Invalid parcel content. Should never happen : %s %i",__FILE__,__LINE__);
        return DOWNLOAD_FAILED;
    }

    // length on 2 bytes in PN544
    lastReadShort = readShortFromParcel(workingParcel);

    if (lastReadShort == 0)
    {
        framework_Warn("Invalid frame content : %s %i",__FILE__,__LINE__);
        return DOWNLOAD_FAILED;
    }

    // Store Frame length
    frameLenght = lastReadShort;
    // Store Frame content (pointer remains valid until we delete workingParcel)
    frameContent = framework_ParcelDataAtCurrentPosition(workingParcel);

    // Writing(concat) partial frame => the Payload of 0x0D frame.
    framework_ParcelWriteRaw(context->parcelSendFrame,frameContent,frameLenght);

    if (context->frame0xCSize == 0)
    {
        // It's the first chunked frame we need to get complete unchunked frame size :
        lastReadByte = framework_ParcelReadByte(workingParcel);
        // [0x0D] [Length on 2 bytes] [Payload : [0x0C] [Length on 2 bytes] ... a part of 0x0C frame ]
        // Should be the 0x0C command
        if (lastReadByte != 0x0C)
        {
            framework_Error("Invalid parcel content. Should never happen : %s %i",__FILE__,__LINE__);
            return DOWNLOAD_FAILED;
        }

        // Then the size is here => 2 bytes
        lastReadShort = readShortFromParcel(workingParcel);

        if (lastReadShort == 0)
        {
            framework_Warn("Invalid frame content : %s %i",__FILE__,__LINE__);
            return DOWNLOAD_FAILED;
        }

        context->frame0xCSize = lastReadShort + 1 + 2; // + 0x0C initial command + Length (2 bytes)
    }else
    {
        // A 0x0C frame is currently send.
        // Only need to concat this chunk to the rest of what we already have.
        uint32_t chunkedSize = framework_ParcelGetSize(context->parcelSendFrame);

        // We are currently processing a chunked frame.
        if (chunkedSize == context->frame0xCSize)
        {
            // We receive the whole frame => ready to read back it
            context->writeStatus = WRITESTATUS_FRAMEBUILT;
        }else if (chunkedSize > context->frame0xCSize)
        {
            // There is an issue ...
            framework_Error("Unexpected behavior. Should never happen : %s %i",__FILE__,__LINE__);
            return DOWNLOAD_FAILED;
        }
    }

    return DOWNLOAD_SUCCESS;
}

/**
 * @brief Parse an unchunked frame. (0x0C command) from the given parcel.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @param workingParcel Parcel object containing the frame.
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error
 **/
static eDownloadResult parse0x0CFrameLocked(tDownloadWrapperIntelContext_t *context,void *workingParcel)
{
    uint8_t        lastReadByte  = 0;
    uint16_t       lastReadShort = 0;
    uint32_t       frameLenght   = 0;
    const uint8_t *frameContent  = NULL;

    // Unchunked frame are as following :
    // [0x0C] [Length on 2 bytes] [Address on 3 bytes] [Payload length on 2 bytes] [Payload]

    // Check again if first byte is 0x0C
    lastReadByte = framework_ParcelReadByte(workingParcel);

    if (lastReadByte != 0x0C)
    {
        framework_Error("Invalid parcel content. Should never happen : %s %i",__FILE__,__LINE__);
        return DOWNLOAD_FAILED;
    }

    // length
    lastReadShort = readShortFromParcel(workingParcel);

    if (lastReadShort == 0)
    {
        framework_Warn("Invalid frame content : %s %i",__FILE__,__LINE__);
        return DOWNLOAD_FAILED;
    }

    // Skip address : 3 bytes
    framework_ParcelForward(workingParcel,3);

    // Get frame length : 2 bytes
    lastReadShort = readShortFromParcel(workingParcel);

    if (lastReadShort == 0)
    {
        framework_Warn("Invalid frame content : %s %i",__FILE__,__LINE__);
        return DOWNLOAD_FAILED;
    }

    // Rewind the parcel, we store the full frame with 0x0C [length] ...
    framework_ParcelRewind(workingParcel);

    // Store Frame length
    frameLenght = framework_ParcelGetRemainingDataSize(workingParcel);
    // Store Frame content (pointer remains valid until we delete workingParcel)
    frameContent = framework_ParcelDataAtCurrentPosition(workingParcel);

    // Writing frame
    framework_ParcelWriteRaw(context->parcelSendFrame,frameContent,frameLenght);

    // We receive the whole frame ready to read back it
    context->writeStatus = WRITESTATUS_FRAMEBUILT;

    return DOWNLOAD_SUCCESS;
}

/**
 * @brief Handle main written frame process.
 * Manager internal status & call correct function depending on frame type.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @param data Frame buffer
 * @param size size of the frame.
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error
 **/
static eDownloadResult parseWrittenFrame(tDownloadWrapperIntelContext_t *context,const uint8_t *data,size_t size)
{
    void            *workingParcel = NULL;
    eDownloadResult  parseResult   = 0;

    framework_LockMutex(context->lockSendFrame);

    // check if we handle this frame.
    if ((data[0] != 0x0D) && (data[0] != 0x0C))
    {
        // NOTE : We ignore these frames.
        framework_UnlockMutex(context->lockSendFrame);
        return DOWNLOAD_SUCCESS;
    }

    // Check process status ...
    if (context->writeStatus == WRITESTATUS_NONE)
    {
        // ... Writing is in progress
        // We start recording chunk or unchunked secure write frame.
        framework_ParcelClear(context->parcelSendFrame);

        context->writeStatus  = WRITESTATUS_INPROG;
        context->frame0xCSize = 0;

    }else if (context->writeStatus != WRITESTATUS_INPROG)
    {
        // Status is not WRITESTATUS_INPROG, that's mean data are sent but previous commands where not ACK yet.
        framework_Error("A previous write firmware command was not verified/ACK : %s %i",__FILE__,__LINE__);
    }

    if (size == 0)
    {
        framework_Error("Invalid frame size\n");
        framework_UnlockMutex(context->lockSendFrame);
        return DOWNLOAD_FAILED;
    }

    // Create a working parcel to parse written frame.
    framework_ParcelCreate(&workingParcel);

    // Fill up parcel
    framework_ParcelSetData(workingParcel,data,(uint32_t)size);

    if (data[0] == 0x0C)
    {
        // Secure Write
        parseResult = parse0x0CFrameLocked(context,workingParcel);
    }else if (data[0] == 0x0D)
    {
        // Secure Write Chunk
        parseResult = parse0x0DFrameLocked(context,workingParcel);
    }

    framework_ParcelDelete(workingParcel);
    framework_UnlockMutex(context->lockSendFrame);

    return parseResult;
}

/**
 * @brief Send the command for read back mechanism.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error
 **/
static eDownloadResult readBackFrameLocked(tDownloadWrapperIntelContext_t *context)
{
    void        *workingParcel = NULL;
    uint8_t      i             = 0;
    uint16_t     sizeToRead    = 0;
    eHECIStatus  heciStatus;

    // Create the parcel for READ command.
    framework_ParcelCreate(&workingParcel);
    // Adding header
    framework_ParcelWriteByte(workingParcel,0x07);  // Read command
    writeShortToParcel(workingParcel,0x0005);       // payload length

    for (i = 0; i < 3 ;i++)
    {
        framework_ParcelWriteByte(workingParcel,context->addrBuffer[i]);
    }

    // Compute remaining size to read
    sizeToRead = context->sizeToReadBack - (uint16_t)framework_ParcelGetSize(context->parcelReadBackFrame);
    // If greater than maximum allowed ...
    if (sizeToRead > 29)
    {
        // ... lock it.
        sizeToRead = 29;
    }

    //Writing the size to our read cmd frame
    writeShortToParcel(workingParcel,sizeToRead);

    // We have the complete frame => send it !
    heciStatus = phHECI_FWDataSend(context->heciContext,framework_ParcelGetData(workingParcel),framework_ParcelGetSize(workingParcel));

    // Release read cmd frame parcel.
    framework_ParcelDelete(workingParcel);


    if ((heciStatus != HECI_SUCCESS)&&((heciStatus != HECI_PENDING)))
    {
        framework_Error("Failed phHECI_FWDataSend\n");
        return DOWNLOAD_FAILED;
    }

    return DOWNLOAD_SUCCESS;
}

/**
 * @brief Update the next address to read.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @param toInc offset to increment address.
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error.
 **/
static eDownloadResult updateReadAdressLocked(tDownloadWrapperIntelContext_t *context,uint32_t toInc)
{
    uint32_t addr;
    uint32_t b1,b2,b3;

    b1 = context->addrBuffer[0];
    b2 = context->addrBuffer[1];
    b3 = context->addrBuffer[2];

    addr = (b1 << 16) | (b2 << 8) | b3;

    addr += toInc;

    context->addrBuffer[0] = (addr >> 16)& 0xFF;
    context->addrBuffer[1] = (addr >> 8) & 0xFF;
    context->addrBuffer[2] =  addr       & 0xFF;

    return DOWNLOAD_SUCCESS;
}

/**
 * @brief This function should be called when a Send Firmware frame is ACK.
 * This function will handle read back mechanism automatically.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error.
 **/
static eDownloadResult handleSendAck(tDownloadWrapperIntelContext_t *context)
{
    // Secure access.
    framework_LockMutex(context->lockSendFrame);
    // Check if the frame is built.
    if (context->writeStatus == WRITESTATUS_FRAMEBUILT)
    {
        // Change status : Start Read back
        context->writeStatus = WRITESTATUS_SAVE_LAST_WRITE_ANSWER;
        // The latest frame has been written, we do not ACK and we ask to save the last write size value.
        // In that way, when read back will be completed, we can ACK upper layer Write callback with this size value.
        framework_UnlockMutex(context->lockSendFrame);
        return DOWNLOAD_SUCCESS_NOACK_SAVE_SIZE;
    }
    // Check if read back is started ...
    if (context->writeStatus == WRITESTATUS_READBACK)
    {
        uint32_t     sizeExpected = 0;
        eHECIStatus  heciStatus;

        // Compute remaining size to read
        sizeExpected = context->sizeToReadBack - framework_ParcelGetSize(context->parcelReadBackFrame);
        // If greater than maximum allowed ...
        if (sizeExpected > 29)
        {
            // ... lock it.
            sizeExpected = 29;
        }

        // Adding header answer : at most 29 + 3 = 32 maximum allowed size.
        sizeExpected += 3;

        // Arm read to receive answer.
        heciStatus = phHECI_FWDataReceive(context->heciContext,context->internalBuffer,sizeExpected);

        if ((heciStatus != HECI_SUCCESS) && (heciStatus != HECI_PENDING) && (heciStatus != HECI_ALREADY_IN_PROG))
        {
            framework_UnlockMutex(context->lockSendFrame);
            return DOWNLOAD_FAILED;
        }
        framework_UnlockMutex(context->lockSendFrame);
        return DOWNLOAD_SUCCESS_NOACK;
    }

    framework_UnlockMutex(context->lockSendFrame);
    return DOWNLOAD_SUCCESS;
}


/**
 * @brief This function should be called when a Receive Firmware frame is ACK.
 * This function will handle read back mechanism automatically.
 *
 * @param context tDownloadWrapperIntelContext_t
 * @param buffer Received buffer
 * @param sz Received buffer size.
 * @return eDownloadResult DOWNLOAD_SUCCESS if no error.
 **/
static eDownloadResult handleReadAck(tDownloadWrapperIntelContext_t *context,const uint8_t* buffer,size_t sz)
{
    // Secure access.
    framework_LockMutex(context->lockSendFrame);

    // Check if it was the last write frame from upper layer.
    if (context->writeStatus == WRITESTATUS_SAVE_LAST_WRITE_ANSWER)
    {
        uint16_t        workingShort = 0;
        uint8_t         i            = 0;
        eDownloadResult result;

        // The last write is completed, we save the answer of this write command for further use.

        // Clear previous one.
        framework_ParcelClear(context->parcelLastWriteAnswer);
        // Copy content
        framework_ParcelSetData(context->parcelLastWriteAnswer,buffer,(uint32_t)sz);
        // Initiate read back !
        context->writeStatus = WRITESTATUS_READBACK;

        // Start Read back mechanism (Initial Read)
        // The frame is completed ! We receive answer to the last Write Chunk.

        // Cleanup Read back parcel.
        framework_ParcelClear(context->parcelReadBackFrame);

        // Gathering address from written frame. (this frame was previously saved !)
        // Rewind parcel
        framework_ParcelRewind(context->parcelSendFrame);
        // Skip 3 bytes [0x0C + 2 bytes length]
        framework_ParcelForward(context->parcelSendFrame,3);
        // Duplicate address on 3 bytes
        for (i = 0; i < 3;i++)
        {
            context->addrBuffer[i] = framework_ParcelReadByte(context->parcelSendFrame);
        }

        // We need to compute the length we have to read
        // context->parcelSendFrame is after the address => THE LENGTH of written DATA !
        workingShort = readShortFromParcel(context->parcelSendFrame);
        // Save the complete size to read back.
        context->sizeToReadBack = workingShort;

        // Now we have all informations we can start a read back !
        // Initiate read ...
        result = readBackFrameLocked(context);

        if (result != DOWNLOAD_SUCCESS)
        {
            framework_UnlockMutex(context->lockSendFrame);
            return DOWNLOAD_FAILED;
        }

        framework_UnlockMutex(context->lockSendFrame);
        // We do not ACK yet. We will do it when full read back process will be completed.
        return DOWNLOAD_SUCCESS_NOACK;
    }

    // We are in read back mode
    if (context->writeStatus == WRITESTATUS_READBACK)
    {
        // Check if answer is a success.
        if (buffer[0] == 00)
        {
            void     *workingParcel = NULL;
            uint16_t  workingShort = 0;
            uint16_t  addrOffset   = 0;

            // NOTE : there is a minimal size from receive frame from HECI.
            // Maybe this check is useless.
            if (sz == 7)
            {
                // Check if the buffer is not larger than real size.
                if (buffer[2] < 4)
                {
                    sz = sz - (4 - buffer[2]);
                }
            }

            framework_ParcelCreate(&workingParcel);
            // Put received data into parcel.
            framework_ParcelSetData(workingParcel,buffer,(uint32_t)sz);
            // Ignore status byte.
            framework_ParcelForward(workingParcel,1);
            // Read length.
            workingShort = readShortFromParcel(workingParcel);
            // Check sanity ...
            if (workingShort != framework_ParcelGetRemainingDataSize(workingParcel))
            {
                framework_Error("Invalid parcel content. Should never happen : %s %i",__FILE__,__LINE__);
            }

            // We store the size of this frame to offset address for next read.
            addrOffset = workingShort;

            // Merging with read back data.
            framework_ParcelWriteRaw(context->parcelReadBackFrame,framework_ParcelDataAtCurrentPosition(workingParcel),framework_ParcelGetRemainingDataSize(workingParcel));

            // Delete working parcel.
            framework_ParcelDelete(workingParcel);

            // Check if we read all ?
            if (framework_ParcelGetSize(context->parcelReadBackFrame) == context->sizeToReadBack)
            {
                const uint8_t *sentData;
                const uint8_t *readBackData;
                uint32_t sentSize     = 0;
                uint32_t readBackSize = 0;

                // We got all we need to proceed.

                // Gather sent data buffer from context->parcelSendFrame
                framework_ParcelRewind(context->parcelSendFrame);
                // Skip 0x0C, 2 bytes length, 3 bytes addr.
                framework_ParcelForward(context->parcelSendFrame,6);
                // Get Payload size
                sentSize = readShortFromParcel(context->parcelSendFrame);
                // Get Payload
                sentData = framework_ParcelDataAtCurrentPosition(context->parcelSendFrame);

                // Read back frame contains only data
                readBackSize = framework_ParcelGetSize(context->parcelReadBackFrame);
                readBackData = framework_ParcelGetData(context->parcelReadBackFrame);

                // Comparing frame !

                if (readBackSize != sentSize)
                {
                    framework_Error("Failed to write firmware, Size mismatch read back size : %i Written size : %i",readBackSize,sentSize);
                    // Size mismatch !
                    framework_UnlockMutex(context->lockSendFrame);
                    // NACK last write.
                    context->writeStatus = WRITESTATUS_NACK_LAST_WRITE;
                    return DOWNLOAD_FAILED;
                }
#ifdef TEST_READBACK_FAILURE
                {
                    static uint8_t failed = 0;
                    failed++;
                    framework_Log(">>>>>>>>>>>>>>>>>>> FAILED counter : %i\n",failed);
#endif // def TEST_READBACK_FAILURE
#ifdef TEST_READBACK_FAILURE
                if ((memcmp(sentData,readBackData,sentSize) != 0) ||(failed == 10))
#else
                if (memcmp(sentData,readBackData,sentSize) != 0)
#endif // def TEST_READBACK_FAILURE
                {
#ifdef TEST_READBACK_FAILURE
                    failed = 0;
#endif // def TEST_READBACK_FAILURE
                    framework_Error("Failed to write firmware, Content mismatch : ");
                    framework_Error("Written Data : ");
                    framework_HexDump(sentData,sentSize);
                    framework_Error("Read back Data : ");
                    framework_HexDump(readBackData,sentSize);
                    // Content mismatch
                    framework_UnlockMutex(context->lockSendFrame);
                    // NACK last write.
                    context->writeStatus = WRITESTATUS_NACK_LAST_WRITE;
                    return DOWNLOAD_FAILED;
                }
#ifdef TEST_READBACK_FAILURE
                }
#endif // def TEST_READBACK_FAILURE
                // Succeed !
                // ACK Last Write from upper layer
                context->writeStatus = WRITESTATUS_ACK_LAST_WRITE;
                framework_UnlockMutex(context->lockSendFrame);
                return DOWNLOAD_SUCCESS;

            }else if (framework_ParcelGetSize(context->parcelReadBackFrame) > context->sizeToReadBack)
            {
                // Read more data than expected ...
                framework_Error("Fail to read back get more data than expected : %s %i",__FILE__,__LINE__);
                framework_UnlockMutex(context->lockSendFrame);
                return DOWNLOAD_FAILED;
            }else
            {
                eDownloadResult result;

                // Start a new read !
                // Update address to read.
                result = updateReadAdressLocked(context,addrOffset);

                if (result != DOWNLOAD_SUCCESS)
                {
                    framework_UnlockMutex(context->lockSendFrame);
                    return DOWNLOAD_FAILED;
                }
                // read data from device.
                result = readBackFrameLocked(context);

                if (result != DOWNLOAD_SUCCESS)
                {
                    framework_UnlockMutex(context->lockSendFrame);
                    return DOWNLOAD_FAILED;
                }

                framework_UnlockMutex(context->lockSendFrame);
                return DOWNLOAD_SUCCESS_NOACK;
            }
        }else
        {
            // read back failed
            framework_Error("Read back failed : %s %i",__FILE__,__LINE__);
            framework_HexDump(buffer,(uint32_t)sz);
            framework_UnlockMutex(context->lockSendFrame);
            return DOWNLOAD_FAILED;
        }
    }

    framework_UnlockMutex(context->lockSendFrame);
    return DOWNLOAD_SUCCESS;
}

// HECI Layer Send Callback.
static void sendRawCallback(void* ctx,eHECIStatus status,size_t sz)
{
    tDownloadWrapperIntelContext_t *context = (tDownloadWrapperIntelContext_t*)ctx;
#ifdef USE_READBACK
    if (status == HECI_SUCCESS)
    {
        eDownloadResult result = handleSendAck(context);
        if (result == DOWNLOAD_SUCCESS)
        {
            // ACK SEND for upper layer
            phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,sz);
        }else if (result == DOWNLOAD_FAILED)
        {
            // NACK SEND for upper layer.
            phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,sz);
        }else if (result == DOWNLOAD_SUCCESS_NOACK_SAVE_SIZE)
        {
            // We save last frame size in order to ACK later.
            context->lastWrittenSize = sz;
        }// else DOWNLOAD_SUCCESS_NOACK => do nothing !
    }else
    {
        framework_Error("phDownloadWrapper_SendComplete() failed eHECIStatus : %X\n",status);
        phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,sz);
    }
#else
    if (status == HECI_SUCCESS)
    {
        phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,sz);
    }else
    {
        framework_Error("SEND FW FAILED");
        phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,sz);
    }
#endif // def USE_READBACK
}

// HECI Layer Receive Callback.
static void receiveRawCallback(void* ctx,eHECIStatus status,uint8_t* buffer,size_t sz)
{
    tDownloadWrapperIntelContext_t *context = (tDownloadWrapperIntelContext_t*)ctx;
#ifdef USE_READBACK
    if (status == HECI_SUCCESS)
    {
        eDownloadResult result = handleReadAck(context,buffer,sz);
        if (result == DOWNLOAD_SUCCESS)
        {
            // Secure access
            framework_LockMutex(context->lockSendFrame);
            if (context->writeStatus == WRITESTATUS_ACK_LAST_WRITE)
            {
                context->writeStatus = WRITESTATUS_NONE;
                // Status modified release lock.
                framework_UnlockMutex(context->lockSendFrame);

                // NOTE : We ACK a previously pending "Send".
                phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,context->lastWrittenSize);

                framework_HexDump(framework_ParcelGetData(context->parcelLastWriteAnswer),framework_ParcelGetSize(context->parcelLastWriteAnswer));

                // NOTE : We ACK a previously pending "Read"
                phDownloadWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,(uint8_t*)framework_ParcelGetData(context->parcelLastWriteAnswer),framework_ParcelGetSize(context->parcelLastWriteAnswer));
            }else
            {
                // Normal ACK
                framework_UnlockMutex(context->lockSendFrame);
                phDownloadWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,buffer,sz);
            }
        }else if (result == DOWNLOAD_FAILED)
        {
            framework_LockMutex(context->lockSendFrame);
            if (context->writeStatus == WRITESTATUS_NACK_LAST_WRITE)
            {
                context->writeStatus = WRITESTATUS_NONE;
                framework_UnlockMutex(context->lockSendFrame);
                // NOTE : We NACK a previously pending "Send".
                phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,context->lastWrittenSize);
                // NOTE : We will not NACK read : for upper layer Send failed so read do not need to be ACK.
            }else if (context->writeStatus == WRITESTATUS_READBACK)
            {
                context->writeStatus = WRITESTATUS_NONE;
                framework_UnlockMutex(context->lockSendFrame);
                // NOTE : We NACK a previously pending "Send".
                phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,context->lastWrittenSize);
            }else
            {
                // Normal ACK
                framework_UnlockMutex(context->lockSendFrame);
                phDownloadWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_FAILED,buffer,sz);
            }
        } // else DOWNLOAD_SUCCESS_NOACK => do nothing !
    }else
    {
        phDownloadWrapper_SendComplete(context->libNfcCtx,NFCSTATUS_FAILED,sz);
    }
#else
    if (status == HECI_SUCCESS)
    {
        phDownloadWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_SUCCESS,buffer,sz);
    }else
    {
        phDownloadWrapper_ReceiveComplete(context->libNfcCtx,NFCSTATUS_FAILED,buffer,sz);
    }
#endif // def USE_READBACK
}


NFCSTATUS phDownloadWrapper_InitDownloadMode(void** stackContext,void *coreContext)
{
    NFCSTATUS                       result     = NFCSTATUS_SUCCESS;
    eHECIStatus                     heciStatus = HECI_SUCCESS;
    tDownloadWrapperIntelContext_t *newContext = NULL;

    framework_Log("--> %s",__FUNCTION__);
    newContext = framework_AllocMem(sizeof(tDownloadWrapperIntelContext_t));

    memset(newContext,0,sizeof(tDownloadWrapperIntelContext_t));

    newContext->libNfcCtx = coreContext;
    newContext->writeStatus = WRITESTATUS_NONE;

    framework_ParcelCreate(&(newContext->parcelSendFrame));
    framework_ParcelCreate(&(newContext->parcelReadBackFrame));
    framework_ParcelCreate(&(newContext->parcelLastWriteAnswer));
    framework_CreateMutex(&(newContext->lockSendFrame));

    // Open a normal connection to HECI.
    // NOTE : Even if we are in download mode, this connect should succeed.
    heciStatus = phHECI_ConnectNormalMode(&(newContext->heciContext),newContext,sendRawCallback,receiveRawCallback,1);

    if (heciStatus == HECI_SUCCESS)
    {
        // Power ON the radio
        // NOTE : we ignore result as if it's not supported, the radio is ON.
        phHECI_ChangePowerModeRadio(newContext->heciContext,1);

        // Switching to download mode

        // NOTE : 10 secs delay for async connect is handled internaly.
        heciStatus = phHECI_ChangeMode(newContext->heciContext,HECI_DOWNLOAD);

        if (heciStatus == HECI_SUCCESS)
        {
            *stackContext = newContext;
        }else
        {
            framework_Error("%s : Failed to switch to download mode",__FUNCTION__);

            heciStatus = phHECI_Disconnect(newContext->heciContext);

            if (heciStatus != HECI_SUCCESS)
            {
                framework_Error("%s : Disconnect from HECI failed : heciStatus = %X",__FUNCTION__,heciStatus);
            }

            *stackContext = NULL;
            result = NFCSTATUS_FAILED;
        }
    }else
    {
        framework_Error("%s : Failed to connect to HECI",__FUNCTION__);
        *stackContext = NULL;
        result = NFCSTATUS_FAILED;
    }

    framework_Log("<-- %s result 0x%2X",__FUNCTION__,result);

    return result;
}

NFCSTATUS phDownloadWrapper_DeInitDownloadMode(void* stackContext)
{
    tDownloadWrapperIntelContext_t *context    = (tDownloadWrapperIntelContext_t*)stackContext;
    eHECIStatus                     heciStatus = HECI_SUCCESS;

    // Download is supposed to be completed
    // We should switch to normal mode.

    // NOTE : where should we wait for 10 secs delay for connect ?
    // -> Handle the process internaly in phHECI_XXXX ?

    heciStatus = phHECI_ChangeMode(context->heciContext,HECI_NORMAL);

    if(heciStatus != HECI_SUCCESS)
    {
        framework_Error("%s : Failed to switch to normal mode.",__FUNCTION__);
    }

    heciStatus = phHECI_Disconnect(context->heciContext);

    if (heciStatus != HECI_SUCCESS)
    {
        framework_Error("%s : Failed to disconnect from HECI",__FUNCTION__);
    }

    framework_ParcelDelete(context->parcelSendFrame);
    framework_ParcelDelete(context->parcelReadBackFrame);
    framework_ParcelDelete(context->parcelLastWriteAnswer);
    framework_DeleteMutex(context->lockSendFrame);

    framework_FreeMem(context);

    return NFCSTATUS_SUCCESS;
}

NFCSTATUS phDownloadWrapper_ReadPacket(void* stackContext,uint8_t *data,size_t size)
{
    tDownloadWrapperIntelContext_t *context = (tDownloadWrapperIntelContext_t*)stackContext;
    eHECIStatus                     eResult = HECI_FAILED;

    eResult = phHECI_FWDataReceive(context->heciContext,data,size);

    if (eResult != HECI_PENDING)
    {
        return NFCSTATUS_FAILED;
    }

    return NFCSTATUS_PENDING;
}

NFCSTATUS phDownloadWrapper_WritePacket(void* stackContext,uint8_t *data,size_t size)
{
    tDownloadWrapperIntelContext_t *context = (tDownloadWrapperIntelContext_t*)stackContext;
    eHECIStatus                     eResult = HECI_FAILED;
#ifdef USE_READBACK
    parseWrittenFrame(context,data,size);
#endif // def USE_READBACK
    eResult = phHECI_FWDataSend(context->heciContext,data,size);

    if (eResult != HECI_PENDING)
    {
#ifdef USE_READBACK
        resetReadBackProcess(context);
#endif // def USE_READBACK
        return NFCSTATUS_FAILED;
    }
    return NFCSTATUS_PENDING;
}
