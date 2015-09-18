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

#define LOG_TAG "Camera_CaptureUnit"

#include "UtilityMacros.h"
#include "MediaController.h"
#include "LogHelper.h"
#include "GuiLogTraces.h"
#include "CaptureUnit.h"

namespace android {
namespace camera2 {

CaptureUnit::CaptureUnit(int camId):
    mCameraId(camId),
    mMediaCtl(NULL),
    mIsys(NULL),
    mSensor(NULL),
    mRawStream(NULL),
    mLastReqIDReturned(0),
    mMessageQueue("Camera_CaptureUnit", (int)MESSAGE_ID_MAX),
    mMessageThead(NULL),
    mThreadRunning(false),
    mStreamingStarted(false)
{
    LOG1("@%s: id: %d", __FUNCTION__, camId);
    CLEAR(mCaptureConfig);
}

CaptureUnit::~CaptureUnit()
{
    LOG1("@%s", __FUNCTION__);

    if(mSensor != NULL && mSensor->isStarted()) {
        mSensor->stop();
        mSensor->requestExitAndWait();
        mSensor.clear();
        mSensor = NULL;
    }

    if (mMessageThead != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }

    if (mIsys != NULL) {
        if (mIsys->isStarted())
            mIsys->stop();
        mIsys.clear();
        mIsys = NULL;
    }
}

status_t CaptureUnit::init()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mMediaCtl = new MediaController("/dev/media0");
    if (mMediaCtl == NULL) {
        LOGE("@%s: Error creating MediaController", __FUNCTION__);
        return NO_INIT;
    }

    status = mMediaCtl->init();
    if (status != NO_ERROR) {
        LOGE("@%s: Error initializing Media Controller", __FUNCTION__);
        return status;
    }

    mIsys = new InputSystem(mCameraId, this, mMediaCtl);
    if (mIsys == NULL) {
        LOGE("@%s: Error creating input system", __FUNCTION__);
        return NO_INIT;
    }

    status = mIsys->init();
    if (status != NO_ERROR) {
        LOGE("@%s: Error initializing InputSystem", __FUNCTION__);
        return status;
    }

    mSensor = new SensorHw(mCameraId, mMediaCtl);
    if (mSensor == NULL) {
        LOGE("@%s: Error creating SensorHw", __FUNCTION__);
        return NO_INIT;
    }

    status = mSensor->init();
    if (status != NO_ERROR) {
        LOGE("%s:Cannot initialize SensorHW (status= 0x%X)", __FUNCTION__, status);
        return status;
    }

    mMessageThead = new MessageThread(this, "CaptureUnit");
    if (mMessageThead == NULL) {
        LOGE("@%s: Error creating message thread", __FUNCTION__);
        return NO_INIT;
    }
    /**
     * cache the static metadata values we are going to need in the capture unit
     **/
    initStaticMetadata();
    return status;
}

/**
 * configStreams
 *
 * Configures CaptureUnit and InputSystem based on the
 * stream configuration received from the client.
 *
 * \param activeStreams [IN]: New stream configuration
 * \param configChanged [OUT]: Whether CaptureUnit and ISYS have been reconfigured and restarted
 */
status_t CaptureUnit::configStreams(Vector<camera3_stream_t*> &activeStreams, bool &configChanged)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mActiveStreams.clear();
    for (unsigned int i = 0; i < activeStreams.size(); i++) {
         mActiveStreams.push_back(activeStreams.editItemAt(i));
    }

    // Check if capture pipe needs to be reconfigured
    status = getMediaCtlCamConfig(configChanged);
    if (status != NO_ERROR) {
        LOGE("%s: Cannot retrieve MediaCtl pipe configuration", __FUNCTION__);
        return status;
    }

    /**
     * Find if there is a Raw stream. This will speed up the process of finding
     * RAW buffers for each request.
     */
    camera3_stream_t* rawStream = findRawStream();

    // Check if RAW stream is added or removed from the active stream config
    // as it affects the buffering
    if (!configChanged) {
        if ((mRawStream == NULL && rawStream == NULL) ||
            (mRawStream != NULL && rawStream != NULL)) {
            LOG1("@%s: no need to reconfigure ISYS", __FUNCTION__);
            return status;
        }
        configChanged = true;
    }
    mRawStream = rawStream;
    LOG1("@%s: restarting and reconfiguring ISYS", __FUNCTION__);

    if(mSensor->isStarted())
        mSensor->stop();

    // Stop streaming and reconfigure ISYS
    if (mIsys->isStarted()) {
        status = mIsys->stop();
        if (status != NO_ERROR) {
            LOGE("Failed to stop streaming!");
            return status;
        }
        mStreamingStarted = false;
    }
    freeBuffers();

    // Configure ISYS based on the requested frame size. Get output format from ISYS.
    status = mIsys->configure(mCaptureConfig.width, mCaptureConfig.height, mCaptureConfig.format);
    if (status != NO_ERROR) {
        LOGE("@%s: Error configuring InputSystem", __FUNCTION__);
        return status;
    }

    status = mSensor->start();
    if (status != NO_ERROR) {
        LOGE("%s:Cannot start sensor (status= 0x%X)", __FUNCTION__, status);
        return status;
    }


    // create and set buffer pool to video nodes
    // TODO: output nodes and pools should be per stream that CaptureUnit
    // is requested to provide.

    status = createBufferPools(sBufferPoolSize + sSensorSettingsDelay);
    if (status != NO_ERROR) {
        LOGE("%s: Failed to create buffer pools (status= 0x%X)", __FUNCTION__, status);
        return status;
    }

    // If there is no RAW stream, allocate internal buffers.
    // Otherwise use RAW buffers from capture requests.
    if (mRawStream == NULL) {
        status = allocateCaptureBuffers(sBufferPoolSize + sSensorSettingsDelay);
        if (status != NO_ERROR) {
            LOGE("%s: Failed to allocate buffers (status= 0x%X)", __FUNCTION__, status);
            return status;
        }
    } else {
        // Allocate extra buffers required for skipping
        status = allocateCaptureBuffers(sSensorSettingsDelay);
        if (status != NO_ERROR) {
            LOGE("%s: Failed to allocate buffers (status= 0x%X)", __FUNCTION__, status);
            return status;
        }
    }

    // TODO: make sure that this is when the sensor data changes. If not,
    // the event needs to be sent somewhere else.
    ICaptureEventListener::CaptureMessage outMsg;
    outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT;
    outMsg.data.event.timestamp.tv_sec  = 0;
    outMsg.data.event.timestamp.tv_usec = 0;
    outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_NEW_SENSOR_DESCRIPTOR;
    notifyListeners(&outMsg);

    return status;
}

status_t CaptureUnit::getMediaCtlCamConfig(bool &needIsysRestart)
{
    LOG1("@%s", __FUNCTION__);

    unsigned int width = 0;
    unsigned int height = 0;
    needIsysRestart = false;

    // find the largest stream
    for (unsigned int i = 0; i < mActiveStreams.size(); i++) {
        if (width < mActiveStreams[i]->width || height < mActiveStreams[i]->height) {
            width = mActiveStreams[i]->width;
            height = mActiveStreams[i]->height;
        }
    }

    // check if the configuration needs to be changed
    if ((unsigned int) mCaptureConfig.width != width ||
        (unsigned int) mCaptureConfig.height != height) {
        CLEAR(mCaptureConfig);
        mCaptureConfig.width = width;
        mCaptureConfig.height = height;
        needIsysRestart = true;
    }

    LOG1("Selected MediaCtl pipe config: %dx%d %s", mCaptureConfig.width, mCaptureConfig.height,
                                    needIsysRestart ? "(config changed, ISYS restart needed)":"");
    return NO_ERROR;
}

status_t CaptureUnit::createBufferPools(int numBufs)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    int nodeCount = 0;

    Vector<sp<V4L2VideoNode> > *nodes = NULL;

    status = mIsys->getOutputNodes(&nodes, nodeCount);
    if (status != NO_ERROR) {
        LOGE("@%s: ISYS video output nodes not configured", __FUNCTION__);
        return status;
    }

    for (int i = 0; i < nodeCount; i++) {
        if (nodes->itemAt(i)->getType() == VIDEO_RAW_BAYER) {
            nodes->itemAt(i)->getConfig(mCaptureConfig);
            LOG1("@%s: creating RAW BAYER buffer pool (size: %d)", __FUNCTION__, numBufs);
            createCaptureBufferPool(numBufs, VIDEO_RAW_BAYER);
        }
        // TODO: handle other node types
    }

    return status;
}

status_t CaptureUnit::createCaptureBufferPool(int numBufs, VideoNodeType nodetype)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    // Fill up the empty v4L2 buffers
    Vector<struct v4l2_buffer> v4l2Buffers;
    struct v4l2_buffer v4l2Buf;
    CaptureBuffer *captureBufPtr = NULL;
    CLEAR(v4l2Buf);
    v4l2Buffers.clear();

    for (int i = 0; i < numBufs; i++) {
        v4l2Buffers.push(v4l2Buf);
    }

    status = mIsys->setBufferPool(nodetype, v4l2Buffers, true);
    if (status != NO_ERROR) {
        LOGE("Failed to set the raw buffer pool in ISYS (status: 0x%X", status);
        return status;
    }
    /**
     * Init the pool of capture buffer structs.
     * This pool contains the V4L2 buffers that
     * are registered to the V4L2 device.
     */
    mCaptureItemsPool.init(numBufs);
    mQueuedCaptureBuffers.setCapacity(numBufs);

    for (int i = 0; i < numBufs; i++) {
        mCaptureItemsPool.acquireItem(&captureBufPtr);
        captureBufPtr->v4l2Buf = v4l2Buffers[i];
        captureBufPtr->owner = this;
        mCaptureItemsPool.releaseItem(captureBufPtr);
    }
    HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mCaptureItemsPool",
                                                    numBufs);
    return status;
}

status_t CaptureUnit::allocateCaptureBuffers(int numBufs)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    sp<CameraBuffer> tmpBuf = NULL;
    sp<CameraBuffer> *camBufPtr = NULL;

    mRawBufPool.init(numBufs);
    mQueuedRawBuffers.setCapacity(numBufs);

    for (int i = 0; i < numBufs; i++) {
        tmpBuf = MemoryUtils::allocateHeapBuffer(mCaptureConfig.width,
                                                 mCaptureConfig.height,
                                                 mCaptureConfig.stride,
                                                 mCaptureConfig.format,
                                                 mCameraId);
        if (tmpBuf == NULL) {
            LOGE("Failed to allocate internal RAW buffers");
            return NO_MEMORY;
        }
        mRawBufPool.acquireItem(&camBufPtr);
        *camBufPtr = tmpBuf;
        tmpBuf.clear();
        mRawBufPool.releaseItem(camBufPtr);
    }
    // TODO: allocate other buffers
    return status;
}

status_t CaptureUnit::freeBuffers()
{
    LOG1("@%s", __FUNCTION__);

    mCaptureItemsPool.deInit();
    mRawBufPool.deInit();
    mQueuedCaptureBuffers.clear();
    mQueuedRawBuffers.clear();
    HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mCaptureItemsPool", 0);

    return NO_ERROR;
}

status_t CaptureUnit::flush()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    // flush the poll messages
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    mMessageQueue.remove(MESSAGE_ID_CAPTURE);
    status = mMessageQueue.send(&msg, MESSAGE_ID_FLUSH);
    return status;
}

status_t CaptureUnit::doCapture(Camera3Request* request,
                                AiqCaptureSettings &aiqCaptureSettings)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_CAPTURE;
    msg.data.request.request = request;
    msg.data.request.settings = &aiqCaptureSettings;
    status = mMessageQueue.send(&msg);
    return status;
}

status_t CaptureUnit::getSensorData(ia_aiq_exposure_sensor_descriptor *aiqSensorDescriptor)
{
    LOG2("%s", __FUNCTION__);

    status_t status = NO_ERROR;
    int integration_step = 0;
    int integration_max = 0;
    int pixel = 0;

    if (aiqSensorDescriptor == NULL) {
        LOGE("%s: aiqSensorDescriptor is NULL", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    status = mSensor->getPixelRate(pixel);
    if (status != NO_ERROR) {
        LOGE("%s: failed to get pixel clock", __FUNCTION__);
        return status;
    }
    aiqSensorDescriptor->pixel_clock_freq_mhz = (float)pixel/1000000;

    status = mSensor->getFrameDuration((unsigned int&)aiqSensorDescriptor->pixel_periods_per_line,
                                        (unsigned int&)aiqSensorDescriptor->line_periods_per_field);
    if (status != NO_ERROR) {
        LOGE("%s: failed to get frame Durations", __FUNCTION__);
        return status;
    }

    status = mSensor->getExposureRange((int&)aiqSensorDescriptor->coarse_integration_time_min,
                                        (int&) integration_max, integration_step);
    if (status != NO_ERROR) {
        LOGE("%s: failed to get Exposure Range", __FUNCTION__);
        return status;
    }

    LOG2("%s: Exposure range coarse :min = %d, max = %d, step = %d",
                                    __FUNCTION__,
                                    aiqSensorDescriptor->coarse_integration_time_min,
                                    integration_max, integration_step);

    aiqSensorDescriptor->coarse_integration_time_max_margin =
                        aiqSensorDescriptor->line_periods_per_field - integration_max;

    //INFO: fine integration is not supported by v4l2
    aiqSensorDescriptor->fine_integration_time_min = 0;
    aiqSensorDescriptor->fine_integration_time_max_margin = 0;

    status = mSensor->getVBlank((unsigned int&)aiqSensorDescriptor->line_periods_vertical_blanking);

    LOG2("%s: pixel clock = %f  ppl = %d, lpf =%d, int_min = %d, int_max_range %d",
                                    __FUNCTION__,
                                    aiqSensorDescriptor->pixel_clock_freq_mhz,
                                    aiqSensorDescriptor->pixel_periods_per_line,
                                    aiqSensorDescriptor->line_periods_per_field,
                                    aiqSensorDescriptor->coarse_integration_time_min,
                                    aiqSensorDescriptor->coarse_integration_time_max_margin);
    return status;
}

status_t CaptureUnit::applySensorParams(Camera3Request* request,
                                        AiqCaptureSettings* aiqCaptureSettings)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(request);
    status_t status = NO_ERROR;

    status = mSensor->setParameters(aiqCaptureSettings->aiqResults.aeResults);

    return status;
}

status_t CaptureUnit::getOutputConfig(FrameInfo &info)
{
    LOG1("@%s", __FUNCTION__);
    if (mCaptureConfig.width == 0 || mCaptureConfig.height == 0)
        return NO_INIT;

    info = mCaptureConfig;

    return NO_ERROR;
}

status_t CaptureUnit::handleMessageFlush(Message &msg)
{
    LOG1("@%s:", __FUNCTION__);
    UNUSED(msg);
    status_t status = NO_ERROR;
    mMessageQueue.reply(MESSAGE_ID_FLUSH, status);
    return status;
}

/**
 * enqueueBuffers
 *
 * Check if we have a raw stream and get a buffer from the stream or
 * acquire a buffer from the capture buffer pool and do a v4l2 putframe
 * of the capture buffer
 * \param request [IN] Capture request
 */
status_t CaptureUnit::enqueueBuffers(Camera3Request *request)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    CaptureBuffer *capBufPtr = NULL;
    sp<CameraBuffer> *rawBufPtr = NULL;

    /**
     * If we have a RAW stream then pick up the buffer from the request.
     * If not then grab one from the internal pool.
     */
    if (mRawStream != NULL && request != NULL) {
        status = findRawBuffer(request, &capBufPtr);
        if (status != NO_ERROR) {
            LOGE("Failed to find a RAW buffer!");
            return UNKNOWN_ERROR;
        }
    } else {
        // get a RAW buffer from the internal pool
        status = mRawBufPool.acquireItem(&rawBufPtr);
        if (status != NO_ERROR) {
            LOGE("Failed to get a RAW buffer!");
            return UNKNOWN_ERROR;
        }

        // get a capture buffer from the pool
        status = mCaptureItemsPool.acquireItem(&capBufPtr);
        if (status != NO_ERROR) {
            LOGE("Failed to get a capture buffer!");
            return UNKNOWN_ERROR;
        }
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mCaptureItemsPool",
                           mCaptureItemsPool.availableItems());

        capBufPtr->buf = *rawBufPtr;
        capBufPtr->v4l2Buf.m.userptr = (long unsigned int) capBufPtr->buf->data();
    }

    status = mIsys->putFrame(VIDEO_RAW_BAYER, &(capBufPtr->v4l2Buf));
    if (status != NO_ERROR) {
        LOGE("Failed to queue a buffer!");
        return UNKNOWN_ERROR;
    }
    mQueuedCaptureBuffers.add(capBufPtr);
    if (rawBufPtr != NULL)
        mQueuedRawBuffers.add(rawBufPtr);

    return status;
}

status_t CaptureUnit::handleMessageCapture(Message &msg)
{
    HAL_GUITRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE,
                      msg.data.request.request->getId());
    status_t status = NO_ERROR;
    Camera3Request *request = NULL;
    AiqCaptureSettings* captureSettings = NULL;

    // Need to skip only when the streaming gets started
    bool needSkipping = !mStreamingStarted;
    if (needSkipping) {
        LOG1("@%s: enqueue %d skip buffers", __FUNCTION__, sSensorSettingsDelay);
        for (int i = 0; i < sSensorSettingsDelay; i++) {
            status = enqueueBuffers(NULL);
            if (status != NO_ERROR) {
                LOGE("@%s: Failed to enqueue SKIP buffers!", __FUNCTION__);
                return UNKNOWN_ERROR;
            }
        }
    }

    request = msg.data.request.request;
    captureSettings = msg.data.request.settings;

    status = enqueueBuffers(request);
    if (status != NO_ERROR) {
        LOGE("@%s: Failed to enqueue buffers!", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (!mStreamingStarted) {
        LOG1("@%s: Starting ISYS", __FUNCTION__);
        status = mIsys->start();
        if (status != NO_ERROR) {
            LOGE("@%s: Failed to start streaming!", __FUNCTION__);
            return status;
        }
        mStreamingStarted = true;
    }

    // FIXME: if statement ONLY while AE is not complete.
    // This prevents exp flickering in AE mode.
    if (captureSettings->aeEnabled == false)
        applySensorParams(request, captureSettings);

    if (needSkipping) {
        LOG1("@%s: enqueue %d skip capture requests to ISYS", __FUNCTION__,
                                                        sSensorSettingsDelay);
        for (int i = 0; i < sSensorSettingsDelay; i++) {
            mIsys->capture(mLastReqIDReturned - sSensorSettingsDelay + i);
        }
    }

    int requestId = request->getId();
    mIsys->capture(requestId);

    return status;
}

status_t CaptureUnit::handleMessageReturnBuffer(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    CaptureBuffer* buffer = msg.data.buffer.captureBufPtr;

    // First return internal RAW buffer to the pool (if needed)
    if (mQueuedRawBuffers.size() > 0) {
        sp<CameraBuffer> *camBufPtr = NULL;
        for (size_t i = 0; i < mQueuedRawBuffers.size(); i++) {
            camBufPtr = mQueuedRawBuffers[i];
            if (buffer->v4l2Buf.m.userptr == (long unsigned int) (*camBufPtr)->data() ) {
                mQueuedRawBuffers.removeAt(i);
                break;
            }
        }

        status = mRawBufPool.releaseItem(camBufPtr);
        if (status != NO_ERROR) {
            LOGE("Failed to return consumed RAW buffer to the pool");
        } else {
            LOG2("RAW buffer for request %d recycled", buffer->buf->requestId());
        }
    }

    // Then return the CaptureBuffer to the pool
    for (size_t i = 0; i < mQueuedCaptureBuffers.size(); i++) {
        if (buffer->v4l2Buf.m.userptr == mQueuedCaptureBuffers[i]->v4l2Buf.m.userptr) {
            mQueuedCaptureBuffers.removeAt(i);
            break;
        }
    }

    status = mCaptureItemsPool.releaseItem(buffer);
    if (status != NO_ERROR) {
        LOGE("Failed to return consumed capture buffer to the pool");
    } else {
        LOG2("capture buffer for request %d recycled", buffer->buf->requestId());
    }
    HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mCaptureItemsPool",
                       mCaptureItemsPool.availableItems());

    return status;
}

status_t CaptureUnit::handleMessageIsysEvent(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    ICaptureEventListener::CaptureMessage outMsg;

    struct v4l2_buffer_info *outBuf = &msg.data.buffer.v4l2Buf;
    VideoNodeType nodeType = msg.data.buffer.nodeType;
    int requestId = msg.data.buffer.requestId;
    mLastReqIDReturned = requestId;

    // Notify listeners, first fill the observer message
    outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT;
    outMsg.data.event.timestamp.tv_sec  = outBuf->vbuffer.timestamp.tv_sec;
    outMsg.data.event.timestamp.tv_usec = outBuf->vbuffer.timestamp.tv_usec;
    outMsg.data.event.sequence = outBuf->vbuffer.sequence;

    for (size_t i = 0; i < mQueuedCaptureBuffers.size(); i++) {
        if (outBuf->vbuffer.m.userptr == mQueuedCaptureBuffers[i]->v4l2Buf.m.userptr) {
            mQueuedCaptureBuffers[i]->buf->setRequestId(requestId);
            outMsg.data.event.buffer = mQueuedCaptureBuffers.editItemAt(i);
            break;
        }
    }

    switch(nodeType) {
    case VIDEO_RAW_BAYER:
        // notify shutter event
        if (requestId >= 0) {
            outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_SHUTTER;
            notifyListeners(&outMsg);
        }

        /**
         * aeLoop workaround starts
         * In case of skip frame, forward the statistics to ControlUnit but
         * discard the frame the following workaround code goes to AA_STATS
         * case once the stats are coming for real
         */
        outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_2A_STATISTICS;
        notifyListeners(&outMsg);  // remove this line when moving to AA_STATS
        // aeLoop workaround ends
        if (requestId < 0) {
            LOG2("@%s: skip frame %d received", __FUNCTION__, requestId);
            returnBuffer(outMsg.data.event.buffer);
            return status;
        }
        outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_RAW_BAYER;
        break;
    case VIDEO_MIPI_COMPRESSED:
        outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_MIPI_COMPRESSED;
        break;
    case AA_STATS:
        outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_2A_STATISTICS;
        break;
    // TODO: handle other types
    default:
        outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_ERROR;
        LOGW("Unsupported event was returned from input system!");
        break;
    }

    // Send notification
    if (outMsg.id == ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT) {
        LOG2("Css ISYS event %d arrived", outMsg.data.event.type);
        notifyListeners(&outMsg);
    }

    return status;
}


void CaptureUnit::messageThreadLoop(void)
{
    LOG1("@%s: Start", __FUNCTION__);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);

        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            mThreadRunning = false;
            status = NO_ERROR;
            break;

        case MESSAGE_ID_CAPTURE:
            status = handleMessageCapture(msg);
            break;

        case MESSAGE_ID_FLUSH:
            status = handleMessageFlush(msg);
            break;

        case MESSAGE_ID_RETURN_BUFFER:
            status = handleMessageReturnBuffer(msg);
            break;

        case MESSAGE_ID_ISYS_EVENT:
            status = handleMessageIsysEvent(msg);
            break;

        default:
            LOGE("@%s: Unknown message: %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("error %d in handling message: %d", status, (int)msg.id);

    }
    LOG1("%s: Exit", __FUNCTION__);
}

/**
 * Attach a Listening client to a particular event
 *
 * @param observer interface pointer to attach
 * @param event concrete event to listen to
 */
status_t CaptureUnit::attachListener(ICaptureEventListener *observer,
                                     ICaptureEventListener::CaptureEventType event)
{
    LOG1("@%s: %p to event type %d", __FUNCTION__, observer, event);
    status_t status = NO_ERROR;
    if (observer == NULL)
        return BAD_VALUE;

    // Check event to be in the range of allowed events.
    if ((event < ICaptureEventListener::CAPTURE_EVENT_MIPI_COMPRESSED ) ||
        (event > ICaptureEventListener::CAPTURE_EVENT_MAX)) {
        LOGE("Event is outside the range of allowed events.");
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

    List<ICaptureEventListener*>::iterator it = theList.begin();
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
 * @param observer interface pointer to detach
 * @param event identifier for event type to detach
 */
status_t CaptureUnit::detachListener(ICaptureEventListener *listener,
                                     ICaptureEventListener::CaptureEventType event)
{
    LOG1("@%s:%d", __FUNCTION__, event);

    if (listener == NULL)
        return BAD_VALUE;

    Mutex::Autolock _l(mCaptureEventListenerLock);
    status_t status = BAD_VALUE;
    int index = mListeners.indexOfKey(event);
    if (index == NAME_NOT_FOUND) {
       LOGW("No listener register for this event, ignoring");
       return NO_ERROR;
    }

    listener_list_t &theList = mListeners.editValueAt(index);
    List<ICaptureEventListener*>::iterator it = theList.begin();
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

status_t CaptureUnit::notifyListeners(ICaptureEventListener::CaptureMessage *msg)
{
    LOG2("@%s", __FUNCTION__);
    bool ret = false;
    Mutex::Autolock _l(mCaptureEventListenerLock);
    listener_list_t &theList = mListeners.editValueFor(msg->data.event.type);
    List<ICaptureEventListener*>::iterator it = theList.begin();
    for (;it != theList.end(); ++it)
        ret |= (*it)->notifyCaptureEvent((ICaptureEventListener::CaptureMessage*)msg);

    return ret;
}

/**
 * findRawStream
 * analyze the streams in mActiveStreams to detect if there is a RAW stream
 * configured.
 * Currently only one supported.
 */
camera3_stream_t* CaptureUnit::findRawStream()
{
    LOG1("@%s", __FUNCTION__);

    for (size_t i = 0; i < mActiveStreams.size(); i++) {
        if (mActiveStreams[i]->format == HAL_PIXEL_FORMAT_RAW_SENSOR) {
            LOG1("@%s: found raw stream = %p", __FUNCTION__,
                                              mActiveStreams[i]);
            return mActiveStreams[i];
        }
    }
    return NULL;
}

/**
 * findRawBuffer
 *
 * Find a RAW buffer from the request and initialize one of the pooled
 * buffers with it's data.
 *
 */
status_t CaptureUnit::findRawBuffer(Camera3Request *request,
                                    CaptureBuffer **capBuf)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    CaptureBuffer *capBufPtr = NULL;

    /**
     * pick one buffer from the capture buffer pool and pick up the the buffer
     * from the request
     */
    mCaptureItemsPool.acquireItem(&capBufPtr);

    capBufPtr->buf = request->findBuffer((CameraStreamNode*)mRawStream->priv);
    if (CC_UNLIKELY(capBufPtr->buf == NULL)) {
        LOGE("Raw buffer not found - BUG ");
        return INVALID_OPERATION;
    }

    status = capBufPtr->buf->lock();
    if (status != NO_ERROR) {
        LOGE("%s: Failed to lock buffer handle", __FUNCTION__);
        return status;
    }

    status = capBufPtr->buf->waitOnAcquireFence();
    if (status != NO_ERROR) {
        LOGW("%s: Wait on fence for buffer %p timed out",  __FUNCTION__, capBufPtr->buf->data());
    }
    capBufPtr->buf->setRequestId(request->getId());
    capBufPtr->v4l2Buf.m.userptr = (unsigned long int)capBufPtr->buf->data();

    *capBuf = capBufPtr;

    return status;
}

void CaptureUnit::returnBuffer(CaptureBuffer* buffer)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_RETURN_BUFFER;
    msg.data.buffer.captureBufPtr = buffer;
    status = mMessageQueue.send(&msg);
}

void CaptureUnit::notifyIsysEvent(IsysMessage &isysMsg)
{
    status_t status = NO_ERROR;

    if (isysMsg.id == ISYS_MESSAGE_ID_EVENT) {
        HAL_GUITRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE,
                          isysMsg.data.event.requestId);
        LOG2("@%s: request ID: %d, node type: %d", __FUNCTION__,
                isysMsg.data.event.requestId, isysMsg.data.event.nodeType);
        Message msg;
        msg.id = MESSAGE_ID_ISYS_EVENT;
        msg.data.buffer.requestId = isysMsg.data.event.requestId;
        msg.data.buffer.nodeType = isysMsg.data.event.nodeType;
        msg.data.buffer.v4l2Buf = *isysMsg.data.event.buffer;
        status = mMessageQueue.send(&msg);
    } else {
        LOGE("@%s: Error from input system", __FUNCTION__);
    }
}

/**
 * initStaticMetadata
 *
 * Create CameraMetadata object to retrieve the static tags used in this class
 * we cache them as members so that we do not need to query CameraMetadata class
 * everytime we need them. This is more efficient since find() is not cheap
 */
void
CaptureUnit::initStaticMetadata(void)
{
    sPipelineDepth = 0;
    sBufferPoolSize = 0;
    sSensorSettingsDelay = 0;
    /**
    * Initialize the CameraMetadata object with the static metadata tags
    */
    camera_metadata_t* plainStaticMeta;
    plainStaticMeta = (camera_metadata_t*)PlatformData::getStaticMetadata(mCameraId);
    CameraMetadata *staticMeta = new CameraMetadata(plainStaticMeta);

    camera_metadata_entry entry;
    entry = staticMeta->find(ANDROID_REQUEST_PIPELINE_MAX_DEPTH);
    if(entry.count == 1) {
        sPipelineDepth = entry.data.u8[0];
    }

    if (sPipelineDepth <= 0) {
        sPipelineDepth = DEFAULT_PIPELINE_DEPTH;
        LOGW("Pipeline depth from XML file was <= 0. Check your config");
    }

    sBufferPoolSize = sPipelineDepth - 1;

    const IPU4CameraCapInfo *info = getIPU4CameraCapInfo(mCameraId);
    sSensorSettingsDelay = MAX(info->mExposureLag, info->mGainLag);

    LOG1("%s: pipeline depth %d, buffer pool : %d, sensor delay %d",
         __FUNCTION__, sPipelineDepth, sBufferPoolSize, sSensorSettingsDelay);
}
}; // namespace camera2
}; // namespace android
