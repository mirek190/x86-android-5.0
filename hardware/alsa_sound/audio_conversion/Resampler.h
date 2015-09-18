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

#pragma once

#include "AudioConverter.h"

namespace android_audio_legacy {

class Resampler : public AudioConverter {

public:
    Resampler(SampleSpecItem sampleSpecItem);

    virtual ~Resampler();

    /**
     * Resamples buffer from source to destination sample rate.
     * Resamples input frames of the provided input buffer into the destination buffer already
     * allocated by the converter or given by the client.
     * Before using this function, configure must have been called.
     *
     * @param[in] src the source buffer.
     * @param[out] dst the destination buffer, caller to ensure the destination
     *             is large enough.
     * @param[in] inFrames number of input frames.
     * @param[out] outFrames output frames processed.
     *
     * @return error code.
     */
    android::status_t resampleFrames(const void *src,
                                     void *dst,
                                     const uint32_t inFrames,
                                     uint32_t *outFrames);

    /**
     * Configures the resampler.
     * It configures the resampler that may be used to convert samples from the source
     * to destination sample rate.
     *
     * @param[in] ssSrc source sample specifications.
     * @param[in] ssDst destination sample specification.
     *
     * @return status OK, error code otherwise.
     */
    virtual android::status_t configure(const SampleSpec &ssSrc, const SampleSpec &ssDst);

private:
    // forbid copy
    Resampler(const Resampler &);
    Resampler &operator =(const Resampler &);


    android::status_t allocateBuffer();

    void convertShort2Float(int16_t *inp, float *out, size_t sz) const;

    void convertFloat2Short(float *inp, int16_t *out, size_t sz) const;

    static const int BUF_SIZE = (1 << 13);
    size_t  _maxFrameCnt;  /* max frame count the buffer can store */
    void *_context;      /* handle used to do resample */
    float *_floatInp;     /* here sample size is 4 bytes */
    float *_floatOut;     /* here sample size is 4 bytes */
};

}; // namespace android
