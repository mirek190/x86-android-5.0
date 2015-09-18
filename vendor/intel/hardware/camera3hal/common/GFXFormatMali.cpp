/*
 * Copyright (C) 2014 Intel Corporation
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

#include "Camera3GFXFormat.h"
#include "LogHelper.h"
#include "PlatformData.h"
#include "gralloc_priv.h"

namespace android {
namespace camera2 {

void *getPlatNativeHandle(buffer_handle_t *handle) {
    if (handle == NULL) {
        ALOGE("No gralloc handle provided");
        return NULL;
    }
    private_handle_t *h = (private_handle_t *) *handle;
    if (h == NULL) {
        ALOGE("gralloc handle is NULL");
    }

    return h;
}
int getNativeHandleStride(buffer_handle_t *handle) {
    if (handle == NULL) {
        ALOGE("No gralloc handle provided");
        return 0;
    }

    private_handle_t *h = (private_handle_t *)getPlatNativeHandle(handle);
    if (h == NULL) {
        ALOGE("gralloc private handle not found");
        return 0;
    }
    LOG2("@%s handle %p returning stride: %d for width: %d", __FUNCTION__, h, h->stride, h->w);
    return h->stride;
}
void checkGrallocBuffer(int w, int v4l2fmt, buffer_handle_t *handle) {
    UNUSED(w);
    UNUSED(v4l2fmt);
    UNUSED(handle);
    return;
}
int getNativeHandleWidth(buffer_handle_t *handle)
{
    if (handle == NULL) {
        ALOGE("No gralloc handle provided");
        return 0;
    }

    private_handle_t *h = (private_handle_t *)getPlatNativeHandle(handle);
    if (h == NULL) {
        ALOGE("gralloc private handle not found");
        return 0;
    }
    LOG2("@%s handle %p width %d", __FUNCTION__, h, h->w);
    return h->w;
}

int getActualGFXFmt(uint32_t usage, int fmt)
{
    UNUSED(usage);
    return fmt;
}

int v4L2Fmt2GFXFmt(int v4l2Fmt, int cameraId)
{
    int gfxFmt = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;

    switch (v4l2Fmt) {
    case V4L2_PIX_FMT_JPEG:
        gfxFmt = HAL_PIXEL_FORMAT_BLOB;
        break;
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SRGGB8:
    case V4L2_PIX_FMT_SRGGB10:
        gfxFmt = HAL_PIXEL_FORMAT_RAW_SENSOR;
        break;
    case V4L2_PIX_FMT_YVU420:
        gfxFmt = HAL_PIXEL_FORMAT_YV12;
        break;
    case V4L2_PIX_FMT_NV21:
        gfxFmt = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    default:
        if (v4l2Fmt != PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, cameraId))
            ALOGE("%s: no gfx format for v4l2 %s!", __FUNCTION__, v4l2Fmt2Str(v4l2Fmt));
        break;
    }

    return gfxFmt;
}

int GFXFmt2V4l2Fmt(int gfxFmt, int cameraId)
{
    int v4l2Fmt = PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, cameraId);

    switch (gfxFmt) {
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
        v4l2Fmt = PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_YCbCr_420_888, cameraId);
        break;
    case HAL_PIXEL_FORMAT_BLOB:
        v4l2Fmt = V4L2_PIX_FMT_JPEG;
        break;
    case HAL_PIXEL_FORMAT_RAW_SENSOR:
        v4l2Fmt = PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_RAW_SENSOR, cameraId);
        break;
    case HAL_PIXEL_FORMAT_YV12:
        v4l2Fmt = V4L2_PIX_FMT_YVU420;
        break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        v4l2Fmt = V4L2_PIX_FMT_NV21;
        break;
    default:
        if (gfxFmt != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            ALOGE("%s: no v4l2 format for GFX 0x%x!", __FUNCTION__, gfxFmt);
        break;
    }

    return v4l2Fmt;
}

int widthToStride(int fourcc, int width)
{
    /**
     * Raw format has special aligning requirements
     */
    if (isBayerFormat(fourcc))
        return ALIGN128(width);
    else
        return ALIGN64(width);
}

} // namespace camera2
} // namespace android
