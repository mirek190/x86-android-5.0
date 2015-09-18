/* AudioHardwareALSA.cpp
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
#include <string>
#include <cutils/properties.h>

#define LOG_TAG "AudioHardwareALSA"

#include <utils/Log.h>
#include <utils/String8.h>

#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>
#include "Property.h"
#include "ParameterMgrPlatformConnector.h"
#include "SelectionCriterionInterface.h"
#include "AudioHardwareALSA.h"
#include "AudioStreamInALSA.h"
#include "AudioStreamOutALSA.h"
#include "ALSAStreamOps.h"
#include "Utils.h"

#include <AudioCommsAssert.hpp>
#include <hardware/audio_effect.h>
#include <audio_utils/echo_reference.h>
#include <audio_effects/effect_aec.h>

#include "AudioRouteManager.h"
#include "AudioConversion.h"

#define DEFAULTGAIN "1.0"

using namespace std;
using namespace android;

namespace android_audio_legacy
{
extern "C"
{
//
// Function for dlsym() to look up for creating a new AudioHardwareInterface.
//
AudioHardwareInterface *createAudioHardware(void) {
    return AudioHardwareALSA::create();
}
}         // extern "C"

// Default sampling rate in case the value is not found in xml file
const uint32_t AudioHardwareALSA::DEFAULT_SAMPLE_RATE = 48000;
const uint32_t AudioHardwareALSA::DEFAULT_CHANNEL_COUNT = 2;
const uint32_t AudioHardwareALSA::DEFAULT_FORMAT = AUDIO_FORMAT_PCM_16_BIT;

AudioHardwareInterface *AudioHardwareALSA::create() {

    ALOGD("Using Audio HAL Configurable");

    return new AudioHardwareALSA();
}

AudioHardwareALSA::AudioHardwareALSA() :
    mRouteMgr(new CAudioRouteManager(this))
{
    // Start the route manager service
    if (mRouteMgr->start() != NO_ERROR) {

        ALOGE("%s: could not start route manager, NO ROUTING AVAILABLE", __FUNCTION__);
    }
}

AudioHardwareALSA::~AudioHardwareALSA()
{
    // Delete route manager, it will detroy all the registered routes
    delete mRouteMgr;
}

status_t AudioHardwareALSA::initCheck()
{

    if (mRouteMgr->isStarted())

        return NO_ERROR;
    else
        return NO_INIT;
}

status_t AudioHardwareALSA::setVoiceVolume(float volume)
{
    return mRouteMgr->setVoiceVolume(volume);
}

status_t AudioHardwareALSA::setMasterVolume(float __UNUSED volume)
{
    ALOGW("%s: missing implementation", __FUNCTION__);
    return NO_ERROR;
}

AudioStreamOut* AudioHardwareALSA::openOutputStream(uint32_t devices,
                                    int *format,
                                    uint32_t *channels,
                                    uint32_t *sampleRate,
                                    status_t *status)
{
    ALOGD("%s: called for devices: 0x%08x", __FUNCTION__, devices);

    LOG_ALWAYS_FATAL_IF(status == NULL);

    status_t &err = *status;
    /* output flags is passed through status parameter */
    audio_output_flags_t flags = static_cast<audio_output_flags_t>(*status);

    if (!audio_is_output_device(devices)) {

        ALOGD("%s: called with bad devices", __FUNCTION__);
        err = BAD_VALUE;
        return NULL;
    }

    AudioStreamOutALSA* out = new AudioStreamOutALSA(this, flags);

    err = out->set(format, channels, sampleRate);
    if (err != NO_ERROR) {

        ALOGE("%s: set error.", __FUNCTION__);
        delete out;
        return NULL;
    }

    // Informs the route manager of stream creation
    mRouteMgr->addStream(out);

    ALOGD("%s: output created with status=%d", __FUNCTION__, err);
    return out;
}

void AudioHardwareALSA::closeOutputStream(AudioStreamOut* out)
{
    // Informs the route manager of stream destruction
    mRouteMgr->removeStream((AudioStreamOutALSA* )out);
    delete out;
}

AudioStreamIn* AudioHardwareALSA::openInputStream(uint32_t devices,
                                   int *format,
                                   uint32_t *channels,
                                   uint32_t *sampleRate,
                                   status_t *status,
                                   AudioSystem::audio_in_acoustics acoustics)
{
    ALOGD("%s: IN", __FUNCTION__);

    LOG_ALWAYS_FATAL_IF(status == NULL);

    status_t &err = *status;

    if (!AudioUtils::isAudioInputDevice(devices)) {
        err = BAD_VALUE;
        return NULL;
    }

    AudioStreamInALSA* in = new AudioStreamInALSA(this, acoustics);

    err = in->set(format, channels, sampleRate);
    if (err != NO_ERROR) {

        ALOGE("%s: Set err", __FUNCTION__);
        delete in;
        return NULL;
    }
    // Informs the route manager of stream creation
    mRouteMgr->addStream(in);

    ALOGD("%s: OUT status=%d", __FUNCTION__, err);
    return in;
}

void AudioHardwareALSA::closeInputStream(AudioStreamIn* in)
{
    // Informs the route manager of stream destruction
    mRouteMgr->removeStream((AudioStreamInALSA* )in);

    delete in;
}

status_t AudioHardwareALSA::setMicMute(bool state)
{
    ALOGD("Set MicMute to %s state", state? "true" : "false");
    mRouteMgr->setMicMute(state);
    return NO_ERROR;
}

status_t AudioHardwareALSA::getMicMute(bool *state)
{
    *state = mRouteMgr->getMicMute();
    return NO_ERROR;
}

size_t AudioHardwareALSA::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{
    switch (sampleRate) {
    case 8000:
    case 11025:
    case 12000:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        break;
    default:
        ALOGW("getInputBufferSize bad sampling rate: %d", sampleRate);
        return 0;
    }
    if (format != AudioSystem::PCM_16_BIT) {
        ALOGW("getInputBufferSize bad format: %d", format);
        return 0;
    }
    if ((channelCount < 1) || (channelCount > 2)) {
        ALOGW("getInputBufferSize bad channel count: %d", channelCount);
        return 0;
    }

    // Returns 20ms buffer size per channel
    return (sampleRate / 25) * channelCount;
}

status_t AudioHardwareALSA::dump(int __UNUSED fd, const Vector<String16> __UNUSED &args)
{
    return NO_ERROR;
}

status_t AudioHardwareALSA::setParameters(const String8& keyValuePairs)
{
    ALOGD("%s", __FUNCTION__);
    return mRouteMgr->setParameters(keyValuePairs);
}

String8 AudioHardwareALSA::getParameters(const String8& keyValuePairs)
{
    ALOGD("%s", __FUNCTION__);
    return mRouteMgr->getParameters(keyValuePairs);
}

//
// Direct Output Stream to be identified by new KeyValuePair
//
// set Stream Parameters
status_t AudioHardwareALSA::setStreamParameters(ALSAStreamOps* pStream, const String8& keyValuePairs)
{
    ALOGD("%s: key value pair %s", __FUNCTION__, keyValuePairs.string());
    return mRouteMgr->setStreamParameters(pStream, keyValuePairs, mode());
}

const pcm_config& AudioHardwareALSA::getDefaultPcmConfig(bool bIsOut, uint32_t uiFlags) const
{
    return mRouteMgr->getDefaultPcmConfig(bIsOut, uiFlags);
}

status_t AudioHardwareALSA::startStream(ALSAStreamOps *stream)
{
    AUDIOCOMMS_ASSERT(stream != NULL, "requesting to start NULL pointer stream");
    return mRouteMgr->startStream(stream);
}

status_t AudioHardwareALSA::stopStream(ALSAStreamOps *stream)
{
    AUDIOCOMMS_ASSERT(stream != NULL, "requesting to stop NULL pointer stream");
    return mRouteMgr->stopStream(stream);
}

void AudioHardwareALSA::resetEchoReference(struct echo_reference_itfe* reference)
{
    mRouteMgr->resetEchoReference(reference);
}

struct echo_reference_itfe* AudioHardwareALSA::getEchoReference(int format,
                                                                uint32_t channel_count,
                                                                uint32_t sampling_rate)
{
    return mRouteMgr->getEchoReference(format, channel_count, sampling_rate);
}


}       // namespace android
