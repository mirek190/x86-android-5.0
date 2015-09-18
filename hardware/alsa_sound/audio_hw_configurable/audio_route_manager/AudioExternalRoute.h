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

#include "AudioRoute.h"

namespace android_audio_legacy
{

class CAudioExternalRoute : public CAudioRoute
{
public:

    CAudioExternalRoute(int iRouteIndex, CAudioPlatformState* platformState) : CAudioRoute(iRouteIndex, platformState) {}

    virtual RouteType getRouteType() const { return CAudioRoute::EExternalRoute; }

    /**
     * Filters the unroute/route
     * External route class provides a basic rectricted needReconfiguration implementation
     * that may be overriden to cover specific needs.
     * A route is set to be reconfigured if all these conditions are true:
     *      -it is currently used
     *      -it will be used after reconsider routing
     *      -output or input devices changed (matching the direction of the route)
     *      -input source changed (input direction only).
     *
     * @param[in] isOut: direction of the audio external route.
     *
     * @return true if route needs to be reconfigured, false otherwise.
     */
    virtual bool needReconfiguration(bool isOut) const;
};
};        // namespace android

