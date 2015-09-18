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

#include "AudioStreamRoute.h"

namespace android_audio_legacy
{

/**
 * Class that factorizes common routes behavior.
 *
 * This class represents the SSP FIFO flushing workaroud
 * that avoids a problem of audio quality when doing CSV
 * or voip call + mixing, or even VOIP + audio capture.
 *
 * This problem is present on all platforms.
 */
class AudioStreamRouteIaSspWorkaround : public CAudioStreamRoute
{
public:
    AudioStreamRouteIaSspWorkaround(uint32_t routeIndex, CAudioPlatformState *platformState) :
        CAudioStreamRoute(routeIndex, platformState) { }

    virtual void checkAndSetNeedRerouting(bool isOut);
};

} // namespace android
