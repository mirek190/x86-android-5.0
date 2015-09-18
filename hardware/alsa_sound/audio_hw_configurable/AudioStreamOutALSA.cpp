/* AudioStreamOutALSA.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <utils/Log.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "AudioStreamOutAlsa"

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include <tinyalsa/asoundlib.h>

#include "AudioStreamOutALSA.h"
#include "AudioStreamRoute.h"
#include <AudioCommsAssert.hpp>

#define base ALSAStreamOps

namespace android_audio_legacy
{

const uint32_t AudioStreamOutALSA::MAX_AGAIN_RETRY = 2;
const uint32_t AudioStreamOutALSA::WAIT_TIME_MS = 20;
const uint32_t AudioStreamOutALSA::WAIT_BEFORE_RETRY_US = 10000; //10ms
/**
 * Is aligned on one period time
 */
const uint32_t AudioStreamOutALSA::USEC_PER_MSEC = 1000;

AudioStreamOutALSA::AudioStreamOutALSA(AudioHardwareALSA *parent, audio_output_flags_t flags) :
    base(parent, "AudioOutLock"),
    mFrameCount(0),
    _flags(flags),
    mEchoReference(NULL)
{
}

AudioStreamOutALSA::~AudioStreamOutALSA()
{
}

uint32_t AudioStreamOutALSA::channels() const
{
    return ALSAStreamOps::channels();
}

status_t AudioStreamOutALSA::setVolume(float __UNUSED left, float __UNUSED right)
{
    return NO_ERROR;
}

size_t AudioStreamOutALSA::generateSilence(size_t bytes)
{
    // No HW will drive the timeline:
    //       we are here because of hardware error or missing route availability.
    // Also, keep time sync by sleeping the equivalent amount of time.
    usleep(mSampleSpec.convertFramesToUsec(mSampleSpec.convertBytesToFrames(bytes)));
    return bytes;
}

ssize_t AudioStreamOutALSA::write(const void *buffer, size_t bytes)
{
    setStandby(false);

    AutoR lock(_streamLock);

    // Check if the audio route is available for this stream
    if (!isRouteAvailableL()) {

        ALOGW("%s(buffer=%p, bytes=%d) No route available. Generating silence.",
            __FUNCTION__, buffer, bytes);
        return generateSilence(bytes);
    }

    AUDIOCOMMS_ASSERT(mHandle != NULL, "unexpected NULL handle on audio device");

    ssize_t srcFrames = mSampleSpec.convertBytesToFrames(bytes);
    size_t dstFrames = 0;
    char *dstBuf = NULL;
    status_t status;

    // Dump audio output before any conversion.
    // FOR DEBUG PURPOSE ONLY
    if (getDumpObjectBeforeConv() != NULL) {
        getDumpObjectBeforeConv()->dumpAudioSamples(buffer,
                                                    bytes,
                                                    isOut(),
                                                    mSampleSpec.getSampleRate(),
                                                    mSampleSpec.getChannelCount(),
                                                    "before_conversion");
    }

    pushEchoReference(buffer, srcFrames);

    status = applyAudioConversion(buffer, (void**)&dstBuf, srcFrames, &dstFrames);

    if (status != NO_ERROR) {

        return status;
    }
    ALOGV("%s: srcFrames=%lu, bytes=%d dstFrames=%d", __FUNCTION__, srcFrames, bytes, dstFrames);

    ssize_t ret = writeFrames(dstBuf, dstFrames);

    if (ret < 0) {

        if (ret != -EPIPE) {

            // Returns asap to catch up the returned error else, trash the audio data
            // and sleep the time the driver may need to consume it.
            ALOGD("%s(buffer=%p, bytes=%d) %s. Generating silence.",
                __FUNCTION__, buffer, bytes, pcm_get_error(mHandle));
            generateSilence(bytes);
        }

        return ret;
    }
    ALOGV("%s: returns %u", __FUNCTION__, mSampleSpec.convertFramesToBytes(
              CAudioUtils::convertSrcToDstInFrames(ret, mHwSampleSpec, mSampleSpec)));

    // Dump audio output after eventual conversions
    // FOR DEBUG PURPOSE ONLY
    if (getDumpObjectAfterConv() != NULL) {
      getDumpObjectAfterConv()->dumpAudioSamples((const void*)dstBuf,
                                                 mHwSampleSpec.convertFramesToBytes(dstFrames),
                                                 isOut(),
                                                 mHwSampleSpec.getSampleRate(),
                                                 mHwSampleSpec.getChannelCount(),
                                                 "after_conversion");
    }

    return mSampleSpec.convertFramesToBytes(AudioUtils::convertSrcToDstInFrames(ret,
                                                                                 mHwSampleSpec,
                                                                                 mSampleSpec));
}

ssize_t AudioStreamOutALSA::writeFrames(void* buffer, ssize_t frames)
{
    int ret;
    uint32_t retryCount = 0;

    do {
        ret = pcm_write(mHandle, (char *)buffer, pcm_frames_to_bytes(mHandle, frames));

        ALOGV("%s %d %d", __FUNCTION__, ret, pcm_frames_to_bytes(mHandle, frames));

        if (ret != 0) {
            ALOGE("%s: write error: %d %s", __FUNCTION__, ret, pcm_get_error(mHandle));

            // For debug purposes, dump registers
            printLPEfwDebugInfo();

            LOG_ALWAYS_FATAL_IF(++retryCount >= MAX_READ_WRITE_RETRIES,
                                    "Hardware not responding, restarting media server");

            // Get the number of microseconds to sleep, inferred from the number of
            // frames to write.
            size_t sleepUsecs = mHwSampleSpec.convertFramesToUsec(frames);

            // Go sleeping before trying I/O operation again.
            if (safeSleep(sleepUsecs) != NO_ERROR) {
                // If some error arises when trying to sleep, try I/O operation anyway.
                // Error counter will provoke the restart of mediaserver.
                ALOGE("%s:  Error while calling nanosleep interface", __FUNCTION__);
            }
        }
    } while (ret != 0);

    return frames;
}

status_t AudioStreamOutALSA::dump(int , const Vector<String16>& )
{
    return NO_ERROR;
}

status_t AudioStreamOutALSA::open(int __UNUSED mode)
{
    return setStandby(false);
}

//
// Called from Route Manager Context -> WLocked
//
status_t AudioStreamOutALSA::attachRouteL()
{
    status_t status = base::attachRouteL();
    if (status != NO_ERROR) {

        return status;
    }

    // Need to generate silence?
    AUDIOCOMMS_ASSERT(getCurrentRouteL() != NULL, "NULL route pointer");
    AUDIOCOMMS_ASSERT(mHandle != NULL, "NULL audio device handle");

    uint32_t uiSilenceMs = getCurrentRouteL()->getOutputSilencePrologMs();
    if (uiSilenceMs) {

        // Allocate a 1Ms buffer in stack
        uint32_t uiBufferSize = mHwSampleSpec.convertFramesToBytes(mHwSampleSpec.convertUsecToframes(1 * USEC_PER_MSEC));
        void* pSilenceBuffer = alloca(uiBufferSize);
        memset(pSilenceBuffer, 0, uiBufferSize);

        uint32_t uiMsCount;
        for (uiMsCount = 0; uiMsCount < uiSilenceMs; uiMsCount++) {

            pcm_write(mHandle,
                          (const char*)pSilenceBuffer,
                          uiBufferSize);
        }
    }

    return NO_ERROR;
}


status_t AudioStreamOutALSA::close()
{
    return setStandby(true);
}

status_t AudioStreamOutALSA::standby()
{
    mFrameCount = 0;

    return setStandby(true);
}

uint32_t AudioStreamOutALSA::latency() const
{
    return base::latency();
}

size_t AudioStreamOutALSA::bufferSize() const
{
    return getBufferSize(_flags);
}

// return the number of audio frames written by the audio dsp to DAC since
// the output has exited standby
status_t AudioStreamOutALSA::getRenderPosition(uint32_t *dspFrames)
{
    *dspFrames = mFrameCount;
    return NO_ERROR;
}

// flush the data down the flow. It is similar to drop.
status_t AudioStreamOutALSA::flush()
{
    AutoR lock(_streamLock);

    // Check if there is an available audio route to flush
    if (!isRouteAvailableL()) {

        ALOGW("%s: No route available. There is no pcm to flush.", __FUNCTION__);
        return NO_ERROR;
    }

    LOG_ALWAYS_FATAL_IF(mHandle == NULL);

    status_t status = pcm_stop(mHandle);
    ALOGD("pcm stop status %d", status);
    return status;
}


void AudioStreamOutALSA::setFlags(uint32_t uiFlags)
{
    AutoW lock(_streamLock);
    _flags = uiFlags;

    updateLatency(uiFlags);
}

status_t  AudioStreamOutALSA::setParameters(const String8& keyValuePairs)
{
    // Give a chance to parent to handle the change
    status_t status = mParent->setStreamParameters(this, keyValuePairs);

    if (status != NO_ERROR) {

        return status;
    }

    return ALSAStreamOps::setParameters(keyValuePairs);
}

void AudioStreamOutALSA::addEchoReference(struct echo_reference_itfe* reference)
{
    ALOGD("%s(reference = %p): note mEchoReference = %p", __FUNCTION__, reference, mEchoReference);

    AutoW lock(_streamLock);
    LOG_ALWAYS_FATAL_IF(reference == NULL);

    // Called from a WLocked context
    mEchoReference = reference;
}

void AudioStreamOutALSA::removeEchoReference(struct echo_reference_itfe* reference)
{
    AutoW lock(_streamLock);
    removeEchoReferenceL(reference);
}

void AudioStreamOutALSA::removeEchoReferenceL(struct echo_reference_itfe* reference)
{
    if (reference == NULL) {

        return ;
    }
    LOG_ALWAYS_FATAL_IF(mEchoReference == NULL);
    ALOGD("%s(reference = %p): note mEchoReference = %p", __FUNCTION__, reference, mEchoReference);
    // Called from a WLocked context
    if (mEchoReference == reference) {

        mEchoReference->write(mEchoReference, NULL);
        mEchoReference = NULL;
    }
}

int AudioStreamOutALSA::getPlaybackDelay(ssize_t frames, struct echo_reference_buffer* buffer)
{
    size_t kernel_frames;
    int status;
    status = pcm_get_htimestamp(mHandle, &kernel_frames, &buffer->time_stamp);
    if (status < 0) {

        buffer->time_stamp.tv_sec  = 0;
        buffer->time_stamp.tv_nsec = 0;
        buffer->delay_ns           = 0;
        ALOGE("get_playback_delay(): pcm_get_htimestamp error,"
              "setting playbackTimestamp to 0");
        return status;
    }
    kernel_frames = pcm_get_buffer_size(mHandle) - kernel_frames;

    /* adjust render time stamp with delay added by current driver buffer.
     * Add the duration of current frame as we want the render time of the last
     * sample being written.
     */
    buffer->delay_ns = mSampleSpec.convertFramesToUsec(kernel_frames + frames);

    ALOGV("%s: kernel_frames=%d buffer->time_stamp.tv_sec=%lu,"
          "buffer->time_stamp.tv_nsec =%lu buffer->delay_ns=%d",
          __FUNCTION__,
          kernel_frames,
          buffer->time_stamp.tv_sec,
          buffer->time_stamp.tv_nsec,
          buffer->delay_ns);

    return 0;
}

void AudioStreamOutALSA::pushEchoReference(const void *buffer, ssize_t frames)
{
    if (mEchoReference != NULL)
    {
        struct echo_reference_buffer b;
        b.raw = (void *)buffer;
        b.frame_count = frames;
        getPlaybackDelay(b.frame_count, &b);
        mEchoReference->write(mEchoReference, &b);
    }
}

status_t AudioStreamOutALSA::detachRouteL()
{
    removeEchoReferenceL(mEchoReference);

    return base::detachRouteL();
}

}       // namespace android
