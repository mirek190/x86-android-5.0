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

#include <tinyalsa/asoundlib.h>
#include <hardware_legacy/AudioHardwareBase.h>
#include "AudioPlatformState.h"
#include "AudioCompressedStreamRoute.h"
#include "AudioExternalRoute.h"
#include "AudioRoute.h"
#include "AudioRouteManager.h"
#include "AudioStreamRouteIaSspWorkaround.h"


namespace android_audio_legacy
{

void AudioStreamRouteIaSspWorkaround::checkAndSetNeedRerouting(bool isOut)
{
   /**
    * Conditions to be met in order to proceed to SSP FIFO flushing
    * (rerouting) when considering reconfiguration of an input stream:
    *
    * 1) The stream was active before rerouting and will remain active after
    *    rerouting.
    * 2) Output device has changed
    * 3) Changed output device is earpiece or speaker
    *
    * Outside these conditions, no rerouting can take place.
    *
    * Conditions to be met in order to proceed to SSP FIFO flushing (rerouting)
    * when considering reconfiguration of an output stream:
    *
    * 1) The stream was active before rerouting and will remain active after
    *    rerouting.
    * 2) Output device has changed
    * 3) Changed output device is earpiece or headset
    *
    * Outside these conditions, no rerouting can take place.
    */
    bool needRerouting = CAudioRoute::needReconfiguration(isOut) &&
        _pPlatformState->hasPlatformStateChanged(CAudioPlatformState::EOutputDevicesChange) &&
        (isOut ? (_pPlatformState->getDevices(CUtils::EOutput) &
        (AudioSystem::DEVICE_OUT_EARPIECE | AudioSystem::DEVICE_OUT_WIRED_HEADSET)) :
        (_pPlatformState->getDevices(CUtils::EOutput) &
        (AudioSystem::DEVICE_OUT_EARPIECE | AudioSystem::DEVICE_OUT_SPEAKER)));

    CAudioStreamRoute::setNeedRerouting(needRerouting, isOut);
}

} // namespace android
