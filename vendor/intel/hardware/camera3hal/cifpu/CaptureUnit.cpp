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
//#include "MediaController.h"
#include "LogHelper.h"
#include "CaptureUnit.h"
#include "CameraStream.h"
#include "CIFVideoNode.h"
#include <hardware/camera3.h>

namespace android {
namespace camera2 {

/**
 * REQUEST_QUEUE_DEPTH is the minimum number of requests that must be enqueued
 * before ISYS can be started
 */
static const unsigned int REQUEST_QUEUE_DEPTH = DEFAULT_PIPELINE_DEPTH - 1;
static const int BUFFER_POOL_SIZE = REQUEST_QUEUE_DEPTH;
static const char *CAMERA_OVERLAY_DEV_NAME = "/dev/video0";
static const char *CAMERA_ISP_DEV_NAME     = "/dev/video1";
static const char *CAMERA_CAPTURE_DEV_NAME = "/dev/video2";

CaptureUnit::CaptureUnit(int camId):
    mCameraId(camId),
    mOverlayVideoNode(NULL),
    mCaptureVideoNode(NULL),
    mISPVideoNode(NULL),
    mRawStream(NULL),
    mMessageQueue("Camera_CaptureUnit", (int)MESSAGE_ID_MAX),
    mMessageThead(NULL),
    mThreadRunning(false)
{
    LOG1("@%s: id: %d", __FUNCTION__, camId);
    CLEAR(mCaptureConfig);
    CLEAR(mOverlayConfig);
    CLEAR(mOverlayExtraBuffer);
    CLEAR(mCaptureExtraBuffer);
}

CaptureUnit::~CaptureUnit()
{
    LOG1("@%s", __FUNCTION__);

    if (mCaptureVideoNode != NULL) {
        mCaptureVideoNode->close();
    }
    if (mISPVideoNode != NULL) {
        mISPVideoNode->close();
    }
    if (mOverlayVideoNode != NULL) {
        mOverlayVideoNode->close();
    }

    if (mPollerThread != NULL) {
        mPollerThread->requestExitAndWait();
        mPollerThread.clear();
    }

    if (mMessageThead != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }

    mOverlayExtraBuffer.buf = NULL;
    mCaptureExtraBuffer.buf = NULL;
}

status_t CaptureUnit::init()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mOverlayVideoNode = new CIFVideoNode(CAMERA_OVERLAY_DEV_NAME);
    mCaptureVideoNode = new CIFVideoNode(CAMERA_CAPTURE_DEV_NAME);
    mISPVideoNode = new CIFVideoNode(CAMERA_ISP_DEV_NAME);
    mPollerThread = new PollerThread("CIF Device Poller");

    if (mOverlayVideoNode == NULL || mCaptureVideoNode == NULL ||
        mISPVideoNode == NULL || mPollerThread == NULL) {
        ALOGE("@%s: Error creating videonodes or poller", __FUNCTION__);
        return NO_INIT;
    }

    status = mOverlayVideoNode->open();
    if (status != OK) {
        ALOGE("Couldn't open overlay videonode");
        goto bail;
    } else
        LOG1("Successfully opened the overlay videonode");

    status = mCaptureVideoNode->open();
    if (status != OK) {
        ALOGE("Couldn't open capture videonode");
        goto bail;
    } else
        LOG1("Successfully opened the capture videonode");

    status = mISPVideoNode->open();
    if (status != OK) {
        ALOGE("Couldn't open ISP videonode");
        goto bail;
    } else
        LOG1("Successfully opened the ISP videonode");

    mOverlayVideoNode->setInputBufferType(V4L2_BUF_TYPE_VIDEO_OVERLAY);
    mOverlayVideoNode->setInput(mCameraId);

    mDevices.clear();
    mDevices.push(mOverlayVideoNode);
    mDevices.push(mCaptureVideoNode);
    mDevices.push(mISPVideoNode);

    mPollDevices.clear();
    mPollDevices.push(mOverlayVideoNode);
    //mPollDevices.push(mCaptureVideoNode);
    //mPollDevices.push(mISPVideoNode);
    //
    mPollerThread->init(mPollDevices, this);

    mMessageThead = new MessageThread(this, "CaptureUnit");
    if (mMessageThead == NULL) {
        LOGE("@%s: Error creating message thread", __FUNCTION__);
        goto bail;
    }

    return status;

bail:
    mOverlayVideoNode->close();
    mCaptureVideoNode->close();
    mISPVideoNode->close();
    return NO_INIT;
}

status_t CaptureUnit::notifyPollEvent(PollEventMessage *pollMsg)
{
    LOG2("@%s", __FUNCTION__);
    Message msg;
    CLEAR(msg);
    if (pollMsg->data.activeDevices == NULL   ||
        pollMsg->data.inactiveDevices == NULL ||
        pollMsg->data.polledDevices == NULL)
        return BAD_VALUE;

    // figure out which device(s) have data
    int activeDeviceCount = pollMsg->data.activeDevices->size();
    for (int i = 0; i < activeDeviceCount; i++) {
        if (pollMsg->data.activeDevices->itemAt(i) == mOverlayVideoNode)
            msg.data.notify.overlay = true;
        if (pollMsg->data.activeDevices->itemAt(i) == mCaptureVideoNode)
            msg.data.notify.capture = true;
        if (pollMsg->data.activeDevices->itemAt(i) == mISPVideoNode)
            msg.data.notify.isp = true;
    }

    if (pollMsg->data.activeDevices->size() == 0) {
        ALOGW("No active devices, dropping the event");
        return OK;
    }

    if (pollMsg->id != IPollEventListener::POLL_EVENT_ID_EVENT) {
        ALOGW("dropping the notify event, it was a timeout");
        return OK;
    }

    status_t status = NO_ERROR;

    // There are two alternatives here, either touch the video node directly
    // here and grab from it (not entirely thread safe necessarily)
    // or pass the work synchronously to the capture unit thread, which is a bit
    // slower if it is busy. We pass the work now, to be sure. Asynchronous
    // message would not work as the poll would just return immediately in a
    // busy-loop
    msg.id = MESSAGE_ID_NOTIFY;
    bool captureDropped = false;
    bool overlayDropped = false;
    msg.data.notify.captureDropped = &captureDropped;
    msg.data.notify.overlayDropped = &overlayDropped;
    status = mMessageQueue.send(&msg, MESSAGE_ID_NOTIFY);

    const Vector<sp<V4L2DeviceBase> > *inactiveDevices = pollMsg->data.inactiveDevices;
    LOG2("polled %d devices inactive was %d C:%d O:%d", pollMsg->data.polledDevices->size(), inactiveDevices->size(), captureDropped, overlayDropped);
    if (pollMsg->data.polledDevices->size() == 2 && (captureDropped || overlayDropped)) {
        // because extra buffers (which are "dropped") are always polled out one-by-one, this should never happen..
        // but it has happened due to bad bugs, which needed to be fixed, so leaving this here for now
        // this abort() and the "dropped" booleans are to be removed once the HAL operation stabilizes!
        ALOGE("This can't happen, polled two devices, one dropped buffer. Serious bug, things will go havoc, debug this! Aborting..");
        abort();
    }

    if (inactiveDevices->size() > 0) {
        pollMsg->data.polledDevices->clear();
        *pollMsg->data.polledDevices = *inactiveDevices; // retry with failed devices
        return -EAGAIN;
    }

    return OK;
}

// TODO refactor these poll-handlers so that they don't have as much copy-paste-code between each other
status_t CaptureUnit::handleOverlayPoll()
{
    HAL_TRACE_CALL(2);
    CameraStream *stream = (CameraStream*) mOverlayStream->priv;
    v4l2_buffer_info bufferInfo;
    status_t status = mOverlayVideoNode->grabFrame(&bufferInfo);
    if (status < 0) {
        ALOGE("Couldn't grab the frame");
        status = UNKNOWN_ERROR;
        goto bail;
    }
    status = OK;

    ICaptureEventListener::CaptureMessage outMsg;
    outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT;
    outMsg.data.event.v4l2buffer = bufferInfo.vbuffer;
    // the following 3 can be probably removed as redundant
    outMsg.data.event.timestamp.tv_sec  = bufferInfo.vbuffer.timestamp.tv_sec;
    outMsg.data.event.timestamp.tv_usec = bufferInfo.vbuffer.timestamp.tv_usec;
    outMsg.data.event.sequence = bufferInfo.vbuffer.sequence;

    if (bufferInfo.vbuffer.index == mOverlayExtraBuffer.v4l2Buf.index) {
        LOG2("extra overlay buffer dequeued, dropping it");
        mOverlayExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_FREE;
        return BAD_INDEX;
    } else {
        outMsg.data.event.buffer = mOverlayCaptureBuffers[bufferInfo.vbuffer.index];
    }

    outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_RAW_BAYER;

    notifyListeners(&outMsg);

    mOverlayItemsPool.releaseItem(mOverlayCaptureBuffers[bufferInfo.vbuffer.index]);

bail:
    return status;
}

status_t CaptureUnit::handleCapturePoll()
{
    HAL_TRACE_CALL((mCaptureConfig.format == V4L2_PIX_FMT_JPEG) ? 1 : 2);
    CameraStream *stream = (CameraStream*) mCaptureStream->priv;
    v4l2_buffer_info bufferInfo;
    status_t status = mCaptureVideoNode->grabFrame(&bufferInfo);
    if (status < 0) {
        ALOGE("Couldn't grab the frame");
        status = UNKNOWN_ERROR;
        goto bail;
    }
    status = OK;

    ICaptureEventListener::CaptureMessage outMsg;
    outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT;
    outMsg.data.event.v4l2buffer = bufferInfo.vbuffer;
    // the following 3 can be probably removed as redundant
    outMsg.data.event.timestamp.tv_sec  = bufferInfo.vbuffer.timestamp.tv_sec;
    outMsg.data.event.timestamp.tv_usec = bufferInfo.vbuffer.timestamp.tv_usec;
    outMsg.data.event.sequence = bufferInfo.vbuffer.sequence;

    if (bufferInfo.vbuffer.index == mCaptureExtraBuffer.v4l2Buf.index) {
        LOG2("extra capture buffer dequeued, dropping it");
        mCaptureExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_FREE;
        return BAD_INDEX;
    } else {
        outMsg.data.event.buffer = mRawCaptureBuffers[bufferInfo.vbuffer.index];
    }

    outMsg.data.event.buffer = mRawCaptureBuffers[bufferInfo.vbuffer.index];
    outMsg.data.event.type = ICaptureEventListener::CAPTURE_EVENT_RAW_BAYER;

    if (mCaptureConfig.format == V4L2_PIX_FMT_JPEG) {
        // TODO consider if we could just update the full v4l2Buf in CaptureBuffer by overwriting it with bufferInfo
        // here we store the actual JPEG data size into bytesused, without the JFIF header part - controlunit will need the size
        mRawCaptureBuffers[bufferInfo.vbuffer.index]->v4l2Buf.bytesused = bufferInfo.vbuffer.bytesused - JFIF_HEADER_SIZE;
    }

    notifyListeners(&outMsg);

    mCaptureItemsPool.releaseItem(mRawCaptureBuffers[bufferInfo.vbuffer.index]);

bail:
    return status;
}

status_t CaptureUnit::handleMessageNotify(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = OK;

    if (msg.data.notify.isp) {
        ALOGE("Unfortunately, statistics buffer handling is not implemented yet. Aborting.");
        abort();
    }

    if (msg.data.notify.overlay)
        status = handleOverlayPoll();

    if (status == BAD_INDEX) {
        // frame was the extra buffer, and it was dropped. Which is ok, for now.
        *msg.data.notify.overlayDropped = true;
        status = OK;
    }

    if (status != OK && status != BAD_INDEX)
        return status;

    if (msg.data.notify.capture)
        status = handleCapturePoll();

    if (status == BAD_INDEX) {
        // frame was the extra buffer, and it was dropped. Which is ok, for now.
        *msg.data.notify.captureDropped = true;
        status = OK;
    }

    mMessageQueue.reply(MESSAGE_ID_NOTIFY, status);

    return status;
}

CaptureUnit::CaptureUseCase CaptureUnit::figureOutUseCase(Vector<camera3_stream_t*> &activeStreams)
{
    if (activeStreams.size() == 3) {
        LOG1("Video use case");
        return VIDEO;
    } else {
        LOG1("Still use case");
        return STILL;
    }
}


status_t CaptureUnit::configStreams(Vector<camera3_stream_t*> &activeStreams)
{
    LOG1("@%s", __FUNCTION__);
    sp<V4L2VideoNode> videoNode = NULL;
    status_t status = NO_ERROR;
    // this ought to be message based IMO

    Vector<sp<V4L2DeviceBase> >::iterator it = mDevices.begin();
    for (;it != mDevices.end(); ++it) {
        videoNode = reinterpret_cast<V4L2VideoNode *>(it->get());
        status = videoNode->stop();
        LOG1("stopping video device %s", videoNode->name());
    }

    CLEAR(mCaptureConfig);
    CLEAR(mOverlayConfig);
    freeBuffers();
    mActiveStreams.clear();
    mOverlayStream = NULL;
    mCaptureStream = NULL;
    mOverlayExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_FREE;
    mCaptureExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_FREE;
    ItemPool<CaptureBuffer> *itemPool = NULL;
    Vector<CaptureBuffer*> *buffers = NULL;

    CaptureUseCase useCase = figureOutUseCase(activeStreams);

    if (useCase == STILL) {
        for (unsigned int i = 0; i < activeStreams.size(); i++) {
            camera3_stream_t *stream = activeStreams.editItemAt(i);
            mActiveStreams.push_back(stream);

            // todo here (well, during configure streams) we should figure out how
            // to configure devices with regards to the stream configuration.
            FrameInfo *config = NULL;
            if (stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
                stream->format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
                LOG1("TODO: remove this print and make proper stream-videonode mappings. Found the YUV STREAM!");
                config = &mOverlayConfig;
                mOverlayConfig.format = V4L2_PIX_FMT_NV21;
                mOverlayStream = stream;
                videoNode = mOverlayVideoNode;
                itemPool = &mOverlayItemsPool;
                buffers = &mOverlayCaptureBuffers;
                stream->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            }

            if (stream->format == HAL_PIXEL_FORMAT_BLOB) {
                LOG1("TODO: remove this print and make proper stream-videonode mappings. Found the JPEG STREAM!");
                config = &mCaptureConfig;
                mCaptureConfig.format = V4L2_PIX_FMT_JPEG;
                mCaptureStream = stream;
                videoNode = mCaptureVideoNode;
                itemPool = &mCaptureItemsPool;
                buffers = &mRawCaptureBuffers;
            }

            config->width = stream->width;
            config->height = stream->height;
            config->bufsNum = stream->max_buffers;
            config->stride = /* TODO FIXME */ stream->width;

            if (videoNode != NULL) {
                status = videoNode->setFormat(*config);
                if (status != OK) {
                    ALOGE("Could not configure videonode");
                }
                if (itemPool != NULL && buffers != NULL) {
                    LOG1("@%s creating buffer pool with size %d", __FUNCTION__, config->bufsNum);
                    createBufferPool(config->bufsNum, videoNode, *itemPool, *buffers);
                }

                videoNode = NULL; // clear it for next loop round
            }
        }
    } else {
        bool firstYuvStreamFound = false;
        for (unsigned int i = 0; i < activeStreams.size(); i++) {
            camera3_stream_t *stream = activeStreams.editItemAt(i);

            // todo here (well, during configure streams) we should figure out how
            // to configure devices with regards to the stream configuration.
            FrameInfo *config = NULL;
            if (stream->format == HAL_PIXEL_FORMAT_YCrCb_420_SP ||stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
                if (!firstYuvStreamFound) {
                    LOG1("TODO: remove this print and make proper stream-videonode mappings. Found the YUV STREAM!");
                    firstYuvStreamFound = true;
                    config = &mOverlayConfig;
                    mOverlayConfig.format = V4L2_PIX_FMT_NV21;
                    mOverlayStream = stream;
                    videoNode = mOverlayVideoNode;
                    itemPool = &mOverlayItemsPool;
                    buffers = &mOverlayCaptureBuffers;
                    stream->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                } else {
                    LOG1("TODO: remove this print and make proper stream-videonode mappings. Found the YUV STREAM 2!");
                    config = &mCaptureConfig;
                    mCaptureConfig.format = V4L2_PIX_FMT_NV21;
                    mCaptureStream = stream;
                    videoNode = mCaptureVideoNode;
                    itemPool = &mCaptureItemsPool;
                    buffers = &mRawCaptureBuffers;
                    stream->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
                }
            } else {
                LOG1("No config for this stream format 0x%x", stream->format);
                continue;
            }
            mActiveStreams.push_back(stream);

            config->width = stream->width;
            config->height = stream->height;
            config->bufsNum = stream->max_buffers;
            config->stride = /* TODO FIXME */ stream->width;

            if (videoNode != NULL) {
                status = videoNode->setFormat(*config);
                if (status != OK) {
                    ALOGE("Could not configure videonode");
                }
                if (itemPool != NULL && buffers != NULL) {
                    LOG1("@%s creating buffer pool with size %d", __FUNCTION__, config->bufsNum);
                    createBufferPool(config->bufsNum, videoNode, *itemPool, *buffers);
                }

                videoNode = NULL; // clear it for next loop round
            }
        }
    }

    allocateExtraBuffers();

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

status_t CaptureUnit::getMediaCtlCamConfig()
{
    for (unsigned int i = 0; i < mActiveStreams.size(); i++) {
        if ((unsigned int)mCaptureConfig.width < mActiveStreams[i]->width &&
            (unsigned int)mCaptureConfig.height < mActiveStreams[i]->height) {
                mCaptureConfig.width = mActiveStreams[i]->width;
                mCaptureConfig.height = mActiveStreams[i]->height;
        }
   }

    LOG1("Selected MediaCtl pipe config: %dx%d",mCaptureConfig.width,
                                                    mCaptureConfig.height);
    return NO_ERROR;
}

status_t CaptureUnit::createBufferPool(int numBufs, sp<V4L2VideoNode> videoNode,
                                       ItemPool<CaptureBuffer> &itemPool,
                                       Vector<CaptureBuffer*> &buffers)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = OK;

    // Fill up the empty v4L2 buffers
    Vector<struct v4l2_buffer> v4l2Buffers;
    struct v4l2_buffer v4l2Buf;
    CaptureBuffer *capBufPtr = NULL;
    CLEAR(v4l2Buf);
    v4l2Buffers.clear();

    // "numBufs +1" is a CIF driver quirk - we need an extra v4l2_buffer due to
    // a hw bug which makes the driver hold one buffer always, so to get the
    // buffer out we sometimes send an extra buffer into the driver
    // TODO FIXME remove this +1 quirk when possible
    for (int i = 0; i < numBufs + 1; i++) {
        v4l2Buf.index = i;
        v4l2Buffers.push(v4l2Buf);
    }

    videoNode->setBufferPool(v4l2Buffers, false);

    itemPool.init(numBufs);

    for (int i = 0; i < numBufs; i++) {
        itemPool.acquireItem(&capBufPtr);
        capBufPtr->v4l2Buf = v4l2Buffers[i];
        capBufPtr->owner = this;
        buffers.push_back(capBufPtr);
        itemPool.releaseItem(capBufPtr);
    }

    return status;
}

status_t CaptureUnit::allocateExtraBuffers()
{
    HAL_TRACE_CALL(1);
    // The format handling seems to be a mess.
    // GFX allocation wants GFX format types, heap allocations seem to want
    // v4l2 format types. HAL FrameInfo has some rather oddly named duplicate
    // format fields like format and originalFormat?? facepalm here..
    int format = HAL_PIXEL_FORMAT_YCrCb_420_SP; // just support NV21 now

    // overlay device always needs one extra buffer
    mOverlayExtraBuffer.buf =
            MemoryUtils::allocateGraphicBuffer(mOverlayConfig.stride,
                                               mOverlayConfig.height,
                                               format,
                                               mCameraId);

    if (mOverlayExtraBuffer.buf == NULL) {
        ALOGE("Couldn't allocate extra overlay buffer for CIF");
        return NO_MEMORY;
    }

    mOverlayExtraBuffer.buf->lock();
    mOverlayExtraBuffer.v4l2Buf.m.userptr = (long unsigned int) mOverlayExtraBuffer.buf->data();
    LOG1("allocated ovl buf of size: %d", mOverlayExtraBuffer.buf->size());

    // capture device needs one extra buffer, if it is NOT configured for JPEG
    if (mCaptureConfig.format != V4L2_PIX_FMT_JPEG) {
        // just support NV21 now
        format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        mCaptureExtraBuffer.buf =
                MemoryUtils::allocateGraphicBuffer(mCaptureConfig.stride,
                                                   mCaptureConfig.height,
                                                   format,
                                                   mCameraId);

        if (mCaptureExtraBuffer.buf == NULL) {
            ALOGE("Couldn't allocate extra capture buffer for CIF");
            return NO_MEMORY;
        }
        mCaptureExtraBuffer.buf->lock();
        mCaptureExtraBuffer.v4l2Buf.m.userptr = (long unsigned int) mCaptureExtraBuffer.buf->data();
        LOG1("allocated cap buf of size: %d", mCaptureExtraBuffer.buf->size());
    } else {
        mCaptureExtraBuffer.buf = NULL;
    }

    mOverlayExtraBuffer.v4l2Buf.index = mOverlayConfig.bufsNum;
    mCaptureExtraBuffer.v4l2Buf.index = mCaptureConfig.bufsNum;

    return OK;
}

status_t CaptureUnit::allocateCaptureBuffers(int numBufs)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    sp<CameraBuffer> tmpBuf = NULL;
    CaptureBuffer *capBufPtr = NULL;

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
        mCaptureItemsPool.acquireItem(&capBufPtr);
        capBufPtr->buf = tmpBuf;
        capBufPtr->v4l2Buf.m.userptr = (long unsigned int) tmpBuf->data();
        capBufPtr->v4l2Buf.index = i;
        tmpBuf.clear();
        mCaptureItemsPool.releaseItem(capBufPtr);
    }
    // TODO: allocate other buffers
    return status;
}

status_t CaptureUnit::freeBuffers()
{
    LOG1("@%s", __FUNCTION__);

    mCaptureItemsPool.deInit();
    mRawCaptureBuffers.clear();
    mOverlayItemsPool.deInit();
    mOverlayCaptureBuffers.clear();

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

//    status = mSensor->getPixelRate(pixel);
//    if (status != NO_ERROR) {
//        LOGE("%s: failed to get pixel clock", __FUNCTION__);
//        return status;
//    }
    aiqSensorDescriptor->pixel_clock_freq_mhz = (float)pixel/1000000;

//    status = mSensor->getFrameDuration((unsigned int&)aiqSensorDescriptor->pixel_periods_per_line,
//                                        (unsigned int&)aiqSensorDescriptor->line_periods_per_field);
//    if (status != NO_ERROR) {
//        LOGE("%s: failed to get frame Durations", __FUNCTION__);
//        return status;
//    }

//    status = mSensor->getExposureRange((int&)aiqSensorDescriptor->coarse_integration_time_min,
//                                        (int&) integration_max, integration_step);
//    if (status != NO_ERROR) {
//        LOGE("%s: failed to get Exposure Range", __FUNCTION__);
//        return status;
//    }

    LOG2("%s: Exposure range coarse :min = %d, max = %d, step = %d",
                                    __FUNCTION__,
                                    aiqSensorDescriptor->coarse_integration_time_min,
                                    integration_max, integration_step);

    aiqSensorDescriptor->coarse_integration_time_max_margin =
                        aiqSensorDescriptor->line_periods_per_field - integration_max;

    //INFO: fine integration is not supported by v4l2
    aiqSensorDescriptor->fine_integration_time_min = 0;
    aiqSensorDescriptor->fine_integration_time_max_margin = 0;

//    status = mSensor->getVBlank((unsigned int&)aiqSensorDescriptor->line_periods_vertical_blanking);

    LOG2("%s: pixel clock = %f  ppl = %d, lpf =%d, int_min = %d, int_max_range %d",
                                    __FUNCTION__,
                                    aiqSensorDescriptor->pixel_clock_freq_mhz,
                                    aiqSensorDescriptor->pixel_periods_per_line,
                                    aiqSensorDescriptor->line_periods_per_field,
                                    aiqSensorDescriptor->coarse_integration_time_min,
                                    aiqSensorDescriptor->coarse_integration_time_max_margin);
    return status;
}

int CaptureUnit::setExposure(unsigned int exposure, unsigned int gain)
{
    int ret;
    struct v4l2_ext_control exp_gain[2];
    struct v4l2_ext_controls ctrls;

    exp_gain[0].id = V4L2_CID_EXPOSURE;
    exp_gain[0].value = exposure;
    exp_gain[1].id = V4L2_CID_GAIN;
    exp_gain[1].value = gain;

    ctrls.count = 2;
    ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ctrls.controls = exp_gain;
    ctrls.reserved[0] = 0;
    ctrls.reserved[1] = 0;

    LOGV("%s 0x%x 0x%x 0x%x",__func__, VIDIOC_S_EXT_CTRLS, V4L2_CID_EXPOSURE, V4L2_CID_GAIN);

    ret = ioctl(mOverlayVideoNode->getFd(), VIDIOC_S_EXT_CTRLS, &ctrls);

    if (ret < 0)
    {
        LOGE("ERR(%s):set of  AE seting to sensor config failed\n", __func__);
        return ret;
    }

    return ret;
}

status_t CaptureUnit::applySensorParams(Camera3Request* request,
                                        AiqCaptureSettings* aiqCaptureSettings)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(aiqCaptureSettings);
    UNUSED(request);
    status_t status = NO_ERROR;

//    status = mSensor->setParameters(aiqCaptureSettings->aiqResults.aeResults);

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

bool CaptureUnit::isBufferForStream(const Vector<camera3_stream_buffer> *buffers, camera3_stream_t *stream)
{
    if (!buffers || !stream)
        return false;

    for(uint32_t i = 0; i < buffers->size(); i++) {
        if (buffers->itemAt(i).stream->priv == stream->priv) {
            return true;
        }
    }
    return false;
}

void CaptureUnit::handleOverlayExtraBuffer()
{
    if (mOverlayExtraBuffer.v4l2Buf.reserved == EXTRA_BUFFER_QUEUED) {
        Vector<sp<V4L2DeviceBase> > xtraDevice;
        xtraDevice.add(mOverlayVideoNode);
        mPollerThread->pollRequest(0, 1000, &xtraDevice); // the request id is bogus for the extra buffer
        LOG2("@%s requesting to poll the extra overlay buffer", __FUNCTION__);
        mOverlayExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_QUEUED_AND_POLLING;
    }
}

void CaptureUnit::handleCaptureExtraBuffer()
{
    if (mCaptureExtraBuffer.v4l2Buf.reserved == EXTRA_BUFFER_QUEUED) {
        Vector<sp<V4L2DeviceBase> > xtraDevice;
        xtraDevice.add(mCaptureVideoNode);
        mPollerThread->pollRequest(0, 1000, &xtraDevice); // the request id is bogus for the extra buffer
        LOG2("@%s requesting to poll the extra overlay buffer", __FUNCTION__);
        mCaptureExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_QUEUED_AND_POLLING;
    }
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

    const Vector<camera3_stream_buffer> *outBuffers = request->getOutputBuffers();
    Vector<sp<V4L2DeviceBase> > devices;

    if (!outBuffers) {
        ALOGE("NO output buffers!");
        return UNKNOWN_ERROR;
    }

    if (isBufferForStream(outBuffers, mOverlayStream)) {
        // unfortunately we need to wait for doing these extra buffer polls until here, when
        // the client wants to queue new buffers - otherwise stream-off can fail (driver issue while polling)
        handleOverlayExtraBuffer();

        status = findOverlayBuffer(request, &capBufPtr);
        if (status == OK) {

            sp<V4L2VideoNode> videoNode = mOverlayVideoNode;
            devices.add(videoNode);

            int ret = videoNode->putFrame(&capBufPtr->v4l2Buf);
            if (!videoNode->isStarted()) {
                videoNode->start(0);
                if (mCameraId == 0)
                    setExposure(0, 160); // temporary, to ease debugging
                else
                    setExposure(240, 240); // temporary, to ease debugging
            }
        }
    }

    if (isBufferForStream(outBuffers, mCaptureStream)) {
        // unfortunately we need to wait for doing these extra buffer polls until here, when
        // the client wants to queue new buffers - otherwise stream-off can fail (driver issue while polling)
        handleCaptureExtraBuffer();
        {// CIF driver quirk start - if the format is JPG, stream off and reset everything
            if (mCaptureConfig.format == V4L2_PIX_FMT_JPEG) {
                if (mCaptureVideoNode->getBufsInDeviceCount() > 0)
                    ALOGW("TODO: handle the stream-off with multiple buffers in device");

                mCaptureVideoNode->stop(false);

                status = mCaptureVideoNode->setFormat(mCaptureConfig);
                if (status != OK) {
                    ALOGE("Could not configure videonode");
                }
                LOG1("@%s creating buffer pool with size %d", __FUNCTION__, mCaptureConfig.bufsNum);
                mCaptureItemsPool.deInit();
                createBufferPool(mCaptureConfig.bufsNum, mCaptureVideoNode, mCaptureItemsPool, mRawCaptureBuffers);
            }
        }// CIF driver quirk end

        status = findRawBuffer(request, &capBufPtr);
        if (status == OK) {
            sp<V4L2VideoNode> videoNode = mCaptureVideoNode;
            devices.add(videoNode);
            int ret = videoNode->putFrame(&capBufPtr->v4l2Buf);
            if (!videoNode->isStarted())
                videoNode->start(0);
        }
        status = OK; // we don't mind if we did not find a capture buffer, not all requests carry one
    }

    mPollerThread->pollRequest(request->getId(), 1000, &devices); // for CIF, we allow 1000ms timeouts

    return status;
}

void CaptureUnit::dump(int fd)
{
    String8 message;
    message.appendFormat(LOG_TAG ":@%s\n", __FUNCTION__);
    message.appendFormat(LOG_TAG " overlay device buffer count:%d capture device buffer count:%d overlay xtra used:%d capture xtra used %d\n",
                          mOverlayVideoNode->getBufsInDeviceCount(),
                          mCaptureVideoNode->getBufsInDeviceCount(),
                          mOverlayExtraBuffer.v4l2Buf.reserved,
                          mCaptureExtraBuffer.v4l2Buf.reserved);
    write(fd, message.string(), message.size());
}

status_t CaptureUnit::handleMessageCapture(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    Camera3Request *request = NULL;
    AiqCaptureSettings* captureSettings = NULL;
    MessageRequest input;

    request = msg.data.request.request;
    captureSettings = msg.data.request.settings;

    status = enqueueBuffers(request);
    if (status != NO_ERROR) {
        LOGE("@%s: Failed to enqueue buffers!", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    return status;
}

status_t CaptureUnit::handleMessageReturnBuffer(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    CaptureBuffer* buffer = msg.data.buffer.captureBufPtr;
    for (size_t i = 0; i < mRawCaptureBuffers.size(); i++) {
        if (buffer->v4l2Buf.m.userptr == mRawCaptureBuffers[i]->v4l2Buf.m.userptr) {
            mRawCaptureBuffers.removeAt(i);
            break;
        }
    }

    status = mCaptureItemsPool.releaseItem(buffer);
    if (status != NO_ERROR) {
        LOGE("Failed to return consumed capture buffer to the pool");
    } else {
        LOG2("capture buffer for request %d recycled", buffer->buf->requestId());
    }

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

    // Notify listeners, first fill the observer message
    outMsg.id = ICaptureEventListener::CAPTURE_MESSAGE_ID_EVENT;
    outMsg.data.event.timestamp.tv_sec  = outBuf->vbuffer.timestamp.tv_sec;
    outMsg.data.event.timestamp.tv_usec = outBuf->vbuffer.timestamp.tv_usec;
    outMsg.data.event.sequence = outBuf->vbuffer.sequence;

    for (size_t i = 0; i < mRawCaptureBuffers.size(); i++) {
        if (outBuf->vbuffer.m.userptr == mRawCaptureBuffers[i]->v4l2Buf.m.userptr) {
            mRawCaptureBuffers[i]->buf->setRequestId(requestId);
            outMsg.data.event.buffer = mRawCaptureBuffers.editItemAt(i);
            break;
        }
    }

    switch(nodeType) {
    case VIDEO_RAW_BAYER:
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

status_t CaptureUnit::handleExtraBufferQueuing()
{
    HAL_TRACE_CALL(2);
    if (mMessageQueue.size() == 0) {
        // queue is empty so it is time to check if the devices
        // need extra buffers
        if (mOverlayVideoNode->getBufsInDeviceCount() == 1 &&  // one in device
            mOverlayExtraBuffer.v4l2Buf.reserved == EXTRA_BUFFER_FREE) {  // it is not the extra buffer
            LOG2("overlay device has only one buffer, it won't come out");
            // mark the extra buffer as queued via the reserved field
            mOverlayExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_QUEUED;
            // queue the extra buffer
            mOverlayVideoNode->putFrame(&mOverlayExtraBuffer.v4l2Buf);
        }

        // capture device can return a single buffer IFF it is a JPEG buffer
        if (mCaptureVideoNode->getBufsInDeviceCount() == 1   &&  // one in device
            mCaptureConfig.format != V4L2_PIX_FMT_JPEG &&  // it is not JPEG
            mCaptureExtraBuffer.v4l2Buf.reserved == EXTRA_BUFFER_FREE) {   // it is not the extra buffer
            LOG2("capture device has only one buffer, it won't come out");
            // mark the extra buffer as queued via the reserved field
            mCaptureExtraBuffer.v4l2Buf.reserved = EXTRA_BUFFER_QUEUED;
            // queue the extra buffer
            mCaptureVideoNode->putFrame(&mCaptureExtraBuffer.v4l2Buf);
        }
    }

    // some logging to help figuring out situations where things have gotten.. stuck..
    if (mOverlayVideoNode->getBufsInDeviceCount() == 1 &&  // one in device
        mOverlayExtraBuffer.v4l2Buf.reserved == EXTRA_BUFFER_QUEUED)
        LOG1("Only the extra buffer in overlay node");

    if (mCaptureVideoNode->getBufsInDeviceCount() == 1 &&  // one in device
        mCaptureExtraBuffer.v4l2Buf.reserved == EXTRA_BUFFER_QUEUED)
        LOG1("Only the extra buffer in capture node");

    return OK;
}

void CaptureUnit::messageThreadLoop(void)
{
    LOG1("@%s: Start", __FUNCTION__);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        handleExtraBufferQueuing();

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

        case MESSAGE_ID_NOTIFY:
            status = handleMessageNotify(msg);
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
 * findOverlayBuffer
 *
 * Find a overlay buffer from the request and initialize one of the pooled
 * buffers with it's data.
 */
status_t CaptureUnit::findOverlayBuffer(Camera3Request *request,
                                        CaptureBuffer **capBuf)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    CaptureBuffer *capBufPtr = NULL;

    /**
     * pick one buffer from the capture buffer pool and pick up the the buffer
     * from the request
     */
    mOverlayItemsPool.acquireItem(&capBufPtr);

    capBufPtr->buf = request->findBuffer((CameraStreamNode*)mOverlayStream->priv);
    if (CC_UNLIKELY(capBufPtr->buf == NULL)) {
        LOGE("Overlay buffer not found - BUG ");
        mOverlayItemsPool.releaseItem(capBufPtr);
        return INVALID_OPERATION;
    }

    status = capBufPtr->buf->waitOnAcquireFence();
    if (status != NO_ERROR) {
        LOGW("%s: Wait on fence for buffer %p timed out",  __FUNCTION__, capBufPtr->buf->data());
    }

    status = capBufPtr->buf->lock();
    status = OK;
//    if (status != NO_ERROR) {
//        LOGE("%s: Failed to lock buffer handle", __FUNCTION__);
//        return status;
//    }
    capBufPtr->buf->setRequestId(request->getId());
    capBufPtr->v4l2Buf.m.userptr = (unsigned long int)capBufPtr->buf->data();

    *capBuf = capBufPtr;

    return status;
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
    capBufPtr->buf = NULL;

    capBufPtr->buf = request->findBuffer((CameraStreamNode*)mCaptureStream->priv);
    if (CC_UNLIKELY(capBufPtr->buf == NULL)) {
        LOGE("Raw buffer not found - BUG ");
        mCaptureItemsPool.releaseItem(capBufPtr);
        return INVALID_OPERATION;
    }

    status = capBufPtr->buf->waitOnAcquireFence();
    if (status != NO_ERROR) {
        LOGW("%s: Wait on fence for buffer %p timed out",  __FUNCTION__, capBufPtr->buf->data());
    }

    status = capBufPtr->buf->lock();
    if (status != NO_ERROR) {
        LOGE("%s: Failed to lock buffer handle", __FUNCTION__);
        return status;
    }
    capBufPtr->buf->setRequestId(request->getId());
    capBufPtr->v4l2Buf.length = capBufPtr->buf->size();
    capBufPtr->v4l2Buf.m.userptr = (unsigned long int) capBufPtr->buf->data();
    if (mCaptureConfig.format == V4L2_PIX_FMT_JPEG) {
        // add offset so that exif can be written to start of buffer
        capBufPtr->v4l2Buf.m.userptr += JPEG_DATA_START_OFFSET;
        capBufPtr->v4l2Buf.m.userptr -= JFIF_HEADER_SIZE; // we will strip the JFIF
        // now CIF driver will write the DQT tag (0xffdb) to the position userptr + JPEG_DATA_START_OFFSET
        // and the exifmaker etc. need to write SOI + APP1 + APP2 between: [0, JPEG_DATA_START_OFFSET - 1]
    }

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

//void CaptureUnit::notifyIsysEvent(IsysMessage &isysMsg)
//{
//    LOG2("@%s", __FUNCTION__);
//    status_t status = NO_ERROR;
//
//    if (isysMsg.id == ISYS_MESSAGE_ID_EVENT) {
//        LOG2("@%s: request ID: %d, node type: %d", __FUNCTION__, isysMsg.data.event.requestId, isysMsg.data.event.nodeType);
//        Message msg;
//        msg.id = MESSAGE_ID_ISYS_EVENT;
//        msg.data.buffer.requestId = isysMsg.data.event.requestId;
//        msg.data.buffer.nodeType = isysMsg.data.event.nodeType;
//        msg.data.buffer.v4l2Buf = *isysMsg.data.event.buffer;
//        status = mMessageQueue.send(&msg);
//    } else {
//        LOGE("@%s: Error from input system", __FUNCTION__);
//    }
//}
//
} // namespace camera2
} // namespace android
