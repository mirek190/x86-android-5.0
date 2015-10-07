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

/*!
 * \file gdctool_log.h
 * \brief Definitions of debug print functions.
*/

#ifndef _GDCTOOL_LOG_H_
#define _GDCTOOL_LOG_H_

#include <utils/Log.h>
#include <cutils/atomic.h>

/* 0 = only error logs
 * 1 = all logs         */
extern int32_t gGDCLogLevel;

static void setLogLevel(int level) {
    android_atomic_write(level, &gGDCLogLevel);
}

#define GDC_ERROR_TAG "CAMERA_GDC ERROR: "

#define GDC_LOG_ERR(...)  ((void)ALOG(LOG_ERROR, GDC_ERROR_TAG, __VA_ARGS__))
#define GDC_LOG(...) ALOGD_IF(gGDCLogLevel,  __VA_ARGS__)


/**
 * Runtime selection of debugging level.
 */
void setGDCDebugLevel(void);


#endif /* _CAMERAGDC_LOG_H_ */
