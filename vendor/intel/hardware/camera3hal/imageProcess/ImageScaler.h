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

#ifndef _IMAGESCALER_H_
#define _IMAGESCALER_H_

#include "CameraBuffer.h"

namespace android {
namespace camera2 {

/**
 * \class ImageScaler
 *
 */
class ImageScaler {
public:
    static void downScaleImage(CameraBuffer *src, CameraBuffer *dst,
            int src_skip_lines_top = 0, int src_skip_lines_bottom = 0);
    static void downScaleImage(void *src, void *dest,
            int dest_w, int dest_h, int dest_stride,
            int src_w, int src_h, int src_stride,
            int format, int src_skip_lines_top = 0,
            int src_skip_lines_bottom = 0);
protected:
    static void downScaleYUY2Image(unsigned char *dest, const unsigned char *src,
        const int dest_w, const int dest_h, const int src_w, const int src_h);

    static void downScaleAndCropNv12Image(
        unsigned char *dest, const unsigned char *src,
        const int dest_w, const int dest_h, const int dest_stride,
        const int src_w, const int src_h, const int src_stride,
        const int src_skip_lines_top = 0,
        const int src_skip_lines_bottom = 0);

    static void trimNv12Image(
        unsigned char *dest, const unsigned char *src,
        const int dest_w, const int dest_h, const int dest_stride,
        const int src_w, const int src_h, const int src_stride,
        const int src_skip_lines_top = 0,
        const int src_skip_lines_bottom = 0);

    static void downScaleAndCropNv12ImageQvga(
        unsigned char *dest, const unsigned char *src,
        const int dest_stride, const int src_stride);

    static void downScaleAndCropNv12ImageQcif(
        unsigned char *dest, const unsigned char *src,
        const int dest_stride, const int src_stride);

    static void downScaleNv12ImageFrom800x600ToQvga(
        unsigned char *dest, const unsigned char *src,
        const int dest_stride, const int src_stride);
};

};
};

#endif // _IMAGESCALER_H_
