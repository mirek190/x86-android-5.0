/*
 * Copyright (C) 2012,2013,2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BATTERYSTATUS_H_
#define _BATTERYSTATUS_H_


namespace android {
namespace camera2 {
/*********************************************************************
 * enum Battery Status
 */
enum BatteryStatus {
    BATTERY_STATUS_INVALID = -1,
    BATTERY_STATUS_NORMAL,
    BATTERY_STATUS_WARNING,
    BATTERY_STATUS_ALERT,
    BATTERY_STATUS_CRITICAL
};


/**
 * Return Battery Status which got from camera.flash.throttling_levels property.
 */
int getBatteryStatus(void);

}
}

#endif // _BATTERYSTATUS_H_
