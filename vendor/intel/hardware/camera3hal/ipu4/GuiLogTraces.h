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

#ifndef _IPU4_GUILOGTRACES_H_
#define _IPU4_GUILOGTRACES_H_

#include <utils/Log.h>
#include <utils/Timers.h>

/**
 * global log level
 * This global variable is set from system properties
 * It is used to control the level of verbosity of the traces in logcat
 * It is also used to store the status of certain RD features
 */
extern int32_t gLogLevel;
extern int32_t gPerfLevel;

/**
 *  HAL_GUITRACE_CALL traces the beginning and end of the current scope.
 *  It uses a format that can be used to visualize the trace graphically
 *  This traces are used mainly to trac the flow of requests
 */

#define HAL_GUITRACE_CALL(level, reqId) \
    ScopedGuiTrace ___guitracer(level, LOG_TAG, __FUNCTION__, reqId )

/**
 * HAL_GUITRACE_VALUE
 * Produces a trace to track the value of a variable over time. It is formatted
 * so that any tool can represent this info graphically
 */
#define HAL_GUITRACE_VALUE(level, name, value) \
{\
    ALOGD_IF(gLogLevel & level,"CAMGUILOG;%lld;valueabs;%d;%s-%s", \
    systemTime(), value, LOG_TAG, name);\
}

#define HAL_GUITRACE_VARIABLE(level, var)  \
        HAL_GUITRACE_VALUE(level, #var, var)

namespace android {
namespace camera2 {

/**
 * \class ScopedGuiTrace
 *
 *
 */
class ScopedGuiTrace {
public:
inline ScopedGuiTrace(int level, const char* module,
                       const char* method, unsigned int reqId) :
       mLevel(level),
       mModuleName(module),
       mMethodName(method),
       mReq(reqId)
{

    ALOGD_IF(gLogLevel & mLevel,"CAMGUILOG;%lld;namedevent;start;%s-%s;reqId %d",
                systemTime(), mModuleName, mMethodName, mReq);
}

inline ~ScopedGuiTrace()
{
    float timeNow = (float)systemTime()/(float)1000000000; // Time in seconds
    ALOGD_IF(gLogLevel & mLevel,"CAMGUILOG;%lld;namedevent;stop;%s-%s",
            systemTime(), mModuleName, mMethodName);

}

private:
    int mLevel;             /*!> Trace level used */
    const char* mModuleName;      /*!> Name of this module (LOGTAG) */
    const char* mMethodName;      /*!> Name of the method */
    unsigned int mReq;             /*!> Request ID*/
};

}  // namespace camera2

} // namespace android;
#endif // _IPU4_GUILOGTRACES_H_
