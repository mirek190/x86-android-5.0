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

#define LOG_TAG "AudioConversion"

#include "AudioConversion.h"
#include "AudioConverter.h"
#include "AudioReformatter.h"
#include "AudioRemapper.h"
#include "AudioResampler.h"
#include "AudioUtils.h"
#include <media/AudioBufferProvider.h>
#include <cutils/log.h>
#include <stdlib.h>

using namespace android;
using namespace std;

namespace android_audio_legacy{

const uint32_t AudioConversion::MAX_RATE = 92000;

const uint32_t AudioConversion::MIN_RATE = 8000;

AudioConversion::AudioConversion() :
    _convOutBufferIndex(0),
    _convOutFrames(0),
    _convOutBufferSizeInFrames(0),
    _convOutBuffer(NULL)
{
    _audioConverter[ChannelCountSampleSpecItem] = new AudioRemapper(ChannelCountSampleSpecItem);
    _audioConverter[FormatSampleSpecItem] = new AudioReformatter(FormatSampleSpecItem);
    _audioConverter[RateSampleSpecItem] = new AudioResampler(RateSampleSpecItem);
}

AudioConversion::~AudioConversion()
{
    for (int i = 0; i < NbSampleSpecItems; i++) {

        delete _audioConverter[i];
        _audioConverter[i] = NULL;
    }

    free(_convOutBuffer);
    _convOutBuffer = NULL;
}

status_t AudioConversion::configure(const SampleSpec &ssSrc, const SampleSpec &ssDst)
{
    status_t ret = NO_ERROR;

    emptyConversionChain();

    free(_convOutBuffer);

    _convOutBuffer = NULL;
    _convOutBufferIndex = 0;
    _convOutFrames = 0;
    _convOutBufferSizeInFrames = 0;

    _ssSrc = ssSrc;
    _ssDst = ssDst;

    if (ssSrc == ssDst) {

        LOGD("%s: no convertion required", __FUNCTION__);
        return ret;
    }

    SampleSpec tmpSsSrc = ssSrc;

    // Start by adding the remapper, it will add consequently the reformatter and resampler
    // This function may alter the source sample spec
    ret = configureAndAddConverter(ChannelCountSampleSpecItem, &tmpSsSrc, &ssDst);
    if (ret != NO_ERROR) {

        return ret;
    }

    // Assert the temporary sample spec equals the destination sample spec
    LOG_ALWAYS_FATAL_IF(tmpSsSrc != ssDst);

    return ret;
}

status_t AudioConversion::getConvertedBuffer(void *dst,
                                             const uint32_t outFrames,
                                             AudioBufferProvider *bufferProvider)
{
    LOG_ALWAYS_FATAL_IF(bufferProvider == NULL);
    LOG_ALWAYS_FATAL_IF(dst == NULL);

    status_t status = NO_ERROR;

    if (_activeAudioConvList.empty()) {

        LOGE("%s: conversion called with empty converter list", __FUNCTION__);
        return NO_INIT;
    }

    //
    // Realloc the Output of the conversion if required (with margin of the worst case)
    //
    if (_convOutBufferSizeInFrames < outFrames) {

        _convOutBufferSizeInFrames = outFrames + (MAX_RATE / MIN_RATE) * 2;
        _convOutBuffer = static_cast<int16_t *>(realloc(_convOutBuffer,
                                _ssDst.convertFramesToBytes(_convOutBufferSizeInFrames)));
    }

    size_t framesRequested = outFrames;

    //
    // Frames are already available from the ConvOutBuffer, empty it first!
    //
    if (_convOutFrames) {

        size_t frameToCopy = min(framesRequested, _convOutFrames);
        framesRequested -= frameToCopy;
        _convOutBufferIndex += _ssDst.convertFramesToBytes(frameToCopy);
    }

    //
    // Frames still needed? (_pConvOutBuffer emptied!)
    //
    while (framesRequested != 0) {

        //
        // Outputs in the convOutBuffer
        //
        AudioBufferProvider::Buffer &buffer(_convInBuffer);

        // Calculate the frames we need to get from buffer provider
        // (Runs at ssSrc sample spec)
        // Note that is is rounded up.
        buffer.frameCount = AudioUtils::convertSrcToDstInFrames(framesRequested, _ssDst, _ssSrc);

        //
        // Acquire next buffer from buffer provider
        //
        status = bufferProvider->getNextBuffer(&buffer);
        if (status != NO_ERROR) {

            return status;
        }

        //
        // Convert
        //
        size_t convertedFrames;
        char *convBuf = reinterpret_cast<char *>(_convOutBuffer) + _convOutBufferIndex;
        status = convert(buffer.raw, reinterpret_cast<void **>(&convBuf),
                         buffer.frameCount, &convertedFrames);
        if (status != NO_ERROR) {

            bufferProvider->releaseBuffer(&buffer);
            return status;
        }

        _convOutFrames += convertedFrames;
        _convOutBufferIndex += _ssDst.convertFramesToBytes(convertedFrames);

        size_t framesToCopy = min(framesRequested, convertedFrames);
        framesRequested -= framesToCopy;

        //
        // Release the buffer
        //
        bufferProvider->releaseBuffer(&buffer);
    }

    //
    // Copy requested outFrames from the output buffer of the conversion.
    // Move the remaining frames to the beginning of the convOut buffer
    //
    memcpy(dst, _convOutBuffer, _ssDst.convertFramesToBytes(outFrames));
    _convOutFrames -= outFrames;

    //
    // Move the remaining frames to the beginning of the convOut buffer
    //
    if (_convOutFrames) {

        memmove(_convOutBuffer,
                reinterpret_cast<char *>(_convOutBuffer) + _ssDst.convertFramesToBytes(outFrames),
                _ssDst.convertFramesToBytes(_convOutFrames));
    }

    // Reset buffer Index
    _convOutBufferIndex = 0;

    return NO_ERROR;
}

status_t AudioConversion::convert(const void *src,
                                  void **dst,
                                  const uint32_t inFrames,
                                  uint32_t *outFrames)
{
    const void *srcBuf = src;
    void *dstBuf = NULL;
    size_t srcFrames = inFrames;
    size_t dstFrames = 0;
    status_t status = NO_ERROR;

    if (_activeAudioConvList.empty()) {

        // Empty converter list -> No need for convertion
        // Copy the input on the ouput if provided by the client
        // or points on the imput buffer
        if (*dst) {

            memcpy(*dst, src, _ssSrc.convertFramesToBytes(inFrames));
            *outFrames = inFrames;
        } else {

            *dst = (void *)src;
            *outFrames = inFrames;
        }
        return NO_ERROR;
    }

    AudioConverterListIterator it;
    for (it = _activeAudioConvList.begin(); it != _activeAudioConvList.end(); ++it) {

        AudioConverter *pConv = *it;
        dstBuf = NULL;
        dstFrames = 0;

        if (*dst && (pConv == _activeAudioConvList.back())) {

            // Last converter must output within the provided buffer (if provided!!!)
            dstBuf = *dst;
        }
        status = pConv->convert(srcBuf, &dstBuf, srcFrames, &dstFrames);
        if (status != NO_ERROR) {

            return status;
        }
        srcBuf = dstBuf;
        srcFrames = dstFrames;
    }
    *dst = dstBuf;
    *outFrames = dstFrames;

    return status;
}

void AudioConversion::emptyConversionChain()
{
    _activeAudioConvList.clear();
}

status_t AudioConversion::doConfigureAndAddConverter(SampleSpecItem sampleSpecItem,
                                                     SampleSpec *ssSrc,
                                                     const SampleSpec *ssDst)
{
    LOG_ALWAYS_FATAL_IF(sampleSpecItem >= NbSampleSpecItems);

    SampleSpec tmpSsDst = *ssSrc;
    tmpSsDst.setSampleSpecItem(sampleSpecItem, ssDst->getSampleSpecItem(sampleSpecItem));
    if (sampleSpecItem == ChannelCountSampleSpecItem) {

        tmpSsDst.setChannelsPolicy(ssDst->getChannelsPolicy());
    }

    status_t ret = _audioConverter[sampleSpecItem]->configure(*ssSrc, tmpSsDst);
    if (ret != NO_ERROR) {

        return ret;
    }
    _activeAudioConvList.push_back(_audioConverter[sampleSpecItem]);
    *ssSrc = tmpSsDst;

    return NO_ERROR;
}

status_t AudioConversion::configureAndAddConverter(SampleSpecItem sampleSpecItem,
                                                   SampleSpec *ssSrc,
                                                   const SampleSpec *ssDst)
{
    LOG_ALWAYS_FATAL_IF(sampleSpecItem >= NbSampleSpecItems);

    // If the input format size is higher, first perform the reformat
    // then add the resampler
    // and perform the reformat (if not already done)
    if (ssSrc->getSampleSpecItem(sampleSpecItem) > ssDst->getSampleSpecItem(sampleSpecItem)) {

        status_t ret = doConfigureAndAddConverter(sampleSpecItem, ssSrc, ssDst);
        if (ret != NO_ERROR) {

            return ret;
        }
    }

    if ((sampleSpecItem + 1) < NbSampleSpecItems) {
        // Dive
        status_t ret = configureAndAddConverter((SampleSpecItem)(sampleSpecItem + 1), ssSrc,
                                                ssDst);
        if (ret != NO_ERROR) {

            return ret;
        }
    }

    // Handle the case of destination sample spec item is higher than input sample spec
    // or destination and source channels policy are different
    if (!SampleSpec::isSampleSpecItemEqual(sampleSpecItem, *ssSrc, *ssDst)) {

        return doConfigureAndAddConverter(sampleSpecItem, ssSrc, ssDst);
    }
    return NO_ERROR;
}

}; // namespace android
