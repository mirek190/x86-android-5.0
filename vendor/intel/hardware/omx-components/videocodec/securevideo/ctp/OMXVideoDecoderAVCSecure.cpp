/*
* Copyright (c) 2009-2014 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


//#define LOG_NDEBUG 0
#define LOG_TAG "OMXVideoDecoderAVCSecure"
#include <utils/Log.h>

#include "OMXVideoDecoderAVCSecure.h"
#include "IMRDataBuffer.h"

// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* AVC_MIME_TYPE = "video/avc";
static const char* AVC_SECURE_MIME_TYPE = "video/avc-secure";

#define FLUSH_WAIT_INTERVAL     (30 * 1000) //30 ms



OMXVideoDecoderAVCSecure::OMXVideoDecoderAVCSecure()
    : mSessionPaused(false)
    , mImplDRM(NULL)
{
    LOGV("constructor enter");
    mVideoDecoder = createVideoDecoder(AVC_SECURE_MIME_TYPE);
    if (!mVideoDecoder) {
        LOGE("createVideoDecoder failed for \"%s\"", AVC_SECURE_MIME_TYPE);
    }
    // Override default native buffer count defined in the base class
    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;

    BuildHandlerList();

    LOGV("constructor exit");
}
// End of OMXVideoDecoderAVCSecure::OMXVideoDecoderAVCSecure()

OMXVideoDecoderAVCSecure::~OMXVideoDecoderAVCSecure() {
    LOGV("destructor enter");

    // Do NOT delete this pointer, since it points to one of the
    // class members.
    mImplDRM = NULL ;
}
// End of OMXVideoDecoderAVCSecure::~OMXVideoDecoderAVCSecure()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::InitInputPortFormatSpecific(
    OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput) {

    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)AVC_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    // OMX_VIDEO_PARAM_AVCTYPE
    memset(&mParamAvc, 0, sizeof(mParamAvc));
    SetTypeHeader(&mParamAvc, sizeof(mParamAvc));
    mParamAvc.nPortIndex = INPORT_INDEX;
    // TODO: check eProfile/eLevel
    mParamAvc.eProfile = OMX_VIDEO_AVCProfileHigh; //OMX_VIDEO_AVCProfileBaseline;
    mParamAvc.eLevel = OMX_VIDEO_AVCLevel41; //OMX_VIDEO_AVCLevel1;

    this->ports[INPORT_INDEX]->SetMemAllocator(MemAllocIMR, MemFreeIMR, this);
    LOGI("Set memory allocator to MemAllocIMR") ;

    for (int i = 0; i < INPORT_ACTUAL_BUFFER_COUNT; i++) {
        mIMRSlot[i].offset = IMR_START_OFFSET + i * INPORT_BUFFER_SIZE;
        mIMRSlot[i].owner = NULL;
    }

    return OMX_ErrorNone;
}
// End of OMXVideoDecoderAVCSecure::InitInputPortFormatSpecific()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorInit(void) {
    mSessionPaused = false;
    return OMXVideoDecoderBase::ProcessorInit();
}
// End of OMXVideoDecoderAVCSecure::ProcessorInit()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorDeinit(void) {

    if(mImplDRM)
        mImplDRM->Deinit() ;
    mIEDControl.Enable(false);
    return OMXVideoDecoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorStart(void) {

    mIEDControl.Enable(false) ;

    mSessionPaused = false;
    return OMXVideoDecoderBase::ProcessorStart();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorStop(void) {

    if(mImplDRM) {
        int32_t result = mImplDRM->HandleProcessorStop() ;
        LOGV("HandleProcessorStop returned %d", result) ;
    }

    return OMXVideoDecoderBase::ProcessorStop();
}


OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32 numberBuffers) {

    OMX_BUFFERHEADERTYPE *pInput = *pBuffers[INPORT_INDEX];
    /* TODO: we don't need to check size of 0, there are other ways of
     * error handling.  However, the code inside of if clause is useful to
     * flush the pipeline and should be extracted into its own function,
     * to be called on unrecoverable errors.
     *
    IMRDataBuffer *imrBuffer = (IMRDataBuffer *)pInput->pBuffer;
    if (imrBuffer->size == 0) {
        // error occurs during decryption.
        LOGW("size of returned IMR buffer is 0, decryption fails.");
        mVideoDecoder->flush();
        usleep(FLUSH_WAIT_INTERVAL);
        OMX_BUFFERHEADERTYPE *pOutput = *pBuffers[OUTPORT_INDEX];
        pOutput->nFilledLen = 0;
        // reset IMR buffer size
        imrBuffer->size = INPORT_BUFFER_SIZE;
        this->ports[INPORT_INDEX]->FlushPort();
        this->ports[OUTPORT_INDEX]->FlushPort();
        return OMX_ErrorNone;
    }
     */

    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::ProcessorProcess(pBuffers, retains, numberBuffers);
    if (ret != OMX_ErrorNone) {
        LOGE("OMXVideoDecoderBase::ProcessorProcess failed. Result: %#x", ret);
        return ret;
    }

    if (mSessionPaused && (retains[OUTPORT_INDEX] == BUFFER_RETAIN_GETAGAIN)) {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
        OMX_BUFFERHEADERTYPE *pOutput = *pBuffers[OUTPORT_INDEX];
        pOutput->nFilledLen = 0;
        this->ports[INPORT_INDEX]->FlushPort();
        this->ports[OUTPORT_INDEX]->FlushPort();
    }

    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorPause(void) {
    mImplDRM->HandleProcessorPause() ;
    return OMXVideoDecoderBase::ProcessorPause();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorResume(void) {
    mImplDRM->HandleProcessorResume() ;
    return OMXVideoDecoderBase::ProcessorResume();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareConfigBuffer(VideoConfigBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareConfigBuffer(p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareConfigBuffer");
    p->flag |=  WANT_SURFACE_PROTECTION;
    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareDecodeBuffer(
    OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) {

    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareDecodeBuffer");

    if (buffer->nFilledLen == 0) {
        return OMX_ErrorNone;
    }

    p->flag |= HAS_COMPLETE_FRAME;

    if (buffer->nOffset != 0) {
        LOGW("buffer offset %lu is not zero!!!", buffer->nOffset);
    }

    IMRDataBuffer *imrBuffer = (IMRDataBuffer *)buffer->pBuffer;
    p->data = imrBuffer->data;

    int32_t wvres = OMXImplBase::OMXDRM_SUCCESS ;
    if (mImplDRM == NULL) {
        
        // We don't yet know, which DRM we need to support.

        if (imrBuffer->magic != IMR_DATA_BUFFER_MAGIC) 
        {
            LOGE("PrepareDecodeBuffer: IMR buffer has wrong magic: 0x%08x", imrBuffer->magic) ;
            return OMX_ErrorHardware ;
        }

        if (imrBuffer->drmScheme == DRM_SCHEME_WV_CLASSIC)
        {
            mImplDRM = &mWVClassic ;
            wvres = mWVClassic.Init() ;
            if (wvres != OMXImplBase::OMXDRM_SUCCESS) {
                LOGE("WV Classic init failed (%d, 0x%08x)", wvres, wvres) ;
                return OMX_ErrorNotReady ;
            }
            LOGI("Initialized WV Classic") ;
        }
        else if (imrBuffer->drmScheme == DRM_SCHEME_WV_MODULAR)
        {
            mImplDRM = &mWVModular ;
            wvres = mWVModular.Init(imrBuffer->sessionId) ;
            if (wvres != OMXImplBase::OMXDRM_SUCCESS) {
                LOGE("WV Modular init failed (%d, 0x%08x)", wvres, wvres) ;
                return OMX_ErrorNotReady ;
            }
            LOGI("Initialized WV Modular") ;
        }
        else
        {
            LOGE("PrepareDecodeBuffer: unknown DRM scheme %u", imrBuffer->drmScheme) ;
            return OMX_ErrorNotReady ;
        }

        mIEDControl.Enable(false);
    }

    wvres = mImplDRM->HandlePrepareDecodeBuffer(buffer, retain, p) ;

    switch (wvres)
    {
        case OMXImplBase::OMXDRM_ERROR_NOT_READY:
            mSessionPaused = true;
            ret = OMX_ErrorNotReady ;
            break ;

        case OMXImplBase::OMXDRM_ERROR_HW:
            mSessionPaused = false;
            ret = OMX_ErrorHardware ;
            break ;

        case OMXImplBase::OMXDRM_SUCCESS:
            ret = OMX_ErrorNone;
            break;

        default:
            LOGE("HandlePrepareDecodeBuffer() failed (%d, 0x%08x)", wvres, wvres) ;
            ret = OMX_ErrorHardware ;
            break;
    }
    // End of switch

    return ret;
}
// End of OMXVideoDecoderAVCSecure::PrepareDecodeBuffer()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::BuildHandlerList(void) {
    OMXVideoDecoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoAvc, GetParamVideoAvc, SetParamVideoAvc);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoAVCProfileLevel, SetParamVideoAVCProfileLevel);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::GetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    memcpy(p, &mParamAvc, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::SetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortAvcParam implementation - Can we make simple copy????
    memcpy(&mParamAvc, p, sizeof(mParamAvc));
    return OMX_ErrorNone;
}


OMX_ERRORTYPE OMXVideoDecoderAVCSecure::GetParamVideoAVCProfileLevel(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,1);

    p->eProfile = mParamAvc.eProfile;
    p->eLevel = mParamAvc.eLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::SetParamVideoAVCProfileLevel(OMX_PTR pStructure) {
    LOGW("SetParamVideoAVCProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_U8* OMXVideoDecoderAVCSecure::MemAllocIMR(OMX_U32 nSizeBytes, OMX_PTR pUserData) {
    OMXVideoDecoderAVCSecure* p = (OMXVideoDecoderAVCSecure *)pUserData;
    if (p) {
        return p->MemAllocIMR(nSizeBytes);
    }
    LOGE("NULL pUserData.");
    return NULL;
}

void OMXVideoDecoderAVCSecure::MemFreeIMR(OMX_U8 *pBuffer, OMX_PTR pUserData) {
    OMXVideoDecoderAVCSecure* p = (OMXVideoDecoderAVCSecure *)pUserData;
    if (p) {
        p->MemFreeIMR(pBuffer);
        return;
    }
    LOGE("NULL pUserData.");
}

OMX_U8* OMXVideoDecoderAVCSecure::MemAllocIMR(OMX_U32 nSizeBytes) {
    // Ignore passed nSizeBytes, IMRDataBuffer internally reserves enough space

    for (int i = 0; i < INPORT_ACTUAL_BUFFER_COUNT; i++) {
        if (mIMRSlot[i].owner == NULL) {
            IMRDataBuffer *pBuffer = new IMRDataBuffer;
            if (pBuffer == NULL) {
                LOGE("Failed to allocate memory.");
                return NULL;
            }
            
            Init_IMRDataBuffer(pBuffer) ;

            pBuffer->frameStartOffset = mIMRSlot[i].offset;
            pBuffer->frameSize = INPORT_BUFFER_SIZE;
            mIMRSlot[i].owner = (OMX_U8 *)pBuffer;

            LOGI("Allocating buffer = %p, IMR offset = %#x(%d), data = %p",
                pBuffer, mIMRSlot[i].offset, mIMRSlot[i].offset, pBuffer->data);
            return (OMX_U8 *) pBuffer;
        }
    }
    LOGE("IMR slot is not available.");
    return NULL;
}

void OMXVideoDecoderAVCSecure::MemFreeIMR(OMX_U8 *pBuffer) {
    IMRDataBuffer *p = (IMRDataBuffer*) pBuffer;
    if (p == NULL) {
        return;
    }

    if (p->magic != IMR_DATA_BUFFER_MAGIC)
    {
        LOGE("MemFreeIMR: IMR buffer has wrong magic: 0x%08x", p->magic) ;
        return ;
    }

    for (int i = 0; i < INPORT_ACTUAL_BUFFER_COUNT; i++) {
        if (pBuffer == mIMRSlot[i].owner) {
            LOGV("Freeing IMR offset = %d", mIMRSlot[i].offset);
            delete p;
            mIMRSlot[i].owner = NULL;
            return;
        }
    }
    LOGE("Invalid buffer %#x to de-allocate", (uint32_t)pBuffer);
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::SetMaxOutputBufferCount(OMX_PARAM_PORTDEFINITIONTYPE *p) {
    OMX_ERRORTYPE ret;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, OUTPORT_INDEX);

    p->nBufferCountActual = OUTPORT_NATIVE_BUFFER_COUNT;
    return OMXVideoDecoderBase::SetMaxOutputBufferCount(p);
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.AVC.secure", "video_decoder.avc", OMXVideoDecoderAVCSecure);
