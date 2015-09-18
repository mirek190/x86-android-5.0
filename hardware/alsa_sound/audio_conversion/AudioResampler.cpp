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

#define LOG_TAG "AudioResampler"

#include <cutils/log.h>

#include "AudioResampler.h"
#include "Resampler.h"

#define base AudioConverter

using namespace android;

namespace android_audio_legacy{

AudioResampler::AudioResampler(SampleSpecItem sampleSpecItem) :
    base(sampleSpecItem),
    _resampler(new Resampler(RateSampleSpecItem)),
    _pivotResampler(new Resampler(RateSampleSpecItem)),
    _activeResamplerList()
{
}

AudioResampler::~AudioResampler()
{
    _activeResamplerList.clear();
    delete _resampler;
    delete _pivotResampler;
}

status_t AudioResampler::configure(const SampleSpec &ssSrc, const SampleSpec &ssDst)
{
    _activeResamplerList.clear();

    status_t status = base::configure(ssSrc, ssDst);
    if (status != NO_ERROR) {

        return status;
    }

    status = _resampler->configure(ssSrc, ssDst);
    if (status != NO_ERROR) {

        //
        // Our resampling lib does not support all conversions
        // using 2 resamplers
        //
        LOGD("%s: trying to use working sample rate @ 48kHz", __FUNCTION__);
        SampleSpec pivotSs = ssDst;
        pivotSs.setSampleRate(PIVOT_SAMPLE_RATE);

        status = _pivotResampler->configure(ssSrc, pivotSs);
        if (status != NO_ERROR) {

            LOGD("%s: trying to use pivot sample rate @ %dkHz: FAILED",
                 __FUNCTION__, PIVOT_SAMPLE_RATE);
            return status;
        }
        _activeResamplerList.push_back(_pivotResampler);

        status = _resampler->configure(pivotSs, ssDst);
        if (status != NO_ERROR) {

            LOGD("%s: trying to use pivot sample rate @ 48kHz: FAILED", __FUNCTION__);
            return status;
        }
    }
    _activeResamplerList.push_back(_resampler);

    return NO_ERROR;
}

status_t AudioResampler::convert(const void *src,
                                  void **dst,
                                  uint32_t inFrames,
                                  uint32_t *outFrames)
{
    const void *srcBuf = static_cast<const void *>(src);
    void *dstBuf = NULL;
    size_t srcFrames = inFrames;
    size_t dstFrames = 0;

    ResamplerListIterator it;
    for (it = _activeResamplerList.begin(); it != _activeResamplerList.end(); ++it) {

        Resampler *conv = *it;
        dstFrames = 0;

        if (*dst && conv == _activeResamplerList.back()) {

            // Last converter must output within the provided buffer (if provided!!!)
            dstBuf = *dst;
        }
        status_t status = conv->convert(srcBuf, &dstBuf, srcFrames, &dstFrames);
        if (status != NO_ERROR) {

            return status;
        }
        srcBuf = dstBuf;
        srcFrames = dstFrames;
    }
    *dst = dstBuf;
    *outFrames = dstFrames;
    return NO_ERROR;
}

}; // namespace android
