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

#include <string>
#include "Utils.h"

#include <hardware_legacy/AudioHardwareBase.h>

namespace android_audio_legacy
{
class ALSAStreamOps;
class CAudioPort;
class CAudioPlatformState;

class CAudioRoute
{

public:

    enum Port {

        EPortSource = 0,
        EPortDest,

        ENbPorts
    };

    enum RouteType {
        EStreamRoute,
        EExternalRoute,
        ECompressedStreamRoute,

        ENbRouteType
    };

    CAudioRoute(uint32_t uiRouteIndex, CAudioPlatformState *pPlatformState);
    virtual           ~CAudioRoute();

    void addPort(CAudioPort* pPort);

    virtual bool isApplicable(uint32_t uidevices, int iMode, bool bIsOut, uint32_t uiMask = 0) const;

    virtual void resetAvailability();

    virtual void setUsed(bool bIsOut);

    virtual bool currentlyUsed(bool bIsOut) const;

    virtual bool willBeUsed(bool bIsOut) const;

    // From Port
    void setBlocked();

    bool isBlocked() const;

    const std::string& getName() const { return _strName; }

    uint32_t getSlaveRoutes() const { return _uiSlaveRoutes; }

    virtual RouteType getRouteType() const = 0;

    /**
     * Route is called during enable routing stage.
     * It is called twice: before setting the audio path and after setting the audio path
     * in order to give flexibility to the route objects.
     *
     * @param[in] isOut direction of the audio route
     * @param[in] isPreEnable flag is true if called before setting the audio path, false after.
     *
     * @return OK if successfull operation, error code otherwise.
     */
    virtual status_t route(bool isOut, bool isPreEnable);

    /**
     * Unroute is called during disable routing stage.
     * It is called twice: before reseting the audio path and after reseting the audio path
     * in order to give flexibility to the route objects.
     *
     * @param[in] isOut direction of the audio route
     * @param[in] isPostDisable flag is true if called after reseting the audio path, false before.
     *
     * @return OK if successfull operation, error code otherwise.
     */
    virtual void unroute(bool isOut, bool isPostDisable);

    virtual void configure(bool __UNUSED bIsOut) { return ; }

    uint32_t getRouteId() const { return _uiRouteId; }

    // Filters the unroute/route
    // Returns true if a route is currently used, will be used
    // after reconsiderRouting but needs to be reconfigured
    virtual bool needReconfiguration(bool bIsOut) const;

    virtual bool isPreEnableRequired() { return false; }

    virtual bool isPostDisableRequired() { return false; }

    /**
     * Expressed conditions in which the route will be closed/opened again.
     * If rerouting is requested, it will be done at the configure step.
     *
     * @param[in] isOut: direction of the route.
     *
     * @return true if route need to be closed/opened, false otherwise.
     */
    virtual bool needRerouting(bool isOut) const { return _needRerouting[isOut]; }

    /**
     * Evaluate if a route needs to be rerouted.
     * If rerouting flag is raised, the route will unrouted / routed again.
     *
     * @param[in] needRerouting: set if route needs to be rerouted.
     * @param[in] isOut: direction of the stream.
     */
    virtual void checkAndSetNeedRerouting(bool __UNUSED isOut) { }

    /**
     * Sets a route to be rerouted during routing processus.
     * If rerouting flag is raised, the route will unrouted / routed again.
     *
     * @param[in] needRerouting: set if route needs to be rerouted.
     * @param[in] isOut: direction of the stream.
     */
    void setNeedRerouting(bool needRerouting, bool isOut);

protected:
    CAudioRoute(const CAudioRoute &);
    CAudioRoute& operator = (const CAudioRoute &);

private:
    void initRoute(bool bIsOut, uint32_t uiRouteIndex);

    bool isModeApplicable(int iMode, bool bIsOut) const;

    std::string _strName;

    // A route is connected to 2 port, one call be NULL if no mutual exclusion exists on this port
    CAudioPort* _pPort[ENbPorts];

    bool _bBlocked;

    uint32_t _uiRouteId;

protected:
    struct {

        bool bCurrently;
        bool bAfterRouting;
    } _stUsed[CUtils::ENbDirections];

public:
    struct {

        uint32_t uiDevices;

        uint32_t uiModes;

        uint32_t uiMask;

        uint32_t uiType;

    } _applicabilityRules[CUtils::ENbDirections];

    uint32_t _uiSlaveRoutes;

    CAudioPlatformState* _pPlatformState;

    /**
     * Used to track down the need to reinitialize the
     * audio route, for both directions.
     */
    bool _needRerouting[CUtils::ENbDirections];
};

};        // namespace android

