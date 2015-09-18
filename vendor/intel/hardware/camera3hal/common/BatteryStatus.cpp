/*
 * Copyright (C) 2013 Intel Corporation
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

#define LOG_TAG "BatteryStatus"

#include <stdlib.h>
#include <cutils/properties.h>
#include "BatteryStatus.h"

namespace android {
namespace camera2 {

/**
 * Return Battery Status which got from camera.flash.throttling_levels property.
 */
int getBatteryStatus(void)
{
    char gPowerLevelProp[PROPERTY_VALUE_MAX];
    int throttlingLevel = BATTERY_STATUS_INVALID;

    if (property_get("camera.flash.throttling_levels", gPowerLevelProp, NULL)) {
        throttlingLevel = atoi(gPowerLevelProp);
    }

    return throttlingLevel;
}

}
}
