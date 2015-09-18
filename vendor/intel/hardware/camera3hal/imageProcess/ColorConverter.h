/*
 * Copyright (C) 2011,2012,2013 Intel Corporation
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

#ifndef _CAMERA3_HAL_COLOR_CONVERTER_H_
#define _CAMERA3_HAL_COLOR_CONVERTER_H_

#include <utils/Errors.h>

namespace android {
namespace camera2 {

void YUV420ToRGB565(int width, int height, void *src, void *dst);

void trimConvertNV12ToRGB565(int width, int height, int srcStride, void *src, void *dst);

void convertYV12ToNV21(int width, int height, int srcStride, int dstStride, void *src, void *dst);
void copyYV12ToYV12(int width, int height, int srcStride, int dstStride, void *src, void *dst);

void trimConvertNV12ToNV21(int width, int height, int srcStride, void *src, void *dst);

void align16ConvertNV12ToYV12(int width, int height, int srcStride, void *src, void *dst);

void NV12ToP411(int width, int height, int stride, void *src, void *dst);
void NV12ToP411Separate(int width, int height, int stride,
                                    void *srcY, void *srcUV, void *dst);

void YUY2ToP411(int width, int height, int stride, void *src, void *dst);
void NV12ToIMC3(int width, int height, int stride,void *srcY, void *srcUV, void *dst);
void NV12ToIMC1(int width, int height, int stride, void *srcY, void *srcUV, void *dst);
void convertYUYVToYV12(int width, int height, int srcStride, int dstStride, void *src, void *dst);

void convertYUYVToNV21(int width, int height, int srcStride, void *src, void *dst);

void convertBuftoYV12(int format, int width, int height, int srcStride, int
                      dstStride, void *src, void *dst);

void convertBuftoNV21(int format, int width, int height, int srcStride, int
                      dstStride, void *src, void *dst);

void repadYUV420(int width, int height, int srcStride, int dstStride, void *src, void *dst);

}; // namespace camera2
}; // namespace android
#endif // _CAMERA3_HAL_COLOR_CONVERTER_H_
