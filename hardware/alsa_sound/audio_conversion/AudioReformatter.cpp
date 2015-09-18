/*
 **
 ** Copyright 2013 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "AudioReformatter"

#include "AudioReformatter.h"
#include <cutils/log.h>

#define base AudioConverter

using namespace android;

namespace android_audio_legacy{

AudioReformatter::AudioReformatter(SampleSpecItem sampleSpecItem) :
    base(sampleSpecItem)
{
}

status_t AudioReformatter::configure(const SampleSpec &ssSrc, const SampleSpec &ssDst)
{
    status_t status = base::configure(ssSrc, ssDst);
    if (status != NO_ERROR) {

        return status;
    }

    if (ssSrc.getFormat() == AUDIO_FORMAT_PCM_16_BIT &&
            ssDst.getFormat() == AUDIO_FORMAT_PCM_8_24_BIT) {

        _convertSamplesFct =
                static_cast<SampleConverter>(&AudioReformatter::convertS16toS24over32);

    } else if (ssSrc.getFormat() == AUDIO_FORMAT_PCM_8_24_BIT &&
               ssDst.getFormat() == AUDIO_FORMAT_PCM_16_BIT) {

        _convertSamplesFct =
                static_cast<SampleConverter>(&AudioReformatter::convertS24over32toS16);

    } else {

        LOGE("%s: reformatter not available", __FUNCTION__);
        return INVALID_OPERATION;
    }
    return NO_ERROR;
}

status_t AudioReformatter::convertS16toS24over32(const void *src,
                                                  void *dst,
                                                  const uint32_t inFrames,
                                                  uint32_t *outFrames)
{
    uint32_t i;
    const int16_t *src16 = (const int16_t *)src;
    uint32_t *dst32 = (uint32_t *)dst;
    size_t n = inFrames * _ssSrc.getChannelCount();

    for (i = 0; i < n; i ++) {

        *(dst32 + i) = (uint32_t)((int32_t) *(src16 + i) << 16) >> 8;
    }
    // Transformation is "iso"frames
    *outFrames = inFrames;

    return NO_ERROR;
}

status_t AudioReformatter::convertS24over32toS16(const void *src,
                                                  void *dst,
                                                  const uint32_t inFrames,
                                                  uint32_t *outFrames)
{
    const uint32_t *src32 = (const uint32_t *)src;
    int16_t *dst16 = (int16_t *)dst;
    uint32_t i;

    size_t n = inFrames * _ssSrc.getChannelCount();

    for (i = 0; i < n; i ++) {

         *(dst16 + i) = (int16_t) (((int32_t)(*(src32 + i)) <<  8) >> 16);
    }
    // Transformation is "iso"frames
    *outFrames = inFrames;

    return NO_ERROR;
}

}; // namespace android
