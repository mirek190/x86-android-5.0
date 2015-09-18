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

#define LOG_TAG "AudioConverter"

#include "AudioConverter.h"
#include "AudioUtils.h"
#include <cutils/log.h>

using namespace android;

namespace android_audio_legacy {

AudioConverter::AudioConverter(SampleSpecItem sampleSpecItem) :
    _convertSamplesFct(NULL),
    _ssSrc(),
    _ssDst(),
    _convertBuf(NULL),
    _convertBufSize(0),
    _sampleSpecItem(sampleSpecItem)
{
}

AudioConverter::~AudioConverter()
{
    delete [] _convertBuf;
}

//
// This function gets an output buffer suitable
// to convert inFrames input frames
//
void* AudioConverter::getOutputBuffer(ssize_t inFrames)
{
    status_t ret = NO_ERROR;
    size_t outBufSizeInBytes = _ssDst.convertFramesToBytes(convertSrcToDstInFrames(inFrames));

    if (outBufSizeInBytes > _convertBufSize) {

        ret = allocateConvertBuffer(outBufSizeInBytes);
        if (ret != NO_ERROR) {

            LOGE("%s: could not allocate memory for operation", __FUNCTION__);
            return NULL;
        }
    }
    return (void* )_convertBuf;
}

status_t AudioConverter::allocateConvertBuffer(ssize_t bytes)
{
    status_t ret = NO_ERROR;
    // Allocate one more frame for resampler
    _convertBufSize = bytes +
            (audio_bytes_per_sample(_ssDst.getFormat()) * _ssDst.getChannelCount());

    delete []_convertBuf;
    _convertBuf = NULL;

    _convertBuf = new char[_convertBufSize];

    if (!_convertBuf) {

        LOGE("cannot allocate resampler tmp buffers.\n");
        ret = NO_MEMORY;
    }
    return ret;
}

status_t AudioConverter::configure(const SampleSpec &ssSrc, const SampleSpec &ssDst)
{
    _ssSrc = ssSrc;
    _ssDst = ssDst;

    for (int i = 0; i < NbSampleSpecItems; i++) {

        if (i == _sampleSpecItem) {

            if (SampleSpec::isSampleSpecItemEqual(static_cast<SampleSpecItem>(i), ssSrc, ssDst)) {

                // The Sample spec items on which the converter is working
                // are the same...
                return INVALID_OPERATION;
            }
            continue;
        }
        if (!SampleSpec::isSampleSpecItemEqual(static_cast<SampleSpecItem>(i), ssSrc, ssDst)) {

            // The Sample spec items on which the converter is NOT working
            // MUST BE the same...
            LOGE("%s: not supported", __FUNCTION__);
            return INVALID_OPERATION;
        }
    }

    // Reset the convert function pointer
    _convertSamplesFct = NULL;

    // force the size to 0 to clear the buffer
    _convertBufSize = 0;

    return NO_ERROR;
}

status_t AudioConverter::convert(const void *src,
                                  void **dst,
                                  const uint32_t inFrames,
                                  uint32_t *outFrames)
{
    void *outBuf;
    status_t ret = NO_ERROR;

    // output buffer might be provided by the caller
    outBuf = *dst != NULL ? *dst : getOutputBuffer(inFrames);
    if (!outBuf) {

        return NO_MEMORY;
    }
    if (_convertSamplesFct != NULL) {

        ret = (this->*_convertSamplesFct)(src, outBuf, inFrames, outFrames);
    }
    *dst = outBuf;

    return ret;
}

size_t AudioConverter::convertSrcToDstInFrames(ssize_t frames) const
{
    return AudioUtils::convertSrcToDstInFrames(frames, _ssSrc, _ssDst);
}

size_t AudioConverter::convertSrcFromDstInFrames(ssize_t frames) const
{
    return AudioUtils::convertSrcToDstInFrames(frames, _ssDst, _ssSrc);
}

}; // namespace android
