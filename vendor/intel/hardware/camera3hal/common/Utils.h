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

#ifndef CAMERA3_HAL_UTILS_H
#define CAMERA3_HAL_UTILS_H

namespace android {
namespace camera2 {

bool validateString(const char* value,  const char* supportList);

// Parse string like "640x480" or "10000,20000"
// copy from android CameraParameters.cpp
int parsePair(const char *str, int *first, int *second, char delim, char **endptr = NULL);

//Resize a 2D (uint16_t) array with linear interpolation
//For some cases, we need to upscale or downscale a 2D array.
//For example, Android requests lensShadingMapSize must be smaller than 64*64,
//but for some sensors, the lens shading map is bigger than this, so need to do resize.
int resize2dArrayUint16(
    const unsigned short* a_src,
    int a_src_w, int a_src_h,
    unsigned short* a_dst,
    int a_dst_w, int a_dst_h);

}
}
#endif // CAMERA3_HAL_UTILS_H

