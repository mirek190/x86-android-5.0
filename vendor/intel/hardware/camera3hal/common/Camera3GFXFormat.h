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

#ifndef _CAMERA3_GFX_FORMAT_H_
#define _CAMERA3_GFX_FORMAT_H_

#include <binder/MemoryHeapBase.h>
#include <system/window.h>
#include <linux/atomisp.h>
#include <linux/videodev2.h>

#include "UtilityMacros.h"
namespace android {
namespace camera2 {


/* ********************************************************************
 * Common GFX data structures and methods
 */

/**
 *  \struct CameraFormatBridge
 *  Structure to hold static array of formats with their depths, description
 *  and information often needed in image buffer processing.
 *  This is derived concept from camera driver (atomisp_format_bridge), with
 *  redefinition of info we are interested.
 */
struct CameraFormatBridge {
    unsigned int pixelformat;
    unsigned int depth;
    bool planar;
    bool bayer;
};
extern const struct CameraFormatBridge sV4l2PixelFormatBridge[];
const struct CameraFormatBridge* getCameraFormatBridge(unsigned int fourcc);

/**
 * return pixels based on bytes
 *
 * commonly used to calculate bytes-per-line as per pixel width
 */
static int bytesToPixels(int fourcc, int bytes)
{
    const CameraFormatBridge* afb = getCameraFormatBridge(fourcc);

    if (afb->planar) {
        // All our planar YUV formats are with depth 8 luma
        // and here byte means one pixel. Chroma planes are
        // to be handled according to fourcc respectively
        return bytes;
    }

    return (bytes * 8) / afb->depth;
}

/**
 * return bytes-per-line based on given pixels
 */
static int pixelsToBytes(int fourcc, int pixels)
{
    const CameraFormatBridge* afb = getCameraFormatBridge(fourcc);

    if (afb->planar) {
        // All our planar YUV formats are with depth 8 luma
        // and here byte means one pixel. Chroma planes are
        // to be handled according to fourcc respectively
        return pixels;
    }

    return ALIGN8(afb->depth * pixels) / 8;
}

/**
 * Return frame size (in bytes) based on image format description
 */
static int frameSize(int fourcc, int width, int height)
{
    // JPEG buffers are generated from HAL_PIXEL_FORMAT_BLOB, where
    // the "stride" (width here)is the full size of the buffer in bytes, so use it
    // as the buffer size.
    if (fourcc == V4L2_PIX_FMT_JPEG)
        return width;
    const CameraFormatBridge* afb = getCameraFormatBridge(fourcc);
    return height * ALIGN8(afb->depth * width) / 8;
}


static int bytesPerLineToWidth(int format, int bytesperline)
{
    int width = 0;
    switch (format) {
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV411P:
    case V4L2_PIX_FMT_YUV422P:
        width = bytesperline;
        break;
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_Y41P:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_SRGGB10:
        width = (bytesperline / 2);
        break;
    default:
        ALOGW("%s: no case for selected pixel format!", __FUNCTION__);
        bytesperline = (width * 2);
        break;
    }

    return width;
}

static const char* v4l2Fmt2Str(int format)
{
    static char fourccBuf[5];
    CLEAR(fourccBuf);
    char *fourccPtr = (char*) &format;
    snprintf(fourccBuf, sizeof(fourccBuf), "%c%c%c%c", *fourccPtr, *(fourccPtr+1), *(fourccPtr+2), *(fourccPtr+3));
    return &fourccBuf[0];
}

bool isBayerFormat(int fourcc);
const char *getLogString(const char *array[], unsigned int index);
int displayBpl(int fourcc, int width);
int widthToStride(int fourcc, int width);

int v4L2Fmt2GFXFmt(int v4l2Fmt, int cameraId);
int GFXFmt2V4l2Fmt(int gfxFmt, int cameraId);
int getActualGFXFmt(uint32_t usage, int fmt);

void *getPlatNativeHandle(buffer_handle_t *handle);
int getNativeHandleStride(buffer_handle_t *handle);
int getNativeHandleWidth(buffer_handle_t *handle);
void checkGrallocBuffer(int w, int v4l2fmt, buffer_handle_t *handle);
}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_GFX_FORMAT_H_
