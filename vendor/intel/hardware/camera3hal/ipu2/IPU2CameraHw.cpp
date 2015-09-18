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
#define LOG_TAG "Camera_IPU2CameraHw"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <linux/media.h>

#include "Aiq3A.h"
#include "Soc3A.h"

#include "LogHelper.h"
#include "PerformanceTraces.h"
#include "IPU2CameraHw.h"
#include "IaDvs2.h"
#include "ipu2/PreviewStream.h"
#include "ipu2/CaptureStream.h"
#include "ipu2/PostProcessStream.h"
#ifdef GRAPHIC_IS_GEN
#include <ufo/graphics.h>
#endif

#define BUFFERS_OF_ZSL_STREAM_CONSUMER 4
/*
 * ASPECT_TOLERANCE: the tolerance between aspect ratios to consider them the same
 */
#define ASPECT_TOLERANCE 0.001

namespace android {
namespace camera2 {

//#define GCAM_TEST

////////////////////////////////////////////////////////////////////
//                          PUBLIC METHODS
////////////////////////////////////////////////////////////////////

// Camera factory
ICameraHw *CreateCss2400Camera(int cameraId) {
    return new IPU2CameraHw(cameraId);
}

IPU2CameraHw::IPU2CameraHw(int cameraId) :
    mCameraId(cameraId)
    ,mGroupIndex(-1)
    ,mIPU2HwSensor(cameraId)
    ,mMode(MODE_NONE)
    ,mEnableRawLock(false)
    ,mInFlightCount(0)
    ,mPreviewHwStream(NULL)
    ,mCaptureHwStream(NULL)
    ,mVideoHwStream(NULL)
    ,mJpegStreamConfigured(false)
    ,mAvailableStreams(0)
    ,mBurstLength(0)
    ,mImgEncoder(NULL)
    ,mJpegMaker(NULL)
    ,m3AControls(NULL)
    ,mAAAProcessor(NULL)
    ,mLowLight(false)
    ,mXnr(0)
    ,mRawDataDumpSize(-1)
    ,mFrameSyncRequested(-1)
    ,m3AStatRequested(-1)
    ,mFrameSyncEnabled(false)
    ,m3AStatscEnabled(false)
    ,mNoiseReductionEdgeEnhancement(false)
    ,mMaxPreviewWidth(0)
    ,mMaxPreviewHeight(0)
    ,mFaceDetectionThread(NULL)
    ,mMaxRegions(NULL)
{
    LOG2("@%s", __FUNCTION__);
    CLEAR(mStreamConfig);
    CLEAR(mMaxJpegConfig);
}

status_t IPU2CameraHw::init(void)
{
    status_t status = NO_ERROR;
    int count = 0;

    Mutex::Autolock lock(mISPCountLock);
    for (int i = 0; i < MAX_CAMERAS; i++) {
        if (mHwIsp->getDevName(i)->in_use == false) {
            mGroupIndex = i;
            mHwIsp->getDevName(i)->in_use = true;
            break;
        }
    }

    if (mGroupIndex < 0) {
        LOGE("No free device. Inititialize Atomisp failed.");
        return NO_INIT;
    }

    mCapInfo = getIPU2CameraCapInfo(mCameraId);
    mStaticMeta = PlatformData::getStaticMetadata(mCameraId);
    getMaxSnapshotResolution();
    getMaxPreviewResolution();
    if (mCapInfo)
        mBurstLength = mCapInfo->getMaxBurstLength();
    mAvailableStreams = 0;

    /**
     * Check the consistency of the information we had in XML file.
     * partial result count is very tightly linked to the PSL
     * implementation
     */
    int xmlPartialCount = PlatformData::getPartialMetadataCount(mCameraId);
    if (xmlPartialCount != PARTIAL_RESULT_COUNT) {
            LOGW("Partial result count does not match current implementation "
                 " got %d should be %d, fix the XML!", xmlPartialCount,
                 PARTIAL_RESULT_COUNT);
            return NO_INIT;
    }

    const CameraCapInfo * cap = PlatformData::getCameraCapInfo(mCameraId);
    if (!cap || cap->sensorType() == SENSOR_TYPE_RAW) {
        m3AControls = new Aiq3A(mCameraId);
    } else {
        m3AControls = new Soc3A(mCameraId);
    }

    status = initDevice();
    if (status != NO_ERROR) {
        return NO_INIT;
    }

    status = init3A();
    if (status != NO_ERROR) {
        return NO_INIT;
    }

    status = initDeviceStreams();
    if (status != NO_ERROR) {
        return NO_INIT;
    }

    mImgEncoder = new ImgEncoder(mCameraId);
    if (mImgEncoder.get() == NULL) {
        LOGE("Error creating ImgEncoder");
        return NO_INIT;
    }
    status = mImgEncoder->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init ImgEncoder!");
        mImgEncoder.clear();
        return NO_INIT;
    }

    mJpegMaker = new JpegMaker(mCameraId);
    if (mJpegMaker == NULL) {
        LOGE("Error creating JpegMaker");
        return NO_INIT;
    }
    status = mJpegMaker->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init JpegMaker!");
        delete mJpegMaker;
        mImgEncoder.clear();
        return NO_INIT;
    }
    mFaceDetectionThread = new FaceDetectionThread(mStaticMeta, mAAAProcessor, mCameraId);
    if (mFaceDetectionThread == NULL)  {
        LOGE("create facedetection thread failed");
        return NO_INIT;
    }
    mMaxRegions = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                               ANDROID_CONTROL_MAX_REGIONS,
                               TYPE_INT32,
                               &count);
    return status;
}

IPU2CameraHw::~IPU2CameraHw()
{
    LOG2("@%s", __FUNCTION__);
    Mutex::Autolock lock(mISPCountLock);
    mHwIsp->getDevName(mGroupIndex)->in_use = false;
    /*
     * The destructor is called when the hw_module close mehod is called. The close method is called
     * in general by the camera client when it's done with the camera device, but it is also called by
     * System Server when the camera application crashes. System Server calls close in order to release
     * the camera hardware module. So, if we are not in MODE_NONE, it means that we are in the middle of
     * something when the close function was called. So it's our duty to stop first, then close the
     * camera device.
     */
    if (mMode != MODE_NONE)
        flush();

    if (mFaceDetectionThread) {
        mFaceDetectionThread->requestExitAndWait();
        delete mFaceDetectionThread;
        mFaceDetectionThread = NULL;
    }
    if (mImgEncoder != NULL && mImgEncoder.get() != NULL) {
        mImgEncoder.clear();
    }

    if (mJpegMaker != NULL) {
        delete mJpegMaker;
    }
    destroy3A();
    destroyDeviceStreams();

    mMainDevice->close();

    destroyDevice();
    // clear the sp to the devices to destroy the objects.

}

const camera_metadata_t * IPU2CameraHw::getDefaultRequestSettings(int type)
{
    isp_metadata ispMetadata;
    camera_metadata_t *metaData = PlatformData::getDefaultMetadata(mCameraId, type);
    mHwIsp->fillDefaultMetadataFromISP(&ispMetadata);
    status_t ret = contructDefaultDynamicMetadata(type, metaData, &ispMetadata);
    return metaData;
}

status_t IPU2CameraHw::contructDefaultDynamicMetadata(int type, camera_metadata_t *metadata, isp_metadata *ispMetadata)
{
    status_t ret = NO_ERROR;
    //Fill dynamic ISP metadata
    MetadataHelper::updateMetadata(metadata, ANDROID_LENS_FOCAL_LENGTH, &ispMetadata->focal_length, 1);
    // fill lens aperture
    int count = 0;
    float* lensAperture = (float*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                                             TYPE_FLOAT,
                                             &count);

    if (count >= 1) {
        MetadataHelper::updateMetadata(metadata, ANDROID_LENS_APERTURE, lensAperture, 1);
    }
    // In video mode the fps should be fixed.
    if (type == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD) {
        for (int i = 0; i < FPS_RANGES_LEN; i++) {
        if(ispMetadata->fps_range[2 * i] == ispMetadata->fps_range[2 * i + 1])
            ispMetadata->fps_range[0] = ispMetadata->fps_range[2 * i];
            ispMetadata->fps_range[1] = ispMetadata->fps_range[2 * i];
        }
    }
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AE_TARGET_FPS_RANGE,ispMetadata->fps_range,2);
    //Fill dynamic 3A metadata
    uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    uint8_t af_mode = ANDROID_CONTROL_AF_MODE_AUTO;
    uint8_t awb_mode = ANDROID_CONTROL_AWB_MODE_AUTO;
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;

    if (mCameraId == 0) {
        if (type == ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW || type == ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG
            || type == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) {
            af_mode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        } else if (type == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD
                   || type == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
            af_mode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        } else if (type == ANDROID_CONTROL_CAPTURE_INTENT_MANUAL) {
            af_mode = ANDROID_CONTROL_AF_MODE_OFF;
        }
    }

    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AF_MODE, &af_mode, 1);


    if (type == ANDROID_CONTROL_CAPTURE_INTENT_MANUAL) {
        awb_mode = ANDROID_CONTROL_AWB_MODE_OFF;
        controlMode = ANDROID_CONTROL_MODE_OFF;
        aeMode = ANDROID_CONTROL_AE_MODE_OFF;
    }
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_MODE, &controlMode, 1);
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AWB_MODE, &awb_mode, 1);
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AE_MODE, &aeMode, 1);

    uint8_t antibanding_mode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AE_ANTIBANDING_MODE, &antibanding_mode, 1);

    uint8_t aePreCaptureTrigger = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePreCaptureTrigger, 1);

    uint8_t afTriger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AF_TRIGGER, &afTriger, 1);

    int64_t bogusValueArray[] = {0, 0, 0, 0, 0};

    if(1 == mMaxRegions[0])
        MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AE_REGIONS, bogusValueArray, 5);
    if(1 == mMaxRegions[2])
        MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AF_REGIONS, bogusValueArray, 5);
    if(1 == mMaxRegions[1])
        MetadataHelper::updateMetadata(metadata, ANDROID_CONTROL_AWB_REGIONS, bogusValueArray, 5);

    //others
    int32_t reqId = 0;
    MetadataHelper::updateMetadata(metadata, ANDROID_REQUEST_ID, &reqId, 1);

    uint8_t edgeMode = ANDROID_EDGE_MODE_FAST;
    MetadataHelper::updateMetadata(metadata, ANDROID_EDGE_MODE, &edgeMode, 1);

    uint8_t noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
    MetadataHelper::updateMetadata(metadata, ANDROID_NOISE_REDUCTION_MODE, &noiseReductionMode, 1);

    uint8_t toneMapMode = ANDROID_TONEMAP_MODE_FAST;
    MetadataHelper::updateMetadata(metadata, ANDROID_TONEMAP_MODE, &toneMapMode, 1);

    uint8_t testPatternMode = ANDROID_SENSOR_TEST_PATTERN_MODE_OFF;
    MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_TEST_PATTERN_MODE, &testPatternMode, 1);

    uint8_t pixelMapMode = ANDROID_HOT_PIXEL_MODE_OFF;
    MetadataHelper::updateMetadata(metadata, ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &pixelMapMode, 1);

    uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    MetadataHelper::updateMetadata(metadata, ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, &lensShadingMapMode, 1);
    // for FULL mode
    // get the supportHardwareLevel
    int* supportHardwareLevel = (int*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
                                             TYPE_BYTE,
                                             &count);
    if (count > 0 && supportHardwareLevel != NULL
        && supportHardwareLevel[0] == ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL) {
       // sensor frameDuration
       int64_t frameDuration = 33333333;
       MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);
       //black level lock
       uint8_t blackLevelLock = ANDROID_BLACK_LEVEL_LOCK_OFF;
       MetadataHelper::updateMetadata(metadata, ANDROID_BLACK_LEVEL_LOCK, &blackLevelLock, 1);
       // shading mode
       uint8_t shadingMode = ANDROID_SHADING_MODE_FAST;
       MetadataHelper::updateMetadata(metadata, ANDROID_SHADING_MODE, &shadingMode, 1);
       // hotpixel mode
       uint8_t hotPixelMode = ANDROID_HOT_PIXEL_MODE_OFF;
       MetadataHelper::updateMetadata(metadata, ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
       // lensFilterDensity
       float lensFilterDensity = 0.0;
       MetadataHelper::updateMetadata(metadata, ANDROID_LENS_FILTER_DENSITY, &lensFilterDensity, 1);
       // colorCorrectionAberrationMode
       uint8_t colorCorrectionAberrationMode = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF;
       MetadataHelper::updateMetadata(metadata, ANDROID_COLOR_CORRECTION_ABERRATION_MODE, &lensFilterDensity, 1);
   }

    return ret;
}

void IPU2CameraHw::getMaxSnapshotResolution()
{
    int count = 0;
    int32_t * sizes = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                     ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
                                     TYPE_INT32,
                                     &count);
    if (sizes == NULL) {
        LOGE("there is no jpeg size");
        return;
    }
    CLEAR(mMaxJpegConfig);
    mMaxJpegConfig.originalFormat = mMaxJpegConfig.format =
        v4L2Fmt2GFXFmt(PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, mCameraId), mCameraId);
    for (int i = 0; i < count-1; i += 2) {
        if ((sizes[i] > mMaxJpegConfig.width)
           || (sizes[i + 1] > mMaxJpegConfig.height)) {
            mMaxJpegConfig.width = sizes[i];
            mMaxJpegConfig.height = sizes[i + 1];
        }
    }
    LOG2("%s: %dx%d", __FUNCTION__, mMaxJpegConfig.width, mMaxJpegConfig.height);
}

void IPU2CameraHw::getMaxPreviewResolution()
{
    int count = 0;
    int32_t * sizes = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                     ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
                                     TYPE_INT32,
                                     &count);
    if (sizes == NULL || count < 2) {
        LOGE("No available processed sizes, keep the default value.");
        mMaxPreviewWidth = 1920;
        mMaxPreviewHeight = 1080;
    } else {
        mMaxPreviewWidth = 0;
        mMaxPreviewHeight = 0;
        for (int i = 0; i < count-1; i += 2) {
            if (sizes[i] * sizes[i+1] > mMaxPreviewWidth * mMaxPreviewHeight) {
                mMaxPreviewWidth  = sizes[i];
                mMaxPreviewHeight = sizes[i + 1];
            }
        }
    }
    LOG2("%s: %dx%d", __FUNCTION__, mMaxPreviewWidth, mMaxPreviewHeight);
}

void IPU2CameraHw::getMaxCaptureInfoWithRatio(FrameInfo *captureInfo, FrameInfo cInfo)
{
    int count = 0;
    float cInfoAspectRatio = 0.0f;
    float captureAspectRatio = 0.0f;
    bool isSupported = false;
    int32_t * supportSizes = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                    ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
                                    TYPE_INT32,
                                    &count);
    if (supportSizes == NULL) {
        LOGE("@%s, there is no jpeg size, use the full resolution", __FUNCTION__);
        *captureInfo = mMaxJpegConfig;
        return;
    }
    cInfoAspectRatio = 1.0 * cInfo.width / cInfo.height;
    captureInfo->originalFormat = captureInfo->format =
        v4L2Fmt2GFXFmt(PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, mCameraId),
        mCameraId);

    //Get the max resolution which the aspect ratio is same with other stream.
    for (int i = 0; i < count-1; i += 2) {
        captureAspectRatio = 1.0 * supportSizes[i] / supportSizes[i + 1];
        if (((supportSizes[i] > captureInfo->width) ||
            (supportSizes[i + 1] > captureInfo->height)) &&
            (fabsf(captureAspectRatio - cInfoAspectRatio) <= ASPECT_TOLERANCE)) {
            isSupported = true;
            captureInfo->width = supportSizes[i];
            captureInfo->height = supportSizes[i + 1];
        }
    }
    if (!isSupported) {
        LOGW("@%s, there isn't same ratio size, use the full resolution", __FUNCTION__);
        *captureInfo = mMaxJpegConfig;
    }
    LOG2("@%s, The capture size is %dx%d", __FUNCTION__, captureInfo->width, captureInfo->height);
}

status_t IPU2CameraHw::initDevice()
{
    status_t status = NO_ERROR;

    mMainDevice.clear();
    mPreviewDevice.clear();
    mVideoDevice.clear();
    mPostviewDevice.clear();
    mIspSubdevice.clear();
    m3AEventSubdevice.clear();
    mFileInjectDevice.clear();
    mHwIsp.clear();

    mMainDevice = new V4L2VideoNode(mHwIsp->getDevName(mGroupIndex)->dev[V4L2_MAIN_DEVICE]);
    mPreviewDevice = new V4L2VideoNode(mHwIsp->getDevName(mGroupIndex)->dev[V4L2_PREVIEW_DEVICE]);
    mVideoDevice = new V4L2VideoNode(mHwIsp->getDevName(mGroupIndex)->dev[V4L2_RECORDING_DEVICE]);
    mPictureDevice = mMainDevice;
    mPostviewDevice = new V4L2VideoNode(mHwIsp->getDevName(mGroupIndex)->dev[V4L2_POSTVIEW_DEVICE]);
    mIspSubdevice = new V4L2Subdevice(mCapInfo->subDeviceName());
    m3AEventSubdevice = new V4L2Subdevice(mCapInfo->subDeviceName());
    mFileInjectDevice = new V4L2VideoNode(mHwIsp->getDevName(mGroupIndex)->dev[V4L2_INJECT_DEVICE],
                                          V4L2VideoNode::OUTPUT_VIDEO_NODE);

    HWControlGroup hwcg;
    hwcg.mSensorCI = &mIPU2HwSensor;
    mHwIsp = new IPU2HwIsp(mMainDevice, mPreviewDevice, mVideoDevice, mIspSubdevice, hwcg, m3AControls);
    if(mHwIsp == NULL) {
       LOGE("Could not allocate CSS ISP class");
       return NO_MEMORY;
    }

    status = mHwIsp->init(mCameraId);
    if (status != NO_ERROR) {
        LOGE("Failed to init ISP!");
        return NO_INIT;
    }
    PERFORMANCE_TRACES_BREAKDOWN_STEP("Open_Main_Device");

    status = mIPU2HwSensor.selectActiveSensor(mMainDevice);
    if (status != NO_ERROR) {
        LOGE("Failed to select active sensor!");
        return NO_INIT;
    }

    // Select the input port to use
    status = mHwIsp->initCameraInput();
    if (status != NO_ERROR) {
        LOGE("Unable to initialize camera input %d", mCameraId);
        return NO_INIT;
    }

    return status;
}

void IPU2CameraHw::destroyDevice()
{
    mHwIsp.clear();

    mMainDevice.clear();
    mPreviewDevice.clear();
    mPostviewDevice.clear();
    mVideoDevice.clear();
    m3AEventSubdevice.clear();
    mIspSubdevice.clear();
    mFileInjectDevice.clear();
}

status_t IPU2CameraHw::initDeviceStreams()
{
    status_t status = NO_ERROR;

    mPreviewHwStream = new PreviewStream(mPreviewDevice, mHwIsp, &mIPU2HwSensor, mCameraId, PREVIEW_STREAM_NAME);
    if(mPreviewHwStream == NULL) {
       LOGE("Could not allocate PreviewStream");
       return NO_MEMORY;
    }

    mPreviewHwStream->DebugStreamFps("PREVIEW_FPS");
    status = mPreviewHwStream->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init Preview Stream!");
        return NO_INIT;
    }

    mHwIsp->setDeviceStreams(mPreviewHwStream,
                             ICssBufferMaintainer::DEVICE_STREAM_TYPE_PREVIEW);

    mVideoHwStream = new PreviewStream(mVideoDevice, mHwIsp, &mIPU2HwSensor, mCameraId, VIDEO_STREAM_NAME);
    if(mVideoHwStream == NULL) {
       LOGE("Could not allocate VideoStream");
       return NO_MEMORY;
    }

    mVideoHwStream->DebugStreamFps("VIDEO_FPS");
    status = mVideoHwStream->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init video Stream!");
        return NO_INIT;
    }

    mHwIsp->setDeviceStreams(mVideoHwStream,
                             ICssBufferMaintainer::DEVICE_STREAM_TYPE_VIDEO);

    mCaptureHwStream = new CaptureStream(mPictureDevice, mPostviewDevice, mHwIsp, &mIPU2HwSensor,
                                             mAAAProcessor, mCameraId, CAPTURE_STREAM_NAME);
    if(mCaptureHwStream == NULL) {
       LOGE("Could not allocate CaptureStream");
       return NO_MEMORY;
    }

    mCaptureHwStream->DebugStreamFps("CAPTURE_FPS");
    status = mCaptureHwStream->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init Capture Stream!");
        return NO_INIT;
    }

    mHwIsp->setDeviceStreams(mCaptureHwStream,
                             ICssBufferMaintainer::DEVICE_STREAM_TYPE_CAPTURE);

    return NO_ERROR;
}

void IPU2CameraHw::destroyDeviceStreams()
{
    LOG1("%s",__FUNCTION__);
    if(mCaptureHwStream != NULL) {
       delete mCaptureHwStream;
       mCaptureHwStream = NULL;
    }

    if(mVideoHwStream != NULL) {
       delete mVideoHwStream;
       mVideoHwStream = NULL;
    }

    if (mPreviewHwStream != NULL) {
       delete mPreviewHwStream;
       mPreviewHwStream = NULL;
    }

    mPostStreams.clear();
}

status_t IPU2CameraHw::init3A()
{
    LOG1("%s",__FUNCTION__);

    if (m3AControls == NULL) {
        LOGE("Error create 3AControl");
        return UNKNOWN_ERROR;
    }

    HWControlGroup hwcg;
    hwcg.mIspCI = mHwIsp.get();
    hwcg.mSensorCI = &mIPU2HwSensor;
    hwcg.mFlashCI = mHwIsp.get();
    hwcg.mLensCI = mHwIsp.get();
    if (m3AControls->attachHwControl(hwcg) != NO_ERROR) {
        LOGE("Error attach HW group");
        return UNKNOWN_ERROR;
    }

    if (m3AControls->init3A() != NO_ERROR) {
        LOGE("Error initializing 3A controls");
        return UNKNOWN_ERROR;
    }

    mAAAProcessor = new Aiq3AThread(this, NULL, m3AControls, hwcg, mStaticMeta);
    if (mAAAProcessor == NULL) {
        LOGE("Error create RequestThread");
        return UNKNOWN_ERROR;
    }
    mHwIsp->attachListener(&mIPU2HwSensor, ICssIspListener::ISP_EVENT_TYPE_SOF);
    mHwIsp->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_SOF);
    mHwIsp->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY);
    mHwIsp->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY);
    PERFORMANCE_TRACES_BREAKDOWN_STEP("Done");

    return NO_ERROR;
}

void IPU2CameraHw::destroy3A()
{
    mHwIsp->detachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_SOF);
    mHwIsp->detachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY);
    mHwIsp->detachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY);
    mHwIsp->detachListener(&mIPU2HwSensor, ICssIspListener::ISP_EVENT_TYPE_SOF);

    if (mAAAProcessor) {
        mAAAProcessor->requestExitAndWait();
        delete mAAAProcessor;
        mAAAProcessor = NULL;
    }
    if (m3AControls) {
        m3AControls->deinit3A();
        delete m3AControls;
    }
}

status_t IPU2CameraHw::processRequest(Camera3Request *request, int inFlightCount)
{
    status_t status = NO_ERROR;
    mInFlightCount = inFlightCount;
    bool needReStart = false;
    // Capture without preview
    bool needCheckBlock = (((mAvailableStreams | STREAM_CAPTURE) == STREAM_CAPTURE)
                           && ((mAvailableStreams & STREAM_PREVIEW) == 0)
                           && (mStreamConfig.mCfgMode == CONFIG_CONTINUOUS
                           || mStreamConfig.mCfgMode == CONFIG_CONTINUOUS_ZSL));
    {
    const camera3_capture_request *req3 = request->getUserRequest();
    ReadWriteRequest rwRequest(request);
    Camera3Request::Members & members = rwRequest.mMembers;
    LOG2("@%s: <Request %d>", __FUNCTION__, request->getId());

    if (req3->settings != NULL) {
        setRequestedStreams(request);
        StreamConfigMode cfgMode = analyzeConfigMode(members.mSettings);
        if (cfgMode == CONFIG_NONE) {
            return BAD_VALUE;
        }

        StreamConfig newCfg;
        needReStart = analyzeStreams(cfgMode, newCfg);
        LOG2("@%s: <Request %d> Isp mode %d -> %d, restart? %d", __FUNCTION__,
                request->getId(), mMode, newCfg.mIspMode, needReStart);
        if (inFlightCount > 1 && needReStart) {
            return WOULD_BLOCK;
        }
        // If inFlightCount is 1, it is possible that the requests are not sent
        // continuously. As the sensor settings takes effect 2 frames later,
        // the first 2 frames are invalid with the unexpected settings. This
        // happens a lot in capture mode, like ITS cases.
        // So here we make a workaround before we find a better solution.
        // When mode is capture and inFlightCount is 1, restart.
        if (inFlightCount == 1 && mMode == MODE_CAPTURE)
            needReStart = true;

        if (!needReStart && needCheckBlock) {
            // When switch from capture (without preview) to preview,
            // the first preview request might be finished before the last
            // capture request.
            // And the first preview request might get the same expected exp_id
            // as one capture request (especially in burst capture).
            // So block the first preview request to avoid error.
            needCheckBlock = ((mStreamConfig.mCfgMode == CONFIG_CONTINUOUS
                           || mStreamConfig.mCfgMode == CONFIG_CONTINUOUS_ZSL)
                           && (mOutYuvStreams.size() && newCfg.mPreview == mOutYuvStreams[0]));

            if (inFlightCount > 1 && needCheckBlock) {
                LOG2("%s: block when switch capture (without preview) to preview",
                    __FUNCTION__);
                return WOULD_BLOCK;
            }
        }

        if (needReStart) {
            status = restart(newCfg, members.mSettings);
            mStreamConfig = newCfg;
        } else
            mHwIsp->processSettings(members.mSettings);

       checkCaptureWithoutPreview(mStreamConfig);
    }
    }

    if (status == NO_ERROR) {
        // Check whether preview/capture/video available or not
        mAvailableStreams = 0;
        for (unsigned int i = 0; i < mOutStreams.size(); i++) {
            if (mStreamConfig.mPreview && mOutStreams[i] == mStreamConfig.mPreview)
                mAvailableStreams |= STREAM_PREVIEW;
            if (mStreamConfig.mCapture && mOutStreams[i] == mStreamConfig.mCapture)
                mAvailableStreams |= STREAM_CAPTURE;
            if (mStreamConfig.mVideo && mOutStreams[i] == mStreamConfig.mVideo)
                mAvailableStreams |= STREAM_VIDEO;
        }

        mAAAProcessor->processRequest(request, mAvailableStreams);
        mFaceDetectionThread->processRequest(request);

        // skip frame(s) if necessary
        unsigned int initialSkips = 0;
        if (needReStart) {
            switch (mMode) {
            case MODE_PREVIEW:
            case MODE_CAPTURE:
            case MODE_CONTINUOUS_CAPTURE:
                // For preview and capture, as sensor settings take effect
                // after several frames, need skip frames to make sensor
                // settings take effect per-frame.
                initialSkips = mCapInfo->exposureLag();
                break;
            case MODE_VIDEO:
            case MODE_CONTINUOUS_VIDEO:
                // For video mode, DVS and zoom require skipping a few
                // frames. The skip number can be get from cap info.
                initialSkips = mCapInfo->frameInitialSkip();
                break;
            default:
                LOG2("No need to skip frame for mode %d", mMode);
                break;
            }
        } else if (inFlightCount == 1 &&
                   request->getUserRequest()->settings != NULL &&
                   mAvailableStreams & STREAM_CAPTURE) {
            // If inFlightCount is 1, it is possible that requests are not sent
            // continuously. As the sensor settings takes effect 2 frames later,
            // the first 2 frames are invalid with the unexpected settings. This
            // happens at the first payload request in Gcam.
            // In such case we should insert some fake buffers.
            // workaround: add one more fake buffer because in some case the ISP
            //             can't return the frame even EOF arrived
            initialSkips = mCapInfo->exposureLag() + 1;
            LOG2("It maybe the first reqeust of payload, insert %d fake buffers", initialSkips);
        }

        // when capture image with flash needed, it will work in online capture mode.
        // when 1 yuv image is captured, sensor already output several frames.
        // so 1 fake bufer is enough.
        if (mAAAProcessor->isPreFlashUsed() && mMode == MODE_CAPTURE)
            initialSkips = 1;

        while (initialSkips > 0) {
            if (mMode == MODE_PREVIEW || mMode == MODE_CONTINUOUS_CAPTURE) {
                mPreviewHwStream->capture(NULL, NULL);
            } else if (mMode == MODE_VIDEO || mMode == MODE_CONTINUOUS_VIDEO) {
                mVideoHwStream->capture(NULL, NULL);
                mPreviewHwStream->capture(NULL, NULL);
            } else if (mMode == MODE_CAPTURE) {
                mCaptureHwStream->capture(NULL, NULL);
            }
            initialSkips--;
        }

        mFakeStreamMgr.processRequest(request);
    }

    return status;
}

status_t IPU2CameraHw::restart(StreamConfig & cfg, const CameraMetadata &settings)
{
    stop();
    processIspSettings(cfg.mIspMode, settings);
    status_t status = configureIsp(cfg.mIspMode, cfg.mEnableRawLock);
    if (status == NO_ERROR) {
        mFakeStreamMgr.start(cfg.mFakeStream);
        status = bindStreams(cfg);
    }
    if (status == NO_ERROR) {
        //zoomRatio value should be re-set after setFormat.
        status |= mHwIsp->reSetZoomRatio();
        if (m3AControls->switchModeAndRate(mMode, mIPU2HwSensor.getFrameRate()) != NO_ERROR)
            LOGE("ERROR @%s: Failed switching 3A at %.2f fps",
                __FUNCTION__, mIPU2HwSensor.getFrameRate());
    }
    status |= mAAAProcessor->reset(mMode);
    return status;
}

status_t IPU2CameraHw::checkCaptureWithoutPreview(StreamConfig & config)
{
    for (unsigned int i = 0; i < mOutStreams.size(); i++) {
        if (config.mPreview == mOutStreams[i]) {
            LOG2("@%s: capture with preview", __FUNCTION__);
            mCaptureHwStream->enableCaptureWithoutPreview(false);
            return NO_ERROR;
        }
    }
    LOG2("@%s: capture without preview", __FUNCTION__);
    mCaptureHwStream->enableCaptureWithoutPreview(true);
    return NO_ERROR;
}

status_t IPU2CameraHw::flush()
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    stop();
    return NO_ERROR;
}

status_t IPU2CameraHw::stop()
{
    LOG2("@%s current mode:%d", __FUNCTION__, mMode);
    if (mMode == MODE_NONE)
        return NO_ERROR;

    // Stop ISP
    mHwIsp->stop();
    ContinuousCaptureConfig cfg;
    switch (mMode) {
    case MODE_PREVIEW:
        mPreviewHwStream->stop();
        break;
    case MODE_CAPTURE:
        mCaptureHwStream->stop();
        break;
    case MODE_VIDEO:
        mPreviewHwStream->stop();
        mVideoHwStream->stop();
        break;
    case MODE_CONTINUOUS_CAPTURE:
        mHwIsp->requestContCapture(0,0,0);
        mCaptureHwStream->stop();
        mPreviewHwStream->stop();
        break;
    case MODE_CONTINUOUS_VIDEO:
        mHwIsp->requestContCapture(0,0,0);
        mCaptureHwStream->stop();
        mPreviewHwStream->stop();
        mVideoHwStream->stop();
        break;
    default:
        break;
    }

    mFakeStreamMgr.stop();

    mHwIsp->setMode(IPU2HwIsp::CSS_ISP_MODE_NONE);
    cfg.numCaptures = 0;
    cfg.offset = 0;
    cfg.skip = 0;
    mHwIsp->configureContinuousMode(cfg, false);
    mMode = MODE_NONE;

    CLEAR(mStreamConfig);

    // deattach listener
    mPreviewHwStream->detachListener();
    mCaptureHwStream->detachListener();
    mVideoHwStream->detachListener();
    mIPU2HwSensor.setCaptureSyncOwner(NULL);
    mIPU2HwSensor.clearRequestQueue();

    //unbind
    LOG2("%s: ------------ unbind -----------------", __FUNCTION__);
    CameraStreamNode *streamNode;
    for (unsigned int i = 0; i < mActiveStreams.size(); i++) {
        streamNode = mActiveStreams.editItemAt(i);
        CameraStream::unbind(streamNode, NULL);
    }

    return NO_ERROR;
}

status_t IPU2CameraHw::configStreams(Vector<camera3_stream_t*> &activeStreams)
{
    LOG1("@%s", __FUNCTION__);
    uint32_t maxBufs, usage;
    int32_t count ;

    int32_t *availStreamConfig = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                 ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                                 TYPE_INT32,
                                 &count);
    if (availStreamConfig == NULL) {
        ALOGE("Cannot get stream configuration from static metadata.");
        return BAD_VALUE;
    }
    // check the stream size reasonable
    for (uint32_t i = 0; i < activeStreams.size(); i++) {
        for (uint32_t j = 0; j < (uint32_t)count; j = j + 4) {
            if ((activeStreams[i]->width == (uint32_t)availStreamConfig[j + 1]) &&
                    (activeStreams[i]->height == (uint32_t)availStreamConfig[j + 2])) {
                break;
            }
            // match size is not found
            if (j == (uint32_t)(count - 4)) {
                ALOGE("Unknown stream size %dx%d!", activeStreams[i]->width, activeStreams[i]->height);
                return BAD_VALUE;
            }
        }
    }

    // The real burst length is limited by memory
    int numOfStreamsWithFullSize = 0;
    int fullSize = mMaxJpegConfig.width * mMaxJpegConfig.height * 0.9;
    int hasZslStream = false;
    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        if (activeStreams[i]->width * activeStreams[i]->height >= (unsigned int)fullSize)
            numOfStreamsWithFullSize++;
        if (activeStreams[i]->stream_type != CAMERA3_STREAM_OUTPUT)
            hasZslStream = true;
    }
    // Limit burst length if zsl stream is configured
    if (numOfStreamsWithFullSize == 1 && hasZslStream)
        numOfStreamsWithFullSize++;

    // because the current design uses a hardcoded request queue size, the
    // max_buffers needs to match it. A bigger number would mean extra
    // allocations which can never get into the HAL at the same time, and
    // a smaller number would risk violating the max_buffers rule in camera.3h
    maxBufs = mCapInfo->getMaxBurstLength();
    if (numOfStreamsWithFullSize)
        maxBufs /= numOfStreamsWithFullSize;
    if (maxBufs < (unsigned int)mCapInfo->getMinDepthOfReqQ())
        maxBufs = (unsigned int)mCapInfo->getMinDepthOfReqQ();
    if (maxBufs > MAX_REQUEST_IN_PROCESS_NUM)
        maxBufs = MAX_REQUEST_IN_PROCESS_NUM;

    /**
     * Here we could give different gralloc flags depending on the stream
     * format, at the moment we give the same to all
     */
    usage = GRALLOC_USAGE_SW_READ_OFTEN |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_HW_CAMERA_READ;

    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        activeStreams[i]->max_buffers = maxBufs;
        activeStreams[i]->usage |= usage;
        /**
         * ZSL stream buffer won't be filled with real raw data in currently flow,
         * and it costs over 10ms to allocate a 5M pixel big buffer in gralloc
         * so set it with flag and tell graphic to allocate a small size buffer.
         */
        if (activeStreams[i]->stream_type == CAMERA3_STREAM_BIDIRECTIONAL)
            activeStreams[i]->usage |= GRALLOC_USAGE_PRIVATE_0;

        LOGD("w=%d,h=%d,format=%x", activeStreams[i]->width,
              activeStreams[i]->height, activeStreams[i]->format);
    }
    mBurstLength = maxBufs;
    return NO_ERROR;
}

status_t IPU2CameraHw::bindStreams(Vector<CameraStreamNode *> activeStreams)
{
    LOG2("@%s: %d", __FUNCTION__, activeStreams.size());
    FrameInfo info;
    CameraStreamNode* stream;
    mActiveStreams.clear();
    mJpegStreamConfigured = false;

    CameraStreamNode* jpegStream = NULL;
    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        stream = activeStreams.editItemAt(i);
        stream->query(&info);
        // To get JPEG size in advance for MODE_CONTINUOUS_CAPTURE
        if (info.format == HAL_PIXEL_FORMAT_BLOB) {
            jpegStream = stream;
            continue;
        } else if (info.format == HAL_PIXEL_FORMAT_RAW_SENSOR) {
            // TODO:
            continue;
        }

        // Sort by resolution, descending order
        FrameInfo tmpInfo;
        bool saved = false;
        for (unsigned int j = 0; j < mActiveStreams.size(); j++) {
            CameraStreamNode * tmp = mActiveStreams.editItemAt(j);
            tmp->query(&tmpInfo);
            if (info.width * info.height > tmpInfo.width * tmpInfo.height) {
                 mActiveStreams.insertAt(stream, j);
                 saved = true;
                 break;
             }
        }
        if (!saved)
             mActiveStreams.push_back(stream);
    }

    // Make sure that the jpegStream always be the first one of the vector for later use.
    if (jpegStream) {
        mJpegStreamConfigured = true;
        mActiveStreams.push_front(jpegStream);
    }

    stop();
    mFakeStreamMgr.clearFakeStream();

    LOG2("%s: ------------ dump %d-----------------", __FUNCTION__, activeStreams.size());
    for (unsigned int i = 0; i < mActiveStreams.size(); i++) {
        stream = mActiveStreams.editItemAt(i);
        FrameInfo info;
        mActiveStreams.editItemAt(i)->query(&info);
        LOG2("    %dx%d(0x%x)", info.width, info.height, info.format);
    }

    return NO_ERROR;
}

status_t IPU2CameraHw::processIspSettings(IspMode mode, const CameraMetadata &settings)
{
    if(mode == MODE_VIDEO && mOutYuvStreams.size()) {
        FrameInfo info;
        mOutYuvStreams.editItemAt(0)->query(&info);
        mHwIsp->setVideoInfo(&info);
    }
    mHwIsp->processSettings(settings);
    return NO_ERROR;
}

status_t IPU2CameraHw::configureIsp(IspMode mode, bool enableRawLock)
{
    LOG2("%s: mode %d", __FUNCTION__, mode);
    ContinuousCaptureConfig cfg;
    cfg.numCaptures = 1;
    cfg.offset = 1;
    cfg.skip = 0;

    status_t status = NO_ERROR;
    mPreviewHwStream->setMode(mode);
    mVideoHwStream->setMode(mode);
    switch (mode) {
    case MODE_PREVIEW:
        status = mHwIsp->setMode(IPU2HwIsp::CSS_ISP_MODE_PREVIEW);
        break;
    case MODE_CAPTURE:
        status = mHwIsp->setMode(IPU2HwIsp::CSS_ISP_MODE_CAPTURE);
        break;
    case MODE_VIDEO:
        status = mHwIsp->setMode(IPU2HwIsp::CSS_ISP_MODE_VIDEO);
        break;
    case MODE_CONTINUOUS_CAPTURE:
        status = mHwIsp->configureContinuousMode(cfg, true, true);
        if (status == NO_ERROR) {
            // the consumer need 4 buffers locked.
            status = mHwIsp->configureContinuousRingBuffer(
                        mBurstLength + BUFFERS_OF_ZSL_STREAM_CONSUMER,
                        enableRawLock);
        }
        if (status == NO_ERROR) {
            // it would be more logical to use CSS_ISP_MODE_CONTINUOUS_CAPTURE
            // but it does not work properly yet in all HW (AE issues)
            status = mHwIsp->setMode(IPU2HwIsp::CSS_ISP_MODE_CONTINUOUS_CAPTURE,
                                     enableRawLock);
        }
        break;
    case MODE_CONTINUOUS_VIDEO:
        status = mHwIsp->configureContinuousMode(cfg, true, true);
        if (status == NO_ERROR) {
            status = mHwIsp->configureContinuousRingBuffer(
                                 mBurstLength + BUFFERS_OF_ZSL_STREAM_CONSUMER);
        }
        if (status == NO_ERROR) {
            status = mHwIsp->setMode(IPU2HwIsp::CSS_ISP_MODE_CONTINUOUS_VIDEO);
        }
        break;
    default:
        break;
    }

    mIPU2HwSensor.initSubDevice();
    mMode = mode;
    mEnableRawLock = enableRawLock;
    return status;
}

void IPU2CameraHw::setRequestedStreams(Camera3Request* request)
{
    const Vector<CameraStreamNode *> * outNodes = request->getOutputStreams();
    const Vector<CameraStreamNode *> * inNodes = request->getInputStreams();
    if (outNodes == NULL) {
        LOGE("out put stream is NULL");
        return;
    }
    LOG2("%s: outNodes %d", __FUNCTION__, outNodes->size());
    if (inNodes != NULL) {
        LOG2("%s: inNodes %d", __FUNCTION__, inNodes->size());
    }
    /*
     * out streams are sorted in descending order of resolution.
     * (Biggest resolution is in index 0)
     */
    CameraStreamNode *streamNode;
    mOutStreams.clear();
    mOutYuvStreams.clear();
    mOutJpegStreams.clear();
    mOutRawStreams.clear();
    mOutZslStreams.clear();
    mInStreams.clear();

    FrameInfo info;
    int numOfYuv = 0;
    FrameInfo lastYuvInfo;
    CLEAR(lastYuvInfo);
    for (unsigned int i = 0; i < outNodes->size(); i++) {
        streamNode = outNodes->itemAt(i);
        streamNode->query(&info);

        if (info.format == HAL_PIXEL_FORMAT_BLOB) {
            mOutJpegStreams.push(streamNode);
        } else if (info.format == HAL_PIXEL_FORMAT_RAW_SENSOR) {
            mOutRawStreams.push(streamNode);
        } else {
            // ISP can output image for stream with IMPLEMENTATION_DEFINED format.
            // To select such streams to bind to HW stream,
            // we put back streams with other YUV formats.
#ifdef GRAPHIC_IS_GEN
            if ((lastYuvInfo.format != HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL)
                    && lastYuvInfo.format != HAL_PIXEL_FORMAT_YCbCr_422_I
                    && (lastYuvInfo.width == info.width)
                    && (lastYuvInfo.height == info.height))
#else
            if ((lastYuvInfo.format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
                 && (lastYuvInfo.width == info.width)
                 && (lastYuvInfo.height == info.height))
#endif
            {
                mOutYuvStreams.insertAt(streamNode, numOfYuv - 1);
            } else
                 mOutYuvStreams.push(streamNode);
            numOfYuv++;
            mOutYuvStreams[numOfYuv- 1]->query(&lastYuvInfo);
        }
        mOutStreams.push(streamNode);
    }
    if(inNodes !=NULL) {
       for (unsigned int i = 0; i < inNodes->size(); i++) {
            streamNode = inNodes->itemAt(i);
            mInStreams.push(streamNode);
       }
    }

    LOG2("%s: ------------ dump -----------------", __FUNCTION__);
    for (unsigned int i = 0; i < mOutYuvStreams.size(); i++) {
        streamNode = mOutYuvStreams.editItemAt(i);
        FrameInfo info;
        mOutYuvStreams.editItemAt(i)->query(&info);
        LOG2("    YUV: %dx%d(0x%x)", info.width, info.height, info.format);
    }
    for (unsigned int i = 0; i < mOutJpegStreams.size(); i++) {
        streamNode = mOutJpegStreams.editItemAt(i);
        FrameInfo info;
        mOutJpegStreams.editItemAt(i)->query(&info);
        LOG2("    JPEG: %dx%d", info.width, info.height);
    }
    for (unsigned int i = 0; i < mOutRawStreams.size(); i++) {
        streamNode = mOutRawStreams.editItemAt(i);
        FrameInfo info;
        mOutRawStreams.editItemAt(i)->query(&info);
        LOG2("    Raw: %dx%d", info.width, info.height);
    }
    for (unsigned int i = 0; i < mInStreams.size(); i++) {
        streamNode = mInStreams.editItemAt(i);
        FrameInfo info;
        mInStreams.editItemAt(i)->query(&info);
        LOG2("    In: %dx%d", info.width, info.height);
    }
}

IPU2CameraHw::StreamConfigMode IPU2CameraHw::analyzeConfigMode(
                                                const CameraMetadata & meta)
{
    StreamConfigMode cfgMode = CONFIG_NONE;
    bool invalid = false;
    bool sdvShouldEnable = false;
    bool fpsHighSpeed = false;

    uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    uint8_t intent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    MetadataHelper::getMetadataValue(meta, ANDROID_REQUEST_TYPE, requestType);
    MetadataHelper::getMetadataValue(meta, ANDROID_CONTROL_CAPTURE_INTENT, intent);

    LOG2("%s: requestType=%d, intent=%d", __FUNCTION__, requestType, intent);
    if (requestType == ANDROID_REQUEST_TYPE_CAPTURE) {
        switch (intent) {
        case ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW:
            // For preview: output 1~3 YUV, no RAW
            if (mOutYuvStreams.size() != 0) {
                cfgMode = (!isPreviewSizeSupported(mOutYuvStreams[0])) ? \
                CONFIG_CAPTURE : \
                (mCapInfo->isOfflineCaptureSupported()) ? \
                CONFIG_CONTINUOUS : CONFIG_PREVIEW;
            } else
                cfgMode = CONFIG_CAPTURE;

            // W/A for cts verifier.
            // the cts verifier will set 1280x960 capture and 1280x720 preview.
            // currently, our ISP doesn't has crop function, the image will be stretched
            // there are 4 conditions totally.
            // 1: the jpeg stream must be configured in the configure_streams()
            // 2: yuv stream must be configured.
            // 3: for the current request, it should has no jpeg stream request.
            // 4: capture stream and preview stream have different resolution ratio.
            // TODO: remove it when the FW has the crop feature
            if (mJpegStreamConfigured
                && mOutYuvStreams.size()
                && (0 == mOutJpegStreams.size())) {
                FrameInfo previewInfo, captureInfo;
                mActiveStreams[0]->query(&captureInfo);
                mOutYuvStreams[0]->query(&previewInfo);
                if (!IS_SAME_RESOLUTION_RATIO(previewInfo.width, previewInfo.height,
                        captureInfo.width, captureInfo.height)) {
                    cfgMode = CONFIG_PREVIEW;
                    LOG2("@%s, set mode to CONFIG_PREVIEW, preW:%d, preH:%d, capW:%d, capH:%d", __FUNCTION__, previewInfo.width, previewInfo.height, captureInfo.width, captureInfo.height);
                }
            }

            invalid = mOutRawStreams.size();
            break;
        case ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
            // For ZSL: output 2~3 YUV, no RAW
            // ISP needs jpeg configuration before stream on if it run in continuous mode
            LOG2("%s: isOfflineCaptureSupported=%d", __FUNCTION__, mCapInfo->isOfflineCaptureSupported());
            if (mCapInfo->isOfflineCaptureSupported()
                && mOutYuvStreams.size() > 0
                && !mOutRawStreams.size()) {

                if (mJpegStreamConfigured
                    && mOutYuvStreams.size() > 1
                    && isPreviewSizeSupported(mOutYuvStreams[1])) {
                    cfgMode = CONFIG_CONTINUOUS_ZSL;
                    mOutZslStreams.push_back(mOutYuvStreams.editItemAt(0));
                    mOutYuvStreams.removeAt(0);
                } else {
                    cfgMode = (!isPreviewSizeSupported(mOutYuvStreams[0])) ? \
                    CONFIG_CAPTURE : CONFIG_CONTINUOUS;
                }
            }
            else {
                invalid = true;
            }
            break;
        case ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE:
            // output 0~3 YUV, 1 JPEG, no RAW (for JPEG snapshot)
            // output 1~3 YUV, 0 JPEG, no RAW (for YUV snapshot)
#ifdef GCAM_TEST
            cfgMode = CONFIG_CONTINUOUS_GCAM;
#else
            if (mAAAProcessor->isPreFlashUsed())
                cfgMode = CONFIG_CAPTURE;
            else
                cfgMode = isCurrentConfigMatchedForCapture() ? \
                                              CONFIG_CONTINUOUS : CONFIG_CAPTURE;
#endif
            invalid = (!(mOutJpegStreams.size() || mOutYuvStreams.size()) || mOutRawStreams.size());
            break;
        case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD:
        case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT:
            // output 1~3 YUV, 0~1 JPEG, no RAW

            // Enable SDV, instead of VS, as per specific situation.
            if (mJpegStreamConfigured && mActiveStreams.size()){
                FrameInfo info;
                mActiveStreams[0]->query(&info);
                LOG2("%s: jpeg steam size w: %d, h: %d", __FUNCTION__, info.width, info.height);
                if (!mCapInfo->supportDynamicSdv()) {
                    // Enable continuous video mode and SDV
                    sdvShouldEnable = true;
                } else {
                    // Check SDV video sizes firstly
                    FrameInfo videoFrameInfo;
                    mOutYuvStreams[0]->query(&videoFrameInfo);
                    LOG2("%s: video steam size w: %d, h: %d", __FUNCTION__, videoFrameInfo.width,
                            videoFrameInfo.height);
                    int count = 0;
                    int32_t * sdvVideoSizes = (int32_t*)mCapInfo->getSdvVideoSizes(&count);
                    for (int i = 0; i < count-1; i += 2) {
                        if (videoFrameInfo.width == sdvVideoSizes[i] 
                                && videoFrameInfo.height == sdvVideoSizes[i+1]){
                            LOG2("%s: sdv video match w: %d, h: %d", __FUNCTION__,
                                    videoFrameInfo.width, videoFrameInfo.height);
                            sdvShouldEnable = true;
                            break;
                        }
                    }

                    // In case no matched video size, use below default limitation.
                    if (info.width > RESOLUTION_1080P_WIDTH && info.height > RESOLUTION_1080P_HEIGHT){
                        // Enable continuous video mode and SDV
                        sdvShouldEnable = true;
                    }
                }
            }

            if (mHwIsp != NULL){
                fpsHighSpeed = mHwIsp->isHighSpeed(meta);
            }

            cfgMode = (mCapInfo->isOfflineCaptureSupported() && sdvShouldEnable && mCapInfo->supportSdv() && !fpsHighSpeed) ? \
                    CONFIG_CONTINUOUS_VIDEO : CONFIG_VIDEO;

            LOG2("%s: sdvShouldEnable: %d, supportSdv: %d, isOfflineCaptureSupported: %d, fpsHighSpeed: %d",
                __FUNCTION__, sdvShouldEnable, mCapInfo->supportSdv(), mCapInfo->isOfflineCaptureSupported(), fpsHighSpeed);

            // Use above SDV enable criteria instead of below temporary solution.
            /*
             * Temporary solution: use online video due to BZ 2476 (VIED)
            cfgMode = CONFIG_VIDEO;
             */

            invalid = (!mOutYuvStreams.size() || mOutRawStreams.size());
            break;

        // TODO: currenly only use it for GCAM
        case ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM:
            if (mCapInfo->isOfflineCaptureSupported())
                cfgMode = CONFIG_CONTINUOUS_GCAM;
            else
                cfgMode = CONFIG_CAPTURE;
            invalid = (!(mOutJpegStreams.size() || mOutYuvStreams.size()));
            break;
        default:
            LOGE("%s: Not implement for %d yet!", __FUNCTION__, intent);
            break;
        }
    } else if (requestType == ANDROID_REQUEST_TYPE_REPROCESS) {
        // TODO: support SOC ZSL
        // In CTS case "test video hint", during video recording the framework send request
        // to do a capture, so in that case, HAL will enter online capture.
        if (mStreamConfig.mCfgMode == CONFIG_VIDEO)
            cfgMode = CONFIG_CAPTURE;
        else
            cfgMode = CONFIG_CONTINUOUS_ZSL;

        invalid = (!mInStreams.size() || !mOutJpegStreams.size() || mOutRawStreams.size());
    }

    LOG2("%s: config mode = %d valid? %d", __FUNCTION__, cfgMode, !invalid);
    LOG2("    type %d, intent %d, YUV/JPEG/RAW/ZSL %d / %d / %d / %d",
            requestType, intent,
            mOutYuvStreams.size(), mOutJpegStreams.size(),
            mOutRawStreams.size(), mOutZslStreams.size());
    return  invalid ? CONFIG_NONE : cfgMode;
}

bool IPU2CameraHw::analyzeStreams(StreamConfigMode cfgMode, StreamConfig & cfg)
{
    LOG2("%s: %d", __FUNCTION__, cfgMode);
    CLEAR(cfg);

    cfg.mCfgMode = cfgMode;
    FrameInfo info;
    // Please refer to analyzeConfigMode()
    switch (cfgMode) {
    // user need: output 1~3 YUV, no JPEG/RAW
    case CONFIG_PREVIEW:
        cfg.mPreview = mOutStreams[0];
        cfg.mIspMode = MODE_PREVIEW;
        break;
    // output 0~3 YUV, 1 JPEG, no RAW
    // or: output 1~3 YUV, 0 JPEG, no RAW
    case CONFIG_CAPTURE:
        cfg.mCapture = mActiveStreams[0];
        cfg.mIspMode = MODE_CAPTURE;
        break;
    // output 1~3 YUV, 0~1 JPEG, no RAW
    case CONFIG_VIDEO:
        cfg.mVideo = mOutYuvStreams[0];
        if (mOutYuvStreams.size() > 1) {
            cfg.mPreview = mOutYuvStreams[1];
        } else {
            cfg.mVideo->query(&info);
            cfg.mPreview = mFakeStreamMgr.getFakeStream(
                                FakeStreamManager::FAKE_VIDEO_NO_PREVIEW,
                                &info,
                                mBurstLength,
                                mCameraId);
            cfg.mFakeStream = cfg.mPreview;
        }
        cfg.mIspMode = MODE_VIDEO;
        break;

    case CONFIG_CONTINUOUS_VIDEO:
        cfg.mVideo = mOutYuvStreams[0];
        if (mOutYuvStreams.size() > 1) {
            cfg.mPreview = mOutYuvStreams[1];
        } else {
            cfg.mVideo->query(&info);
            cfg.mPreview = mFakeStreamMgr.getFakeStream(
                                FakeStreamManager::FAKE_VIDEO_NO_PREVIEW,
                                &info,
                                mBurstLength,
                                mCameraId);

            cfg.mFakeStream = cfg.mPreview;
        }
        if (mJpegStreamConfigured)
            cfg.mCapture = mActiveStreams[0]; // It is client JPEG stream
        cfg.mIspMode = MODE_CONTINUOUS_VIDEO;
        break;
    case CONFIG_CONTINUOUS: {
            if (isCurrentConfigMatchedForCapture()) {
                cfg = mStreamConfig;
            } else {
                int previewHALFormat = PlatformData::previewHALFormat();
                FrameInfo info;
                unsigned int streamIndex = 0;
                if (previewHALFormat != -1) {
                    for (streamIndex = 0; streamIndex < mOutYuvStreams.size(); streamIndex ++) {
                        mOutYuvStreams[streamIndex]->query(&info);
                        if (info.format == previewHALFormat) {
                            break;
                        }
                    }
                    if (streamIndex == mOutYuvStreams.size()) {
                        LOGW("previewHALFormat %d is not found in out streams, use the first out stream as preview.", previewHALFormat);
                        streamIndex = 0;
                        cfg.mPreview = mOutYuvStreams[0];
                    }
                }

                cfg.mPreview = mOutYuvStreams[streamIndex];
                // guess it is for capture (JPEG/YUV)
                if (mOutYuvStreams[streamIndex] != mActiveStreams[0] && mJpegStreamConfigured)
                    cfg.mCapture = mActiveStreams[0];
                cfg.mIspMode = MODE_CONTINUOUS_CAPTURE;
                cfg.mEnableRawLock = mCapInfo->streamConfigEnableRawLock();
            }
            break;
        }
    case CONFIG_CONTINUOUS_ZSL:
        if (mInStreams.size()) {
            // In reprocessing phase
            cfg = mStreamConfig;
        } else {
            // In preview phase
            cfg.mCapture = mActiveStreams[0];  // It is client JPEG stream
            cfg.mPreview = mOutYuvStreams[0];
            cfg.mIspMode = MODE_CONTINUOUS_CAPTURE;
            //cfg.mEnableRawLock = true;
        }
        break;

    case CONFIG_CONTINUOUS_GCAM:
        // Configure it when enter metering(preview) phase
        if (mOutStreams[0] == mActiveStreams[0]) {
            LOG2("%s: Guess it is for payload", __FUNCTION__);
            cfg = mStreamConfig;
        } else {
            LOG2("%s: Guess it is for metering", __FUNCTION__);
            cfg.mCapture = mActiveStreams[0]; // Guess capture stream
            cfg.mPreview = mOutYuvStreams[0];
        }
        cfg.mIspMode = MODE_CONTINUOUS_CAPTURE;
        cfg.mEnableRawLock = mCapInfo->streamConfigEnableRawLock();
        break;

    default:
        LOGE("%s: Not implement for configMode %d yet!", __FUNCTION__, cfgMode);
        break;
    }

    bool needReStart = (mStreamConfig.mCfgMode != cfg.mCfgMode)
              || (mStreamConfig.mPreview != cfg.mPreview)
              || (mStreamConfig.mCapture != cfg.mCapture)
              || (mStreamConfig.mVideo != cfg.mVideo);
    LOG2("%s: -------configure mode = %d needReStart = %d---------", __FUNCTION__, cfg.mCfgMode, needReStart);
    if (cfg.mPreview) {
        cfg.mPreview->query(&info);
        LOG2("    new preview stream: %dx%d", info.width, info.height);
    }
    if (cfg.mVideo) {
        cfg.mVideo->query(&info);
        LOG2("    new video stream: %dx%d", info.width, info.height);
    }
    if (cfg.mCapture) {
        cfg.mCapture->query(&info);
        LOG2("    new capture stream: %dx%d", info.width, info.height);
    }

    return needReStart;
}

bool IPU2CameraHw::isPreviewSizeSupported(CameraStreamNode * stream)
{
    if (!stream)
        return false;

    FrameInfo info;
    stream->query(&info);

    return (info.width * info.height <= mMaxPreviewWidth * mMaxPreviewHeight);
}

bool IPU2CameraHw::isCurrentConfigMatchedForCapture()
{
    if (mMode != MODE_CONTINUOUS_CAPTURE)
        return false;

    StreamConfig & config = mStreamConfig;

    // if the stream is just capture, we know it's single capture
    if (mOutStreams.size() == 1 && mOutStreams[0] == config.mCapture)
        return false;

    if (mOutStreams.size() == 1)
        return (mOutStreams[0] == config.mCapture
            || mOutStreams[0] == config.mPreview);

    if ((mOutStreams.size() == 2)||(mOutStreams.size() == 3))
        return ((mOutStreams[0] == config.mCapture
                    && mOutStreams[1] == config.mPreview)
                || (mOutStreams[1] == config.mCapture
                    && mOutStreams[0] == config.mPreview));

    return false;
}

status_t IPU2CameraHw::bindStreams(StreamConfig & config)
{
    LOG1("%s: ISP mode:%d", __FUNCTION__, config.mIspMode);
    mPostStreams.clear();
    mPreviewHwStream->detachListener();
    mCaptureHwStream->detachListener();
    mVideoHwStream->detachListener();
    status_t status = NO_ERROR;
    int invalidFrames = 0;
    switch (config.mIspMode) {
    case MODE_PREVIEW:
        status = bindStreamsForPreview(config.mPreview);
        pxioctl(mPreviewDevice, ATOMISP_IOC_G_INVALID_FRAME_NUM, &invalidFrames);
        mIPU2HwSensor.setCaptureSyncOwner(mPreviewHwStream, invalidFrames);
        break;
    case MODE_CAPTURE:
        status = bindStreamsForCapture(config.mCapture);
        pxioctl(mPictureDevice, ATOMISP_IOC_G_INVALID_FRAME_NUM, &invalidFrames);
        mIPU2HwSensor.setCaptureSyncOwner(mCaptureHwStream, invalidFrames);
        break;
    case MODE_VIDEO:
        status = bindStreamsForVideo(config.mVideo, config.mPreview);
        pxioctl(mVideoDevice, ATOMISP_IOC_G_INVALID_FRAME_NUM, &invalidFrames);
        mIPU2HwSensor.setCaptureSyncOwner(mVideoHwStream, invalidFrames);
        break;
    case MODE_CONTINUOUS_VIDEO:
        status = bindStreamsForContinuousVideo(
                           config.mCapture,
                           config.mVideo,
                           config.mPreview);
        pxioctl(mVideoDevice, ATOMISP_IOC_G_INVALID_FRAME_NUM, &invalidFrames);
        mIPU2HwSensor.setCaptureSyncOwner(mVideoHwStream, invalidFrames);
        break;
    case MODE_CONTINUOUS_CAPTURE:
        status = bindStreamsForContinuous(config.mCapture, config.mPreview);
        pxioctl(mPictureDevice, ATOMISP_IOC_G_INVALID_FRAME_NUM, &invalidFrames);
        mIPU2HwSensor.setCaptureSyncOwner(mPreviewHwStream, invalidFrames);
        break;
    default:
        break;
    }

    return status;
}

status_t IPU2CameraHw::bindStreamsForPreview(CameraStreamNode * clientStream)
{
    LOG1("%s",__FUNCTION__);

    status_t status = NO_ERROR;

    mPreviewHwStream->setMode(PreviewStream::ONLINE);
    status = bindToHwStream(clientStream, mPreviewHwStream);

    if (status != NO_ERROR)
        return status;

    for (unsigned int i = 0; i < mOutStreams.size(); i++) {
        if (mOutStreams[i] != clientStream) {
            linkToPostProcessStream(mOutStreams[i], mPreviewHwStream);
        }
    }

    mPreviewHwStream->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mPreviewHwStream->attachListener(mFaceDetectionThread,
                                     ICssIspListener::ISP_EVENT_TYPE_FRAME);
    return NO_ERROR;
}

status_t IPU2CameraHw::bindStreamsForCapture(CameraStreamNode * clientStream)
{
    LOG1("%s",__FUNCTION__);
    status_t status = NO_ERROR;

    mCaptureHwStream->setMode(CaptureStream::ONLINE);
    status = bindToHwStream(clientStream, mCaptureHwStream);

    if (status != NO_ERROR)
        return status;

    for (unsigned int i = 0; i < mOutStreams.size(); i++) {
        if (mOutStreams[i] != clientStream)
            linkToPostProcessStream(mOutStreams[i], mCaptureHwStream);
    }

    mCaptureHwStream->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mCaptureHwStream->attachListener(mFaceDetectionThread,
                                     ICssIspListener::ISP_EVENT_TYPE_FRAME);
    return NO_ERROR;
}

/*
    we should dynamic to select the source hw stream for video snapshot.
    in the function bindStreamsForVideo(), it will bind the capture stream to one hw stream.
    we shouldn't select one fixed hw stream for the video snapshot, we should caculate to dynamic select the hw stream by according to the ratio and size.
    this function is to do this.
    this function will always return one hw stream.
*/
PreviewStream* IPU2CameraHw::selectHWStreamForVideoSnapshot(void)
{
    PreviewStream* srcStream = mPreviewHwStream;

    FrameInfo previewInfo, videoInfo, captureInfo;
    CLEAR(previewInfo);
    CLEAR(videoInfo);
    CLEAR(captureInfo);
    mPreviewHwStream->query(&previewInfo);
    mVideoHwStream->query(&videoInfo);
    mActiveStreams[0]->query(&captureInfo);

    float previewRatio = (float)previewInfo.width / (float)previewInfo.height;
    float videoRatio = (float)videoInfo.width / (float)videoInfo.height;
    float captureRatio = (float)captureInfo.width / (float)captureInfo.height;
    int ratio = fabs(captureRatio - previewRatio) - fabs(captureRatio - videoRatio);
    if (ratio == 0)
        srcStream = (videoInfo.width >= previewInfo.width) ? mVideoHwStream : mPreviewHwStream;
    else
        srcStream = (ratio > 0) ? mVideoHwStream : mPreviewHwStream;

    LOG2("%s, previewInfo.width:%d, videoInfo.width:%d, captureInfo.width:%d",__FUNCTION__, previewInfo.width, videoInfo.width, captureInfo.width);
    LOG2("%s, previewRatio:%f, videoRatio:%f, captureRatio:%f, ratio:%d",__FUNCTION__, previewRatio, videoRatio, captureRatio, ratio);

    return srcStream;
}

status_t IPU2CameraHw::bindStreamsForVideo(CameraStreamNode * clientVideo, CameraStreamNode * clientPreview)
{
    LOG2("%s",__FUNCTION__);

    bindToHwStream(clientVideo, mVideoHwStream);
    mPreviewHwStream->setMode(PreviewStream::ONLINE);
    bindToHwStream(clientPreview, mPreviewHwStream);

    for (unsigned int i = 0; i < mOutYuvStreams.size(); i++) {
        if (mOutYuvStreams[i] != clientVideo
            && mOutYuvStreams[i] != clientPreview) {
            linkToPostProcessStream(mOutYuvStreams[i], mVideoHwStream);
        }
    }
    if (!mOutJpegStreams.size() && mJpegStreamConfigured) {
        linkToPostProcessStream(mActiveStreams[0], selectHWStreamForVideoSnapshot());
    }

    mPreviewHwStream->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mPreviewHwStream->attachListener(mHwIsp->getDVSHandler(),
                                         ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mPreviewHwStream->attachListener(mFaceDetectionThread,
                                     ICssIspListener::ISP_EVENT_TYPE_FRAME);

    return NO_ERROR;
}

status_t IPU2CameraHw::bindStreamsForContinuousVideo(
                                              CameraStreamNode * clientCapture,
                                              CameraStreamNode * clientVideo,
                                              CameraStreamNode * clientPreview)
{
    LOG2("%s, size:%d, cfg:%d",__FUNCTION__, mOutJpegStreams.size(), mJpegStreamConfigured);

    mCaptureHwStream->setMode(CaptureStream::OFFLINE);

    if (clientCapture) {
        // W/A for cts case: android.hardware.camera2.cts.RecordingTest#testBurstVideoSnapshot
        // the fw has bug for the burst capture in the SDV case, the fw will timeout if we continous to get data from video0.
        // because the handling of video / preview is faster than the cpature,
        // so when doing the second capture, there is no video / preview
        // buffer left in the fw. so the timeout happens.
        // the W/A solution is that when the capture size is the same as one of
        // the preview size or video size, we will let the capture to be got from the preview or
        // video hw stream, not from the capture stream which is video 0.
        // TODO: remove the W/A when the fw issue has been fixed for the burst capture in SDV
        FrameInfo previewInfo, videoInfo, captureInfo;
        CLEAR(previewInfo);
        CLEAR(videoInfo);
        CLEAR(captureInfo);
        clientVideo->query(&videoInfo);
        clientPreview->query(&previewInfo);
        clientCapture->query(&captureInfo);
        PreviewStream * selectHwStream = NULL; // we need to select one hw stream from video or preview
        if ((captureInfo.width == previewInfo.width)
            && (captureInfo.height == previewInfo.height)) {
            selectHwStream = mPreviewHwStream;
        } else if ((captureInfo.width == videoInfo.width)
                    && (captureInfo.height == videoInfo.height)) {
            selectHwStream = mVideoHwStream;
        }
        if (selectHwStream)
            linkToPostProcessStream(clientCapture, selectHwStream);
        else
            bindToHwStream(clientCapture, mCaptureHwStream);
    } else {
        /*
         * No Jpeg stream, use the max resolution which the ratio is same with video,
         * to configure capture device
         */
        FrameInfo cInfo, captureInfo;
        CLEAR(cInfo);
        CLEAR(captureInfo);
        clientVideo->query(&cInfo);
        getMaxCaptureInfoWithRatio(&captureInfo, cInfo);
        mCaptureHwStream->setFormat(&captureInfo);
    }

    mPreviewHwStream->setMode(PreviewStream::OFFLINE_VIDEO);
    bindToHwStream(clientVideo, mVideoHwStream);
    bindToHwStream(clientPreview, mPreviewHwStream);

    for (unsigned int i = 0; i < mOutYuvStreams.size(); i++) {
        if (mOutYuvStreams[i] != clientVideo
            && mOutYuvStreams[i] != clientPreview
            && mOutYuvStreams[i] != clientCapture) {
            linkToPostProcessStream(mOutYuvStreams[i], mVideoHwStream);
        }
    }

    /* Using mCaptureHwStream for SDV as binded above, instread of mVideoHwStream below which is for VS.
    if (!mOutJpegStreams.size() && mJpegStreamConfigured) {
        linkToPostProcessStream(mActiveStreams[0], mVideoHwStream);
    }
    */

    mPreviewHwStream->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mPreviewHwStream->attachListener(mHwIsp->getDVSHandler(),
                                         ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mPreviewHwStream->attachListener(mFaceDetectionThread,
                                     ICssIspListener::ISP_EVENT_TYPE_FRAME);

    return NO_ERROR;
}

status_t IPU2CameraHw::bindStreamsForContinuous(CameraStreamNode * clientCapture, CameraStreamNode * clientPreview)
{
    LOG1("%s",__FUNCTION__);

    mCaptureHwStream->setMode(CaptureStream::OFFLINE);

    if (clientCapture) {
         bindToHwStream(clientCapture, mCaptureHwStream);
    } else {
        /*
         * No Jpeg stream, use the max resolution which the ratio is same with preview,
         * to configure capture device
         */
        FrameInfo cInfo, captureInfo;
        CLEAR(cInfo);
        CLEAR(captureInfo);
        clientPreview->query(&cInfo);
        getMaxCaptureInfoWithRatio(&captureInfo, cInfo);
        mCaptureHwStream->setFormat(&captureInfo);
    }
    mPreviewHwStream->setMode(PreviewStream::OFFLINE_CAPTURE);
    bindToHwStream(clientPreview, mPreviewHwStream);

    if (mOutZslStreams.size()) {
        FrameInfo info;
        sp<PostProcessStream> postZsl = new PostProcessStream(mImgEncoder,
                                                                   mJpegMaker,
                                                                   m3AControls,
                                                                   mHwIsp.get(),
                                                                   &mIPU2HwSensor,
                                                                   mCameraId);
        if (postZsl.get() == NULL) {
            LOGE("%s: no memory for post ZSL stream!", __FUNCTION__);
            return NO_MEMORY;
        }
        mOutZslStreams[0]->query(&info);
        mPostStreams.push(postZsl);
        postZsl->setMode(PostProcessStream::POSTPROCESSMODE_ZSL);
        postZsl->setFormat(&info);
        CameraStream::bind(mOutZslStreams[0], postZsl.get());

        if (mEnableRawLock) {
            // Notify ZSL stream for captureDone
            mPreviewHwStream->attachListener(postZsl.get(), ICssIspListener::ISP_EVENT_TYPE_FRAME);
            // Notify capture stream to trigger zsl capture (reprocess)
            postZsl->attachListener(mCaptureHwStream, ICssIspListener::ISP_EVENT_TYPE_RAW_LOCK);
            // Notify capture stream to trigger normal capture
            // (zsl stream pass related preview frame event to trigger it)
            postZsl->attachListener(mCaptureHwStream, ICssIspListener::ISP_EVENT_TYPE_FRAME);
        } else {
             // ZSL stream return relate frame without image data(captureDone)
             mPreviewHwStream->attachListener(postZsl.get(), ICssIspListener::ISP_EVENT_TYPE_FRAME);
        }
        // Notify zsl stream to return input buffer
        mCaptureHwStream->attachListener(postZsl.get(), ICssIspListener::ISP_EVENT_TYPE_FRAME);
    } else {
        if (mEnableRawLock) {
            // Notify capture stream to trigger normal capture
            // (capture stream will know which RAW buffer is locked)
            mPreviewHwStream->attachListener(mCaptureHwStream, ICssIspListener::ISP_EVENT_TYPE_FRAME);
        }
    }

    // Provide frame event:
    // During preview phase
    mPreviewHwStream->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mPreviewHwStream->attachListener(mFaceDetectionThread,
                            ICssIspListener::ISP_EVENT_TYPE_FRAME);
    // During offline capture phase
    // CaptureStream will send frame event if related request has no preview frame
    mCaptureHwStream->attachListener(mAAAProcessor, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    mCaptureHwStream->attachListener(mFaceDetectionThread,
                            ICssIspListener::ISP_EVENT_TYPE_FRAME);

    for (unsigned int i = 0; i < mActiveStreams.size(); i++) {
        // preview/capture/zsl stream? do nothing for them
        if (mActiveStreams[i] == clientPreview
            || mActiveStreams[i] == clientCapture
            || (mOutZslStreams.size() && mActiveStreams[i] == mOutZslStreams[0]))
            continue;

        // Is it a out stream? If yes,  link to preview hw stream;
        // If no, only YUV_420_888 will link to preview hw stream.
        bool outStream = false;
        for (unsigned int j = 0; j < mOutStreams.size(); j++) {
            if (mActiveStreams[i] == mOutStreams[j]) {
                outStream = true;
                break;
            }
        }

        if (outStream) {
            linkToPostProcessStream(mActiveStreams[i], mPreviewHwStream);
        } else {
            FrameInfo info;
            mActiveStreams[i]->query(&info);
            if (info.originalFormat == HAL_PIXEL_FORMAT_YCbCr_420_888) {
                LOG1("@%s %dx%d, link to preview hw as well", __FUNCTION__, info.width, info.height);
                linkToPostProcessStream(mActiveStreams[i], mPreviewHwStream);
            }
        }
    }

    return NO_ERROR;
}


status_t IPU2CameraHw::bindToHwStream(CameraStreamNode * clientStream,
                                     HwStream * hwStream)
{
    FrameInfo cInfo;
    clientStream->query(&cInfo);
    // HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED and HAL_PIXEL_FORMAT_YCbCr_420_888
    // are mapped to NV12, and can be handled by hw streams directly.
#ifdef GRAPHIC_IS_GEN
    if (cInfo.format == HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL ||
        cInfo.format == HAL_PIXEL_FORMAT_YCbCr_422_I)
#else
    if (cInfo.format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
        cInfo.format == HAL_PIXEL_FORMAT_YCbCr_420_888)
#endif
    {
        hwStream->setFormat(&cInfo);
        CameraStream::bind(clientStream, hwStream);
        return NO_ERROR;
    }

    // ISP doest not support this format
    FrameInfo sInfo = cInfo;
    sInfo.originalFormat = sInfo.format =
       v4L2Fmt2GFXFmt(PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, mCameraId), mCameraId);
    int postMode = PostProcessStream::selectPostProcessMode(cInfo, sInfo);

    HWControlGroup hwcg;
    sp<PostProcessStream> post = new PostProcessStream(mImgEncoder,
                                                           mJpegMaker,
                                                           m3AControls,
                                                           mHwIsp.get(),
                                                           &mIPU2HwSensor,
                                                           mCameraId);
    if (post.get() == NULL) {
        LOGE("%s: no memory for post stream!", __FUNCTION__);
        return NO_MEMORY;
    }
    mPostStreams.push(post);
    post->setMode(postMode);

    post->setFormat(&cInfo);
    hwStream->setFormat(&sInfo);

    CameraStream::bind(clientStream, post.get());
    CameraStream::bind(post.get(), hwStream);
    return NO_ERROR;
}

status_t IPU2CameraHw::linkToPostProcessStream(CameraStreamNode * clientS, HwStream * hwS)
{
    FrameInfo cInfo;
    FrameInfo sInfo;
    clientS->query(&cInfo);
    hwS->query(&sInfo);
    int postMode = PostProcessStream::selectPostProcessMode(cInfo, sInfo);
    LOG2("@%s: postMode %d",__FUNCTION__, postMode);

    HWControlGroup hwcg;
    PostProcessStream * post = new PostProcessStream(mImgEncoder,
                                                           mJpegMaker,
                                                           m3AControls,
                                                           mHwIsp.get(),
                                                           &mIPU2HwSensor,
                                                           mCameraId);
    if (!post) {
        LOGE("%s: no memory for post stream!", __FUNCTION__);
        return NO_MEMORY;
    }

    mPostStreams.push(post);
    post->setMode(postMode);
    post->setFormat(&cInfo);
    CameraStream::bind(clientS, post);
    hwS->attachListener(post, ICssIspListener::ISP_EVENT_TYPE_FRAME);
    return NO_ERROR;
}

void IPU2CameraHw::dump(int fd)
{
    LOG2("@%s", __FUNCTION__);

    String8 message;
    message.appendFormat(LOG_TAG ":@%s\n", __FUNCTION__);
    write(fd, message.string(), message.size());

    if (mAAAProcessor != NULL)
        mAAAProcessor->dump(fd);
}

}; // namespace camera2
} // namespace android
