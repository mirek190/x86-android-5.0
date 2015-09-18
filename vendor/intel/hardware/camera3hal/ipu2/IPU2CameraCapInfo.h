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

#ifndef _CAMERA3_HAL_IPU2CAMERACAPINFO_H_
#define _CAMERA3_HAL_IPU2CAMERACAPINFO_H_

#include <utils/Vector.h>
#include "PlatformData.h"

namespace android {
namespace camera2 {

#define POSTVIEW_SIZES_NUM 10
#define SDV_VIDEO_SIZES_NUM 20
#define PIPELINE_DEPTH_NUM 3

class IPU2CameraCapInfo;
static const IPU2CameraCapInfo * getIPU2CameraCapInfo(int cameraId)
{
    if (cameraId > MAX_CAMERAS) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }
    return (IPU2CameraCapInfo *)(PlatformData::getCameraCapInfo(cameraId));
}

class IPU2CameraCapInfo : public CameraCapInfo {
public:
    IPU2CameraCapInfo(SensorType type);
    ~IPU2CameraCapInfo() {};
    virtual int sensorType(void) const { return mSensorType; }
    virtual int getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat) const;
    int sensorLayout(void) const { return mSensorLayout; };
    int sensorFlipping(void) const { return mSensorFlipping; };
    bool isPreviewSizeSupportedByVFPP(int w, int h) const;
    bool isOfflineCaptureSupported(void) const;// { return (capInfo.maxContinuousRawRingBuffer > 0);};
    int maxContinuousRawRingBuffer(void) const { return mMaxContinuousRawRingBuffer; };
    int shutterLagCompensationMs(void) const { return mShutterLagCompensationMs; };
    bool enableIspPerframeSettings(void) const { return mEnableIspPerframeSettings; }
    bool streamConfigEnableRawLock(void) const { return mStreamConfigEnableRawLock; }
    const uint8_t * getPilelineDepths(int * count) const;
    bool snapshotResolutionSupportedByCVF(int w,int h) const;
    bool exposureSyncEnabled(void) const { return mExposureSync; };
    bool gainExposureCompEnabled(void) const { return mGainExposureComp; };
    int gainLag(void) const { return mGainLag; };
    int exposureLag(void) const { return mExposureLag; };
    const float* fov(void) const { return mFov; };
    int zoomDigitalMax(void) const { return mZoomDigitalMax; };
    int ispVamemType(void) const { return mIspVamemType; };
    const char* supportedHighSpeedResolutionFps(void) const { return mSupportedHighSpeedResolutionFps; };
    const char* maxHighSpeedDvsResolution(void) const { return mMaxHighSpeedDvsResolution; };
    int statisticsInitialSkip(void) const { return mStatisticsInitialSkip; };
    int frameInitialSkip(void) const { return mFrameInitialSkip; };
    bool supportPerFrameSettings(void) const { return mSupportPerFrameSettings; }
    const char* NVMFormat(void) const { return mNVMFormat; };
    const int * getPostViewSizes(int * count) const;
    const int * getSdvVideoSizes(int * count) const;
    int getMinDepthOfReqQ(void) const { return mMinDepthOfReqQ; }
    int getMaxBurstLength(void ) const { return mMaxBurstLength; }
    int getISOAnalogGain1(void ) const { return mISOAnalogGain1; }
    int supportSensorEmbeddedMetadata(void ) const { return mSupportSensorEmbeddedMetadata; }
    const char* subDeviceName(void) const { return mSubDeviceName; }
    bool supportFileInject(void) const { return mSupportFileInject; }
    int getStartExposureTime(void ) const { return mStartExposureTime; }
    int supportSdv(void ) const { return mSupportSdv; }
    int supportDynamicSdv(void ) const { return mSupportDynamicSdv; }

    String8 mSubDeviceName;
    bool mSupportFileInject;
    int mSensorType;
    int mSensorFlipping;
    int mSensorLayout;

    bool mExposureSync;
    bool mGainExposureComp;
    int mGainLag;
    int mExposureLag;
    float mFov[2];
    int mZoomDigitalMax;
    int mIspVamemType;
    String8 mSupportedHighSpeedResolutionFps;
    String8 mMaxHighSpeedDvsResolution;
    int mStatisticsInitialSkip;
    int mFrameInitialSkip;
    bool mSupportPerFrameSettings;
    int mPipelineDepthCount;
    uint8_t mPipelineDepths[PIPELINE_DEPTH_NUM];
    String8 mNVMFormat;
    int mPostviewSizeCount;
    int mPostviewSizes[POSTVIEW_SIZES_NUM];
    int mSdvVideoSizeCount;
    int mSdvVideoSizes[SDV_VIDEO_SIZES_NUM];

    int mMaxVFPPLimitedResolutions[2];

    int mMaxContinuousRawRingBuffer; // = 0 means ISP don't support continuous mode
    int mShutterLagCompensationMs;
    bool mEnableIspPerframeSettings;
    bool mStreamConfigEnableRawLock;
    Vector<Size> mCVFUnsupportedSnapshotResolutions;

    int mMinDepthOfReqQ;
    int mMaxBurstLength;
    int mISOAnalogGain1; //Pseudo ISO corresponding analog gain value 1.0.
    bool mSupportSensorEmbeddedMetadata;
    KeyedVector<int, int> mGfxHalToV4L2PixelFmtTable;  // V4L2 pixel format corresponding to HAL format for the camera
    int mStartExposureTime; // defualt exposure time got from cpf file
    bool mSupportSdv;
    bool mSupportDynamicSdv;
};

} // namespace camera2
} // namespace android
#endif // _CAMERA3_HAL_IPU2CAMERACAPINFO_H_
