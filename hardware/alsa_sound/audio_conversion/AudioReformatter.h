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

#include "AudioConverter.h"

namespace android_audio_legacy {

class AudioReformatter : public AudioConverter {

public:
    AudioReformatter(SampleSpecItem sampleSpecItem);

private:
    virtual android::status_t configure(const SampleSpec &ssSrc, const SampleSpec &ssDst);

    android::status_t convertS16toS24over32(const void *src,
                                            void *dst,
                                            const uint32_t inFrames,
                                            uint32_t *outFrames);

    android::status_t convertS24over32toS16(const void *src,
                                            void *dst,
                                            const uint32_t inFrames,
                                            uint32_t *outFrames);
};

}; // namespace android

