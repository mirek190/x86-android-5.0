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

class CAudioCompressedStreamRoute : public CAudioRoute
{
public:
    CAudioCompressedStreamRoute(int iRouteIndex, CAudioPlatformState* platformState) : CAudioRoute(iRouteIndex, platformState) {}

    virtual RouteType getRouteType() const { return CAudioRoute::ECompressedStreamRoute; }

    virtual bool isApplicable(uint32_t uidevices, int iMode, bool bIsOut, uint32_t uiMask = 0) const;

    // Filters the unroute/route
    virtual bool needReconfiguration(bool bIsOut) const;
};
};        // namespace android

