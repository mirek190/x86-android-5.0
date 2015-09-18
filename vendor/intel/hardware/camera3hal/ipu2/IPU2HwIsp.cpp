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

#define LOG_TAG "Camera_IPU2HwIsp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <linux/media.h>
#include <SchedulingPolicyService.h>
#include "Utils.h"
#include "IaDvs2.h"
#include "IPU2HwIsp.h"
#include "LogHelper.h"
#include "CameraMetadataHelper.h"
#include "UtilityMacros.h"


namespace android {
namespace camera2 {

#define CSS_EVENT_POLL_TIMEOUT 500      // 500 msec, timeout to poll any event
#define CSS_EVENT_POLL_TIMEOUT_COUNT 6

#define MAX_EXP_ID 250
#define MAX_STATISTICS_QUEUE_SIZE 8

#define EPSILON 0.00001

/* Keep in sync with the enum in IPU2HwIsp header file */
const char *IPU2HwIsp::sMode2String[] = { "CSS_ISP_MODE_NONE",
                                        "CSS_ISP_MODE_PREVIEW",
                                        "CSS_ISP_MODE_CAPTURE",
                                        "CSS_ISP_MODE_VIDEO",
                                        "CSS_ISP_MODE_CONTINUOUS_CAPTURE",
                                        "CSS_ISP_MODE_CONTINUOUS_VIDEO"};

const char *IPU2HwIsp::sEvent2String[] = {  "CSS_ISP_EVENT_FRAME",
                                            "CSS_ISP_EVENT_SOF",
                                            "CSS_ISP_EVENT_3A_STATS_READY",
                                            "CSS_ISP_EVENT_MAX"};

////////////////////////////////////////////////////////////////////
//                          STATIC DATA
////////////////////////////////////////////////////////////////////
static struct devNameGroup devName[MAX_CAMERAS] = {
    {{"/dev/video0", "/dev/video1", "/dev/video2", "/dev/video3","/dev/video4"},
        false,},
    {{"/dev/video5", "/dev/video6", "/dev/video7", "/dev/video8","/dev/video9"},
        false,},
};

IPU2HwIsp::cameraInfo IPU2HwIsp::sCamInfo[MAX_CAMERA_NODES];


IPU2HwIsp::IPU2HwIsp(sp<V4L2VideoNode> &mainNode,
                       sp<V4L2VideoNode> &previewNode,
                       sp<V4L2VideoNode> &videoNode,
                       sp<V4L2Subdevice> &eventSubdev,
                       HWControlGroup &hwcg,
                       I3AControls *aaa)
    : mFlashTorchSetting(-1),
      mMainNode(mainNode),
      mPreviewNode(previewNode),
      mVideoNode(videoNode),
      mEventNode(eventSubdev),
      mCameraId(0),
      mCapInfo(NULL),
      mMaxZoomFactor(0),
      mMaxZoomRatio(1.0),
      mCssMajorVersion(2),
      mCssMinorVersion(1),
      mIspHwMajorVersion(2401),
      mIspHwMinorVersion(0),
      mMode(CSS_ISP_MODE_NONE),
      mRawLockEnabled(false),
      mRawLockReady(false),
      mContCaptPrepared(false),
      mContCaptPriority(false),
      mStarted(false),
      mMessageQueue("Camera_IPU2HwIsp", (int)MESSAGE_ID_MAX),
      mMessageThead(NULL),
      mThreadRunning(false),
      mZoomRatio(1.0),
      mColorEffect(V4L2_COLORFX_NONE),
      mNoiseReductionEdgeEnhancement(true),
      mIspHandle(NULL),
      mSensorEmbeddedMetaData(NULL),
      mSensorEmbeddedMetaDataSupported(false),
      mIspPerframeSettingsEnabled(false),
      mTrigger3A(0),
      mHasLensActuator(true),
      mDvsEnabled(false),
      mDvs(NULL),
      mDvs6AxisCfg(NULL),
      mDvsCoefs(NULL),
      mSensorCI(hwcg.mSensorCI),
      mVideoFps(DEFAULT_RECORDING_FPS),
      m3AControls(aaa),
      mPreviewHwStream(NULL),
      mVideoHwStream(NULL),
      mCaptureHwStream(NULL),
      mDvsConfig(NULL),
      mDvsConfigChange(false),
      mFocalLength(0),
      mAvailableBits(0)
{
    CLEAR(mPreviewInfo);
    CLEAR(mVideoInfo);
    CLEAR(mContCaptConfig);
    CLEAR(mCachedStatsEventMsg);
    CLEAR(mIspConfStruct);
    CLEAR(mWeightedLsc);
    CLEAR(mWeightedLscForIsp);
    CLEAR(mSensorArraySize);

    if (PlatformData::AiqConfig) {
        ia_cmc_t *cmc = PlatformData::AiqConfig.getCMCHandler();
        if (cmc && cmc->cmc_general_data) {
            int bitDepth = cmc->cmc_general_data->bit_depth;
            mAvailableBits = ~((~0) << bitDepth);
        }
    }
}

IPU2HwIsp::~IPU2HwIsp()
{
    stop();

    if (mMainNode->isOpen())
        mMainNode->close();

    if (mEventNode->isOpen())
        mEventNode->close();

    if (mSensorEmbeddedMetaData != NULL) {
        delete mSensorEmbeddedMetaData;
        mSensorEmbeddedMetaData = NULL;
    }

    if (mMessageThead != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }
    ia_isp_2_2_deinit(mIspHandle);
    deinit3AStatisticsQueue();

    if (mDvs != NULL) {
        delete mDvs;
        mDvs = NULL;
    }
    delete mWeightedLsc.cmc_lens_shading;
    delete mWeightedLsc.cmc_lsc_grids;
    delete mWeightedLsc.cmc_lsc_rg_bg_ratios;
    delete[] mWeightedLsc.lsc_grids;
    delete mWeightedLscForIsp.cmc_lens_shading;
    delete mWeightedLscForIsp.cmc_lsc_grids;
    delete mWeightedLscForIsp.cmc_lsc_rg_bg_ratios;
    delete[] mWeightedLscForIsp.lsc_grids;
    // delete makeNotes in metadataQueue
    Mutex::Autolock l(mMetadataQueueLock);
    aaaMetadataInfo *metadata = NULL;
    for (unsigned int i = 0; i < mMetadataQueue.size(); i++) {
         metadata = mMetadataQueue.editItemAt(i);
         clearMetadata(metadata);
    }
    mMetadataQueue.clear();
}

/**
 * Init()
 * second phase constructor for the ISP class.
 */
status_t IPU2HwIsp::init(int cameraId)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mCameraId = cameraId;
    status = mMainNode->open();
    if (status != NO_ERROR && !mMainNode->isOpen()) {
        LOGE("Failed to open ISP main device");
        return status;
    }

    status = mEventNode->open();
    if (status != NO_ERROR && !mEventNode->isOpen()) {
        LOGE("Failed to open ISP event subdevice");
        return status;
    }

    status = registerToEvents();
    if (status != NO_ERROR) {
        LOGE("Failed to register to ISP events");
        return status;
    }

    HWControlGroup hwcg;
    hwcg.mIspCI = this;
    mSensorEmbeddedMetaData = new SensorEmbeddedMetaData(hwcg, m3AControls);

    mMessageThead = new MessageThread(this, "IPU2HwIsp", PRIORITY_CAMERA);
    if (mMessageThead == NULL) {
        LOGE("Error create IPU2HwIsp in init");
        return NO_INIT;
    }

    pid_t pid = getpid();
    pid_t tid = mMessageThead->getTid();

    int err = requestPriority(pid, tid, kCameraPriority);
    if (err != 0) {
        LOGW("Policy SCHED_FIFO priority %d is unavailable for pid %d tid %d error %d",
             kCameraPriority , pid, tid, err);
    }

    mStaticMeta = PlatformData::getStaticMetadata(cameraId);
    mCapInfo = getIPU2CameraCapInfo(cameraId);
    getSensorActiveArray();
    getZoomRatio();

    // TODO: remove this code when isp per-frame settings is totally ready.
    if (mCapInfo)
        mIspPerframeSettingsEnabled = mCapInfo->supportPerFrameSettings() &&
                                    mCapInfo->enableIspPerframeSettings();

    // check lens actuator
    int count = 0;
    float *min_focal_distance = (float *)MetadataHelper::getMetadataValues(mStaticMeta,
                                     ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
                                     TYPE_FLOAT,
                                     &count);
    // min focus distance 0.00 means the focus is fixed
    if (count == 1 && min_focal_distance != NULL) {
        if (min_focal_distance[0] >= -EPSILON && min_focal_distance[0] <= EPSILON)
            mHasLensActuator = false;
    }

    initDVS();

    CLEAR(mWeightedLsc);
    mWeightedLsc.cmc_lens_shading = new cmc_lens_shading_t;
    if (mWeightedLsc.cmc_lens_shading == NULL)
        return NO_MEMORY;
    CLEAR(*mWeightedLsc.cmc_lens_shading);
    mWeightedLsc.cmc_lsc_grids = new cmc_lsc_grid_t;
    if (mWeightedLsc.cmc_lsc_grids == NULL)
        return NO_MEMORY;
    CLEAR(*mWeightedLsc.cmc_lsc_grids);
    mWeightedLsc.cmc_lsc_rg_bg_ratios = new cmc_lsc_rg_bg_t;
    if (mWeightedLsc.cmc_lsc_rg_bg_ratios == NULL)
        return NO_MEMORY;
    CLEAR(*mWeightedLsc.cmc_lsc_rg_bg_ratios);

    CLEAR(mWeightedLscForIsp);
    mWeightedLscForIsp.cmc_lens_shading = new cmc_lens_shading_t;
    if (mWeightedLscForIsp.cmc_lens_shading == NULL)
        return NO_MEMORY;
    CLEAR(*mWeightedLscForIsp.cmc_lens_shading);
    mWeightedLscForIsp.cmc_lsc_grids = new cmc_lsc_grid_t;
    if (mWeightedLscForIsp.cmc_lsc_grids == NULL)
        return NO_MEMORY;
    CLEAR(*mWeightedLscForIsp.cmc_lsc_grids);
    mWeightedLscForIsp.cmc_lsc_rg_bg_ratios = new cmc_lsc_rg_bg_t;
    if (mWeightedLscForIsp.cmc_lsc_rg_bg_ratios == NULL)
        return NO_MEMORY;
    CLEAR(*mWeightedLscForIsp.cmc_lsc_rg_bg_ratios);

    status = initMetadataQueue();

    return status;
}

int IPU2HwIsp::getExposureLag(void)
{
    return mCapInfo->exposureLag();
}

status_t IPU2HwIsp::initMetadataQueue(void)
{
    // Initialize the metadata queues
    LOG2("@%s", __FUNCTION__);
    Mutex::Autolock l(mMetadataQueueLock);
    mMetadataQueue.clear();
    mMetadataQueue.setCapacity(MAX_METADATA_QUEUE_SIZE);

    // init the metadataQueue
    aaaMetadataInfo *initMetadata;
    // get the exposure lag
    int exposureLag = getExposureLag();
    int count = 0;
    int expTime = mCapInfo->getStartExposureTime(); // unit ms
    int aecApexTv = -1.0 * (log10((double)expTime/1000) / log10(2.0)) * 65536;
    float aperture = 2.53;

    float* apertureAv = (float*) MetadataHelper::getMetadataValues(mStaticMeta,
                         ANDROID_LENS_INFO_AVAILABLE_APERTURES,TYPE_FLOAT, &count);
    if (count == 0 || apertureAv == NULL) {
        apertureAv = &aperture;
        LOGE("can't get apertureAv");
    }

    for (int i = 0; i < exposureLag; i ++) {
         initMetadata = new aaaMetadataInfo;
         if (initMetadata == NULL) {
             LOGE("there is not enough memory");
             goto exit;
         }
         CLEAR(*initMetadata);
         initMetadata->reqId = i;
         initMetadata->aeConfig.expTime = expTime;
         initMetadata->aeConfig.aecApexTv = aecApexTv;
         initMetadata->aeConfig.aecApexAv =  apertureAv[0] * 65536;
         mMetadataQueue.push_back(initMetadata);
    }
    return NO_ERROR;
exit:
    aaaMetadataInfo *metadata = NULL;
    for (int i = 0;i < exposureLag; i ++) {
         metadata = mMetadataQueue.editItemAt(i);
         clearMetadata(metadata);
    }
    mMetadataQueue.clear();
    return NO_MEMORY;
}

struct devNameGroup* IPU2HwIsp::getDevName(unsigned int index)
{
    if (index >= MAX_CAMERAS) {
        LOGE("@%s, passed index:%d, max camera:%d, use the first one", __FUNCTION__, index, MAX_CAMERAS);
        index = 0;
    }
    return &(devName[index]);
}

void IPU2HwIsp::getSensorActiveArray()
{
    CLEAR(mSensorArraySize);
    int count = 4;
    const int32_t * range = (int32_t *)MetadataHelper::getMetadataValues( \
        mStaticMeta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, TYPE_INT32, &count);

    if (range) {
        mSensorArraySize[0] = range[2];
        mSensorArraySize[1] = range[3];
    }
    LOG2("%s: %dx%d", __FUNCTION__, mSensorArraySize[0], mSensorArraySize[1]);
}

void IPU2HwIsp::getZoomRatio()
{
    mMaxZoomFactor = mCapInfo->zoomDigitalMax();
    mMaxZoomRatio= 16.0;
    MetadataHelper::getMetadataValue(mStaticMeta,
                                     ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
                                     mMaxZoomRatio);
}

bool IPU2HwIsp::isRAW() const
{
    if (mCapInfo) {
        return (mCapInfo->sensorType() == SENSOR_TYPE_RAW);
    } else {
        LOGE("NO default parameters!");
        return false;
    }
}

void IPU2HwIsp::setDeviceStreams(ICssBufferMaintainer *aDeviceStream,
                          ICssBufferMaintainer::DeviceStreamType type)
{
    switch (type) {
    case ICssBufferMaintainer::DEVICE_STREAM_TYPE_PREVIEW:
        mPreviewHwStream = aDeviceStream;
        break;
    case ICssBufferMaintainer::DEVICE_STREAM_TYPE_VIDEO:
        mVideoHwStream   = aDeviceStream;
        break;
    case ICssBufferMaintainer::DEVICE_STREAM_TYPE_CAPTURE:
        mCaptureHwStream = aDeviceStream;
        break;
    default:
        LOGE("Unknown device stream type %d", type);
        break;
    }
    return;
}

status_t IPU2HwIsp::fillDefaultMetadataFromISP(isp_metadata * metadata)
{
    LOG2("@%s",__FUNCTION__);
    /**
     * FOCAL LENGTH
     */
    int count = 0;
    float *available_focal_length = (float *)MetadataHelper::getMetadataValues(mStaticMeta,
                                     ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
                                     TYPE_FLOAT,
                                     &count);
    if (!available_focal_length)
        return UNKNOWN_ERROR;

    if(count == 1) {
        metadata->focal_length = available_focal_length[0];
    }

    //FPS range
    int32_t * availableFpsRanges = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                     ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                                     TYPE_INT32,
                                     &count);
    if (availableFpsRanges != NULL) {
        for (int i = 0; i < 2 * FPS_RANGES_LEN; i++)
            metadata->fps_range[i] = 0;

        count = MIN(2 * FPS_RANGES_LEN, count);
        for (int i = 0; i < count; i++)
            metadata->fps_range[i] = availableFpsRanges[i];
    }

    return NO_ERROR;
}

/**
 * Trigger to notify the ISP to start polling the event device
 *
 */
status_t IPU2HwIsp::start()
{
    LOG1("@%s",__FUNCTION__);
    Mutex::Autolock _l(mIspStateLock);
    if (mStarted)
        return OK;

    mStarted = true;

    /** The reason to call the function "mSensorEmbeddedMetadata.init()" here is
     * the mSensorEmbeddedMetadata need the metadata buffer size from ISP to
     * malloc the buffer, but the size should be ready after format setting.
     */
    bool supportSensorEmbeddedMetadata = mCapInfo->supportSensorEmbeddedMetadata();
    if (supportSensorEmbeddedMetadata && mSensorEmbeddedMetaData) {
        mSensorEmbeddedMetaData->init();
        mSensorEmbeddedMetaDataSupported = mSensorEmbeddedMetaData->isSensorEmbeddedMetaDataSupported();
    }

    Message msg;
    msg.id = MESSAGE_ID_POLL;
    mMessageQueue.send(&msg);
    return OK;
}

/**
 *
 */
status_t IPU2HwIsp::stop()
{
    LOG1("@%s",__FUNCTION__);
    bool state = false;
    {
    Mutex::Autolock _l(mIspStateLock);
    state = mStarted;
    mStarted = false;
    mDvsConfigChange = false;
    mDvsConfig = NULL;
    }

    if (state) {
        // force current poll to exit
        int fakeEvent = V4L2_EVENT_FRAME_END;
        if (pxioctl(mMainNode, ATOMISP_IOC_INJECT_A_FAKE_EVENT, &fakeEvent) < 0) {
            LOGW("failed to inject fake event, will waiting for poll timeout");
        }
    }

    /** TODO:
     * We need a way to wake up the poller thread. Currently is just waiting for
     * the timeout and then in notifyPolledEvent() we handle that case.
     * The correct way to wake up should be to un-subscribe to all events
     * this change is still in V4L2 upstream review. Other method is to change
     * poll for a select that we wait on 2 fd, one is the device.
     * another one is the ctrl wake up. Thiscoudl be implemented inside the
     * V4L2 convenience classes.
     */

    //flush the message in thread
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    mMessageQueue.remove(MESSAGE_ID_POLL);
    return  mMessageQueue.send(&msg, MESSAGE_ID_FLUSH);
}

status_t IPU2HwIsp::setRawBufferLockMode(bool enable)
{
    LOG2("@%s :%s", __FUNCTION__, enable ? "true" : "false");
    int ret = 0;
    ret = mMainNode->setControl(V4L2_CID_ENABLE_RAW_BUFFER_LOCK,
        enable, "Raw buffer lock mode");
    mRawLockEnabled = enable;
    LOG2("%s IOCTL V4L2_CID_ENABLE_RAW_BUFFER_LOCK: %d\n", __FUNCTION__, ret);
    return ret;
}

status_t IPU2HwIsp::unlockRawBuffer(int expId)
{
    PERFORMANCE_HAL_ATRACE_PARAM1("expId", expId);
    int ret = 0;
    if (mRawLockEnabled) {
        LOG2("@%s ID:%d", __FUNCTION__, expId);
        ret = pxioctl(mMainNode, ATOMISP_IOC_EXP_ID_UNLOCK, &expId);
        LOG2("%s IOCTL ATOMISP_IOC_EXP_ID_UNLOCK ret: %d\n", __FUNCTION__, ret);
    }
    return ret;
}

status_t IPU2HwIsp::triggerCaptureByLockedRaw(int expId)
{
    PERFORMANCE_HAL_ATRACE_PARAM1("expId", expId);
    LOG2("@%s ID:%d", __FUNCTION__, expId);
    int ret;
    if (!mRawLockEnabled) {
        LOGW("invalide raw buffer capture call in mode:%d", mMode);
        return INVALID_OPERATION;
    }

    ret = pxioctl(mMainNode, ATOMISP_IOC_EXP_ID_CAPTURE, &expId);
    LOG2("%s IOCTL ATOMISP_IOC_EXP_ID_CAPTURE ret: %d\n", __FUNCTION__, ret);
    return ret;
}

status_t IPU2HwIsp::processSettings(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;

    if (mMode == CSS_ISP_MODE_NONE)
        status |= processSettingDVS(settings);

    status |= processSettingZoom(settings);
    status |= processSettingRecordingFPS(settings);
    status |= processSettingFocalLength(settings);
    status |= processTestPattern(settings);
    return status;
}

status_t IPU2HwIsp::updateSettingPipelineDepth(CameraMetadata &settings)
{
    status_t status = OK;
    int count = 0;
    uint8_t pipeline_depth;
    //the array of pipeline_depths corresponding to preview, still, video
    uint8_t * pipeline_depths = (uint8_t*)mCapInfo->getPilelineDepths(&count);
    switch(mMode) {
    case CSS_ISP_MODE_NONE:
    case CSS_ISP_MODE_PREVIEW:
         pipeline_depth = pipeline_depths[0];
         break;
    case CSS_ISP_MODE_CONTINUOUS_CAPTURE:
    case CSS_ISP_MODE_CAPTURE:
         pipeline_depth = pipeline_depths[1];
         break;
    case CSS_ISP_MODE_CONTINUOUS_VIDEO:
    case CSS_ISP_MODE_VIDEO:
         pipeline_depth = pipeline_depths[2];
         break;
    default:
         pipeline_depth = pipeline_depths[0];
         break;
    }
    settings.update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipeline_depth, 1);
    return status;
}

/**
 * Sets focal length
 *
 * This takes effect in actuator when the value is set.
 * But now only fixed focal length is supported
 */
status_t IPU2HwIsp::processSettingFocalLength(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_LENS_FOCAL_LENGTH);
    if (entry.count == 1) {
        int count = 0;
        float *available_focal_length = (float *)MetadataHelper::getMetadataValues(mStaticMeta,
                                         ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
                                         TYPE_FLOAT,
                                         &count);
        if (!available_focal_length)
            return UNKNOWN_ERROR;

        for (int i = 0; i < count; i++) {
            if(entry.data.f[0] == available_focal_length[i])
                mFocalLength = round(entry.data.f[0] * 100);
        }
        if (mFocalLength == 0)
            mFocalLength = round(available_focal_length[0] * 100);
    }

    return status;

}

bool IPU2HwIsp::isHighSpeed(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    bool highSpeed = false;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);

    if (entry.count == 2 && entry.data.i32[0] == entry.data.i32[1]) {

        int fps = entry.data.i32[0];

        LOG2("@%s: FPS range: %d, %d", __FUNCTION__, entry.data.i32[0], entry.data.i32[1]);
        if (fps > DEFAULT_RECORDING_FPS){
            highSpeed = true;
        }
    } else if (entry.count == 2 && entry.data.i32[0] != entry.data.i32[1]) {
        int maxFps = MAX(entry.data.i32[0], entry.data.i32[1]);

        LOG2("@%s: FPS range: %d, %d", __FUNCTION__, entry.data.i32[0], entry.data.i32[1]);
        if (maxFps > DEFAULT_RECORDING_FPS){
            highSpeed = true;
        }
    } else if (mVideoFps > DEFAULT_RECORDING_FPS) {
        highSpeed = true;
    }

    LOG2("@%s: highSpeed: %d", __FUNCTION__, highSpeed);
    return highSpeed;
}

status_t IPU2HwIsp::processSettingRecordingFPS(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
    // Here using the metadata to be compatible with V1 API (set preview frame rate)
    // to support High speed and fps change
    if (entry.count == 2 && entry.data.i32[0] == entry.data.i32[1]) {
        int fps = entry.data.i32[0];
        if (fps > 0) {
            int count = 0;
            int32_t * availableFpsRanges = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
                                             TYPE_INT32,
                                             &count);
            //considering of the high speed feature, the fps should be greater than the minimum
            //of the fps range at least
            if (availableFpsRanges != NULL) {
                setRecordingFramerate(fps);
            }
        }
    }
    // If  the value of the pair are different, it's the V3 usage.
    if (entry.count == 2 && entry.data.i32[0] != entry.data.i32[1]) {
        int minFps = MIN(entry.data.i32[0], entry.data.i32[1]);
        int maxFps = MAX(entry.data.i32[0], entry.data.i32[1]);
        status = m3AControls->setManualFrameDurationRange(minFps, maxFps);
    }

    return status;
}

status_t IPU2HwIsp::processSettingDVS(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    int count = 0;
    int* availableVideoStabilizationMode =
                            (int*)MetadataHelper::getMetadataValues(mStaticMeta,
                            ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                            TYPE_BYTE,
                            &count);
    if (count == 1 && availableVideoStabilizationMode != NULL && availableVideoStabilizationMode[0]
                      == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF) {
        LOGW("availableVideoStabilizationMode is OFF");
        return status;
    }

    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
    if (entry.count == 1) {
        status = setDVS(entry.data.u8[0] == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON);
    }

    return status;
}

status_t IPU2HwIsp::processSettingZoom(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    float zoomRatio;

    camera_metadata_ro_entry entry = settings.find(ANDROID_SCALER_CROP_REGION);
    if (entry.count == 4) {
        if (entry.data.i32[2] != 0 && entry.data.i32[3] != 0) {
            zoomRatio = MIN(((float)(mSensorArraySize[0]) / (float)entry.data.i32[2]),
                            ((float)(mSensorArraySize[1]) / (float)entry.data.i32[3]));

            if (fabs(mZoomRatio - zoomRatio) < 0.01)
                return NO_ERROR;
            if ((zoomRatio - mMaxZoomRatio) > 0.01)
                return BAD_VALUE;
            setZoomRatio(zoomRatio);
        }
    }

    return status;
}

status_t IPU2HwIsp::processTestPattern(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    camera_metadata_ro_entry entry;
    int16_t testPatternData[4];
    int32_t testMode = 0;
    int count = 0;
    int* testPatternMode = (int*) MetadataHelper::getMetadataValues(mStaticMeta,
                         ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,TYPE_INT32, &count);

    if ((count == 1 && testPatternMode != NULL
        && testPatternMode[0] ==  ANDROID_SENSOR_TEST_PATTERN_MODE_OFF)
        || testPatternMode == NULL) {
        LOGD("availableTestPatternModes is OFF or isn't supported");
        return status;
    }

    entry = settings.find(ANDROID_SENSOR_TEST_PATTERN_MODE);
    if (entry.count == 1 &&
        entry.data.u8[0] == ANDROID_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR) {
        testMode = ANDROID_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR;
        entry = settings.find(ANDROID_SENSOR_TEST_PATTERN_DATA);
        if (entry.count == 4) {
            testPatternData[0] = entry.data.i32[0] & mAvailableBits;
            testPatternData[1] = entry.data.i32[1] & mAvailableBits;
            testPatternData[2] = entry.data.i32[2] & mAvailableBits;
            testPatternData[3] = entry.data.i32[3] & mAvailableBits;
            setTestPattern(testMode, testPatternData);
        }
    } else if (entry.count == 1) {
        testMode = entry.data.u8[0];
        setTestPattern(testMode);
    }
    return status;
}


/**
 * Configures the ISP continuous capture mode
 *
 * This takes effect in kernel when preview is started.
 */
status_t IPU2HwIsp::configureContinuousMode(ContinuousCaptureConfig &aCfg,
                                           bool enable,
                                           bool continuousVF)
{
    LOG2("@%s", __FUNCTION__);
    if (mMainNode->setControl(V4L2_CID_ATOMISP_CONTINUOUS_MODE, enable,
                              "Continuous mode") < 0)
            return UNKNOWN_ERROR;

   /* We need to update this, CVF cannot run if ISP cannot process captures fast enough
    * for the ISP-preview pipeline not run out of buffers-
    * Until then we hard-coded to false
    *  enable = (!mContCaptPriority &&
              mDefaultParams->snapshotResolutionSupportedByCVF(mPictureConfig.width,
                                                               mPictureConfig.height));*/
    if (mMainNode->setControl(V4L2_CID_ATOMISP_CONTINUOUS_VIEWFINDER, continuousVF,
                              "Continuous viewfinder") < 0)
            return UNKNOWN_ERROR;

    mContCaptConfig = aCfg;
    return NO_ERROR;
}

/**
 * Configures the ISP ringbuffer size in continuous mode.
 *
 * Set all ISP parameters that affect RAW ringbuffer
 * sizing in continuous mode.
 *
 * \see setContCaptureOffset(), setContCaptureSkip() and
 *      setContCaptureNumCaptures()
 */
status_t IPU2HwIsp::configureContinuousRingBuffer(int numOfCapture, bool enableRawLock)
{
    int numBuffers = (enableRawLock) ? (numOfCapture + 4) : 7;
    int captures = mContCaptConfig.numCaptures;
    int offset = mContCaptConfig.offset;
    int lookback = abs(offset);

    if (lookback > captures)
        numBuffers += lookback;
    else
        numBuffers += captures;

    if (numBuffers > mCapInfo->maxContinuousRawRingBuffer())
        numBuffers = mCapInfo->maxContinuousRawRingBuffer();

    LOG1("continuous mode ringbuffer size to %d (captures %d, offset %d)",
         numBuffers, captures, offset);

    if (mMainNode->setControl(V4L2_CID_ATOMISP_CONTINUOUS_RAW_BUFFER_SIZE,
                              numBuffers,
                              "Continuous raw ringbuffer size") < 0)
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

/**
 * Requests driver to capture main and postview images
 *
 * This call is used to trigger captures from the continuously
 * updated RAW ringbuffer maintained in ISP.
 *
 * Call is implemented using the the IOC_S_CONT_CAPTURE_CONFIG
 * atomisp ioctl.
 */
status_t IPU2HwIsp::requestContCapture(int numCaptures, int offset, unsigned int skip)
{
    LOG2("@%s", __FUNCTION__);

    struct atomisp_cont_capture_conf conf;

    CLEAR(conf);
    conf.num_captures = numCaptures;
    conf.offset = offset;
    conf.skip_frames = skip;

    int res = pxioctl(mMainNode, ATOMISP_IOC_S_CONT_CAPTURE_CONFIG, &conf);
    LOG1("@%s: CONT_CAPTURE_CONFIG num %d, offset %d, skip %u, res %d",
         __FUNCTION__, numCaptures, offset, skip, res);
    if (res != 0) {
        LOGE("@%s: error with CONT_CAPTURE_CONFIG, res %d", __FUNCTION__, res);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t IPU2HwIsp::setMode(CssIspMode aMode, bool enableRawLock)
{
    LOG1("@%s: %s", __FUNCTION__,getLogString(sMode2String,aMode));
    struct v4l2_streamparm parm;
    status_t status;
    CLEAR(parm);

    mMode = aMode;
    if (mMode == CSS_ISP_MODE_NONE) {
        setRawBufferLockMode(false);
        mRawLockReady = false;
        if (mSensorEmbeddedMetaData)
            mSensorEmbeddedMetaData->reset();
        return NO_ERROR;
    }

    setRawBufferLockMode(enableRawLock);

    switch (aMode) {
    case CSS_ISP_MODE_PREVIEW:
        parm.parm.capture.capturemode = CI_MODE_PREVIEW;
        break;
    case CSS_ISP_MODE_CAPTURE:
        parm.parm.capture.capturemode = CI_MODE_STILL_CAPTURE;
        break;
    case CSS_ISP_MODE_VIDEO:
        parm.parm.capture.capturemode = CI_MODE_VIDEO;
        break;
    case CSS_ISP_MODE_CONTINUOUS_CAPTURE:
        //parm.parm.capture.capturemode = CI_MODE_CONTINUOUS;
        parm.parm.capture.capturemode = CI_MODE_PREVIEW;
        break;
    case CSS_ISP_MODE_CONTINUOUS_VIDEO:
        parm.parm.capture.capturemode = CI_MODE_VIDEO;
        break;
    case CSS_ISP_MODE_NONE:
        parm.parm.capture.capturemode = CI_MODE_NONE;
        break;
    default:
        LOGE("error invalid ISP mode index= %d", aMode);
        break;
    }

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    status = mMainNode->setParameter(&parm);
    if (status != NO_ERROR) {
        LOGE("error setting the mode %s", getLogString(sMode2String,aMode));
        return UNKNOWN_ERROR;
    }

    mMode = aMode;
    // high speed mode
    if (aMode == CSS_ISP_MODE_VIDEO && mVideoFps > DEFAULT_RECORDING_FPS) {
        struct v4l2_streamparm parm;
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        parm.parm.capture.capturemode = CI_MODE_NONE;
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = mVideoFps;
        LOG1("setting fps: %d", mVideoFps);
        status = mMainNode->setParameter(&parm);
        if (status != NO_ERROR) {
            LOGE("error setting fps: %d", mVideoFps);
            return UNKNOWN_ERROR;
        }
        // Update the configuration with fps from the driver
        if (parm.parm.capture.timeperframe.denominator && parm.parm.capture.timeperframe.numerator) {
            mVideoFps = parm.parm.capture.timeperframe.denominator / parm.parm.capture.timeperframe.numerator;
            LOG1("Sensor fps: %d", mVideoFps);
        } else {
            LOGW("Sensor driver returned invalid frame rate, using default");
            mVideoFps = DEFAULT_RECORDING_FPS;
        }
    }
    return NO_ERROR;
}

status_t IPU2HwIsp::getMakerNote(atomisp_makernote_info *info)
{
    LOG1("@%s: info = %p", __FUNCTION__, info);

    info->focal_length = 0;
    info->f_number_curr = 0;
    info->f_number_range = 0;

    if (pxioctl(mMainNode, ATOMISP_IOC_ISP_MAKERNOTE, info) < 0) {
        LOGW("WARNING: get maker note from driver failed!");
        return UNKNOWN_ERROR;
    }
    // update focal length by app setting, unit is (mm * 100)
    if (mFocalLength != 0)
        info->focal_length = mFocalLength;

    return NO_ERROR;
}

status_t IPU2HwIsp::reSetZoomRatio()
{
    LOG1("@%s", __FUNCTION__);
    return setZoomRatio(mZoomRatio);
}

/**
 * Calculate driver zoom with the following formula:
 * ratio = MaxZoomFactor / (MaxZoomFactor - ZoomDrive)
 */
status_t IPU2HwIsp::setZoomRatio(float zoomRatio)
{
    LOG1("@%s: zoomRatio = %f", __FUNCTION__, zoomRatio);
    status_t status = NO_ERROR;
    if ((mMode == CSS_ISP_MODE_VIDEO || mMode == CSS_ISP_MODE_CONTINUOUS_VIDEO) && mDvs && isRAW()) {
        status = mDvs->setZoom(zoomRatio);

        // Set capture zoom for continuous video mode
        if (mMode == CSS_ISP_MODE_CONTINUOUS_VIDEO) {
            int driverZoom = mMaxZoomFactor - ((float)mMaxZoomFactor)/ zoomRatio;
            int ret = mMainNode->setControl (V4L2_CID_ZOOM_ABSOLUTE, driverZoom, "zoom");
            if (ret < 0) {
                LOGE("Error setting zoom to %4.2f", zoomRatio);
                return UNKNOWN_ERROR;
            }
        }
    } else {
        int driverZoom = mMaxZoomFactor - ((float)mMaxZoomFactor)/ zoomRatio;
        int ret = mMainNode->setControl (V4L2_CID_ZOOM_ABSOLUTE, driverZoom, "zoom");
        if (ret < 0) {
            LOGE("Error setting zoom to %4.2f", zoomRatio);
            return UNKNOWN_ERROR;
        }
    }
    mZoomRatio = zoomRatio;
    return status;
}

status_t IPU2HwIsp::setTestPattern(int32_t testPatternMode, int16_t *testPatternData)
{
    LOG1("@%s, testPatternMode=%d", __FUNCTION__, testPatternMode);
    status_t status = NO_ERROR;

    int ret = mMainNode->setControl (V4L2_CID_TEST_PATTERN, testPatternMode, "Test Pattern");
    if (ret < 0) {
        LOGE("Error setting test pattern mode to %d", testPatternMode);
        return UNKNOWN_ERROR;
    }
    if (testPatternData != NULL && testPatternMode == ANDROID_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR) {
        ret = mMainNode->setControl (V4L2_CID_TEST_PATTERN_COLOR_R, testPatternData[0], "Test Pattern Solid Color R");
        if (ret < 0) {
            LOGE("Error setting test pattern data to %d", testPatternData[0]);
            return UNKNOWN_ERROR;
        }
        ret = mMainNode->setControl (V4L2_CID_TEST_PATTERN_COLOR_GR, testPatternData[1], "Test Pattern Solid Color GR");
        if (ret < 0) {
            LOGE("Error setting test pattern data to %d", testPatternData[1]);
            return UNKNOWN_ERROR;
        }
        ret = mMainNode->setControl (V4L2_CID_TEST_PATTERN_COLOR_GB, testPatternData[2], "Test Pattern Solid Color GB");
        if (ret < 0) {
            LOGE("Error setting test pattern data to %d", testPatternData[2]);
            return UNKNOWN_ERROR;
        }
        ret = mMainNode->setControl (V4L2_CID_TEST_PATTERN_COLOR_B, testPatternData[3], "Test Pattern Solid Color B");
        if (ret < 0) {
            LOGE("Error setting test pattern data to %d", testPatternData[3]);
            return UNKNOWN_ERROR;
        }
    }
    return status;
}


status_t IPU2HwIsp::initDVS()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    HWControlGroup hwcg;
    hwcg.mIspCI = (IHWIspControl*) this;
    hwcg.mSensorCI = mSensorCI;
    mDvs = new IaDvs2(hwcg);
    if (!mDvs)
        return NO_INIT;

    status = mDvs->dvsInit();
    if (status != NO_ERROR) {
        LOGE("%s: Failed to initiate DVS", __FUNCTION__);
        delete mDvs;
        mDvs = NULL;
    }
    return status;
}

status_t IPU2HwIsp::setDVS(bool enable)
{
    LOG1("@%s: %d", __FUNCTION__, enable);
    status_t status = NO_ERROR;

    if (mMode != CSS_ISP_MODE_NONE) {
        LOGW("@%s: Cannot change DVS status while ISP is running, mode = %d", __FUNCTION__, mMode);
        return INVALID_OPERATION;
    }

    if (enable && mDvs && !mDvs->isDvsValid()) {
        enable = false;
        LOGW("@%s: Cannot start DVS due to some restrictions in size or frame rate", __FUNCTION__);
    }

    status = mMainNode->setControl(V4L2_CID_ATOMISP_VIDEO_STABLIZATION,
                                    enable, "Video Stabilization");
    if(status != 0)
    {
        LOGE("Error setting DVS in the driver");
        status = INVALID_OPERATION;
    } else {
        mDvsEnabled = enable;
    }
    return status;
}

status_t IPU2HwIsp::registerToEvents()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = OK;

    status =  mEventNode->subscribeEvent(V4L2_EVENT_FRAME_END);

    if (status == OK)
        status =  mEventNode->subscribeEvent(V4L2_EVENT_ATOMISP_3A_STATS_READY);

    if (status == OK)
        status =  mEventNode->subscribeEvent(V4L2_EVENT_ATOMISP_METADATA_READY);

    if (status == OK)
        status =  mEventNode->subscribeEvent(V4L2_EVENT_ATOMISP_RAW_BUFFERS_ALLOC_DONE);

    return status;
}

status_t IPU2HwIsp::getDvsStatistics(struct atomisp_dis_statistics *stats,
                                   bool *tryAgain) const
{
    /* This is a blocking call, so we do not lock a mutex here. The method
       is const, so the mutex is not needed anyway. */
    status_t status = NO_ERROR;
    int ret;
    ret = pxioctl(mMainNode, ATOMISP_IOC_G_DIS_STAT, stats);
    if (tryAgain)
        *tryAgain = (errno == EAGAIN);
    if (errno == EAGAIN)
        return NO_ERROR;

    if (ret < 0) {
        LOGE("failed to get DVS statistics");
        status = UNKNOWN_ERROR;
    }
    return status;
}

status_t IPU2HwIsp::setDvsCoefficients(const struct atomisp_dis_coefficients *coefs) const
{
    status_t status = NO_ERROR;
    if (pxioctl(mMainNode, ATOMISP_IOC_S_DIS_COEFS, (struct atomisp_dis_coefficients *)coefs) < 0) {
        LOGE("failed to set dvs coefficients");
        status = UNKNOWN_ERROR;
    }
    return status;
}

status_t IPU2HwIsp::getIspDvs2BqResolutions(struct atomisp_dvs2_bq_resolutions *bq_res) const
{
    status_t status = NO_ERROR;
    if (pxioctl(mMainNode, ATOMISP_IOC_G_DVS2_BQ_RESOLUTIONS, bq_res) < 0) {
        LOGE("failed to get ISP dvs2 bq resolutions");
        status = UNKNOWN_ERROR;
    }
    return status;
}


/////////////////////////////////////////////////////////////
//  IspControl
////////////////////////////////////////////////////////////
status_t IPU2HwIsp::getIspParameters(struct atomisp_parm *isp_param) const
{
    status_t status = NO_ERROR;
    if (pxioctl(mMainNode, ATOMISP_IOC_G_ISP_PARM, isp_param) < 0) {
        LOGE("failed to get ISP parameters");
        status = UNKNOWN_ERROR;
    }
    return status;
}

status_t IPU2HwIsp::setColorEffect(v4l2_colorfx effect)
{
    mColorEffect = effect;
    return NO_ERROR;
}

status_t IPU2HwIsp::applyColorEffect()
{
    LOG2("@%s: effect = %d", __FUNCTION__, mColorEffect);
    status_t status = NO_ERROR;

    // Color effect overrides configs that AIC has set
    if (mMainNode->setControl(V4L2_CID_COLORFX, mColorEffect, "Colour Effect") < 0){
        LOGE("Error setting color effect");
        return UNKNOWN_ERROR;
    }
    return status;
}


/**
 * Attach a Listening client to a particular event
 *
 *
 * @param observer interface pointer to attach
 * @param event concrete event to listen to
 */
status_t IPU2HwIsp::attachListener(ICssIspListener *observer,
                                     ICssIspListener::IspEventType event)
{
    LOG1("@%s: %p to event type %d", __FUNCTION__, observer, event);
    status_t status = NO_ERROR;
    if (observer == NULL)
        return BAD_VALUE;
    // Check event to be in the range of allowed events.
    // we do not allow frame event from ISP (maybe in the future?)
    if ((event < ICssIspListener::ISP_EVENT_TYPE_FRAME ) ||
        (event > ICssIspListener::ISP_EVENT_MAX)) {
        return BAD_VALUE;
    }

    // Check if we have any listener registered to this event
    int index = mListeners.indexOfKey(event);
    if (index == NAME_NOT_FOUND) {
        // First time someone registers for this event
        listener_list_t theList;
        theList.push_back(observer);
        mListeners.add(event, theList);
        return NO_ERROR;
    }

    // Now we will have more than one listener to this event
    listener_list_t &theList = mListeners.editValueAt(index);

    List<ICssIspListener*>::iterator it = theList.begin();
    for (;it != theList.end(); ++it)
        if (*it == observer)
            return ALREADY_EXISTS;

    theList.push_back(observer);

    mListeners.replaceValueFor(event,theList);
    return status;
}

/**
 * Detach observer interface
 *
 *
 * @param observer interface pointer to detach
 * @param event identifier for event type to detach
 */
status_t IPU2HwIsp::detachListener(ICssIspListener *listener,
                                     ICssIspListener::IspEventType event)
{
    LOG1("@%s:%d", __FUNCTION__, event);

    if (listener == NULL)
        return BAD_VALUE;

    Mutex::Autolock _l(mListenerLock);
    status_t status = BAD_VALUE;
    int index = mListeners.indexOfKey(event);
    if (index == NAME_NOT_FOUND) {
       LOGW("No listener register for this event, ignoring");
       return NO_ERROR;
    }

    listener_list_t &theList = mListeners.editValueAt(index);
    List<ICssIspListener*>::iterator it = theList.begin();
    for (;it != theList.end(); ++it) {
        if (*it == listener) {
            theList.erase(it);
            if (theList.empty()) {
                mListeners.removeItemsAt(index);
            }
            status = NO_ERROR;
            break;
        }
    }
    return status;
}

status_t IPU2HwIsp::copyLensShading(cmc_parsed_lens_shading_t *srcLenShadingInfo,
                                         cmc_parsed_lens_shading_t *destLenShadingInfo)
{
    LOG1("%s", __FUNCTION__);
    Mutex::Autolock  l(mLscMapLock);
    if (srcLenShadingInfo == NULL || srcLenShadingInfo->cmc_lens_shading == NULL) {
        LOGE("%s invalid source", __FUNCTION__);
        return BAD_VALUE;
    }
    uint16_t grid_width = srcLenShadingInfo->cmc_lens_shading->grid_width;
    uint16_t grid_height = srcLenShadingInfo->cmc_lens_shading->grid_height;
    uint32_t size = 4 * grid_width * grid_height;
    LOG2("grid_width:%d,grid_height:%d", grid_width, grid_height);

    if (destLenShadingInfo->cmc_lens_shading != NULL
        && (destLenShadingInfo->cmc_lens_shading->grid_width != grid_width
        || destLenShadingInfo->cmc_lens_shading->grid_height != grid_height)) {
        if (destLenShadingInfo->lsc_grids != NULL) {
            delete[] destLenShadingInfo->lsc_grids;
            destLenShadingInfo->lsc_grids = NULL;
        }
        destLenShadingInfo->lsc_grids = new uint16_t[size];
        if (destLenShadingInfo->lsc_grids == NULL) {
            LOGW("there is no memory");
            return NO_MEMORY;
        }
    }

    if (srcLenShadingInfo->cmc_lsc_grids != NULL && destLenShadingInfo->cmc_lsc_grids != NULL)
        memcpy(destLenShadingInfo->cmc_lsc_grids, srcLenShadingInfo->cmc_lsc_grids,
               sizeof (cmc_lsc_grid_t));

    if (srcLenShadingInfo->cmc_lsc_rg_bg_ratios != NULL && destLenShadingInfo->cmc_lsc_rg_bg_ratios != NULL)
        memcpy(destLenShadingInfo->cmc_lsc_rg_bg_ratios, srcLenShadingInfo->cmc_lsc_rg_bg_ratios,
               sizeof (cmc_lsc_rg_bg_t));

    if (srcLenShadingInfo->cmc_lens_shading != NULL && destLenShadingInfo->cmc_lens_shading != NULL)
        memcpy(destLenShadingInfo->cmc_lens_shading, srcLenShadingInfo->cmc_lens_shading,
               sizeof (cmc_lens_shading_t));

    if (srcLenShadingInfo->lsc_grids != NULL && destLenShadingInfo->lsc_grids != NULL) {
        memcpy(destLenShadingInfo->lsc_grids, srcLenShadingInfo->lsc_grids,
               size * sizeof(uint16_t));
    }

    return NO_ERROR;
}

status_t IPU2HwIsp::getLensShading(LensShadingInfo *lensShadingInfo)
{
    Mutex::Autolock  l(mLscMapLock);
    uint16_t lscWidth = 0;
    uint16_t lscHeight = 0;

    lscWidth = mWeightedLsc.cmc_lens_shading->grid_width;
    lscHeight = mWeightedLsc.cmc_lens_shading->grid_height;
    int size = 4 * lscWidth * lscHeight;
    if (lensShadingInfo->grid_width != lscWidth ||
        lensShadingInfo->grid_height != lscHeight) {
        if (lensShadingInfo->lsc_grids != NULL) {
            delete[] lensShadingInfo->lsc_grids;
            lensShadingInfo->lsc_grids = NULL;
        }
        lensShadingInfo->lsc_grids = new uint16_t [size];
        if (lensShadingInfo->lsc_grids == NULL) {
            LOGW("there is no memory");
            return NO_MEMORY;
        }
        lensShadingInfo->grid_width = lscWidth;
        lensShadingInfo->grid_height = lscHeight;
    }
    if (lensShadingInfo->lsc_grids != NULL)
        memcpy(lensShadingInfo->lsc_grids, mWeightedLsc.lsc_grids, size * sizeof(uint16_t));

    return NO_ERROR;

}
status_t IPU2HwIsp::initialDvsConfig()
{
    LOG1("%s", __FUNCTION__);
    status_t ret = NO_ERROR;
    struct atomisp_dvs_6axis_config * dvsConfig = mDvs->getMorphTable();
    if (dvsConfig != NULL) {
        ret = pxioctl(mMainNode, ATOMISP_IOC_S_DIS_VECTOR, dvsConfig);
        LOG2("%s IOCTL ATOMISP_IOC_S_PARAMETERS ret: %d\n", __FUNCTION__, ret);
    }

    return ret;
}

status_t IPU2HwIsp::setIspParameters(const ispInputParameters *ispInputParams)
{
    LOG2("@%s, isp_config_id = %d", __FUNCTION__, ispInputParams->isp_config_id);
    int ret = NO_ERROR;
    ia_err iaErr;
    ia_isp_2_2_input_params isp2400Params;
    ia_binary_data output_data;

    CLEAR(isp2400Params);
    CLEAR(output_data);

    const IPU2CameraCapInfo * cap = getIPU2CameraCapInfo(mCameraId);
    if (cap != NULL) {
        isp2400Params.isp_vamem_type = cap->ispVamemType();
    } else {
        isp2400Params.isp_vamem_type = 1;
    }

    // set the ISP independent parameters
    isp2400Params.frame_use = ispInputParams->frame_use;
    isp2400Params.sensor_frame_params = ispInputParams->sensor_frame_params;
    isp2400Params.exposure_results = ispInputParams->exposure_results;
    isp2400Params.awb_results = ispInputParams->awb_results;
    isp2400Params.gbce_results = (ia_aiq_gbce_results *)&ispInputParams->gbce_results;
    isp2400Params.pa_results = ispInputParams->pa_results;
    isp2400Params.manual_brightness = ispInputParams->manual_brightness;
    isp2400Params.manual_contrast = ispInputParams->manual_contrast;
    isp2400Params.manual_hue = ispInputParams->manual_hue;
    isp2400Params.manual_saturation = ispInputParams->manual_saturation;
    isp2400Params.nr_setting.feature_level = ia_isp_feature_level_high;
    isp2400Params.nr_setting.strength = 0;
    isp2400Params.ee_setting.feature_level = ia_isp_feature_level_high;
    isp2400Params.ee_setting.strength = ispInputParams->edgeStrength;
    isp2400Params.effects = ispInputParams->effects;

    // save lensShadingMap locally
    if (ispInputParams->pa_results) {
        if (ispInputParams->pa_results->weighted_lsc != NULL &&
            ispInputParams->pa_results->weighted_lsc->cmc_lens_shading != NULL) {
            // store lens shading map
            copyLensShading(ispInputParams->pa_results->weighted_lsc, &mWeightedLsc);
        } else if (ispInputParams->pa_results->weighted_lsc == NULL && mStarted == false) {
            // load stored lens shading map
            copyLensShading(&mWeightedLsc, &mWeightedLscForIsp);
            isp2400Params.pa_results->weighted_lsc = &mWeightedLscForIsp;
        }
    }

    // convert AIQ output parameters to ISP2400 specific parameters
    iaErr = ia_isp_2_2_run(mIspHandle, &isp2400Params, &output_data);

    if (iaErr == 0 && output_data.data) {
        struct atomisp_parameters *ispConfStruct = &mIspConfStruct;
        memcpy(ispConfStruct, output_data.data, sizeof(struct atomisp_parameters));

        if (ispInputParams->noiseReduction == false) {
            // Disable the Noise Reduction
            CLEAR(*ispConfStruct->nr_config);
        }
        if (ispInputParams->edgeEnhancement == false) {
            // Disable the Edge Enhancement
            CLEAR(*ispConfStruct->ee_config);
            ispConfStruct->ee_config->threshold = 65535;
        }
        if (ispInputParams->lensShading == false && ispConfStruct->shading_table != NULL) {
            // Disable the lens shading
            int size = ispConfStruct->shading_table->width * ispConfStruct->shading_table->height;
            for (int i = 0; i < ATOMISP_NUM_SC_COLORS; i++) // 4 channels
                for (int j = 0; j < size; j++)
                    ispConfStruct->shading_table->data[i][j] = 1;
        }

        if (mNoiseReductionEdgeEnhancement == false) {
            //Disable the Noise Reduction and Edge Enhancement
            CLEAR(*ispConfStruct->ee_config);
            CLEAR(*ispConfStruct->nr_config);
            CLEAR(*ispConfStruct->de_config);
            ispConfStruct->ee_config->threshold = 65535;
            LOG2("Disabled NREE in 3A");
        }

        ispConfStruct->isp_config_id = ispInputParams->isp_config_id;

        //Add dvs 6axis data, the 6axis data is updated in per-frame setting.
        if (mDvsConfigChange) {
            ispConfStruct->dvs_6axis_config = mDvsConfig;
            if (ispConfStruct->isp_config_id != 0)
                mDvsConfig = mDvs->getMorphTable();
        }

        if (ispConfStruct->isp_config_id == 0) {
            ret = pxioctl(mMainNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
            LOG2("%s IOCTL ATOMISP_IOC_S_PARAMETERS ret: %d\n", __FUNCTION__, ret);
            return ret;
        }
        switch (mMode) {
        case CSS_ISP_MODE_PREVIEW:
            ret = pxioctl(mPreviewNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
            LOG2("%s: PREVIEW: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            break;
        case CSS_ISP_MODE_CAPTURE:
            ret = pxioctl(mMainNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
            LOG2("%s: CAPTURE: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            break;
        case CSS_ISP_MODE_VIDEO:
            ret = pxioctl(mVideoNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
            LOG2("%s: VIDEO: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            break;
        case CSS_ISP_MODE_CONTINUOUS_VIDEO:
            // Queue buffer and set isp parameter should be called in pair. For
            // continuous video, need to check whether the buffer is available
            // before setting the parameters.
            if (ispInputParams->availableStreams & STREAM_CAPTURE) {
                ret = pxioctl(mMainNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
                LOG2("%s: CSS_ISP_MODE_CONTINUOUS_VIDEO mode: CONT-capture: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            }
            if (ispInputParams->availableStreams & STREAM_PREVIEW) {
                ret = pxioctl(mPreviewNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
                LOG2("%s: CSS_ISP_MODE_CONTINUOUS_VIDEO mode: CONT-preview: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            }
            if (ispInputParams->availableStreams & STREAM_VIDEO) {
                ret = pxioctl(mVideoNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
                LOG2("%s: CSS_ISP_MODE_CONTINUOUS_VIDEO mode: CONT-video: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            }
            break;
        case CSS_ISP_MODE_CONTINUOUS_CAPTURE:
            // Queue buffer and set isp parameter should be called in pair. For
            // continuous capture, need to check whether the buffer is available
            // before setting the parameters.
            if (ispInputParams->availableStreams & STREAM_CAPTURE) {
                ret = pxioctl(mMainNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
                LOG2("%s: CSS_ISP_MODE_CONTINUOUS_CAPTURE mode: CONT-capture: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            }
            if (ispInputParams->availableStreams & STREAM_PREVIEW) {
                ret = pxioctl(mPreviewNode, ATOMISP_IOC_S_PARAMETERS, ispConfStruct);
                LOG2("%s: CSS_ISP_MODE_CONTINUOUS_CAPTURE mode: CONT-preview: isp_config_id=%d, ret=%d",
                            __FUNCTION__, ispConfStruct->isp_config_id, ret);
            }
            break;
        case CSS_ISP_MODE_NONE:
            LOGE("Isp mode is none, should not set parameters.");
            break;
        default:
            LOGE("Unknown isp mode %d.", mMode);
            break;
        }

        return ret;
    }
    else {
        switch (iaErr) {
        case ia_err_none:
            return NO_ERROR;
        case ia_err_general:
            return UNKNOWN_ERROR;
        case ia_err_nomemory:
            return NO_MEMORY;
        case ia_err_data:
            return BAD_VALUE;
        case ia_err_internal:
            return INVALID_OPERATION;
        case ia_err_argument:
            return BAD_VALUE;
        default:
            return UNKNOWN_ERROR;
        }
    }
}

int IPU2HwIsp::getSensorEmbeddedMetaData(sensor_embedded_metadata *sensorMetaData,
    uint8_t metadata_type)
{
    LOG2("@%s", __FUNCTION__);
    int ret;
    atomisp_metadata_with_type metaData;
    CLEAR(metaData);
    metaData.type =  (enum atomisp_metadata_type)metadata_type;
    metaData.data = sensorMetaData->data;
    metaData.effective_width = (uint32_t*)sensorMetaData->effective_width;
    ret = pxioctl(mMainNode, ATOMISP_IOC_G_METADATA_BY_TYPE, &metaData);
    LOG2("%s IOCTL ATOMISP_IOC_G_METADATA ret: %d\n", __FUNCTION__, ret);
    sensorMetaData->exp_id = metaData.exp_id;
    sensorMetaData->stride = metaData.stride;
    sensorMetaData->height = metaData.height;
    return ret;
}

// Get ISP statistics with exp_id.
// If exp_id is 0, get the latest one and feed back with the real exp_id
status_t IPU2HwIsp::getIspStatistics(ia_aiq_rgbs_grid **out_rgbs_grid,
                                    ia_aiq_af_grid **out_af_grid, unsigned int *exp_id)
{
    LOG2("@%s", __FUNCTION__);
    ia_err iaErr;

    // m3aStatistics needs to have correct atomisp_grid_info when calling the ioctl
    //ret = pxioctl(mMainNode, ATOMISP_IOC_G_3A_STAT, m3aStatistics);
    struct atomisp_3a_statistics * statistics = get3AStatistics(*exp_id);
    if (statistics == NULL) {
        LOGE("Failure in getting ISP Statistics");
        return UNKNOWN_ERROR;
    }
    // The valid exposure id scope: 1--250
    if (*exp_id == 0)
        *exp_id = statistics->exp_id;
    iaErr = ia_isp_2_2_statistics_convert(mIspHandle, statistics,
                                          out_rgbs_grid, out_af_grid);

    if (iaErr != ia_err_none)
        LOGW("iaErr: %d", iaErr);

    switch (iaErr) {
    case ia_err_none:
        return NO_ERROR;
    case ia_err_general:
        return UNKNOWN_ERROR;
    case ia_err_nomemory:
        return NO_MEMORY;
    case ia_err_data:
        return BAD_VALUE;
    case ia_err_internal:
        return INVALID_OPERATION;
    case ia_err_argument:
        return BAD_VALUE;
    default:
        return UNKNOWN_ERROR;
    }

}

status_t IPU2HwIsp::getDecodedExposureParams(ia_aiq_exposure_sensor_parameters* sensor_exp_p,
                                            ia_aiq_exposure_parameters* generic_exp_p, unsigned int exp_id)
{
    LOG2("@%s", __FUNCTION__);
    status_t ret = UNKNOWN_ERROR;
    if (mSensorEmbeddedMetaData)
        ret = mSensorEmbeddedMetaData->getDecodedExposureParams(sensor_exp_p, generic_exp_p, exp_id);

    return ret;
}

bool IPU2HwIsp::checkSensorMetadataAvailable(unsigned int exp_id)
{
    LOG2("@%s", __FUNCTION__);
    bool available = false;
    bool supportSensorEmbeddedMetadata = mCapInfo->supportSensorEmbeddedMetadata();
    if (supportSensorEmbeddedMetadata && mSensorEmbeddedMetaData)
        available = mSensorEmbeddedMetaData->checkSensorMetadataAvailable(exp_id);
    else {
        // todo: for some sensor, they don't support sensor metadata, so set it to true
        available = true;
    }
    return available;
}

status_t IPU2HwIsp::notifyListeners(ICssIspListener::IspMessage *msg)
{
    LOG2("@%s", __FUNCTION__);
    bool ret = false;
    Mutex::Autolock _l(mListenerLock);

    //ignore to send failed event to listener
    if (msg->id == ICssIspListener::ISP_MESSAGE_ID_ERROR ||
        msg->id != ICssIspListener::ISP_MESSAGE_ID_EVENT)
        return true;

    if (mSensorEmbeddedMetaDataSupported) {
        //handle sensor embedded metadata event
        if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY) {
            mSensorEmbeddedMetaData->handleSensorEmbeddedMetaData(msg->data.event.exp_id,
                                                          msg->data.event.metadata_type);
            // If there is cached 3A statistics, check whether need to trig 3a
            if (mTrigger3A & ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY &&
                msg->data.event.exp_id >= mCachedStatsEventMsg.data.event.exp_id)
                if (msg->data.event.exp_id > mCachedStatsEventMsg.data.event.exp_id)
                    LOGW("%s: expecting metadata %d, but get %d", __FUNCTION__,
                        mCachedStatsEventMsg.data.event.exp_id, msg->data.event.exp_id);
                mTrigger3A |= ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY;
        }

        /* When both sensor metadata event and statistics event are ready, then triggers 3A run
         */
        if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY) {
            mTrigger3A |= ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY;
            if (checkSensorMetadataAvailable(msg->data.event.exp_id)) {
                mTrigger3A = 0;
                CLEAR(mCachedStatsEventMsg);
            } else {
                mCachedStatsEventMsg = *msg;
                return true;
            }
        }
    }

    if (mListeners.size() > 0) {
        listener_list_t &theList = mListeners.editValueFor(msg->data.event.type);
        List<ICssIspListener*>::iterator it = theList.begin();
        for (;it != theList.end(); ++it)
            ret |= (*it)->notifyIspEvent((ICssIspListener::IspMessage*)msg);
    }

    if (mSensorEmbeddedMetaDataSupported && (mTrigger3A & ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY)
        && (mTrigger3A & ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY)) {
        if (mListeners.size() > 0) {
            listener_list_t &theList = mListeners.editValueFor(mCachedStatsEventMsg.data.event.type);
            List<ICssIspListener*>::iterator it = theList.begin();
            for (;it != theList.end(); ++it)
                ret |= (*it)->notifyIspEvent((ICssIspListener::IspMessage*)&mCachedStatsEventMsg);
        }
        CLEAR(mCachedStatsEventMsg);
        mTrigger3A = 0;
    }

    return ret;
}

status_t IPU2HwIsp::handleMessagePoll(Message &msg)
{
    LOG2("@%s",__FUNCTION__);
    UNUSED(msg);
    struct v4l2_event event;
    ICssIspListener::IspMessage outMsg;
    int ret = 0;

    int timeOutCount = CSS_EVENT_POLL_TIMEOUT_COUNT;
    while (timeOutCount-- && ret == 0 && mStarted) {
        ret = mEventNode->poll(CSS_EVENT_POLL_TIMEOUT);
    };

    if (!mStarted) {
        return NO_ERROR;
    }

    // TODO implement ISP timeout recover mechanism
    // outMsg.id = ICssIspListener::ISP_MESSAGE_ID_ERROR;
    // todo - should we pass error messages to observers
    // until further client controls?
    if (ret < 0) {
        LOGE("Event sync poll error");
        outMsg.id = ICssIspListener::ISP_MESSAGE_ID_ERROR;
        notifyListeners(&outMsg);
        return UNKNOWN_ERROR;
    }

    if (ret == 0) {
        LOGW("Event sync poll timeout");
        outMsg.id = ICssIspListener::ISP_MESSAGE_ID_ERROR;
        notifyListeners(&outMsg);
        return TIMED_OUT;
    }

    // poll was successful, dequeue the event right away
    ret = mEventNode->dequeueEvent(&event);
    if (ret < 0) {
        LOGW("Dequeue stats event failed");
        return UNKNOWN_ERROR;
    }

    if ((event.type == V4L2_EVENT_ATOMISP_RAW_BUFFERS_ALLOC_DONE ||
        event.type == V4L2_EVENT_FRAME_END ||
        event.type == V4L2_EVENT_ATOMISP_3A_STATS_READY ||
        event.type == V4L2_EVENT_ATOMISP_METADATA_READY) &&
        mStarted) {
        Message pollmsg;
        pollmsg.id = MESSAGE_ID_POLL;
        mMessageQueue.send(&pollmsg);
    }

    if (event.type == V4L2_EVENT_ATOMISP_RAW_BUFFERS_ALLOC_DONE) {
       LOG2("RAW buffer alloc done");
        mRawLockReady = true;
    } else {
        // fill observer message
        outMsg.id = ICssIspListener::ISP_MESSAGE_ID_EVENT;
        outMsg.data.event.timestamp.tv_sec  = event.timestamp.tv_sec;
        outMsg.data.event.timestamp.tv_usec = event.timestamp.tv_nsec / 1000;
        outMsg.data.event.sequence = event.sequence;

        switch(event.type) {
        case V4L2_EVENT_FRAME_END: {
            // We receive EOF at vbi and need delay sending the FrameSync event
            // base on estimated vbi.
            // vbi of EOF n -> SOF n+1, should be calculated per exposure time
            // of FRAME n+1, which should be applied at FRAME n+1-ExposureLag,
            // and is ExposureLag-1 earlier than the latest one.
            unsigned int vbiOffset;
            unsigned int shutterOffset;
            mSensorCI->vbiIntervalForItem(mSensorCI->getExposureDelay()-1,
                                          vbiOffset, shutterOffset);
            LOG2("vbiOffset = %d, shutterOffset = %d", vbiOffset, shutterOffset);
            nsecs_t ts = TIMEVAL2USECS(&outMsg.data.event.timestamp);
            ts = ts + vbiOffset;
            outMsg.data.event.timestamp.tv_sec = ts / 1000000;
            outMsg.data.event.timestamp.tv_usec = (ts % 1000000);
            outMsg.data.event.sequence++;
            outMsg.data.event.exp_id =
                (event.u.frame_sync.frame_sequence == MAX_EXP_ID) ? \
                1 : (event.u.frame_sync.frame_sequence + 1);
            outMsg.data.event.vbiOffset = vbiOffset;
            outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_SOF;
            outMsg.data.event.shutterTime = ts - shutterOffset;
            break;
        }
        case V4L2_EVENT_ATOMISP_3A_STATS_READY: {
            // Get and store the 3a statistics
            unsigned int expoure_id = 0;
            store3AStatistics(&expoure_id);
            outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY;
            outMsg.data.event.exp_id = expoure_id;
            break;
        }
        case V4L2_EVENT_ATOMISP_METADATA_READY:
            outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY;
            outMsg.data.event.metadata_type = event.u.data[0];
            break;
        default:
            outMsg.id = ICssIspListener::ISP_MESSAGE_ID_ERROR;
            LOGW("Unsupported event was returned from ISP device!!");
            break;
        }

        if (outMsg.id == ICssIspListener::ISP_MESSAGE_ID_EVENT) {
            LOG2("Isp event %d arrived", outMsg.data.event.type);
            notifyListeners(&outMsg);
        } else  if (outMsg.id == ICssIspListener::ISP_MESSAGE_ID_ERROR) {
            if(mStarted == true) {
                LOGW("Error while polling isp event device");
                mStarted = false;
            }
            // here is where the ISP timeout recover mechanism is implemented
            // we can decide to ignore the events
            notifyListeners(&outMsg);
        }
    }

    return NO_ERROR;
}

status_t IPU2HwIsp::handleMessageFlush(Message &msg)
{
    status_t status = NO_ERROR;
    UNUSED(msg);
    LOG2("@%s:", __FUNCTION__);
    mMessageQueue.reply(MESSAGE_ID_FLUSH, status);
    return status;
}

void IPU2HwIsp::messageThreadLoop(void)
{
    LOG2("@%s: Start", __FUNCTION__);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);
        PERFORMANCE_HAL_ATRACE_PARAM1("msg", msg.id);

        bool replyImmediately = true;
        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            mThreadRunning = false;
            status = NO_ERROR;
            break;

        case MESSAGE_ID_POLL:
            status = handleMessagePoll(msg);
            break;

        case MESSAGE_ID_FLUSH:
            status = handleMessageFlush(msg);
            replyImmediately = false;
            break;

        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("    error %d in handling message: %d", status, (int)msg.id);
        if (replyImmediately)
            mMessageQueue.reply(msg.id, status);
    }
    LOG2("%s: Exit", __FUNCTION__);
}

void IPU2HwIsp::initIaIspAdaptor(const ia_binary_data *cpfData,
                                unsigned int maxStatsWidth,
                                unsigned int maxStatsHeight,
                                ia_cmc_t *cmc,
                                ia_mkn *mkn)
{
    LOG1("@%s", __FUNCTION__);

    mIspHandle = ia_isp_2_2_init(cpfData,
                                 maxStatsWidth,
                                 maxStatsHeight,
                                 cmc,
                                 mkn);

}

void IPU2HwIsp::sensorModeChanged()
{
    LOG1("@%s", __FUNCTION__);
    //if (m3aStatistics) {
    //    freeStatistics(m3aStatistics);
    //}
    //m3aStatistics = allocateStatistics();
    deinit3AStatisticsQueue();
    init3AStatisticsQueue();
    // in CSS2.0 mDvs is mandatory for zoom functionality
    if ((mMode == CSS_ISP_MODE_VIDEO || mMode == CSS_ISP_MODE_CONTINUOUS_VIDEO) && mDvs != NULL) {
        if (mDvs->reconfigure() == NO_ERROR)
            mDvs->setZoom(mZoomRatio);
    }
}

void IPU2HwIsp::setVideoInfo(FrameInfo *info)
{
    LOG1("@%s", __FUNCTION__);
    mVideoInfo = *info;
}

void IPU2HwIsp::setPreviewInfo(FrameInfo *info)
{
    LOG1("@%s", __FUNCTION__);
    mPreviewInfo = *info;
}

void IPU2HwIsp::setRecordingFramerate(int fps)
{
    LOG1("@%s fps:%d", __FUNCTION__, fps);
    // high speed
    status_t status = NO_ERROR;
    if (fps > DEFAULT_RECORDING_FPS) {
        if (validateHighSpeedResolutionFps(mVideoInfo.width, mVideoInfo.height, fps)) {
            mVideoFps = fps;
            status = m3AControls->setFrameRate(fps);
            if (status != NO_ERROR)
                mVideoFps = DEFAULT_RECORDING_FPS;
        } else {
            mVideoFps = DEFAULT_RECORDING_FPS;
        }
    }

    if (fps <= DEFAULT_RECORDING_FPS && m3AControls) {
        status = m3AControls->setFrameRate(fps);
        if (status == NO_ERROR)
            mVideoFps = fps;
    }

}

void IPU2HwIsp::getVideoSize(int *w, int *h) const
{
    if (w == NULL || h == NULL) {
        LOGE("%s: NULL width or height output parameter", __FUNCTION__);
        return;
    }
    *w = mVideoInfo.width;
    *h = mVideoInfo.height;
}

bool IPU2HwIsp::validateHighSpeedResolutionFps(int width, int height, int fps)
{
    LOG1("@%s size: %dx%d @ %d", __FUNCTION__, width, height, fps);

    if (fps > DEFAULT_RECORDING_FPS) {
        LOG1("high speed video recording mode");
        char sizeFpsStr[20];
        snprintf(sizeFpsStr, 20, "%dx%d@%d", width, height, fps);
        const char* supportedSizeFpsCombos = mCapInfo->supportedHighSpeedResolutionFps();
        if (!validateString(sizeFpsStr, supportedSizeFpsCombos)) {
            LOGE("Unsupported high-speed video size@fps combination: %s, supported: %s", sizeFpsStr, supportedSizeFpsCombos);
            return false;
        }
    }
    return true;
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
const char * IPU2HwIsp::getLogString(const char *array[], unsigned int index)
{
    if (index > sizeof(*array))
        return "-1";
    return array[index];
}

status_t IPU2HwIsp::init3AStatisticsQueue()
{
    LOG1("%s", __FUNCTION__);
    status_t ret = NO_ERROR;
    m3aStatisticsQueue.clear();
    m3aStatisticsQueue.setCapacity(MAX_STATISTICS_QUEUE_SIZE);
    for(unsigned int i = 0; i < m3aStatisticsQueue.capacity(); i++) {
    LOG2("%s: i=%d", __FUNCTION__, i);
        struct atomisp_3a_statistics * statistics = allocateStatistics();
        if (statistics == NULL) {
            ret = NO_MEMORY;
        LOGE("No memory when allocate for %d stats", i);
            break;
        }
        m3aStatisticsQueue.push(statistics);
    }

    // Error free if not success
    if (ret)
        deinit3AStatisticsQueue();

    return ret;
}

void IPU2HwIsp::deinit3AStatisticsQueue()
{
    LOG1("%s", __FUNCTION__);
    for(unsigned int i = 0; i < m3aStatisticsQueue.capacity(); i++)
        freeStatistics(m3aStatisticsQueue[i]);
    m3aStatisticsQueue.clear();
}

status_t IPU2HwIsp::store3AStatistics(unsigned int * exp_id)
{
    LOG2("%s", __FUNCTION__);
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(m3AStatisticsLock);
    struct atomisp_3a_statistics * new3AStatistics = m3aStatisticsQueue.top();
    m3aStatisticsQueue.pop();

    ret = pxioctl(mMainNode, ATOMISP_IOC_G_3A_STAT, new3AStatistics);
    LOG2("%s IOCTL ATOMISP_IOC_G_3A_STAT ret: %d\n", __FUNCTION__, ret);

    if (ret != NO_ERROR) {
        LOGE("Failure in getting ISP Statistics");
        m3aStatisticsQueue.push_back(new3AStatistics);
    } else {
        m3aStatisticsQueue.push_front(new3AStatistics);
        *exp_id = new3AStatistics->exp_id;
    LOG2("%s: Stored 3A statistics of exp_id %d", __FUNCTION__, *exp_id);
    }

    return ret;
}

struct atomisp_3a_statistics * IPU2HwIsp::get3AStatistics(unsigned int exp_id)
{
    Mutex::Autolock lock(m3AStatisticsLock);
    LOG2("%s: size %d, exp_id=%d", __FUNCTION__, m3aStatisticsQueue.size(), exp_id);
    for(unsigned int i = 0; i < m3aStatisticsQueue.size(); i++) {
        // Get the statistics matched by exposure id or latest statistics if exp_id is set 0
    LOG2("%s [%d] exp_id %d", __FUNCTION__, i,
         m3aStatisticsQueue[i]->exp_id);
        if (m3aStatisticsQueue[i]->exp_id == exp_id ||
            (exp_id == 0 && m3aStatisticsQueue[i]->exp_id != 0))
            return m3aStatisticsQueue[i];
    }
    LOGW("3A statistics with exp_id %d is not found", exp_id);
    return NULL;
}

struct atomisp_3a_statistics * IPU2HwIsp::allocateStatistics()
{
    LOG2("@%s", __FUNCTION__);
    struct atomisp_3a_statistics *stats;

    atomisp_parm ispParameters;
    getIspParameters(&ispParameters);

    LOG2("@%s: s3a grid size %dx%d", __FUNCTION__,
        ispParameters.info.s3a_width, ispParameters.info.s3a_height);

    int grid_size = ispParameters.info.s3a_width * ispParameters.info.s3a_height;

    stats = (atomisp_3a_statistics*)malloc(sizeof(*stats));
    if (!stats)
        return NULL;

    stats->data = (atomisp_3a_output*)malloc(grid_size * sizeof(*stats->data));
    if (!stats->data) {
        free(stats);
        stats = NULL;
        return NULL;
    }
    stats->grid_info = ispParameters.info;
    LOG2("@%s success", __FUNCTION__);
    return stats;
}

void IPU2HwIsp::freeStatistics(struct atomisp_3a_statistics *stats)
{
    LOG2("@%s", __FUNCTION__);
    if (stats) {
        if (stats->data) {
            free(stats->data);
            stats->data = NULL;
        }
        free(stats);
        stats = NULL;
    }
}

status_t IPU2HwIsp::selectCameraSensor()
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = NO_ERROR;
    //Choose the camera sensor
    LOG1("Selecting camera sensor: %s", sCamInfo[mCameraId].name);
    ret = mMainNode->setInput(mCameraId);
    if (ret != NO_ERROR) {
        mMainNode->close();
        LOGE("Could not select camera: %s", sCamInfo[mCameraId].name);
        ret = UNKNOWN_ERROR;
    }
    return ret;
}

size_t IPU2HwIsp::setupCameraInfo()
{
    LOG1("@%s", __FUNCTION__);
    status_t status;
    size_t numCameras = 0;
    struct v4l2_input input;

    for (int i = 0; i < PlatformData::numberOfCameras(); i++) {
        CLEAR(input);
        CLEAR(sCamInfo[i]);
        input.index = i;
        status = mMainNode->enumerateInputs(&input);
        if (status != NO_ERROR) {
            sCamInfo[i].port = -1;
            LOGE("Device input enumeration failed for sensor input %d", i);
            if (status == INVALID_OPERATION)
                break;
        } else {
            sCamInfo[i].port = input.reserved[1];
            sCamInfo[i].index = i;
            strncpy(sCamInfo[i].name, (const char *)input.name, sizeof(sCamInfo[i].name)-1);
            LOG1("Detected sensor \"%s\"", sCamInfo[i].name);
        }
        numCameras++;
    }
    return numCameras;
}

/**
 * Maps the requested 'cameraId' to a V4L2 input.
 *
 * Only to be called from constructor
 * The cameraId  is passed to the HAL during creation as is currently stored in
 * the Camera Configuration Class (CPF Store)
 * This CameraID is used to identify a particular camera, it maps always 0 to back
 * camera and 1 to front whereas the index in the sCamInfo is filled from V4L2
 * The order how front and back camera are returned
 * may be different. This Android camera id will be used
 * to select parameters from back or front camera
 */
status_t IPU2HwIsp::initCameraInput()
{
    status_t status = NO_INIT;
    int numCameras = setupCameraInfo();

    // For file injection, cameraId & sensorId are same
    if ((mCapInfo->supportFileInject() == true) && (mCameraId == INTEL_FILE_INJECT_CAMERA_ID)) {
        LOG1("IPU2CameraHw opened with file inject camera id");
        mCameraId = INTEL_FILE_INJECT_CAMERA_ID;
        return NO_ERROR;
    }

    for (int i = 0; i < numCameras; i++) {
        if ((mCameraId == BACK_CAMERA_ID && sCamInfo[i].port == ATOMISP_CAMERA_PORT_PRIMARY)
            || (mCameraId == FRONT_CAMERA_ID && sCamInfo[i].port == ATOMISP_CAMERA_PORT_SECONDARY)) {
            mCameraId = i;
            LOG1("Camera found, v4l2 dev %d, android cameraId %d", i, mCameraId);
            status = NO_ERROR;
            break;
        }
    }

    return status;
}

/////////////////////////////////////////////////////////////
//  FlashControl
////////////////////////////////////////////////////////////
status_t IPU2HwIsp::setFlash(int numFrames)
{
    LOG2("@%s: numFrames = %d", __FUNCTION__, numFrames);
    if (sCamInfo[mCameraId].port != ATOMISP_CAMERA_PORT_PRIMARY) {
        LOGE("Flash is supported only for primary camera!");
        return INVALID_OPERATION;
    }
    if (numFrames) {
        if (mMainNode->setControl(V4L2_CID_FLASH_MODE, ATOMISP_FLASH_MODE_FLASH, "Flash Mode flash") < 0)
            return UNKNOWN_ERROR;
        if (mMainNode->setControl(V4L2_CID_REQUEST_FLASH, numFrames, "Request Flash") < 0)
            return UNKNOWN_ERROR;
    } else {
        if (mMainNode->setControl(V4L2_CID_FLASH_MODE, ATOMISP_FLASH_MODE_OFF, "Flash Mode flash") < 0)
            return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t IPU2HwIsp::setFlashIndicator(int intensity)
{
    LOG1("@%s: intensity = %d", __FUNCTION__, intensity);
    if (sCamInfo[mCameraId].port != ATOMISP_CAMERA_PORT_PRIMARY) {
        LOGE("Indicator intensity is supported only for primary camera!");
        return INVALID_OPERATION;
    }

    if (intensity) {
        if (mMainNode->setControl(V4L2_CID_FLASH_INDICATOR_INTENSITY, intensity, "Indicator Intensity") < 0)
            return UNKNOWN_ERROR;
        if (mMainNode->setControl(V4L2_CID_FLASH_MODE, ATOMISP_FLASH_MODE_INDICATOR, "Flash Mode") < 0)
            return UNKNOWN_ERROR;
    } else {
        if (mMainNode->setControl(V4L2_CID_FLASH_MODE, ATOMISP_FLASH_MODE_OFF, "Flash Mode") < 0)
            return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t IPU2HwIsp::setTorchHelper(int intensity)
{
    if (intensity) {
        if (mMainNode->setControl(V4L2_CID_FLASH_TORCH_INTENSITY, intensity, "Torch Intensity") < 0)
            return UNKNOWN_ERROR;
        if (mMainNode->setControl(V4L2_CID_FLASH_MODE, ATOMISP_FLASH_MODE_TORCH, "Flash Mode") < 0)
            return UNKNOWN_ERROR;
    } else {
        if (mMainNode->setControl(V4L2_CID_FLASH_MODE, ATOMISP_FLASH_MODE_OFF, "Flash Mode") < 0)
            return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t IPU2HwIsp::setTorch(int intensity)
{
    LOG1("@%s: intensity = %d", __FUNCTION__, intensity);

    if (sCamInfo[mCameraId].port != ATOMISP_CAMERA_PORT_PRIMARY) {
        LOGE("Indicator intensity is supported only for primary camera!");
        return INVALID_OPERATION;
    }

    setTorchHelper(intensity);

    // closing the kernel device will not automatically turn off
    // flash light, so need to keep track in user-space
    mFlashTorchSetting = intensity;

    return NO_ERROR;
}

int IPU2HwIsp::setFlashIntensity(int intensity)
{
    LOG2("@%s", __FUNCTION__);
    return mMainNode->setControl(V4L2_CID_FLASH_INTENSITY, intensity, "Set flash intensity");
}

/////////////////////////////////////////////////////////////
//  LensControl
////////////////////////////////////////////////////////////
int IPU2HwIsp::moveFocusToPosition(int position)
{
    LOG2("@%s", __FUNCTION__);
    if (!mHasLensActuator)
        return INVALID_OPERATION;

    // TODO: this code will be removed when the CPF file is valid for saltbay in the future
    if ((strcmp(PlatformData::boardName(), "saltbay") == 0) ||
        (strcmp(PlatformData::boardName(), "baylake") == 0)) {
        position = 1024 - position;
        position = 100 + (position - 370) * 1.7;
        if(position > 900)
            position = 900;
        if (position < 100)
            position = 100;
    }
    return mMainNode->setControl(V4L2_CID_FOCUS_ABSOLUTE, position, "Set focus position");
}

int IPU2HwIsp::moveFocusToBySteps(int steps)
{
    LOG2("@%s", __FUNCTION__);
    if (!mHasLensActuator)
        return INVALID_OPERATION;

    int val = 0, rval;
    rval = mMainNode->getControl(V4L2_CID_FOCUS_ABSOLUTE, &val);
    if (rval)
        return rval;
    return moveFocusToPosition(val + steps);
}

int IPU2HwIsp::getFocusPosition(int * position)
{
    LOG2("@%s", __FUNCTION__);
    return mMainNode->getControl(V4L2_CID_FOCUS_ABSOLUTE , position);
}

int IPU2HwIsp::getFocusStatus(int *status)
{
    LOG2("@%s", __FUNCTION__);
    return mMainNode->getControl(V4L2_CID_FOCUS_STATUS, status);
}
void IPU2HwIsp::clearMetadata(aaaMetadataInfo *metadata)
{
    if (metadata != NULL) {
        if (metadata->aaaMkNote != NULL) {
            char *tmp = reinterpret_cast<char*>(metadata->aaaMkNote->data);
            delete tmp;
            metadata->aaaMkNote->data = NULL;
        }
        delete metadata->aaaMkNote;
        metadata->aaaMkNote = NULL;
    }
    delete metadata;
}

void IPU2HwIsp::storeMetadata(aaaMetadataInfo *aaaMetadata)
{
    LOG2("@%s", __FUNCTION__);
    // Initialize the metadata queues
    Mutex::Autolock l(mMetadataQueueLock);
    if (aaaMetadata->reqId < mCapInfo->exposureLag()) {
        LOGD("drop the first frames which had been valid");
        return;
    }
    // the queue is full, remove the oldest one
    if (mMetadataQueue.size() == MAX_METADATA_QUEUE_SIZE) {
        aaaMetadataInfo *metadata = NULL;
        metadata = mMetadataQueue.editItemAt(0);
        mMetadataQueue.removeAt(0);
        clearMetadata(metadata);
    }
    mMetadataQueue.push_back(aaaMetadata);
}

status_t IPU2HwIsp::getMetadata(int requestId, aaaMetadataInfo *aaaMetadata)
{
    LOG2("@%s, requestId:%d", __FUNCTION__, requestId);
    status_t status = NO_ERROR;
    int lastRequestId = 0;
    int lastRequestIdIndex = 0;
    unsigned int i = 0;
    aaaMetadataInfo *lastAaaMetadata = NULL;
    aaaMetadataInfo *metadata = NULL;
    bool needMakeNote = false;
    Mutex::Autolock l(mMetadataQueueLock);
    if (requestId >= 1)
        lastRequestId = requestId - 1;
    for (i = 0; i < mMetadataQueue.size(); i++) {
        if (mMetadataQueue.itemAt(i)->reqId == requestId) {
            metadata = mMetadataQueue.editItemAt(i);
            break;
        } else if (mMetadataQueue.itemAt(i)->reqId == lastRequestId) {
            lastAaaMetadata = mMetadataQueue.editItemAt(i);
            lastRequestIdIndex = i;
        }
    }
    if (i == mMetadataQueue.size()) {
        LOGD("don't find the right request, use the last one");
        if (lastAaaMetadata != NULL) {
            metadata = lastAaaMetadata;
        } else {
            LOGD("don't find the metadata for this request");
            status = UNKNOWN_ERROR;
        }
    }
    if (metadata != NULL && aaaMetadata != NULL) {
        needMakeNote = aaaMetadata->needMakeNote;
        memcpy((void*)aaaMetadata, (void*)metadata, sizeof(aaaMetadataInfo));
        if (metadata->aaaMkNote != NULL && needMakeNote) {
            LOG2("copy 3a make note");
            aaaMetadata->aaaMkNote = new ia_binary_data;
            if (aaaMetadata->aaaMkNote != NULL) {
                aaaMetadata->aaaMkNote->data = new char[metadata->aaaMkNote->size];
                aaaMetadata->aaaMkNote->size = metadata->aaaMkNote->size;
                if (aaaMetadata->aaaMkNote->data != NULL) {
                    memcpy(aaaMetadata->aaaMkNote->data, metadata->aaaMkNote->data,
                       metadata->aaaMkNote->size);
                } else {
                    LOGE("no memory for 3a maker note");
                    status = UNKNOWN_ERROR;
                }
            }
       }
    }
    return status;

}

} /* namespace camera2 */
} /* namespace android */
