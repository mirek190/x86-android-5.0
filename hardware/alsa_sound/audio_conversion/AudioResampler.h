/*
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

#include <list>
#include "AudioConverter.h"

namespace android_audio_legacy {

class Resampler;

class AudioResampler : public AudioConverter {

    typedef std::list<Resampler *>::iterator ResamplerListIterator;

public:
    AudioResampler(SampleSpecItem sampleSpecItem);

    virtual ~AudioResampler();

private:
    // forbid copy
    AudioResampler(const AudioResampler &);
    AudioResampler &operator =(const AudioResampler &);
    android::status_t resampleFrames(const void *src,
                                     void *dst,
                                     const uint32_t inFrames,
                                     uint32_t *outFrames);

    virtual android::status_t configure(const SampleSpec &ssSrc, const SampleSpec &ssDst);

    virtual android::status_t convert(const void *src,
                                      void **dst,
                                      uint32_t inFrames,
                                      uint32_t *outFrames);

    Resampler *_resampler;
    Resampler *_pivotResampler;

    // List of audio converter enabled
    std::list<Resampler *> _activeResamplerList;

    static const uint32_t PIVOT_SAMPLE_RATE = 48000;
};

}; // namespace android
