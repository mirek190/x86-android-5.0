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
#include "PlatformData.h"
#include "LogHelper.h"
#include <hardware/hardware.h>
#include <ufo/graphics.h>
#include <ufo/gralloc.h>

namespace android {
namespace camera2 {

void *getPlatNativeHandle(buffer_handle_t *handle)
{
    UNUSED(handle);
    return NULL;
}

static bool getBufferInfo(buffer_handle_t *handle, intel_ufo_buffer_details_t *info)
{
    if (NULL == handle || NULL == info) {
        LOGE("@%s, passed parameter is NULL", __FUNCTION__);
        return false;
    }

    int ret;
    gralloc_module_t *pGralloc = NULL;
    CLEAR(*info);
    ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (hw_module_t const**)&pGralloc);
    if (!pGralloc) {
        LOGE("@%s, call hw_get_module fail", __FUNCTION__);
        return false;
    }

    if (0 == ret)
        ret = pGralloc->perform(pGralloc, INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_INFO, (buffer_handle_t)*handle, info);
    if (ret) {
        LOGE("@%s, call perform fail", __FUNCTION__);
        return false;
    }

    return true;
}

int getNativeHandleWidth(buffer_handle_t *handle)
{
    intel_ufo_buffer_details_t info;
    if (getBufferInfo(handle, &info)) {
        LOG2("@%s, w:%d, h:%d, size:%d, f:%d, stride:%d", __FUNCTION__, info.width, info.height, info.size, info.format, info.pitch);
        return info.width;
    }
    return 0;
}

int getNativeHandleStride(buffer_handle_t *handle)
{
    intel_ufo_buffer_details_t info;
    if (getBufferInfo(handle, &info)) {
        switch (info.format) {
        case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
        case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
            LOG2("@%s, w:%d, h:%d, size:%d, f:%d, pitch:%d, stride:%d", __FUNCTION__,
                  info.width, info.height, info.size, info.format, info.pitch, ALIGN64(info.width));
            return ALIGN64(info.width);
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            LOG2("@%s, w:%d, h:%d, size:%d, f:%d, pitch:%d, stride:%d", __FUNCTION__,
                  info.width, info.height, info.size, info.format, info.pitch, ALIGN32(info.width));
            return ALIGN32(info.width);
        case HAL_PIXEL_FORMAT_BLOB:
            LOG2("@%s, w:%d, h:%d, size:%d, f:%d, pitch:%d", __FUNCTION__,
                  info.width, info.height, info.size, info.format, info.pitch);
            return info.pitch;
        default:
            LOGE("@%s,unknown format for GEN w:%d, h:%d, size:%d, f:%d, pitch:%d", __FUNCTION__,
                  info.width, info.height, info.size, info.format, info.pitch);
            break;
        }
    }
    return 0;
}

void checkGrallocBuffer(int w, int v4l2fmt, buffer_handle_t *handle)
{
    UNUSED(w);
    UNUSED(v4l2fmt);
    UNUSED(handle);
    return;
}

int getActualGFXFmt(uint32_t usage, int fmt)
{
    // TODO: Replace with a gralloc perform call that gets actual format from usage and format
    if (fmt == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
    {
        if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
            return HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL;
        } else {
            return HAL_PIXEL_FORMAT_YCbCr_422_I;
        }
    }
    if (fmt == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        return HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL;
    }
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
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SGRBG10:
        gfxFmt = HAL_PIXEL_FORMAT_RAW_SENSOR;
        break;
    case V4L2_PIX_FMT_YVU420:
        gfxFmt = HAL_PIXEL_FORMAT_YV12;
        break;
    case V4L2_PIX_FMT_NV21:
        gfxFmt = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case V4L2_PIX_FMT_NV12:
        gfxFmt = HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL;
        break;
    case V4L2_PIX_FMT_YUYV:
        gfxFmt = HAL_PIXEL_FORMAT_YCbCr_422_I;
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

    case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
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
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        v4l2Fmt = V4L2_PIX_FMT_YUYV;
        break;
    case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
        ALOGW("No need v4l2 format for GFX NV12T format");
        return -1;
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
    int stride = 0;
    if (isBayerFormat(fourcc))
        return ALIGN128(width);
    switch (fourcc) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YVU420:
        stride =  ALIGN64(width);
        break;
    case V4L2_PIX_FMT_YUYV:
        stride =  ALIGN32(width);
        break;
    default:
        stride = ALIGN64(width);
        break;
    }
    return stride;
}

} // namespace camera2
} // namespace android
