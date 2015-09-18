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

#pragma once

#include <list>
#include <stdint.h>
#include <string>

namespace android_audio_legacy
{
class CAudioPortGroup;
class CAudioRoute;

class CAudioPort
{
    typedef std::list<CAudioPortGroup*>::iterator PortGroupListIterator;
    typedef std::list<CAudioPortGroup*>::const_iterator PortGroupListConstIterator;
    typedef std::list<CAudioRoute*>::iterator RouteListIterator;
    typedef std::list<CAudioRoute*>::const_iterator RouteListConstIterator;

public:
    CAudioPort(uint32_t uiPortIndex);
    virtual           ~CAudioPort();

    // From PortGroup
    void setBlocked(bool bBlocked);

    // From Route: this port is now used
    void setUsed(CAudioRoute* pRoute);

    void resetAvailability();

    const std::string& getName() const { return _strName; }

    uint32_t getPortId() const { return _uiPortId; }

    // Add a Route to the list of route that use this port
    void addRouteToPortUsers(CAudioRoute* pRoute);

    // From Group Port
    void addGroupToPort(CAudioPortGroup* portGroup);

private:
    CAudioPort(const CAudioPort &);
    CAudioPort& operator = (const CAudioPort &);

    std::string _strName;

    uint32_t _uiPortId;

    // list of Port groups to which this port belongs - can be null if this port does not have
    // any mutual exclusion issue
    std::list<CAudioPortGroup*> _portGroupList;

    CAudioRoute* _pRouteAttached;

    std::list<CAudioRoute*> mRouteList;

    bool _bBlocked;
    bool _bUsed;
};

};        // namespace android

