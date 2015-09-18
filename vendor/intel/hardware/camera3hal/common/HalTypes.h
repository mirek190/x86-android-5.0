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

#ifndef _HALTYPES_H_
#define _HALTYPES_H_


namespace android {
namespace camera2 {
/* ********************************************************************
 * Common HAL data structures and methods
 */

/**
 *  \struct FrameInfo
 *  Structure to pass resolution, size and and stride info between objects.
 */

struct FrameInfo {
    int format;     // In IPU2 used as both HAL pixel format & v4l2 format
                    // In IPU4 shall be used as HAL pixel format only
    int originalFormat;
    int width;
    int height;

    int stride;     // stride in pixels(can be bigger than width)
    int size;

    int maxWidth;
    int maxHeight;

    int bufsNum;
};

}
}

#endif // _HALTYPES_H_
