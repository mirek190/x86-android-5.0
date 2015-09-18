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

#ifndef _CAMERA3_HAL_CIFCAMERACAPINFO_H_
#define _CAMERA3_HAL_CIFCAMERACAPINFO_H_

#include <utils/Vector.h>
#include "PlatformData.h"
#include "LogHelper.h"

namespace android {
namespace camera2 {

class CIFCameraCapInfo;
static const CIFCameraCapInfo * getCIFCameraCapInfo(int cameraId)
{
    if (cameraId > MAX_CAMERAS) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }
    return (CIFCameraCapInfo *)(PlatformData::getCameraCapInfo(cameraId));
}

class CIFCameraCapInfo : public CameraCapInfo {
public:
    CIFCameraCapInfo(SensorType type);
    ~CIFCameraCapInfo() {};
    virtual int sensorType(void) const { return mSensorType; }
    virtual int getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat) const;
    bool exposureSyncEnabled(void) const { return mExposureSync; };
    bool gainExposureCompEnabled(void) const { return mGainExposureComp; };
    int gainLag(void) const { return mGainLag; };
    int exposureLag(void) const { return mExposureLag; };
    const float* fov(void) const { return mFov; };
    int statisticsInitialSkip(void) const { return mStatisticsInitialSkip; };
    int frameInitialSkip(void) const { return mFrameInitialSkip; };
    const char* NVMFormat(void) const { return mNVMFormat; };

    int mSensorType;
    int mSensorFlipping;
    bool mExposureSync;
    bool mGainExposureComp;
    int mGainLag;
    int mExposureLag;
    float mFov[2];
    int mFrameInitialSkip;
    int mStatisticsInitialSkip;
    String8 mNVMFormat;

    KeyedVector<int, int> mGfxHalToV4L2PixelFmtTable;  // V4L2 pixel format corresponding to HAL format for the camera
};

} // namespace camera2
} // namespace android
#endif // _CAMERA3_HAL_CIFCAMERACAPINFO_H_
