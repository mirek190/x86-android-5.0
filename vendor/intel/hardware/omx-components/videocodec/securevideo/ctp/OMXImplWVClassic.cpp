/*
* Copyright (c) 2014 Intel Corporation.  All rights reserved.
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
#define LOG_TAG "OMXImplWVClassic"
#include <utils/Log.h>

extern "C" {
#include <sepdrm.h>
}

#include "OMXImplWVClassic.h"
#include "IMRDataBuffer.h"

#define WV_SESSION_ID           0x00000011
#define DRM_KEEP_ALIVE_TIMER    1000000
#define KEEP_ALIVE_INTERVAL     5 // seconds

#define ERROR_FROM_DRM_ERROR(err) (OMXDRM_ERROR_BASE - err)


OMXImplWVClassic::OMXImplWVClassic()
: mKeepAliveTimer(0)
{
    LOGV("constructor") ;
}
// End of OMXImplWVClassic::OMXImplWVClassic()

OMXImplWVClassic::~OMXImplWVClassic()
{
    LOGV("destructor") ;
}
// End of OMXImplWVClassic::OMXImplWVClassic()

int32_t OMXImplWVClassic::Init()
{
    //
    // Initialize security library
    //

    sec_result_t sepres = Drm_Library_Init();
    if (sepres != DRM_SUCCESSFUL)
    {
        LOGE("Drm_Library_Init() returned %08X", (unsigned int)sepres);
        return ERROR_FROM_DRM_ERROR(sepres) ;
    }

    LOGV("Drm_Library_Init() succeeded") ;

    //
    // Create WV Classic session
    //

    uint32_t imrOffset = 0;
    uint32_t imrBufferSize = IMR_BUFFER_SIZE;
    uint32_t sessionID = 0;

    sepres = Drm_WV_CreateSession(&imrOffset, &imrBufferSize, &sessionID);
    if (sepres != 0)
    {
        LOGE("Drm_WV_CreateSession failed. Result = %#x", sepres);
        return ERROR_FROM_DRM_ERROR(sepres) ;
    }

    // Note: originally, imrOffset parameter was added to Drm_WV_CreateSession(),
    // in case we would need to support several simultaneous protected content
    // apps (ha-ha), which would have to share IMR.  Since we only support one
    // protected content usage at a time, the imrOffset would always be 0.

    if (sessionID != WV_SESSION_ID)
    {
        LOGE("Invalid session ID %#x created", sessionID);
        return OMXDRM_ERROR_INV_SESSION ;
    }

    LOGI("Drm_WV_CreateSession: IMR Offset = %d, IMR size = %#x", imrOffset, imrBufferSize);

    if (imrBufferSize != IMR_BUFFER_SIZE)
    {
        LOGE("Mismatch in IMR size: Requested: %#x Obtained: %#x", IMR_BUFFER_SIZE, imrBufferSize);
        return OMXDRM_ERROR ;
    }

    //
    // Create keep-alive timer
    //

    int ret;
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = this;
    sev.sigev_notify_function = KeepAliveTimerCallback;

    ret = timer_create(CLOCK_REALTIME, &sev, &mKeepAliveTimer);
    if (ret != 0)
    {
        LOGE("Failed to create timer (%d)", ret);

        // Call Deinit() to destroy session.
        Deinit() ;

        return OMXDRM_ERROR ;
    }
    else
    {
        struct itimerspec its;
        its.it_value.tv_sec = -1; // never expire
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = KEEP_ALIVE_INTERVAL;
        its.it_interval.tv_nsec = 0;

        ret = timer_settime(mKeepAliveTimer, TIMER_ABSTIME, &its, NULL);
        if (ret != 0)
        {
            LOGE("Failed to set timer (%d)", ret);
        }
    }

    LOGI("Created keep-alive timer") ;

    return OMXDRM_SUCCESS ;
}
// End of OMXImplWVClassic::Init()

void OMXImplWVClassic::Deinit()
{
    // Session should be torn down in ProcessorStop, delayed to ProcessorDeinit
    // to allow remaining frames completely rendered.
    LOGI("Calling Drm_DestroySession.");
    sec_result_t sepres = Drm_DestroySession(WV_SESSION_ID);
    if (sepres != DRM_SUCCESSFUL)
    {
        LOGW("Drm_DestroySession returns %#x", sepres);
    }
}
// End of OMXImplWVClassic::Deinit()

int32_t OMXImplWVClassic::HandleProcessorStop()
{
    if (mKeepAliveTimer != 0)
    {
        timer_delete(mKeepAliveTimer);
        mKeepAliveTimer = 0;
    }

    return OMXDRM_SUCCESS ;
}
// End of OMXImplWVClassic::HandleProcessorStop()

void OMXImplWVClassic::HandleProcessorPause()
{
    sec_result_t sepres = Drm_Playback_Pause(WV_SESSION_ID);
    if (sepres != DRM_SUCCESSFUL) {
        LOGE("Drm_Playback_Pause() failed. Result = %#x", sepres);
    }
}
// End of OMXImplWVClassic::HandleProcessorPause()

void OMXImplWVClassic::HandleProcessorResume()
{
    sec_result_t sepres = Drm_Playback_Resume(WV_SESSION_ID);
    if (sepres != 0) {
        LOGE("Drm_Playback_Resume failed. Result = %#x", sepres);
    }
}
// End of OMXImplWVClassic::HandleProcessorResume()

int32_t OMXImplWVClassic::HandlePrepareDecodeBuffer(
    OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p)
{
    if (buffer == NULL || p == NULL)
    {
        return OMXDRM_ERROR_INV_ARG ;
    }

    IMRDataBuffer *imrBuffer = (IMRDataBuffer *)buffer->pBuffer;

    if (imrBuffer->magic != IMR_DATA_BUFFER_MAGIC)
    {
        LOGE("HandlePrepareDecodeBuffer: IMR buffer has wrong magic: 0x%08x", imrBuffer->magic) ;
        return OMXDRM_ERROR_HW ;
    }

    if (imrBuffer->isClear)
    {
        p->data = imrBuffer->data + buffer->nOffset;
        p->size = buffer->nFilledLen;
    }
    else
    {
        // Return NALU headers in the clear, use IMRDataBuffer::data for that.

        imrBuffer->size = NALU_BUFFER_SIZE;
        sec_result_t secres = Drm_WV_ReturnNALUHeaders(
            WV_SESSION_ID, imrBuffer->frameStartOffset, buffer->nFilledLen, imrBuffer->data, &(imrBuffer->size));
        if (secres == DRM_FAIL_FW_SESSION)
        {
            LOGW("HandlePrepareDecodeBuffer: Drm_WV_ReturnNALUHeaders failed (DRM_FAIL_FW_SESSION). Session is disabled.");
            return OMXDRM_ERROR_NOT_READY;
        }
        else if (secres != 0)
        {
            LOGE("HandlePrepareDecodeBuffer: Drm_WV_ReturnNALUHeaders failed. Error = %#x, IMR offset = %u, len = %lu",
                secres, imrBuffer->frameStartOffset, buffer->nFilledLen);
            return OMXDRM_ERROR_HW;
        }
        else
        {
            // Success
            p->data = imrBuffer->data;
            p->size = imrBuffer->size;
            p->flag |= IS_SECURE_DATA;
        }
    }

    return OMXDRM_SUCCESS ;
}
// End of OMXImplWVClassic::HandlePrepareDecodeBuffer()

void OMXImplWVClassic::KeepAliveTimerCallback(sigval v) {
    OMXImplWVClassic *p = static_cast<OMXImplWVClassic *>(v.sival_ptr);
    if (p) {
        p->KeepAliveTimerCallback();
    }
}
// End of OMXImplWVClassic::KeepAliveTimerCallback()

void OMXImplWVClassic::KeepAliveTimerCallback() {
    uint32_t timeout = DRM_KEEP_ALIVE_TIMER;
    sec_result_t sepres =  Drm_KeepAlive(WV_SESSION_ID, &timeout);
    if (sepres != 0) {
        LOGE("Drm_KeepAlive failed. Result = %#x", sepres);
    }
}
// End of OMXImplWVClassic::KeepAliveTimerCallback()
