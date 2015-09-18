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
#include "hal_public.h"

namespace android {
namespace camera2 {

/**
 * getPlatNativeHandle
 *
 * Get the platform native handle of gralloc buffer to get actual values of
 * the created gralloc. This is a platform specific function because it depends
 * on the Gfx subsystem. Currently supports Imagination PVR. In platforms with
 * GEN Gfx an equivalent version should be written.
 *
 * \param buffer_handle_t [IN] gralloc buffer handle
 * \return pointer to the native buffer handle. It should be cast to the GFX
 *  specific type based on platform when used
 */
void *getPlatNativeHandle(buffer_handle_t *handle)
{
    if (handle == NULL) {
        LOGE("No gralloc handle provided");
        return NULL;
    }
    IMG_native_handle_t *h = (IMG_native_handle_t *) *handle;
    if (h == NULL) {
        LOGE("gralloc handle is NULL");
        return 0;
    }

    LOG2("IMG_native_handle_t iWidth=%d, iHeight=%d, iFormat=0x%08x bpp = %u\n\n",
         h->iWidth, h->iHeight, h->iFormat,h->uiBpp);
    return h;

}

/**
 * getNativeHandleStride
 *
 * Get the stride value from the platform native handle of gralloc buffer.
 * This is a platform specific function because it depends
 * on the Gfx subsystem. Currently supports Imagination PVR. In platforms with
 * GEN Gfx an equivalent version should be written.
 *
 * \param buffer_handle_t [IN] gralloc buffer handle
 * \return stride value fromt the native buffer handle
 */
int getNativeHandleStride(buffer_handle_t *handle)
{
    if (handle == NULL) {
        LOGE("No gralloc handle provided");
        return 0;
    }

    IMG_native_handle_t *h = (IMG_native_handle_t *)getPlatNativeHandle(handle);
    if (h == NULL) {
        LOGE("gralloc handle is NULL");
        return 0;
    }
    return ALIGN(h->iWidth, CAMERA_ALIGN);
}

/**
 * getNativeHandleWidth
 *
 * Get the width value from the platform native handle of gralloc buffer.
 * This is a platform specific function because it depends
 * on the Gfx subsystem. Currently supports Imagination PVR. In platforms with
 * GEN Gfx an equivalent version should be written.
 *
 * \param buffer_handle_t [IN] gralloc buffer handle
 * \return width value fromt the native buffer handle
 */
int getNativeHandleWidth(buffer_handle_t *handle)
{
    if (handle == NULL) {
        LOGE("No gralloc handle provided");
        return 0;
    }

    IMG_native_handle_t *h = (IMG_native_handle_t *)getPlatNativeHandle(handle);
    int tmpWidth = h ? h->iWidth : 0;
    return tmpWidth;
}

/**
 * checkGrallocBuffer
 *
 * Check the allocated graphic buffer handle parameters against what
 * was requested. Print warnings if there is a mismatch.This is a platform
 * specific function because it depends on the Gfx subsystem. Currently
 * supports Imagination PVR. In platforms with GEN Gfx an equivalent version
 * should be written.
 *
 * \param w [IN] requested width
 * \param v4l2fmt [IN] requested v4l2 format
 * \param buffer_handle_t [IN] gralloc buffer handle
 */
void checkGrallocBuffer(int w, int v4l2fmt, buffer_handle_t *handle)
{
    LOG2("@%s", __FUNCTION__);

    IMG_native_handle_t *h = (IMG_native_handle_t *)getPlatNativeHandle(handle);

    if (h == NULL)
        return;

    // Check width
    if (w != h->iWidth)
        LOG1("%s: Requested buffer width = %d, Allocated gralloc width = %d",
             __FUNCTION__, w, h->iWidth);

    // Check format
    if (v4l2fmt != h->iFormat)
        LOG1("%s: Requested buffer format = %x, Allocated gralloc format = %x",
             __FUNCTION__, v4l2fmt, h->iFormat);
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
    default:
        if (v4l2Fmt != PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                                                 cameraId))
            ALOGE("%s: no gfx format for v4l2 %s!", __FUNCTION__, v4l2Fmt2Str(v4l2Fmt));
        break;
    }

    return gfxFmt;
}

int GFXFmt2V4l2Fmt(int gfxFmt, int cameraId)
{
    int v4l2Fmt = PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                                            cameraId);

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
