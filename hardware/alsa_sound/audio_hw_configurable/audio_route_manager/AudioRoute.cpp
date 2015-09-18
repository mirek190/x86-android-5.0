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

#define LOG_TAG "RouteManager/Route"

#include <utils/Log.h>
#include <string>

#include "AudioRoute.h"
#include "AudioPort.h"

#include "AudioPlatformHardware.h"


namespace android_audio_legacy
{

CAudioRoute::CAudioRoute(uint32_t uiRouteIndex, CAudioPlatformState* pPlatformState) :
    _strName(CAudioPlatformHardware::getRouteName(uiRouteIndex)),
    _uiRouteId(CAudioPlatformHardware::getRouteId(uiRouteIndex)),
    _uiSlaveRoutes(CAudioPlatformHardware::getSlaveRoutes(uiRouteIndex)),
    _pPlatformState(pPlatformState)
{
    initRoute(CUtils::EInput, uiRouteIndex);
    initRoute(CUtils::EOutput, uiRouteIndex);
}

CAudioRoute::~CAudioRoute()
{

}

void CAudioRoute::initRoute(bool bIsOut, uint32_t uiRouteIndex)
{
    _applicabilityRules[bIsOut].uiDevices = CAudioPlatformHardware::getRouteApplicableDevices(uiRouteIndex, bIsOut);
    _applicabilityRules[bIsOut].uiModes = CAudioPlatformHardware::getRouteApplicableModes(uiRouteIndex, bIsOut);
    _applicabilityRules[bIsOut].uiMask = CAudioPlatformHardware::getRouteApplicableMask(uiRouteIndex, bIsOut);

    _pPort[bIsOut] = NULL;
    _stUsed[bIsOut].bCurrently =  false;
    _stUsed[bIsOut].bAfterRouting =  false;
    _needRerouting[bIsOut] = false;
}

void CAudioRoute::addPort(CAudioPort* pPort)
{
    if (pPort) {

        ALOGV("%s: %d to route %s", __FUNCTION__, pPort->getPortId(), getName().c_str());
        pPort->addRouteToPortUsers(this);
        if (!_pPort[EPortSource]) {

            _pPort[EPortSource]= pPort;
        }
        else {

            assert(!pPort[EPortDest]);
            _pPort[EPortDest] = pPort;
        }
    }
}

status_t CAudioRoute::route(bool isOut, bool isPreEnable)
{
    if (!isPreEnable) {
        /**
         * Update the status only once the enable routing stage is done.
         */
        _stUsed[isOut].bCurrently = true;
    }
    return NO_ERROR;
}

void CAudioRoute::unroute(bool isOut, bool isPostDisable)
{
    if (isPostDisable) {
        /**
         * Update the status only once the enable unrouting stage is done.
         */
        _stUsed[isOut].bCurrently = false;
    }
}

void CAudioRoute::resetAvailability()
{
    _bBlocked = false;
    _stUsed[CUtils::EOutput].bAfterRouting = false;
    _stUsed[CUtils::EInput].bAfterRouting = false;
    _needRerouting[CUtils::EOutput] = false;
    _needRerouting[CUtils::EInput] = false;
}

bool CAudioRoute::isApplicable(uint32_t uiDevices, int iMode, bool bIsOut, uint32_t) const
{
    ALOGV("%s: is Route %s applicable?", __FUNCTION__, getName().c_str());
    ALOGV("%s: \t\t\t (isBlocked()=%d willBeBusy(%s)=%d) && ", __FUNCTION__,
          isBlocked(), bIsOut? "output" : "input", willBeUsed(bIsOut));
    ALOGV("%s: \t\t\t ((1 << iMode)=0x%X & uiApplicableModes[%s]=0x%X) && ", __FUNCTION__,
          (1 << iMode), bIsOut? "output" : "input", _applicabilityRules[bIsOut].uiModes);
    ALOGV("%s: \t\t\t (uiDevices=0x%X & _uiApplicableDevices[%s]=0x%X)", __FUNCTION__,
          uiDevices, bIsOut? "output" : "input", _applicabilityRules[bIsOut].uiDevices);

    if (!isBlocked() &&
            !willBeUsed(bIsOut) &&
            isModeApplicable(iMode, bIsOut) &&
            (uiDevices & _applicabilityRules[bIsOut].uiDevices)) {

        ALOGV("%s: Route %s is applicable", __FUNCTION__, getName().c_str());
        return true;
    }
    return false;
}

bool CAudioRoute::isModeApplicable(int iMode, bool bIsOut) const
{
    return (1 << iMode) & _applicabilityRules[bIsOut].uiModes;
}

void CAudioRoute::setUsed(bool bIsOut)
{
    if (_stUsed[bIsOut].bAfterRouting) {

        // Route is already in use in in/out, associated port already in use
        // Bailing out
        return ;
    }

    ALOGV("%s: route %s is now in use in %s", __FUNCTION__, getName().c_str(), bIsOut? "PLAYBACK" : "CAPTURE");

    _stUsed[bIsOut].bAfterRouting = true;

    // Propagate the in use attribute to the ports
    // used by this route
    for (int i = 0; i < ENbPorts ; i++) {

        if (_pPort[i]) {

            _pPort[i]->setUsed(this);
        }
    }
}

void CAudioRoute::setNeedRerouting(bool needRerouting, bool isOut)
{
    _needRerouting[isOut] = needRerouting;
}

bool CAudioRoute::needReconfiguration(bool bIsOut) const
{
    //
    // Base class just check at least that the route is currently used
    // and will remain used after routing
    //
    return currentlyUsed(bIsOut) && willBeUsed(bIsOut);
}

bool CAudioRoute::currentlyUsed(bool bIsOut) const
{
    return _stUsed[bIsOut].bCurrently;
}

bool CAudioRoute::willBeUsed(bool bIsOut) const
{
    return _stUsed[bIsOut].bAfterRouting;
}

void CAudioRoute::setBlocked()
{
    if (_bBlocked) {

        // Bailing out
        return ;
    }

    ALOGV("%s: route %s is now blocked", __FUNCTION__, getName().c_str());
    _bBlocked = true;
}

bool CAudioRoute::isBlocked() const
{
    return _bBlocked;
}

}       // namespace android
