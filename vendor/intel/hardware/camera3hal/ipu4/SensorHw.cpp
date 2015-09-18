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

#define LOG_TAG "Camera_SensorHw"

#include "SensorHw.h"
#include "v4l2device.h"
#include "MediaController.h"
#include "MediaEntity.h"
#include "LogHelper.h"
#include "Camera3GFXFormat.h"


namespace android {
namespace camera2 {

SensorHw::SensorHw(int cameraId, sp<MediaController> mediaCtl):
    mCameraId(cameraId)
    ,mMediaCtl(mediaCtl)
    ,mSensorType(SENSOR_TYPE_NONE)
    ,mFrameSyncSource(FRAME_SYNC_NA)
    ,mPixelRate(0)
    ,mHorzBlank(0)
    ,mVertBlank(0)
    ,mCropWidth(0)
    ,mCropHeight(0)
    ,mMessageQueue("Camera_SensorHw", (int)MESSAGE_ID_MAX)
    ,mMessageThread(NULL)
    ,mFilled(false)
    ,mStarted(false)
    ,mThreadRunning(false)

{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mCapInfo = getIPU4CameraCapInfo(cameraId);
    mMessageThread = new MessageThread(this, "SensorHw", PRIORITY_DISPLAY);
    if (mMessageThread == NULL) {
        LOGE("Error create SensorHw");
   }
}

SensorHw::~SensorHw()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    if (mPollerThread != NULL) {
       mPollerThread->flush();
       mPollerThread->requestExitAndWait();
       mPollerThread.clear();
       mPollerThread = NULL;
    }

    mMessageQueue.remove(MESSAGE_ID_SOF);
    mMessageQueue.remove(MESSAGE_ID_EOF);
    mMessageQueue.remove(MESSAGE_ID_SET_PARAMS);

    if (mMessageThread != NULL) {
        mMessageThread->requestExitAndWait();
        mMessageThread.clear();
        mMessageThread = NULL;
    }
}

status_t SensorHw::setSubdev(sp<MediaEntity> entity, sensorEntityType type)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    sp<V4L2Subdevice> subdev = NULL;
    entity->getDevice((sp<V4L2DeviceBase>&) subdev);

    switch (type) {
    case SUBDEV_PIXEL_ARRAY:
        if (entity->getType() != SUBDEV_SENSOR) {
            LOGE("%s is not sensor subdevice", entity->getName());
            return BAD_VALUE;
        }
        mSensorSubdev = subdev;
        break;
    case SUBDEV_BINNER:
        if (entity->getType() != SUBDEV_GENERIC) {
            LOGE("%s is not binner subdevice", entity->getName());
            return BAD_VALUE;
        }
        mBinnerSubdev = subdev;
        break;
    case SUBDEV_SCALER:
        if (entity->getType() != SUBDEV_GENERIC) {
            LOGE("%s is not scaler subdevice", entity->getName());
            return BAD_VALUE;
        }
        mScalerSubdev = subdev;
        break;
    case SUBDEV_ISYSBACKEND:
        if (entity->getType() != SUBDEV_GENERIC) {
            LOGE("%s is not backend subdevice", entity->getName());
            return BAD_VALUE;
        }
        mIsysBackendSubdev = subdev;
        break;
    default:
        LOGE("%s: entity type (%d) not existing", __FUNCTION__, type);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t SensorHw::init()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_INIT;
    return mMessageQueue.send(&msg, MESSAGE_ID_INIT);
}

status_t SensorHw::handleMessageInit()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;
    String8 entityName;
    sp<MediaEntity> mediaEntity = NULL;

    mSensorType = (SensorType)mCapInfo->sensorType();

    // set pixel array
    const IPU4CameraCapInfo * cap = getIPU4CameraCapInfo(mCameraId);
    entityName = cap->getMediaCtlEntityName(String8("pixel_array"));
    if (entityName == "none") {
        LOGE("%s: No Pixel Array in this Sensor. Should not happen", __FUNCTION__);
        return UNKNOWN_ERROR;
    } else {
        status = mMediaCtl->getMediaEntity(mediaEntity, entityName.string());
        if (mediaEntity == NULL || status != NO_ERROR) {
            LOGE("%s, Could not retrieve Media Entity %s", __FUNCTION__, entityName.string());
            return UNKNOWN_ERROR;
        }
        status = setSubdev(mediaEntity, SUBDEV_PIXEL_ARRAY);
        if (status != NO_ERROR) {
            LOGE("%s: Cannot set pixel array subDev", __FUNCTION__);
            return status;
        }
    }

    // set binner
    entityName = cap->getMediaCtlEntityName(String8("binner"));
    if (entityName == "none") {
        LOG1("%s: No Binner in this Sensor", __FUNCTION__);
    } else {
        status = mMediaCtl->getMediaEntity(mediaEntity, entityName.string());
        if (mediaEntity == NULL || status != NO_ERROR) {
            LOGE("%s, Could not retrieve Media Entity %s", __FUNCTION__, entityName.string());
            return UNKNOWN_ERROR;
        }
        status = setSubdev(mediaEntity, SUBDEV_BINNER);
        if (status != NO_ERROR) {
            LOGE("%s: Cannot set binner subDev", __FUNCTION__);
            return status;
        }
    }

    // set scaler
    entityName = cap->getMediaCtlEntityName(String8("scaler"));
    if (entityName == "none") {
        LOG1("%s: No Scaler in this Sensor", __FUNCTION__);
    } else {
        status = mMediaCtl->getMediaEntity(mediaEntity, entityName.string());
        if (mediaEntity == NULL || status != NO_ERROR) {
            LOGE("%s, Could not retrieve Media Entity %s", __FUNCTION__, entityName.string());
            return UNKNOWN_ERROR;
        }
        status = setSubdev(mediaEntity, SUBDEV_SCALER);
        if (status != NO_ERROR) {
            LOGE("%s: Cannot set scaler subDev", __FUNCTION__);
            return status;
        }
    }

    // set isys backend
    entityName = cap->getMediaCtlEntityName(String8("back_end"));
    if (entityName == "none") {
        LOGE("%s: No Backend in Input System. Should not happen", __FUNCTION__);
        return UNKNOWN_ERROR;
    } else {
        status = mMediaCtl->getMediaEntity(mediaEntity, entityName.string());
        if (mediaEntity == NULL || status != NO_ERROR) {
            LOGE("%s, Could not retrieve Media Entity %s", __FUNCTION__, entityName.string());
            return UNKNOWN_ERROR;
        }
        status = setSubdev(mediaEntity, SUBDEV_ISYSBACKEND);
        if (status != NO_ERROR) {
            LOGE("%s: Cannot set backend subDev", __FUNCTION__);
            return status;
        }
    }

    // create PollingThread
    mDevicesToPoll.add(mIsysBackendSubdev);

    mPollerThread  = new PollerThread("SensorPollerThread");
    if (mPollerThread == NULL) {
        LOGE("%s: failed to create PollerThread", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    status = mPollerThread->init((Vector<sp<V4L2DeviceBase> >&)mDevicesToPoll, this);
    if (status != NO_ERROR)
        LOGE("%s: failed to init PollerThread", __FUNCTION__);

    mMessageQueue.reply(MESSAGE_ID_INIT, status);
    return status;
}

/**
 * start()
 * virtual start: actually start tracking frame events.
 * The sensor is already started, since the Isys is taking care of it.
 * The principle of this function is to track the SOF or EOF, to not miss
 * any frame synchronization.
 *
 * return NO_ERROR if success.
 */
status_t SensorHw::start()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_START;
    return mMessageQueue.send(&msg, MESSAGE_ID_START);
}

status_t SensorHw::handleMessageStart()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;
    int code;

    Mutex::Autolock lock(mFrameSyncMutex);
    if (mStarted) {
        LOG1("frame event poll, already started");
        return status;
    }

    if (mIsysBackendSubdev == NULL) {
        LOGE("ISYS Backend Sub device to poll is NULL");
        return UNKNOWN_ERROR;
    }

    if (mSensorType == SENSOR_TYPE_RAW) {
        status = mIsysBackendSubdev->subscribeEvent(FRAME_SYNC_SOF);
        if (status != NO_ERROR) {
            LOG1("%s:SOF event not existing on backend node", __FUNCTION__);
            status = mIsysBackendSubdev->subscribeEvent(FRAME_SYNC_EOF);
                if (status != NO_ERROR) {
                    LOGE("%s:EOF event no existing either on backend node", __FUNCTION__);
                    return status;
                }
            mFrameSyncSource = FRAME_SYNC_EOF;
            LOG1("%s: Using EOF event", __FUNCTION__);
        } else {
            mFrameSyncSource = FRAME_SYNC_SOF;
        }

        // retrieving crop values
        status = getActivePixelArraySize(mCropWidth, mCropHeight, code);
        if (status != NO_ERROR) {
            LOGE("%s:Failed to get Active Pixel Array", __FUNCTION__);
            return status;
        }

        // retrieving HBlank
        status = mSensorSubdev->getControl(V4L2_CID_HBLANK, &mHorzBlank);
        if (status != NO_ERROR) {
            LOGE("%s:Failed to get HBLANK", __FUNCTION__);
            return status;
        }

        // retrieving VBlank
        status = mSensorSubdev->getControl(V4L2_CID_VBLANK, &mVertBlank);
        if (status != NO_ERROR) {
            LOGE("%s:Failed to get VBLANK", __FUNCTION__);
            return status;
        }

        // start Thread
        mStarted = true;

    } else {
        //TODO for other types of sensors than SENSOR_TYPE_RAW
        LOGE("ONLY SENSOR_TYPE_RAW SUPPORTED FOR NOW");
        return BAD_VALUE;
    }

    mMessageQueue.reply(MESSAGE_ID_START, status);
    return status;
}

/**
 * stop()
 * virtual stop: actually stop tracking Frame events.
 * The principle of this function is to stop track the SOF or EOF
 *
 */
status_t SensorHw::stop()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_STOP;
    return mMessageQueue.send(&msg, MESSAGE_ID_STOP);
}

status_t SensorHw::handleMessageStop()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Mutex::Autolock lock(mFrameSyncMutex);
    if (mIsysBackendSubdev == NULL) {
        LOGE("ISYS Backend Sub device to poll is NULL");
        return BAD_VALUE;
    }

    if (mFrameSyncSource != FRAME_SYNC_NA) {
        mIsysBackendSubdev->unsubscribeEvent(mFrameSyncSource);
        mFrameSyncSource = FRAME_SYNC_NA;
    }

    mStarted = false;
    mMessageQueue.reply(MESSAGE_ID_STOP, NO_ERROR);
    return NO_ERROR;
}

/**
 * flush()
 * empty queues and request of sensor settings
 *
 */
status_t SensorHw::flush()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    return mMessageQueue.send(&msg);
}

status_t SensorHw::handleMessageFlush()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;

    status = mPollerThread->flush();
    return status;
}

status_t SensorHw::setParameters(ia_aiq_ae_results aeResults)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    Message msg;

    msg.id = MESSAGE_ID_SET_PARAMS;
    msg.data.aeParams.aeResults = aeResults;
    return mMessageQueue.send(&msg);
}

status_t SensorHw::handleMessageSetParams(Message &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    mAeResults = msg.data.aeParams.aeResults;
    mFilled = true;
    return NO_ERROR;
}

status_t SensorHw::handleMessageSOF()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    return NO_ERROR;
}

status_t SensorHw::handleMessageEOF()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    if (mFilled) {
        // exposure
        status = setExposure(mAeResults.exposures->sensor_exposure->coarse_integration_time,
                            mAeResults.exposures->sensor_exposure->fine_integration_time);
        // gain
        status = setGains(mAeResults.exposures->sensor_exposure->analog_gain_code_global,
                         mAeResults.exposures->sensor_exposure->digital_gain_global);

    // FIXME: add blanking
    }
    return NO_ERROR;
}

/* V4L2 for Raw */
int SensorHw::getActivePixelArraySize(int &width, int &height, int &code)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int status = BAD_VALUE;

    status = mSensorSubdev->getPadFormat(0, width, height, code);

    return status;
}

int SensorHw::getSensorOutputSize(int &width, int &height, int &code)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int status = BAD_VALUE;

    status = mScalerSubdev->getPadFormat(1, width, height, code);

    return status;
}


int SensorHw::getPixelRate(int &pixel_rate)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    return mSensorSubdev->getControl(V4L2_CID_PIXEL_RATE, &pixel_rate);
}

int SensorHw::getLinkFreq(int &link_freq)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    return mScalerSubdev->getControl(V4L2_CID_LINK_FREQ, &link_freq);
}

int SensorHw::getPixelClock(int64_t &pixel_clock)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int ret = BAD_VALUE;
    int link_freq = 0;
    v4l2_querymenu menu;
    CLEAR(menu);

    ret = mScalerSubdev->getControl(V4L2_CID_LINK_FREQ, &link_freq);
    if (ret != NO_ERROR)
        return ret;

    menu.id = V4L2_CID_LINK_FREQ;
    menu.index = link_freq;
    ret = mScalerSubdev->queryMenu(menu);
    if (ret != NO_ERROR)
        return ret;

    pixel_clock = menu.value;
    LOG1("pixel clock set to %lld", menu.value);
    return ret;
}

int SensorHw::setExposure(int coarse_exposure, int fine_exposure)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    UNUSED(fine_exposure);
    int ret = BAD_VALUE;

    ret = mSensorSubdev->setControl(V4L2_CID_EXPOSURE,
                                    coarse_exposure, "Exposure Time");
    // TODO: Need fine_exposure whenever supported on kernel.
    return ret;
}

int SensorHw::getExposure(int &coarse_exposure, int &fine_exposure)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int ret = BAD_VALUE;

    ret = mSensorSubdev->getControl(V4L2_CID_EXPOSURE, &coarse_exposure);
    // TODO: Need fine exposure whenever supported in kernel.
    fine_exposure = -1;

    return ret;
}

int SensorHw::getExposureRange(int &exposure_min, int &exposure_max, int &exposure_step)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int ret = BAD_VALUE;
    v4l2_queryctrl exposure;
    CLEAR(exposure);

    exposure.id = V4L2_CID_EXPOSURE;

    ret = mSensorSubdev->queryControl(exposure);
    if (ret != NO_ERROR) {
        LOGE("%s: Couldn't get exposure Range", __FUNCTION__);
        return ret;
    }

    exposure_min = exposure.minimum;
    exposure_max = exposure.maximum;
    exposure_step = exposure.step;

    return ret;
}

int SensorHw::setGains(int analog_gain, int digital_gain)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    UNUSED(digital_gain);
    int ret = BAD_VALUE;

    ret = mSensorSubdev->setControl(V4L2_CID_ANALOGUE_GAIN,
                                    analog_gain, "Analog Gain");
    // TODO: Need digital_gain whenever defined in V4L2.
    return ret;
}

int SensorHw::getGains(int &analog_gain, int &digital_gain)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int ret = BAD_VALUE;

    ret = mSensorSubdev->getControl(V4L2_CID_ANALOGUE_GAIN, &analog_gain);
    // TODO: Need digital_gain whenever defined in V4L2.
    digital_gain = -1;

    return ret;
}
int SensorHw::setFrameDuration(unsigned int llp, unsigned int fll)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int ret = NO_ERROR;
    int horzBlank = mHorzBlank;
    int vertBlank = mVertBlank;

    /* only calculate when not 0 */
    if (llp)
        horzBlank = llp - mCropWidth;
    if (fll)
        vertBlank = fll - mCropHeight;

    ret = mSensorSubdev->setControl(V4L2_CID_HBLANK,
                                    horzBlank, "Horizontal Blanking");

    ret |= mSensorSubdev->setControl(V4L2_CID_VBLANK,
                                     vertBlank, "Vertical Blanking");

    return ret;
}

int SensorHw::getFrameDuration(unsigned int &llp, unsigned int &fll)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int ret = NO_ERROR;

    ret = mSensorSubdev->getControl(V4L2_CID_HBLANK, &mHorzBlank);
    ret |= mSensorSubdev->getControl(V4L2_CID_VBLANK, &mVertBlank);

    if (ret == NO_ERROR) {
        llp = mHorzBlank + mCropWidth;
        fll = mVertBlank + mCropHeight;
    }

    return ret;
}

int SensorHw::getVBlank(unsigned int &vblank)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    vblank = mVertBlank;

    return NO_ERROR;
}

int SensorHw::getAperture(int &aperture)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    return mSensorSubdev->getControl(V4L2_CID_IRIS_ABSOLUTE, &aperture);
}

int SensorHw::getFNumber(unsigned int &fnum_num, unsigned int &fnum_denom)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    int fnum = 0;
    int ret = BAD_VALUE;

    ret = mSensorSubdev->getControl(V4L2_CID_FNUMBER_ABSOLUTE, &fnum);

    fnum_num = (unsigned int)(fnum >> 16);
    fnum_denom = (unsigned int)(fnum & 0xFFFF);
    return ret;
}

int SensorHw::getCurrentCameraId(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    return mCameraId;
}

void SensorHw::messageThreadLoop(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mThreadRunning = true;
    // use somewhere here start and stop
    while (mThreadRunning) {
        status_t status = NO_ERROR;
        //TODO when using the sync manager, using requestId and not count
        static int count = 0;

        Message msg;
        mMessageQueue.receive(&msg);
        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            status = handleMessageExit();
            break;
        case MESSAGE_ID_INIT:
            status = handleMessageInit();
            break;
        case MESSAGE_ID_START:
            status = handleMessageStart();
            break;
        case MESSAGE_ID_STOP:
            status = handleMessageStop();
            break;
        case MESSAGE_ID_FLUSH:
            status = handleMessageFlush();
            break;
        case MESSAGE_ID_SET_PARAMS:
            status = handleMessageSetParams(msg);
            mPollerThread->pollRequest(count);
            count++;
            break;
        case MESSAGE_ID_SOF:
            status = handleMessageSOF();
            break;
        case MESSAGE_ID_EOF:
            status = handleMessageEOF();
            break;
        default:
            LOGE("ERROR @%d: Unknown message %d", status, (int)msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE(" error %d in handling message: %d", status, (int)msg.id);
    }
}

status_t SensorHw::requestExitAndWait(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    return mMessageQueue.send(&msg, MESSAGE_ID_EXIT);
}

status_t SensorHw::handleMessageExit(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;
    mThreadRunning = false;
    mMessageQueue.reply(MESSAGE_ID_EXIT, status);
    return status;
}

/**
 * NotifyPollevent
 * gets notification everytime an event is triggered
 * in the PollerThread SensorHw is subscribed.
 */
status_t SensorHw::notifyPollEvent(PollEventMessage *pollEventMsg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    struct v4l2_event event;
    Message msg;
    status_t status = NO_ERROR;

    CLEAR(event);
    if (pollEventMsg == NULL || pollEventMsg->data.activeDevices == NULL)
        return BAD_VALUE;

    if (pollEventMsg->id != 0) {
        LOGE("%s Polling Failed, id:%d", __FUNCTION__, pollEventMsg->data.reqId);
    } else if (pollEventMsg->data.activeDevices->isEmpty()) {
        LOG1("%s Polling from Flush: succeeded", __FUNCTION__);
        // TODO flush actions.
    } else {
        //cannot be anything else than event if ending up here.
        do {
            status = mIsysBackendSubdev->dequeueEvent(&event);
            if (status < 0) {
                LOGE("%s:dequeueing failed", __FUNCTION__);
                break;
             } else {
                if (mFrameSyncSource == FRAME_SYNC_SOF) {
                    msg.id = MESSAGE_ID_SOF;
                    msg.data.sof.timestamp.tv_sec = event.timestamp.tv_sec;
                    msg.data.sof.timestamp.tv_usec = (event.timestamp.tv_nsec / 1000);
                    msg.data.sof.exp_id = event.sequence;
                } else if (mFrameSyncSource == FRAME_SYNC_EOF) {
                    msg.id = MESSAGE_ID_EOF;
                    msg.data.eof.timestamp.tv_sec = event.timestamp.tv_sec;
                    msg.data.eof.timestamp.tv_usec = (event.timestamp.tv_nsec / 1000);
                    msg.data.eof.exp_id = event.sequence;
                } else {
                    msg.id = MESSAGE_ID_MAX;
                    LOGE("%s: should never end up here", __FUNCTION__);
                }
                mMessageQueue.send(&msg);
                LOG2("%s: EVENT, MessageId: %d, activedev: %x count: %d, event seq: %d",
                    __FUNCTION__, pollEventMsg->id, pollEventMsg->data.activeDevices->size(),
                    pollEventMsg->data.reqId, event.sequence);
            }
        } while (event.pending > 0);
    }
    return OK;
}
}   // namespace camera2
}   // namespace android
