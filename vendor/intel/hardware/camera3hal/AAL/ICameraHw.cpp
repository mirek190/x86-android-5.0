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

#include "ICameraHw.h"
#include "PlatformData.h"
#include "LogHelper.h"

namespace android {
namespace camera2 {

/* Supported platforms */
#ifdef CAMERA_IPU2_SUPPORT
ICameraHw *CreateCss2400Camera(int cameraId);
#endif
#ifdef CAMERA_IPU4_SUPPORT
ICameraHw *CreateIPU4Camera(int cameraId);
#endif
#ifdef CAMERA_CIF_SUPPORT
ICameraHw *CreateCIFCamera(int cameraId);
#endif

ICameraHw *ICameraHw::createCameraHW(int cameraId)
{
    ICameraHw* camHw = NULL;

    CameraHwType hwType = PlatformData::getCameraHwType(cameraId);
    switch (hwType) {
#ifdef CAMERA_IPU2_SUPPORT
    case SUPPORTED_HW_CSS_2400:
        camHw = CreateCss2400Camera(cameraId);
        break;
#endif
#ifdef CAMERA_IPU4_SUPPORT
    case SUPPORTED_HW_CSS_2600:
        camHw = CreateIPU4Camera(cameraId);
        break;
#endif
#ifdef CAMERA_CIF_SUPPORT
    case SUPPORTED_HW_CIF:
        camHw = CreateCIFCamera(cameraId);
        break;
#endif
    default:
        LOGE("No Platform Specific Layer built for this HW type");
        break;
    }

    return camHw;
}

} /* namespace camera2  */
} /* namespace android  */

