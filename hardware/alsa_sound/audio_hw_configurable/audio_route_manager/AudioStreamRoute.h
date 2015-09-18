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

#include <tinyalsa/asoundlib.h>

#include "AudioRoute.h"
#include "SampleSpec.h"

#include <list>

namespace android_audio_legacy
{

class ALSAStreamOps;

class CAudioStreamRoute : public CAudioRoute
{
public:
    CAudioStreamRoute(uint32_t uiRouteIndex,
                      CAudioPlatformState* platformState);

    pcm* getPcmDevice(bool bIsOut) const;

    const SampleSpec getSampleSpec(bool bIsOut) const { return _routeSampleSpec[bIsOut]; }

    virtual RouteType getRouteType() const { return CAudioRoute::EStreamRoute; }

    // Assign a new stream to this route
    status_t setStream(ALSAStreamOps* pStream);

    /**
     * Route is called during enable routing stage.
     * It is called twice: before setting the audio path and after setting the audio path
     * in order to give flexibility to the stream route objects to opens the device at the right
     * step.
     *
     * @param[in] isOut direction of the audio stream route
     * @param[in] isPreEnable flag is true if called before setting the audio path, false after.
     *
     * @return OK if successfull operation, error code otherwise.
     */
    virtual status_t route(bool isOut, bool isPreEnable);

    /**
     * unroute is called during disable routing stage.
     * It is called twice: before reseting the audio path and after reseting the audio path
     * in order to give flexibility to the stream route objects to close the device at the right
     * step.
     *
     * @param[in] isOut direction of the audio stream route
     * @param[in] isPostDisable flag is true if called after reseting the audio path, false before.
     *
     * @return OK if successfull operation, error code otherwise.
     */
    virtual void unroute(bool isOut, bool isPostDisable);

    // Configure order
    virtual void configure(bool bIsOut);

    // Inherited for AudioRoute - called from RouteManager
    virtual void resetAvailability();

    // Inherited from AudioRoute - Called from AudioRouteManager
    virtual bool available(bool bIsOut);

    // Inherited for AudioRoute - called from RouteManager
    virtual bool isApplicable(uint32_t uidevices, int iMode, bool bIsOut, uint32_t uiMask = 0) const;

    // Filters the unroute/route
    virtual bool needReconfiguration(bool bIsOut) const;

    // Get amount of silence delay upon stream opening
    virtual uint32_t getOutputSilencePrologMs() const { return 0; }

    virtual bool isEffectSupported(const effect_uuid_t* uuid) const;

protected:
    struct {

        ALSAStreamOps* pCurrent;
        ALSAStreamOps* pNew;
    } _stStreams[CUtils::ENbDirections];

    std::list<const effect_uuid_t*> _pEffectSupported;

private:
    // Function to be used as the predicate in find_if call.
    struct hasEffect :
            public std::binary_function<const effect_uuid_t*, const effect_uuid_t*, bool> {

        bool operator()(const effect_uuid_t* uuid1,
                           const effect_uuid_t* uuid2) const {

            return memcmp(uuid2, uuid1, sizeof(*uuid2)) == 0;
        }
    };

    int getPcmDeviceId(bool bIsOut) const;

    const pcm_config& getPcmConfig(bool bIsOut) const;

    const char* getCardName() const;

    android::status_t openPcmDevice(bool bIsOut);

    void closePcmDevice(bool bIsOut);

    android::status_t attachNewStream(bool bIsOut);

    void detachCurrentStream(bool bIsOut);

    const char* _pcCardName;
    int _aiPcmDeviceId[CUtils::ENbDirections];
    pcm_config _astPcmConfig[CUtils::ENbDirections];

    pcm* _astPcmDevice[CUtils::ENbDirections];

    SampleSpec _routeSampleSpec[CUtils::ENbDirections];

};
};        // namespace android

