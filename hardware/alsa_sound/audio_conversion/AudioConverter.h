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

#include <utils/Errors.h>
#include <SampleSpec.h>

namespace android_audio_legacy {

class AudioConverter {

public:

    /**
     * conversion function pointer definition.
     *
     * @param[in] src the source buffer.
     * @param[out] dst the destination buffer.
     *                  Note that if the memory is allocated by the converted, it is freed upon next
     *                  call of configure or upon deletion of the converter.
     * @param[in] inFrames number of input frames.
     * @param[out] outFrames output frames processed.
     *
     * @return status OK if conversion is successful, error code otherwise.
     */
    typedef android::status_t (AudioConverter::*SampleConverter)(const void *src,
                                                                  void *dst,
                                                                  uint32_t inFrames,
                                                                  uint32_t *outFrames);

    AudioConverter(SampleSpecItem sampleSpecItem);
    virtual ~AudioConverter();

    /**
     * Configures the converters.
     * It configures the converter that may be used to convert one sample spec item from the source
     * to destination sample spec item.
     *
     * @param[in] ssSrc source sample specifications.
     * @param[in] ssDst destination sample specification.
     *
     * @return status OK if converter is configured properly error code otherwise.
     */
    virtual android::status_t configure(const SampleSpec& ssSrc, const SampleSpec& ssDst);

    /**
     * Converts buffer from source to destination sample spec item.
     * Converts input frames of the provided input buffer into the destination buffer that may be
     * allocated by the client. If not, the converter will allocate the output and give it back
     * to the client.
     * Before using this function, configure must have been called.
     *
     * @param[in] src the source buffer.
     * @param[out] dst the destination buffer.
     *                  Note that if the memory is allocated by the converted, it is freed upon next
     *                  call of configure or upon deletion of the converter.
     * @param[in] inFrames number of input frames.
     * @param[out] outFrames output frames processed.
     *
     * @return status OK is convertion is successfull, error code otherwise.
     */
    virtual android::status_t convert(const void *src,
                                      void **dst,
                                      uint32_t inFrames,
                                      uint32_t *outFrames);

protected:

    /**
     * Converts the number of frames in the destination sample spec in a number of frames in the
     * source sample spec.
     *
     * @param[in] frames in the destination sample spec
     *
     * @return frames in the source sample spec.
     */
    size_t convertSrcFromDstInFrames(ssize_t frames) const;

    /**
     * Converts the number of frames in the source sample spec in a number of frames in the
     * destination sample spec.
     *
     * @param[in] frames in the source sample spec
     *
     * @return frames in the destination sample spec.
     */
    size_t convertSrcToDstInFrames(ssize_t frames) const;

    SampleConverter _convertSamplesFct;

    /**
     * Source audio data sample specifications
     */
    SampleSpec _ssSrc;

    /**
     * Destination audio data sample specifications
     */
    SampleSpec _ssDst;

private:
    // forbid copy
    AudioConverter(const AudioConverter &);
    AudioConverter &operator =(const AudioConverter &);

    void* getOutputBuffer(ssize_t inFrames);
    android::status_t allocateConvertBuffer(ssize_t bytes);

    char *_convertBuf;
    size_t  _convertBufSize;

    // Sample spec item on which the converter is working
    SampleSpecItem _sampleSpecItem;
};

}; // namespace android

