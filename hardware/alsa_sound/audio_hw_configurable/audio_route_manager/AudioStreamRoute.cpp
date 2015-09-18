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

#define LOG_TAG "RouteManager/StreamRoute"

#include "AudioPlatformHardware.h"
#include "AudioStreamRoute.h"
#include "AudioUtils.h"
#include "ALSAStreamOps.h"
#include <tinyalsa/asoundlib.h>
#include <utils/Log.h>
#include <algorithm>

#define base    CAudioRoute

namespace android_audio_legacy
{

CAudioStreamRoute::CAudioStreamRoute(uint32_t uiRouteIndex,
                                     CAudioPlatformState *platformState) :
    CAudioRoute(uiRouteIndex, platformState),
    _pEffectSupported(0),
    _pcCardName(CAudioPlatformHardware::getRouteCardName(uiRouteIndex))
{
    for (int iDir = 0; iDir < CUtils::ENbDirections; iDir++) {

       _stStreams[iDir].pCurrent = NULL;
       _stStreams[iDir].pNew = NULL;
       _astPcmDevice[iDir] = NULL;
       _aiPcmDeviceId[iDir] = CAudioPlatformHardware::getRouteDeviceId(uiRouteIndex, iDir);
       _astPcmConfig[iDir] = CAudioPlatformHardware::getRoutePcmConfig(uiRouteIndex, iDir);

       _routeSampleSpec[iDir].setFormat(
                   AudioUtils::convertTinyToHalFormat(_astPcmConfig[iDir].format));
       _routeSampleSpec[iDir].setSampleRate(_astPcmConfig[iDir].rate);
       _routeSampleSpec[iDir].setChannelCount(_astPcmConfig[iDir].channels);
       _routeSampleSpec[iDir].setChannelsPolicy(CAudioPlatformHardware::getChannelsPolicy(uiRouteIndex, iDir));
    }
}

//
// Basic identical conditions fonction
// To be implemented by derivated classes if different
// route policy
//
bool CAudioStreamRoute::needReconfiguration(bool bIsOut) const
{
    // TBD: what conditions will lead to set the need reconfiguration flag for this route???
    // The route needs reconfiguration except if:
    //      - still used by the same stream
    //      - the stream is using the same device
    if (base::needReconfiguration(bIsOut) &&
             (needRerouting(bIsOut) ||
             (_stStreams[bIsOut].pCurrent != _stStreams[bIsOut].pNew) ||
             (_stStreams[bIsOut].pCurrent->getCurrentDevices() != _stStreams[bIsOut].pNew->getNewDevices()))) {
        return true;
    }
    return false;
}

status_t CAudioStreamRoute::route(bool isOut, bool isPreEnable)
{
    if (isPreEnable == isPreEnableRequired()) {

        status_t err = openPcmDevice(isOut);
        if (err != NO_ERROR) {

            // Failed to open PCM device -> bailing out
            return err;
        }
    }
    if (!isPreEnable) {

        if (_astPcmDevice[isOut] == NULL) {

            ALOGE("%s: audio device not found, cannot route the stream", __FUNCTION__);
            return NO_INIT;
        }
        /**
         * Attach the stream to its route only once routing stage is completed
         * to let the audio-parameter-manager performing the required configuration of the
         * audio path.
         */
        status_t err = attachNewStream(isOut);
        if (err) {

            // Failed to attach the stream -> bailing out
            return err;
        }
    }
    CAudioRoute::route(isOut, isPreEnable);
    return OK;
}

void CAudioStreamRoute::unroute(bool isOut, bool isPostDisable)
{
    if (!isPostDisable) {

        /**
         * Detach the stream from its route at the beginning of unrouting stage.
         * Action of audio-parameter-manager on the audio path may lead to blocking issue, so
         * need to garantee that the stream will not access to the device before unrouting.
         */
        detachCurrentStream(isOut);
    }
    if (isPostDisable == isPostDisableRequired()) {

        closePcmDevice(isOut);
    }
    CAudioRoute::unroute(isOut, isPostDisable);
}

void CAudioStreamRoute::configure(bool bIsOut)
{
    // Same stream is attached to this route, consumme the new device
    if (_stStreams[bIsOut].pCurrent == _stStreams[bIsOut].pNew) {

        // Consume the new device(s)
        _stStreams[bIsOut].pCurrent->setCurrentDevices(_stStreams[bIsOut].pCurrent->getNewDevices());
    } else {

        // Route is still in use, but the stream attached to this route has changed...
        // Unroute previous stream
        detachCurrentStream(bIsOut);

        // route new stream
        attachNewStream(bIsOut);
    }
}

void CAudioStreamRoute::resetAvailability()
{
    for (int iDir = 0; iDir < CUtils::ENbDirections; iDir++) {

        if (_stStreams[iDir].pNew) {

            _stStreams[iDir].pNew->resetRoute();
            _stStreams[iDir].pNew = NULL;
        }
    }
    base::resetAvailability();
}

status_t CAudioStreamRoute::setStream(ALSAStreamOps* pStream)
{
    ALOGV("%s to %s route", __FUNCTION__, getName().c_str());
    bool bIsOut = pStream->isOut();

    assert(!_stStreams[bIsOut].pNew);

    _stStreams[bIsOut].pNew = pStream;

    _stStreams[bIsOut].pNew->setNewRoute(this);

    return NO_ERROR;
}

bool CAudioStreamRoute::isApplicable(uint32_t uiDevices, int iMode, bool bIsOut, uint32_t uiMask) const
{
    ALOGV("%s: is Route %s applicable? ",__FUNCTION__, getName().c_str());
    ALOGV("%s: \t\t\t bIsOut=%s && uiMask=0x%X & _uiApplicableMask[%s]=0x%X", __FUNCTION__,
          bIsOut? "output" : "input",
          uiMask,
          bIsOut ? "output" : "input",
          _applicabilityRules[bIsOut].uiMask);

    if ((uiMask & _applicabilityRules[bIsOut].uiMask) == 0) {

        return false;
    }
    // Base class does not have much work to do than checking
    // if no stream is already using it and if not condemened
    return base::isApplicable(uiDevices, iMode, bIsOut);
}

bool CAudioStreamRoute::available(bool bIsOut)
{
    // A route is available if no stream is already using it and if not condemened
    return !isBlocked() && !_stStreams[bIsOut].pNew;
}

int CAudioStreamRoute::getPcmDeviceId(bool bIsOut) const
{
    return _aiPcmDeviceId[bIsOut];
}

pcm* CAudioStreamRoute::getPcmDevice(bool bIsOut) const
{
    LOG_ALWAYS_FATAL_IF(_astPcmDevice[bIsOut] == NULL);

    return _astPcmDevice[bIsOut];

}

const pcm_config& CAudioStreamRoute::getPcmConfig(bool bIsOut) const
{
    return _astPcmConfig[bIsOut];
}

const char* CAudioStreamRoute::getCardName() const
{
    return _pcCardName;
}

status_t CAudioStreamRoute::attachNewStream(bool bIsOut)
{
    LOG_ALWAYS_FATAL_IF(_stStreams[bIsOut].pNew == NULL);

    status_t err = _stStreams[bIsOut].pNew->attachRoute();

    if (err != NO_ERROR) {

        // Failed to open output stream -> bailing out
        return err;
    }
    _stStreams[bIsOut].pCurrent = _stStreams[bIsOut].pNew;

    return NO_ERROR;
}

void CAudioStreamRoute::detachCurrentStream(bool bIsOut)
{
    LOG_ALWAYS_FATAL_IF(_stStreams[bIsOut].pCurrent == NULL);

    _stStreams[bIsOut].pCurrent->detachRoute();
    _stStreams[bIsOut].pCurrent = NULL;
}

bool CAudioStreamRoute::isEffectSupported(const effect_uuid_t* uuid) const
{
    std::list<const effect_uuid_t*>::const_iterator it;
    it = std::find_if(_pEffectSupported.begin(), _pEffectSupported.end(),
                      std::bind2nd(hasEffect(), uuid));

    return it != _pEffectSupported.end();
}

status_t CAudioStreamRoute::openPcmDevice(bool bIsOut)
{
    LOG_ALWAYS_FATAL_IF(_astPcmDevice[bIsOut] != NULL);

    pcm_config config = getPcmConfig(bIsOut);
    ALOGD("%s called for card (%s,%d)",
                                __FUNCTION__,
                                getCardName(),
                                getPcmDeviceId(bIsOut));
    ALOGD("%s\t\t config=rate(%d), format(%d), channels(%d))",
                                __FUNCTION__,
                                config.rate,
                                config.format,
                                config.channels);
    ALOGD("%s\t\t period_size=%d, period_count=%d",
                                __FUNCTION__,
                                config.period_size,
                                config.period_count);
    ALOGD("%s\t\t startTh=%d, stop Th=%d silence Th=%d",
                                __FUNCTION__,
                                config.start_threshold,
                                config.stop_threshold,
                                config.silence_threshold);

    //
    // Opens the device in BLOCKING mode (default)
    // No need to check for NULL handle, tiny alsa
    // guarantee to return a pcm structure, even when failing to open
    // it will return a reference on a "bad pcm" structure
    //
    uint32_t uiFlags= (bIsOut ? PCM_OUT : PCM_IN);
    _astPcmDevice[bIsOut] = pcm_open(AudioUtils::getCardIndexByName(getCardName()),
                                     getPcmDeviceId(bIsOut), uiFlags, &config);
    if (_astPcmDevice[bIsOut] && !pcm_is_ready(_astPcmDevice[bIsOut])) {

        ALOGE("%s: Cannot open tinyalsa (%s,%d) device for %s stream (error=%s)", __FUNCTION__,
              getCardName(),
              getPcmDeviceId(bIsOut),
              bIsOut? "output" : "input",
              pcm_get_error(_astPcmDevice[bIsOut]));
        goto close_device;
    }

    // Prepare the device (ie allocation of the stream)
    if (pcm_prepare(_astPcmDevice[bIsOut]) != 0) {

        ALOGE("%s: prepare failed with error %s", __FUNCTION__,
                                        pcm_get_error(_astPcmDevice[bIsOut]));
        goto close_device;
    }

    ALOGW_IF((getPcmConfig(bIsOut).period_count * getPcmConfig(bIsOut).period_size) !=
            (pcm_get_buffer_size(_astPcmDevice[bIsOut])),
             "%s, refine done by alsa, ALSA RingBuffer = %d (frames), "
             "expected by AudioHAL and AudioFlinger = %d (frames)",
             __FUNCTION__, pcm_get_buffer_size(_astPcmDevice[bIsOut]),
             getPcmConfig(bIsOut).period_count * getPcmConfig(bIsOut).period_size);

    return NO_ERROR;

close_device:

    pcm_close(_astPcmDevice[bIsOut]);
    _astPcmDevice[bIsOut] = NULL;
    return NO_MEMORY;
}

void CAudioStreamRoute::closePcmDevice(bool bIsOut)
{
    LOG_ALWAYS_FATAL_IF(_astPcmDevice[bIsOut] == NULL);

    ALOGD("%s called for card (%s,%d)",
                                __FUNCTION__,
                                getCardName(),
                                getPcmDeviceId(bIsOut));
    pcm_close(_astPcmDevice[bIsOut]);
    _astPcmDevice[bIsOut] = NULL;
}

}       // namespace android
