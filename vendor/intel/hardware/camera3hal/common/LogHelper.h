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

#ifndef _LOGHELPER_H_
#define _LOGHELPER_H_


#include <utils/Log.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <utils/KeyedVector.h>
#include <utils/Timers.h>
#include <utils/String8.h>
#include <cutils/trace.h>

/**
 * global log level
 * This global variable is set from system properties
 * It is used to control the level of verbosity of the traces in logcat
 * It is also used to store the status of certain RD features
 */
extern int32_t gLogLevel;
extern int32_t gPerfLevel;

/**
 * LOG levels
 *
 * LEVEL 1 is used to track events in the HAL that are relevant during
 * the operation of the camera, but are not happening on a per frame basis.
 * this ensures that the level of logging is not too verbose
 *
 * LEVEL 2 is used to track information on a per request basis
 *
 * REQ_STATE is used to track the state of each request. By state we mean a one
 * of the following request properties:
 *  - metadata result
 *  - buffer
 *  - shutter
 *  - error
 *
 * PERF TRACES enable only traces that provide performance metrics on the opera
 * tion of the HAL
 *
 * PERF TRACES BREAKDOWN provides further level of detail on the performance
 * metrics
 */
enum  {
    /* verbosity level of general traces */
    CAMERA_DEBUG_LOG_LEVEL1 = 1,
    CAMERA_DEBUG_LOG_LEVEL2 = 1 << 1,

    /* Bitmask to enable a concrete set of traces */
    CAMERA_DEBUG_LOG_REQ_STATE = 1 << 2,
    CAMERA_DEBUG_LOG_AIQ = 1 << 3
};

enum {
    CAMERA_DUMP_PREVIEW = 1,
    CAMERA_DUMP_VIDEO = 1<<1,
    CAMERA_DUMP_SNAPSHOT = 1<<2,
    CAMERA_DUMP_JPEG = 1<<3,
    CAMERA_DUMP_RAW = 1<<4,

    CAMERA_DUMP_DVS2 = 1<<6
};

enum  {
    /* Emit well-formed performance traces */
    CAMERA_DEBUG_LOG_PERF_TRACES = 1,

    /* Print out detailed timing analysis */
    CAMERA_DEBUG_LOG_PERF_TRACES_BREAKDOWN = 2,

    /* Print out detailed timing analysis for IOCTL */
    CAMERA_DEBUG_LOG_PERF_IOCTL_BREAKDOWN = 1<<2,

    /* Print out detailed memory information analysis for IOCTL */
    CAMERA_DEBUG_LOG_PERF_MEMORY = 1<<3,

    /*set camera atrace level for pytimechart*/
    CAMERA_DEBUG_LOG_ATRACE_LEVEL = 1<<4
};

enum  {
    CAMERA_POWERBREAKDOWN_DISABLE_PREVIEW = 1<<0,
    CAMERA_POWERBREAKDOWN_DISABLE_FDFR = 1<<1,
    CAMERA_POWERBREAKDOWN_DISABLE_3A = 1<<2,
};

#define LOG1(...) ALOGD_IF(gLogLevel & CAMERA_DEBUG_LOG_LEVEL1, __VA_ARGS__);
#define LOG2(...) ALOGD_IF(gLogLevel & CAMERA_DEBUG_LOG_LEVEL2, __VA_ARGS__);
#define LOGR(...) ALOGD_IF(gLogLevel & CAMERA_DEBUG_LOG_REQ_STATE, __VA_ARGS__);
#define LOG3A(...) ALOGD_IF(gLogLevel & CAMERA_DEBUG_LOG_AIQ, __VA_ARGS__);

#define LOGE    ALOGE
#define LOGD    ALOGD
#define LOGW    ALOGW
#define LOGV    ALOGV

// CAMTRACE_NAME traces the beginning and end of the current scope.  To trace
// the correct start and end times this macro should be declared first in the
// scope body.
#define HAL_TRACE_NAME(level, name) ScopedTrace ___tracer(level, name )
#define HAL_TRACE_CALL(level) HAL_TRACE_NAME(level, __FUNCTION__)
#define HAL_PER_TRACE_NAME(level, name) ScopedPerfTrace  ___tracer(level, name )
#define HAL_PER_TRACE_CALL(level)  HAL_PER_TRACE_NAME(level, __FUNCTION__)
/**
 * HAL_KPI_TRACE_CALL
 * Prints traces of the execution time of the method and checks if it took
 * longer than maxTime. In that case it prints an warning trace
 */
#define HAL_KPI_TRACE_CALL(level, maxTime)  ScopedPerfTrace __kpiTracer(level, __FUNCTION__, maxTime)

namespace android {
namespace camera2 {

class ScopedTrace {
public:
inline ScopedTrace(int level, const char* name) :
        mLevel(level),
        mName(name) {
    ALOGD_IF(gLogLevel & mLevel, "ENTER-%s", name);
}

inline ~ScopedTrace() {
    ALOGD_IF(gLogLevel & mLevel, "EXIT-%s", mName);
}

private:
    int mLevel;
    const char* mName;
};

/**
 * \class ScopedPerfTrace
 *
 * This class allows tracing the execution of a method. By declaring object
 * of this class at the beginning of a method/function the constructor code is
 * executed then.
 * When the method finishes the object is automatically destroyed.
 * The code in the destructor is useful to trace how long it took to execute
 * a method.
 * If a maxExecTime is provided an error message will be printed in case the
 * execution time took longer than expected
 */
class ScopedPerfTrace {
public:
inline ScopedPerfTrace(int level, const char* name, nsecs_t maxExecTime = 0) :
       mLevel(level),
       mName(name),
       mMaxExecTime(maxExecTime)
{
    mStartTime = systemTime();
}

inline ~ScopedPerfTrace()
{
    nsecs_t actualExecTime = systemTime()- mStartTime;
    ALOGD_IF(gPerfLevel & mLevel,"%s took %lld ns", mName, actualExecTime);

    if (CC_UNLIKELY(mMaxExecTime > 0 && actualExecTime > mMaxExecTime)){
        ALOGW("KPI:%s took longer than expected. Actual %lld us expected %lld us",
                mName, actualExecTime/1000, mMaxExecTime/1000);
    }
}

private:
    nsecs_t mStartTime;     /*!> systemTime when this object was created */
    int mLevel;             /*!> Trace level used */
    const char* mName;      /*!> Name of this trace object */
    nsecs_t mMaxExecTime;   /*!> Maximum time this object is expected to live */
};

//Useful print function for different tables
void printTable(unsigned short* table, unsigned int size, unsigned int alignment);

}  // namespace camera2

namespace LogHelper {

/**
 * Runtime selection of debugging level.
 */
void setDebugLevel(void);
bool isDumpImageTypeEnable(int dumpType);

} // namespace LogHelper
} // namespace android;
#endif // _LOGHELPER_H_
