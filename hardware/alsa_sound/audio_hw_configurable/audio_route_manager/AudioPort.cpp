/*
 ** Copyright 2011 Intel Corporation
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

#define LOG_TAG "RouteManager/Port"

#include "AudioPort.h"
#include "AudioPortGroup.h"
#include "AudioRoute.h"

#include "AudioPlatformHardware.h"


namespace android_audio_legacy
{

CAudioPort::CAudioPort(uint32_t uiPortIndex) :
    _strName(CAudioPlatformHardware::getPortName(uiPortIndex)),
    _uiPortId(CAudioPlatformHardware::getPortId(uiPortIndex)),
    _portGroupList(0),
    _pRouteAttached(0),
    _bBlocked(false),
    _bUsed(false)
{
}

CAudioPort::~CAudioPort()
{

}

void CAudioPort::resetAvailability()
{
    _bUsed = false;
    _bBlocked = false;
    _pRouteAttached = NULL;
}

void CAudioPort::addGroupToPort(CAudioPortGroup* portGroup)
{
    ALOGV("%s", __FUNCTION__);
    if(!portGroup) {

        return ;
    }

    _portGroupList.push_back(portGroup);
}

void CAudioPort::setBlocked(bool bBlocked)
{
    if (_bBlocked == bBlocked) {

        return ;
    }

    ALOGV("%s: port %s is blocked", __FUNCTION__, getName().c_str());

    _bBlocked = bBlocked;

    if (bBlocked) {

        assert(!_pRouteAttached);

        // Blocks now all route that use this port except the one that uses the port
        RouteListIterator it;

        // Find the applicable route for this route request
        for (it = mRouteList.begin(); it != mRouteList.end(); ++it) {

            CAudioRoute* pRoute = *it;
            ALOGV("%s:  blocking %s", __FUNCTION__, pRoute->getName().c_str());
            pRoute->setBlocked();
        }
    }
}

// Set the used attribute of the port
// Parse the list of Group Port in which this port is involved
// And condemns all port within these port group that are
// mutual exclusive with this one.
void CAudioPort::setUsed(CAudioRoute *pRoute)
{

    if (_bUsed) {

        // Port is already in use, bailing out
        return ;
    }

    ALOGV("%s: port %s is in use", __FUNCTION__, getName().c_str());

    _bUsed = true;
    _pRouteAttached = pRoute;

    PortGroupListIterator it;

    for (it = _portGroupList.begin(); it != _portGroupList.end(); ++it) {

        CAudioPortGroup* aPortGroup = *it;

        aPortGroup->condemnMutualExclusivePort(this);
    }
}

// This function add the route to this list of routes
// that use this port
void CAudioPort::addRouteToPortUsers(CAudioRoute* pRoute)
{
    assert(pRoute);

    mRouteList.push_back(pRoute);

    ALOGV("%s: added %s route to %s port users", __FUNCTION__, pRoute->getName().c_str(), getName().c_str());
}

}       // namespace android
