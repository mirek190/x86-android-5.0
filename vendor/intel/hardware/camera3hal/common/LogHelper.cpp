/*
 * Copyright (C) 2012,2013 Intel Corporation
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

#define LOG_TAG "Camera_LogHelper"

#include <stdint.h>
#include <limits.h> // INT_MAX, INT_MIN
#include <stdlib.h> // atoi.h
#include <utils/Log.h>
#include <cutils/properties.h>

#include "LogHelper.h"
#include "PerformanceTraces.h"


int32_t gLogLevel = 0;
int32_t gPerfLevel = 0;
int32_t gDumpImageType = 0;

namespace android {
namespace camera2 {

//Print table for each alignment lines
void printTable(unsigned short* table, unsigned int size, unsigned int alignment)
{
    unsigned int i, n = 0;
    char *buf = new char[alignment * 8];
    if (buf == NULL)
        return;
    for (i = 0; i < size; i++) {
        n += snprintf(buf + n, 1024, "%8d", table[i]);
        if (((i + 1) % alignment) == 0) {
            buf[n] = '\0';
            LOG2("%s", buf);
            n = 0;
        }
    }
    if (n > 0) {
        buf[n] = '\0';
        LOG2("%s", buf);
    }
    delete[] buf;
}
}
}

using namespace android;
void android::LogHelper::setDebugLevel(void)
{
    char gLogLevelProp[PROPERTY_VALUE_MAX];
    char gPerfLevelProp[PROPERTY_VALUE_MAX];
    char gDumpImageTypeProp[PROPERTY_VALUE_MAX];
    PerformanceTraces::reset();

    if (property_get("camera.hal.debug", gLogLevelProp, NULL)) {
        gLogLevel = atoi(gLogLevelProp);
        LOGD("Debug level is 0x%x", gLogLevel);

        // Check that the property value is a valid integer
        if (gLogLevel >= INT_MAX || gLogLevel <= INT_MIN) {
            LOGE("Invalid camera.hal.debug property integer value: %s",gLogLevelProp);
            gLogLevel = 0;
        }

        // legacy support: "setprop camera.hal.debug 2" is expected
        // to enable both LOG1 and LOG2 traces
        if (gLogLevel & CAMERA_DEBUG_LOG_LEVEL2)
            gLogLevel |= CAMERA_DEBUG_LOG_LEVEL1;
    }

    //Performance property
    if (property_get("camera.hal.perf", gPerfLevelProp, NULL)) {
        gPerfLevel = atoi(gPerfLevelProp);
        LOGD("Performance level is 0x%x", gPerfLevel);

        // Check that the property value is a valid integer
        if (gPerfLevel >= INT_MAX || gPerfLevel <= INT_MIN) {
            LOGE("Invalid camera.hal.perf property integer value: %s",gPerfLevelProp);
            gPerfLevel = 0;
        }

        // bitmask of tracing categories
        if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_TRACES) {
            PerformanceTraces::Launch2Preview::enable(true);
            PerformanceTraces::Launch2FocusLock::enable(true);
            PerformanceTraces::FaceLock::enable(true);
            PerformanceTraces::Shot2Shot::enable(true);
            PerformanceTraces::ShutterLag::enable(true);
            PerformanceTraces::SwitchCameras::enable(true);
            PerformanceTraces::HDRShot2Preview::enable(true);
        }
        if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_TRACES_BREAKDOWN) {
            PerformanceTraces::PnPBreakdown::enable(true);
        }

        if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN) {
            PerformanceTraces::IOBreakdown::enableBD(true);
        }

        if (gPerfLevel & CAMERA_DEBUG_LOG_PERF_MEMORY) {
            PerformanceTraces::IOBreakdown::enableMemInfo(true);
        }

        if (gPerfLevel & CAMERA_DEBUG_LOG_ATRACE_LEVEL) {
            PerformanceTraces::HalTrace::setTraceLevel(1);
        }
    }

    // dumpImage property, it's used to dump preview/video/snapshot/jpeg.
    if (property_get("camera.hal.dumpImage", gDumpImageTypeProp, NULL)) {
        gDumpImageType = atoi(gDumpImageTypeProp);
        LOGD("DumpImage level is 0x%x", gDumpImageType);

        // Check that the property value is a valid integer
        if (gDumpImageType >= INT_MAX || gDumpImageType <= INT_MIN) {
            LOGE("Invalid camera.hal.dumpImage property integer value: %s",gDumpImageTypeProp);
            gDumpImageType = 0;
        }
    }

}

bool android::LogHelper::isDumpImageTypeEnable(int dumpType)
{
    return (gDumpImageType & dumpType) ? true : false;
}

