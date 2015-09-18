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
#define LOG_TAG "OMXImplWVModular"
#include <utils/Log.h>

#include "OMXImplWVModular.h"
#include "IMRDataBuffer.h"

extern "C"
{
#include "drm_wv_mod_lib.h"
}
#define MAX_NALUS_IN_FRAME      16

OMXImplWVModular::OMXImplWVModular()
: mSessionId(INVALID_SESSION_ID)
{
    LOGV("constructor") ;
}
// End of OMXImplWVModular::OMXImplWVModular()

OMXImplWVModular::~OMXImplWVModular()
{
    LOGV("destructor") ;
}
// End of OMXImplWVModular::OMXImplWVModular()

int32_t OMXImplWVModular::Init(uint32_t sessionId)
{
    LOGV("Init(sessionId=%#x, pVideoDecoder=%p) enter", sessionId) ;

    if (sessionId == INVALID_SESSION_ID)
    {
        LOGE("Init(%u) exit (OMXDRM_ERROR_INV_ARG)", sessionId) ;
        return OMXDRM_ERROR_INV_ARG ;
    }

    if (mSessionId != INVALID_SESSION_ID)
    {
        LOGW("Deinit existing session %u", mSessionId) ;
        this->Deinit() ;
    }

    // Note: calling drm_wv_mod_initialize() multiple times in the same
    // process is allowed.
    // TODO: we should be running in the same MediaServer process, so
    // probably don't really need drm_wv_mod_initialize() call here.
    drm_wv_mod_result_t wvresult = drm_wv_mod_initialize() ;
    if (wvresult != DRM_WV_MOD_SUCCESS)
    {
        LOGE("Init: drm_wv_mod_initialize() returned %d", wvresult) ;
        return OMXDRM_MAKE_WVMOD_ERROR(wvresult) ;
    }

    // Session is created in WV Modular OEMCrypto.
    mSessionId = sessionId ;

    LOGV("Init(mSessionId=%#x) exit (OMXDRM_SUCCESS)", mSessionId) ;
    return OMXDRM_SUCCESS ;
}
// End of OMXImplWVModular::Init()

void OMXImplWVModular::Deinit()
{
    // Since OEMCrypto creates the session, the session should be closed
    // there as well.
    drm_wv_mod_result_t ret = drm_wv_mod_end_playback(mSessionId);
    if (ret != DRM_WV_MOD_SUCCESS) {
        LOGE("%s failed: %s", __FUNCTION__, ret);
    }
    mSessionId = INVALID_SESSION_ID ;
}
// End of OMXImplWVModular::Deinit()

int32_t OMXImplWVModular::HandleProcessorStop()
{
    if (mSessionId == INVALID_SESSION_ID)
    {
        LOGE("HandleProcessorStop: invalid session ID") ;
        return OMXDRM_ERROR_INV_SESSION ;
    }

    return OMXDRM_SUCCESS ;
}
// End of OMXImplWVModular::HandleProcessorStop()

void OMXImplWVModular::HandleProcessorPause()
{
    // Nothing to do for WV Modular.
}
// End of OMXImplWVModular::HandleProcessorPause()

void OMXImplWVModular::HandleProcessorResume()
{
    // Nothing to do for WV Modular.
}
// End of OMXImplWVModular::HandleProcessorResume()


int32_t OMXImplWVModular::HandlePrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer,
                                                    buffer_retain_t *retain,
                                                    VideoDecodeBuffer *p)
{
    // Aggregate sub sample table entries
    uint32_t num_of_nalu;
    uint32_t nalu_index =0;
    uint32_t nalu_size = 0 ;


    if (buffer == NULL || p == NULL)
    {
        return OMXDRM_ERROR_INV_ARG ;
    }

    IMRDataBuffer *imrBuffer = (IMRDataBuffer *)buffer->pBuffer;
    if (imrBuffer == NULL)
    {
        LOGE("HandlePrepareDecodeBuffer: IMR buffer pointer is NULL") ;
        return OMXDRM_ERROR_HW ;
    }
    if (imrBuffer->isClear)
    {
              // Clear buffer is just getting passed through to the video decoder
       p->data = imrBuffer->data + buffer->nOffset;
       p->size = buffer->nFilledLen;
       p->flag |=  IS_SUBSAMPLE_ENCRYPTION;

       LOGV("%s: handling clear data, imrBuffer->data = %p, "
           "buffer->nOffset = %lu, buffer->nFilledLen = %lu, p->data = %p, p->size = %u",
           __FUNCTION__, imrBuffer->data, buffer->nOffset, buffer->nFilledLen,
           p->data, p->size) ;
    }
    else {
        if (mSessionId == INVALID_SESSION_ID){
            LOGE("HandlePrepareDecodeBuffer: invalid session ID") ;
            return OMXDRM_ERROR_INV_SESSION ;
        }
        if (imrBuffer->numNalus == 0){
            p->data = imrBuffer->data + buffer->nOffset;
            p->size = buffer->nFilledLen;
            p->flag |=  IS_SUBSAMPLE_ENCRYPTION;
        }
        else{

            // Copy SPS and PPS to the buffer
            uint8_t type = imrBuffer->data[NALU_HEADER_SIZE - 1] & 0x1F;
            if  ((type == NAL_UNIT_TYPE_SPS) || (type == NAL_UNIT_TYPE_PPS)) {

                mFrameInfo.nalus[nalu_index].length = imrBuffer->size;
                mFrameInfo.nalus[nalu_index].type = imrBuffer->data[buffer->nOffset + NALU_HEADER_SIZE - 1];
                mFrameInfo.nalus[nalu_index].data = imrBuffer->data + buffer->nOffset;
                nalu_index++;
            }
            for(uint32_t i = 0; i < imrBuffer->numNalus; i++) {

                nalu_size = 0 ;
                if( (i+1) < imrBuffer->numNalus) {
                    nalu_size = (imrBuffer->naluOffsets[i+1]- imrBuffer->naluOffsets[i]);
                }
                else {
                    nalu_size = imrBuffer->frameSize - imrBuffer->naluOffsets[i];
                }
                mFrameInfo.nalus[nalu_index].length = nalu_size;
                mFrameInfo.nalus[nalu_index].type = imrBuffer->naluPrefixes[i].bytes[NALU_HEADER_SIZE - 1];
                type = (imrBuffer->naluPrefixes[i].bytes[NALU_HEADER_SIZE - 1]) & 0x1F;
                if ((type  >= NAL_UNIT_TYPE_SLICE) && (type  <= NAL_UNIT_TYPE_SEI)){

                    mFrameInfo.nalus[nalu_index].imr_offset = (imrBuffer->frameStartOffset + imrBuffer->naluOffsets[i]);
                }
                mFrameInfo.nalus[nalu_index].data = NULL;
                LOGV(" Subsample Type: 0x%x, Subsample Length: 0x%x, Offset: 0x%x ",mFrameInfo.nalus[nalu_index].type, mFrameInfo.nalus[nalu_index].length, mFrameInfo.nalus[nalu_index].imr_offset);
                nalu_index ++;

            }
            mFrameInfo.num_nalus = nalu_index;
            p->data = (uint8_t *)&mFrameInfo;
            p->size = sizeof(frame_info_t);
            p->flag |=  IS_SECURE_DATA | IS_SUBSAMPLE_ENCRYPTION;
        }
    }
    return OMXDRM_SUCCESS ;
}

