/*
 * Copyright (c) 2014 Intel Corporation.
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

#define LOG_TAG "Camera3HALUtils"

#include <utils/Log.h>
#include "stdlib.h"
#include "Utils.h"
#include "LogHelper.h"

namespace android {
namespace camera2 {

bool validateString(const char* value,  const char* supportList)
{
    // value should not set if support list is empty
    if (value !=NULL && supportList == NULL) {
        return false;
    }

    if (value == NULL || supportList == NULL) {
        return true;
    }

    size_t len = strlen(value);
    const char* startPtr(supportList);
    const char* endPtr(supportList);
    int bracketLevel(0);

    // divide support list to values and compare those to given values.
    // values are separated with comma in support list, but commas also exist
    // part of values inside bracket.
    while (true) {
        if ( *endPtr == '(') {
            ++bracketLevel;
        } else if (*endPtr == ')') {
            --bracketLevel;
        } else if ( bracketLevel == 0 && ( *endPtr == '\0' || *endPtr == ',')) {
            if (((startPtr + len) == endPtr) &&
                (strncmp(value, startPtr, len) == 0)) {
                return true;
            }

            // bracket can use circle values in supported list
            if (((startPtr + len + 2 ) == endPtr) &&
                ( *startPtr == '(') &&
                (strncmp(value, startPtr + 1, len) == 0)) {
                return true;
            }
            startPtr = endPtr + 1;
        }

        if (*endPtr == '\0') {
            return false;
        }
        ++endPtr;
    }

    return false;
}

// Parse string like "640x480" or "10000,20000"
// copy from android CameraParameters.cpp
int parsePair(const char *str, int *first, int *second, char delim, char **endptr)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimiter does not immediately follow, give up.
    if (*end != delim) {
        LOGE("Cannot find delimiter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimiter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

#define FRAC_BITS_CURR_LOC 8 /* Value of 8 is maximum in order to avoid overflow with 16-bit inputs */

/*!
 * \brief Resize a 2D (uint16_t) array with linear interpolation.
 *
 * @param[in,out]
 *  in a_src                pointer to input array (width-major)
 *  in a_src_w              width of the input array
 *  in a_src_h              height of the input array
 *  in a_dst                pointer to output array (width-major)
 *  in a_dst_w              width of the output array
 *  in a_dst_h              height of the output array
 */
int resize2dArrayUint16(
    const unsigned short* a_src,
    int a_src_w, int a_src_h,
    unsigned short* a_dst,
    int a_dst_w, int a_dst_h)
{
    int i, j, step_size_w, step_size_h, rounding_term;

    if (a_src_w < 2 || a_dst_w < 2 || a_src_h < 2 || a_dst_h < 2) {
        return  -1;
    }
    nsecs_t startTime = systemTime();
    step_size_w = ((a_src_w-1)<<FRAC_BITS_CURR_LOC) / (a_dst_w-1);
    step_size_h = ((a_src_h-1)<<FRAC_BITS_CURR_LOC) / (a_dst_h-1);
    rounding_term = (1<<(2*FRAC_BITS_CURR_LOC-1));
    for (j = 0; j < a_dst_h; ++j) {
        unsigned int curr_loc_h, curr_loc_lower_h;
        curr_loc_h = j * step_size_h;
        curr_loc_lower_h = (curr_loc_h > 0) ? (curr_loc_h-1)>>FRAC_BITS_CURR_LOC : 0;

        for (i = 0; i < a_dst_w; ++i) {
            unsigned int curr_loc_w, curr_loc_lower_w;

            curr_loc_w = i * step_size_w;
            curr_loc_lower_w = (curr_loc_w > 0) ? (curr_loc_w-1)>>FRAC_BITS_CURR_LOC : 0;

            a_dst[a_dst_w*j+i] =
                (a_src[curr_loc_lower_w + curr_loc_lower_h*a_src_w]  *
                        (((curr_loc_lower_w+1)<<FRAC_BITS_CURR_LOC)-curr_loc_w) *
                        (((curr_loc_lower_h+1)<<FRAC_BITS_CURR_LOC)-curr_loc_h) +
                a_src[curr_loc_lower_w+1 + curr_loc_lower_h*a_src_w]  *
                        (curr_loc_w-((curr_loc_lower_w)<<FRAC_BITS_CURR_LOC))   *
                        (((curr_loc_lower_h+1)<<FRAC_BITS_CURR_LOC)-curr_loc_h) +
                a_src[curr_loc_lower_w + (curr_loc_lower_h+1)*a_src_w]  *
                        (((curr_loc_lower_w+1)<<FRAC_BITS_CURR_LOC)-curr_loc_w) *
                        (curr_loc_h-((curr_loc_lower_h)<<FRAC_BITS_CURR_LOC)) +
                a_src[curr_loc_lower_w+1 + (curr_loc_lower_h+1)*a_src_w]  *
                        (curr_loc_w-((curr_loc_lower_w)<<FRAC_BITS_CURR_LOC))   *
                        (curr_loc_h-((curr_loc_lower_h)<<FRAC_BITS_CURR_LOC))
                + rounding_term) >> (FRAC_BITS_CURR_LOC+FRAC_BITS_CURR_LOC);
        }
    }
    LOG2("resize the 2D (uint16_t)array cost %dus",
         (unsigned)((systemTime() - startTime) / 1000));

    return 0;
}


}
}
