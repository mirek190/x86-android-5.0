/* ALSAStreamOps.cpp
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

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <limits>
#include <fstream>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ALSAStreamOps"

#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <cutils/bitops.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "ALSAStreamOps.h"
#include "AudioStreamRoute.h"
#include <AudioConversion.h>
#include "AudioHardwareALSA.h"
#include <AudioCommsAssert.hpp>
#include "Property.h"

#define DEVICE_OUT_BLUETOOTH_SCO_ALL (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO | AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET | AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT)

using namespace android;
using std::numeric_limits;

namespace android_audio_legacy
{

const uint32_t ALSAStreamOps::STR_FORMAT_LENGTH = 32;


const uint32_t ALSAStreamOps::MAX_DEBUG_STREAM_SIZE = 998;

/**
 * Audio dump properties management (set with setprop)
 */
const std::string ALSAStreamOps::dumpBeforeConvProps[CUtils::ENbDirections] = \
    { "media.dump_input.befconv", "media.dump_output.befconv" };

const std::string ALSAStreamOps::dumpAfterConvProps[CUtils::ENbDirections] = \
    { "media.dump_input.aftconv", "media.dump_output.aftconv" };


ALSAStreamOps::ALSAStreamOps(AudioHardwareALSA *parent, const char* pcLockTag) :
    mParent(parent),
    mHandle(NULL),
    mStandby(true),
    mDevices(0),
    dumpBeforeConv(NULL),
    dumpAfterConv(NULL),
    mIsReset(false),
    mCurrentRoute(NULL),
    mNewRoute(NULL),
    mCurrentDevices(0),
    mNewDevices(0),
    mLatencyUs(0),
    mPowerLock(false),
    mPowerLockTag(pcLockTag),
    mAudioConversion(new AudioConversion)
{
    mSampleSpec.setChannelCount(AudioHardwareALSA::DEFAULT_CHANNEL_COUNT);
    mSampleSpec.setSampleRate(AudioHardwareALSA::DEFAULT_SAMPLE_RATE);
    mSampleSpec.setFormat(AudioHardwareALSA::DEFAULT_FORMAT);
}

ALSAStreamOps::~ALSAStreamOps()
{
    AUDIOCOMMS_ASSERT(!isStarted(), "stream deleted while still active");

    delete mAudioConversion;
    delete dumpAfterConv;
    delete dumpBeforeConv;
}

status_t ALSAStreamOps::set(int      *format,
                            uint32_t *channels,
                            uint32_t *rate)
{
    bool bad_channels = false;
    bool bad_rate = false;
    bool bad_format = false;

    ALOGV("%s() -- IN", __FUNCTION__);

    if (channels) {

        if (*channels != 0) {

            ALOGD("%s(requested channels: 0x%x (popcount returns %d))",
                  __FUNCTION__, *channels, popcount(*channels));
            // Always accept the channels requested by the client
            // as far as the channel count is supported
            mSampleSpec.setChannelMask(*channels);

            if (popcount(*channels) > 2) {

                ALOGD("%s: channels=(0x%x, %d) not supported", __FUNCTION__, *channels, popcount(*channels));
                bad_channels = true;
            }
        }
        if ((bad_channels) || (*channels == 0)) {

            // No channels information was provided by the client
            // or not supported channels
            // Use default: stereo
            if (isOut()) {

                *channels = AudioSystem::CHANNEL_OUT_FRONT_LEFT | AudioSystem::CHANNEL_OUT_FRONT_RIGHT;
            }
            else {

                *channels = AudioSystem::CHANNEL_IN_LEFT | AudioSystem::CHANNEL_IN_RIGHT;
            }
            mSampleSpec.setChannelMask(*channels);
        }
        ALOGD("%s: set channels to 0x%x", __FUNCTION__, *channels);

        // Resampler is always working @ the channel count of the HAL
        mSampleSpec.setChannelCount(popcount(mSampleSpec.getChannelMask()));
    }

    if (rate) {

        if (*rate != 0) {

            ALOGD("%s(requested rate: %d))", __FUNCTION__, *rate);
            // Always accept the rate provided by the client
            mSampleSpec.setSampleRate(*rate);
        }
        if ( (bad_rate) || (*rate == 0) ) {

            // No rate information was provided by the client
            // or set rate error
            // Use default HAL rate
            *rate = AudioHardwareALSA::DEFAULT_SAMPLE_RATE;
            mSampleSpec.setSampleRate(*rate);
        }
        ALOGD("%s: set rate to %d", __FUNCTION__, *rate);
    }

    if (format) {

        if (*format != 0) {

            ALOGD("%s(requested format: %d))", __FUNCTION__, *format);
            // Always accept the rate provided by the client
            // as far as this rate is supported
            if (*format != AUDIO_FORMAT_PCM_16_BIT && *format != AUDIO_FORMAT_PCM_8_24_BIT) {

                ALOGD("%s: format=(0x%x) not supported", __FUNCTION__, *format);
                bad_format = true;
            }

            mSampleSpec.setFormat(*format);
        }
        if ( (bad_format) || (*format == 0) ) {

            // No format provided or set format error
            // Use default HAL format
            *format = AudioHardwareALSA::DEFAULT_FORMAT;
            mSampleSpec.setFormat(*format);
        }
        ALOGD("%s : set format to %d (%d)", __FUNCTION__, *format, this->format());
    }


    if (bad_channels || bad_rate || bad_format) {

        return BAD_VALUE;
    }

    mHwSampleSpec = mSampleSpec;

    updateLatency(getApplicabilityMask());

    ALOGD("%s() -- OUT", __FUNCTION__);
    return NO_ERROR;
}

status_t ALSAStreamOps::setParameters(const String8 __UNUSED &keyValuePairs)
{
    return NO_ERROR;
}

String8 ALSAStreamOps::getParameters(const String8& keys)
{
    AudioParameter param = AudioParameter(keys);
    String8 value;
    String8 key = String8(AudioParameter::keyRouting);

    if (param.get(key, value) == NO_ERROR) {

        param.addInt(key, static_cast<int>(getCurrentDevices()));
    }

    LOGV("getParameters() %s", param.toString().string());
    return param.toString();
}

size_t ALSAStreamOps::getBufferSize(uint32_t flags) const
{
    pcm_config pcmConf = mParent->getDefaultPcmConfig(isOut(), flags);
    /**
     * Pcm config might not be in the same sample specification of the stream. So, need first
     * translate it into time domain, before switch back to frames and bytes.
     */
    uint64_t bufferSizeUs = (uint64_t)AudioUtils::USEC_TO_SEC * pcmConf.period_size /
                                                                                  pcmConf.rate;
    LOG_ALWAYS_FATAL_IF(bufferSizeUs > std::numeric_limits<uint32_t>::max());

    uint32_t bufferSizeInFrames = mSampleSpec.convertUsecToframes(bufferSizeUs);
    size_t bytes = mSampleSpec.convertFramesToBytes(bufferSizeInFrames);

    ALOGD("%s: %d (in bytes) for %s stream", __FUNCTION__, bytes, isOut() ? "output" : "input");
    return bytes;
}

uint32_t ALSAStreamOps::latency() const
{
    return AudioUtils::convertUsecToMsec(mLatencyUs);
}

void ALSAStreamOps::updateLatency(uint32_t uiFlags)
{
    pcm_config pcmConf = mParent->getDefaultPcmConfig(isOut(), uiFlags);
    uint64_t latency = (uint64_t)AudioUtils::USEC_TO_SEC * pcmConf.period_count *
                                            pcmConf.period_size  / pcmConf.rate;
    LOG_ALWAYS_FATAL_IF(latency > numeric_limits<uint32_t>::max());
    mLatencyUs = latency;
}

status_t ALSAStreamOps::setStandby(bool isSet)
{
    if (isStarted() == !isSet) {

        return OK;
    }
    setStarted(!isSet);

    return isSet ? mParent->stopStream(this) : mParent->startStream(this);
}

bool ALSAStreamOps::isRouteAvailable() const
{
    AutoR lock(_streamLock);
    return isRouteAvailableL();
}

bool ALSAStreamOps::isRouteAvailableL() const
{
    return mCurrentRoute != NULL;
}

status_t ALSAStreamOps::attachRoute()
{
    AutoW lock(_streamLock);
    return attachRouteL();
}

status_t ALSAStreamOps::attachRouteL()
{
    ALOGD("%s %s stream", __FUNCTION__, isOut()? "output" : "input");
    SampleSpec ssSrc;
    SampleSpec ssDst;

    //
    // Set the new pcm device and sample spec given by the audio stream route
    //
    mHandle = mNewRoute->getPcmDevice(isOut());
    mHwSampleSpec = mNewRoute->getSampleSpec(isOut());

    ssSrc = isOut() ? mSampleSpec : mHwSampleSpec;
    ssDst = isOut() ? mHwSampleSpec : mSampleSpec;

    status_t err = configureAudioConversion(ssSrc, ssDst);
    if (err != NO_ERROR) {

        ALOGE("%s: could not initialize suitable audio conversion chain (err=%d)", __FUNCTION__, err);
        return err;
    }

    // Open successful - Update current route
    mCurrentRoute = mNewRoute;
    mCurrentDevices = mNewDevices;

    return NO_ERROR;
}

status_t ALSAStreamOps::detachRoute()
{
    AutoW lock(_streamLock);
    return detachRouteL();
}

status_t ALSAStreamOps::detachRouteL()
{
    ALOGD("%s %s stream", __FUNCTION__, isOut()? "output" : "input");

    // Clear current route pointer
    mCurrentRoute = NULL;
    mCurrentDevices = 0;
    mHandle = NULL;

    return NO_ERROR;
}

status_t ALSAStreamOps::configureAudioConversion(const SampleSpec &ssSrc, const SampleSpec &ssDst)
{
    return mAudioConversion->configure(ssSrc, ssDst);
}

status_t ALSAStreamOps::getConvertedBuffer(void* dst, const uint32_t outFrames, AudioBufferProvider *pBufferProvider)
{
    return mAudioConversion->getConvertedBuffer(dst, outFrames, pBufferProvider);
}

status_t ALSAStreamOps::applyAudioConversion(const void* src, void** dst, uint32_t inFrames, uint32_t* outFrames)
{
    return mAudioConversion->convert(src, dst, inFrames, outFrames);
}

void ALSAStreamOps::setNewRoute(CAudioStreamRoute *pRoute)
{
    // No need to check Route, NULL pointer accepted
    // and considered as unrouting command
    mNewRoute = pRoute;
}

//
// Called from Route Manager Context -> WLocked
//
// reset the new route to NULL
//
void ALSAStreamOps::resetRoute()
{
    mNewRoute = NULL;
}

// No need to call from a locked context. Internal to route manager
void ALSAStreamOps::setNewDevices(uint32_t uiNewDevices)
{
    mNewDevices = uiNewDevices;
}

// No need to call from a locked context. Internal to route manager
void ALSAStreamOps::setCurrentDevices(uint32_t uiCurrentDevices)
{
    mCurrentDevices = uiCurrentDevices;
}

//
// Called from locked context
//
bool ALSAStreamOps::isStarted()
{
    AutoR lock(_streamLock);
    return !mStandby;
}

//
// Called from locked context
//
void ALSAStreamOps::setStarted(bool isStarted)
{
    _streamLock.writeLock();
    mStandby = !isStarted;
    _streamLock.unlock();

    initAudioDump();
}

void ALSAStreamOps::initAudioDump()
{
    /**
     * Read the dump properties when a new output/input stream is started.
     * False in second argument is the default value. If the property is true
     * then the dump object is created if it doesn't exist. Otherwise if it
     * is set to false, the dump object will be deleted to stop the dump.
     */
    if (TProperty<bool>(dumpBeforeConvProps[isOut()], false)) {
        if (!dumpBeforeConv) {
            LOGI("Debug: create dump object for audio before conversion");
            dumpBeforeConv = new CHALAudioDump();
        }
    }
    else if (dumpBeforeConv) {
        delete dumpBeforeConv;
        dumpBeforeConv = NULL;
    }
    if (TProperty<bool>(dumpAfterConvProps[isOut()], false)) {
        if (!dumpAfterConv) {
            LOGI("Debug: create dump object for audio after conversion");
            dumpAfterConv = new CHALAudioDump();
        }
    }
    else if (dumpAfterConv) {
        delete dumpAfterConv;
        dumpAfterConv = NULL;
    }
}

CHALAudioDump *ALSAStreamOps::getDumpObjectBeforeConv() const
{
    return dumpBeforeConv;
}

CHALAudioDump *ALSAStreamOps::getDumpObjectAfterConv() const
{
    return dumpAfterConv;
}

bool ALSAStreamOps::safeSleep(uint32_t sleepTimeUs)
{
    struct timespec tim, tim2;

    if (sleepTimeUs > MAX_SLEEP_TIME) {
        sleepTimeUs = MAX_SLEEP_TIME;
    }

    tim.tv_sec = 0;
    tim.tv_nsec = sleepTimeUs * NSEC_PER_USEC;

    return nanosleep(&tim, &tim2) > 0;
}

void ALSAStreamOps::printLPEfwDebugInfo() {

    ALOGE("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    ALOGE("^^^^^^   Print LPE firmware debug info   ^^^^^^^");
    ALOGE("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

    const std::string sstFiles[] = {"/d/sst/shim_dump",
                                    "/d/sst/sram_lpe_checkpoint",
                                    "/d/sst/sram_lpe_ia_mailbox",
                                    "/d/sst/sram_ia_lpe_mailbox",
                                    "/d/sst/fw_ssp_reg"};

    vector<std::string> debugFiles(sstFiles, sstFiles + sizeof(sstFiles) / sizeof(std::string));
    vector<std::string>::const_iterator it;

    for (it = debugFiles.begin(); it != debugFiles.end(); ++it) {
        ifstream debugStream;

        ALOGE("Opening file %s and reading it.", it->c_str());
        debugStream.open(it->c_str(), ifstream::in);

        if (debugStream.fail()) {
            ALOGE("Could not open lpe fw SST debug file, error %d", errno);
            continue;
        }

        while (debugStream.good()) {
          char dataToRead[MAX_DEBUG_STREAM_SIZE];

          debugStream.read(dataToRead, MAX_DEBUG_STREAM_SIZE);
          ALOGE("%s", dataToRead);
        }

        debugStream.close();
    }
  }


}    // namespace android
