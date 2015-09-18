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

#define LOG_TAG "Camera3GFXFormat"

#include "Camera3GFXFormat.h"
#include "PlatformData.h"
#include "LogHelper.h"

namespace android {
namespace camera2 {

const struct CameraFormatBridge sV4L2PixelFormatBridge[] = {
    {
        .pixelformat = V4L2_PIX_FMT_NV12,
        .depth = 12,
        .planar = true,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_YUV420,
        .depth = 12,
        .planar = true,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_YVU420,
        .depth = 12,
        .planar = true,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_YUV422P,
        .depth = 16,
        .planar = true,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_YUV444,
        .depth = 24,
        .planar = false,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_NV21,
        .depth = 12,
        .planar = true,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_NV16,
        .depth = 16,
        .planar = true,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_YUYV,
        .depth = 16,
        .planar = false,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_UYVY,
        .depth = 16,
        .planar = false,
        .bayer = false
    }, { /* This one is for parallel sensors! DO NOT USE! */
        .pixelformat = V4L2_PIX_FMT_UYVY,
        .depth = 16,
        .planar = false,
        .bayer = false
    }, {
        .pixelformat = V4L2_PIX_FMT_SBGGR16,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SBGGR8,
        .depth = 8,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SGBRG8,
        .depth = 8,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SGRBG8,
        .depth = 8,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SRGGB8,
        .depth = 8,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SBGGR10,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SGBRG10,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SGRBG10,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SRGGB10,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SBGGR12,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SGBRG12,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SGRBG12,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_SRGGB12,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_RGB32,
        .depth = 32,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_RGB565,
        .depth = 16,
        .planar = false,
        .bayer = true
    }, {
        .pixelformat = V4L2_PIX_FMT_JPEG,
        .depth = 8,
        .planar = false,
        .bayer = false
    },
};

/**
 * returns sCameraFormatBrige* for given V4L2 pixelformat (fourcc)
 */
const struct CameraFormatBridge* getCameraFormatBridge(unsigned int fourcc)
{
    unsigned int i;
    for (i = 0;i < sizeof(sV4L2PixelFormatBridge)/sizeof(CameraFormatBridge);i++) {
        if (fourcc == sV4L2PixelFormatBridge[i].pixelformat)
            return &sV4L2PixelFormatBridge[i];
    }

    /* Relax this until we have fixed the CameraHW refactoring
     *
    LOGE("Unknown pixel format being used : %s, aborting!", v4l2Fmt2Str(fourcc));
    abort();
    return NULL;*/
    LOG1("Unkown pixel format %d being used, use NV12 as default", fourcc);
    return &sV4L2PixelFormatBridge[0];
}

bool isBayerFormat(int fourcc)
{
    const CameraFormatBridge* afb = getCameraFormatBridge(fourcc);
    return afb->bayer;
}

/**
 * Calculates the frame bytes-per-line following the limitations imposed by display subsystem
 * This is used to model the HACK in a atomsisp that forces allocation
 * to be aligned to the bpl that SGX, GEN or other gfx requires.
 *
 *
 * \param format [in] V4L2 pixel format of the image
 * \param width [in] width in pixels
 *
 * \return bpl following the Display subsystem requirement
 **/
int displayBpl(int fourcc, int width)
{
    return pixelsToBytes(fourcc, widthToStride(fourcc, width));
}

/**
 * Helper method to retrieve the string representation of a class enum
 * It performs a boundary check
 *
 * @param array pointer to the static table with the strings
 * @param index enum to convert to string
 *
 * @return string describing enum
 */
const char *getLogString(const char *array[], unsigned int index)
{
    if (index > sizeof(*array))
        return "-1";
    return array[index];
}

}
}
