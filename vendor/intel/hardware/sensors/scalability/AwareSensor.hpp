/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_AWARE_SENSOR_H
#define ANDROID_AWARE_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <deque>

#include <utils/Mutex.h>
#include "PSHSensor.hpp"
/*****************************************************************************/
/*
 * Author: Duan Qin <qin.duan@intel.com>
 * Group: PSI, MCG
 *
 * Virtual sensor that interacts with PSH to report aware events to sensor manager
 * It supports the following four kinds of modes
 *   Instant Mode
 *   Quick Mode
 *   Normal Mode
 *   Statistic Mode
 *
 * When enabled, one thread will be running to interact with PSH to retrieve audio classifier data
 * and report to sensor manager.
 *
 * There's a pipe setup between the thread and sensor manager to report audio classifier event
 */

class AwareSensor: public PSHSensor {
public:
        AwareSensor(SensorDevice &device);
        virtual ~AwareSensor();
        virtual int getData(std::queue<sensors_event_t> &eventQue);
        virtual int activate(int handle, int enabled);
        virtual int setDelay(int32_t handle, int64_t ns);
        virtual int getPollfd();
        virtual bool selftest();
        virtual int flush(int handle);
        virtual void resetEventHandle();
private:
        AwareSensor() {
        };
        VirtualLogical * virtualSensor;
        static VirtualLogical * (*method_get_virtualsensor)(int, int);
        void* virtualsensorHandle;
        bool VirtualSensorMethodsInitialize();
        bool VirtualSensorMethodsFinallize();
};

/*****************************************************************************/
#endif  // ANDROID_AWARE_SENSOR_H
