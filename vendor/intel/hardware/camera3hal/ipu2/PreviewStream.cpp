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
#define LOG_TAG "Camera_PreviewStream"

#include "LogHelper.h"
#include "PreviewStream.h"
#include "CameraBuffer.h"
#ifdef GRAPHIC_IS_GEN
#include <ufo/graphics.h>
#endif

#include <utils/Errors.h>

#define CSS_PREVIEW_POLL_TIMEOUT 500
#define CSS_PREVIEW_POLL_TIMEOUT_COUNT 6
#define CSS_GETFRAME_STARVING_WAIT 33000 // Time to usleep between retry's when stream is starving from buffers.

namespace android {
namespace camera2 {

PreviewStream::PreviewStream(sp<V4L2VideoNode> &aPreviewDevice,
                                   sp<IPU2HwIsp> &theIsp,
                                   IPU2HwSensor * theSensor,
                                   int cameraId,
                                   const char* streamName)
            :mVideoNode(aPreviewDevice)
             ,mIsp(theIsp)
             ,mHwSensor(theSensor)
             ,mDebugFPS(NULL)
             ,mCameraId(cameraId)
             ,mMode(MODE_PREVIEW)
             ,mRawLockEnabled(false)
             ,mFakeBufferIndex(0)
             ,mFakeBufferIndexStart(-1)
             ,mMaxNumOfSkipFrames(0)
             ,mMessageQueue("PreviewStream", (int)MESSAGE_ID_MAX)
             ,mMessageThead(NULL)
             ,mStatus(STOP)
             ,mStreamMode(ONLINE)
             ,mRealBufferIndex(-1)
{
    mStreamName.setTo(streamName ? streamName : "unnamed preview");
    LOG1("@%s, name:%s", __FUNCTION__, getName());
    mDeviceBuffers.clear();
    mBuffersInDevice.clear();
    mFakeBuffers.clear();
    CLEAR(mVideoNodeConfig);
}

/**
 * init
 * Second stage constructor with creation of the objects that poll for data
 * and events
 */
status_t PreviewStream::init()
{
    LOG1("@%s, name:%s", __FUNCTION__, getName());

    mMessageThead = new MessageThread(this, "PreviewStream");
    if (mMessageThead == NULL) {
        LOGE("Error create StreamThread in init");
        return NO_INIT;
    }

    const IPU2CameraCapInfo * capInfo = getIPU2CameraCapInfo(mCameraId);
    if (capInfo)
        mMaxNumOfSkipFrames = capInfo->frameInitialSkip() + capInfo->exposureLag();

    return NO_ERROR;
}

PreviewStream::~PreviewStream()
{
    LOG1("@%s, name:%s", __FUNCTION__, getName());

    if (mMessageThead != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }

    if (mDebugFPS != NULL) {
        mDebugFPS->requestExitAndWait();
        mDebugFPS.clear();
    }

    mBuffersInDevice.clear();
    mDeviceBuffers.clear();
    mVideoNode.clear();
    mIsp.clear();
    mFakeBuffers.clear();
}

void PreviewStream::dump(int fd) const
{
    String8 message;

    message.appendFormat(LOG_TAG ":@%s name:\"%s\"\n", __FUNCTION__, getName());
    write(fd, message.string(), message.size());
}

status_t PreviewStream::setFormat(FrameInfo *aConfig)
{
    LOG1("@%s, name:%s", __FUNCTION__, getName());
#ifdef GRAPHIC_IS_GEN
    if (aConfig->format != HAL_PIXEL_FORMAT_YCbCr_422_I &&
        aConfig->format != HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL)
#else
    if (aConfig->format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED &&
        aConfig->format != HAL_PIXEL_FORMAT_YCbCr_420_888)
#endif
    {
        LOGE("%s: don't support gfx format %d!", __FUNCTION__, aConfig->format);
        return BAD_VALUE;
    }

    status_t status = NO_ERROR;
    if (!mVideoNode->isOpen()) {
        status = mVideoNode->open();
    }

    if (status != NO_ERROR) {
        LOGE("Could not open the preview devices: status(%d)",status);
        return status;
    }
    mVideoNodeConfig = *aConfig;
    mVideoNodeConfig.originalFormat = mVideoNodeConfig.format = GFXFmt2V4l2Fmt(aConfig->format, mCameraId);
    status = mVideoNode->setFormat(mVideoNodeConfig);
    if (status != NO_ERROR) {
        LOGE("Failed to set preview format: status(%d)",status);
        return status;
    }

    PERFORMANCE_TRACES_BREAKDOWN_STEP("PreviewDevice");
    mConfig = *aConfig;
    return status;
}

void PreviewStream::setMode(IspMode mode)
{
    LOG2("@%s, name:%s, mode:%d", __FUNCTION__, getName(), mode);
    mMode = mode;
}

void PreviewStream::setMode(StreamMode mode)
{
    LOG2("@%s, name:%s, mode:%d", __FUNCTION__, getName(), mode);
    mStreamMode = mode;
}

status_t PreviewStream::configure(void)
{
    LOG1("@%s, name:%s", __FUNCTION__, getName());
    status_t status = NO_ERROR;
    Mutex::Autolock _l(mBufBookKeepingLock);
    struct v4l2_buffer v4l2Buf;
    CLEAR(v4l2Buf);

    LOG2("@%s, buffers number: real:%d, skip:%d", __FUNCTION__, REAL_BUF_NUM, mMaxNumOfSkipFrames);
    // create fake buffer for frame skipping
    mFakeBufferIndexStart = mFakeBufferIndex = REAL_BUF_NUM;
    uint32_t bufCount = REAL_BUF_NUM + mMaxNumOfSkipFrames;
    mRealBufferIndex = 0;

    // Fill up the empty v4L2 buffers
    mDeviceBuffers.clear();
    for (uint32_t i = 0 ; i < bufCount; i++) {
        mDeviceBuffers.push(v4l2Buf);
    }

    // Register them to the device
    status = mVideoNode->setBufferPool(mDeviceBuffers, true);
    if (status != NO_ERROR) {
        LOGE("Error registering preview buffers ret=%d", status);
    }

    return status;
}

status_t PreviewStream::capture(sp<CameraBuffer> aBuffer,
                                   Camera3Request* request)
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());

    status_t status = NO_ERROR;
    uint32_t index = 0;
    Mutex::Autolock _l(mBufBookKeepingLock);

    // If the input params are null, means CameraHw requires to inject
    // fake buffer.

    if (aBuffer == NULL && request == NULL && mFakeBufferIndex > 0) {
        LOG2("inject a fake buffer, index:%d", mFakeBufferIndex);
        if (mFakeBufferIndex >= mFakeBufferIndexStart + mMaxNumOfSkipFrames) {
            LOGE("can't get available buffer");
            return UNKNOWN_ERROR;
        }
        if (mDeviceBuffers[mFakeBufferIndex].m.userptr == 0) {
            LOG2("fake buffer is NULL, allocate Heap buffer for it");
            sp<CameraBuffer> buf = MemoryUtils::allocateHeapBuffer(mConfig.width, mConfig.height,
                                                widthToStride(GFXFmt2V4l2Fmt(mConfig.format, mCameraId), mConfig.width),
                                                GFXFmt2V4l2Fmt(mConfig.format, mCameraId), mCameraId);
            if (buf == NULL) {
                LOGE("Allocate Preview Heap buffer failed. width %d height %d stride %d v4l2Fmt %d",
                     mConfig.width, mConfig.height, mConfig.stride, mConfig.format);
                return NO_MEMORY;
            }

            mFakeBuffers.push_front(buf);
            mDeviceBuffers.editItemAt(mFakeBufferIndex).m.userptr = (unsigned long int)buf->data();
        }

        index = mFakeBufferIndex;
        mFakeBufferIndex ++;
        if (mFakeBufferIndex == mFakeBufferIndexStart + mMaxNumOfSkipFrames)
            mFakeBufferIndex = mFakeBufferIndexStart;
        mHwSensor->setExpectedCaptureExpId(-1, this);
    } else if (request != NULL) {
        if (!aBuffer->isLocked()) {
            status = aBuffer->lock();
            if (status != NO_ERROR) {
                LOGE("@%s: Could not lock the buffer", __FUNCTION__);
                return UNKNOWN_ERROR;
            }
        }

        if (aBuffer->waitOnAcquireFence() != NO_ERROR) {
            LOGW("@%s, wait on fence for buffer %p timed out", __FUNCTION__, aBuffer.get());
        }

        index = mRealBufferIndex++;
        mRealBufferIndex = (mRealBufferIndex >= REAL_BUF_NUM) ? 0 : mRealBufferIndex;

        mDeviceBuffers.editItemAt(index).m.userptr = (unsigned long int)aBuffer->data();
        LOG2("@%s, buffer:%p, index:%d", __FUNCTION__, aBuffer->data(), index);
        const camera3_capture_request *req3 = request->getUserRequest();
        if (req3->settings ||
            mIsp->isIspPerframeSettingsEnabled())
            mDeviceBuffers.editItemAt(index).reserved2 =
                ATOMISP_BUFFER_HAS_PER_FRAME_SETTING | (request->getId()+1);
        else
            mDeviceBuffers.editItemAt(index).reserved2 = 0;

        aBuffer->setRequestId(request->getId());
        mBuffersInDevice.push_front(aBuffer); // Need to push like this to simulate FIFO behavior
        mHwSensor->setExpectedCaptureExpId(request->getId(), this);
        LOG2("###==%s: request_id=%d, output_frame = %p", __FUNCTION__, aBuffer->requestId(), aBuffer->data());
    }

    LOG2("@%s, mDeviceBuffers[%d]", __FUNCTION__, index);
    status = mVideoNode->putFrame(&mDeviceBuffers[index]);
    if (status != NO_ERROR) {
        LOGE("Failed to queue a preview buffer!");
        return UNKNOWN_ERROR;
    }

    // If we still have not got any frame out and is not started then start the device
    // Since Isp and timming of setting isp parameters limitation,
    // 6axis data is applied after three frames at least. So we
    // need to set once after stream on.
    if (!mVideoNode->isStarted()) {
        mRawLockEnabled = mIsp->isRawLockEnabled();
        status = mVideoNode->start(0);
        status = mIsp->start();
        if (mMode == MODE_VIDEO || MODE_CONTINUOUS_VIDEO)
            mIsp->initialDvsConfig();
        mStatus = START;
        PERFORMANCE_TRACES_BREAKDOWN_STEP("startPreviewDevice");
    }

    Message msg;
    msg.id = MESSAGE_ID_POLL;
    msg.request = request;
    mMessageQueue.send(&msg);

    return status;
}

status_t PreviewStream::query(FrameInfo *info)
{
    info->width = mConfig.width;
    info->height = mConfig.height;
    info->format = mConfig.format;
    info->originalFormat = mConfig.originalFormat;
    return true;
}

void * PreviewStream::queryBufferPointer(int requestId)
{
    Mutex::Autolock _l(mBufBookKeepingLock);
    LOG2("%s, name:%s, : requestId = %d", __FUNCTION__, getName(), requestId);
    for (unsigned int i = 0; i < mBuffersInDevice.size(); i++) {
        LOG2("%s: i=%d, stored requestId = %d", __FUNCTION__, i, mBuffersInDevice.itemAt(i)->requestId());
        if (requestId == mBuffersInDevice.itemAt(i)->requestId())
            return mBuffersInDevice.itemAt(i)->data();
    }
    return NULL;
}

void PreviewStream::DebugStreamFps(const char *streamName)
{
    mDebugFPS = new DebugFrameRate(streamName);
    mDebugFPS->run();
}

status_t PreviewStream::flushMessage()
{
    LOG1("@%s", __FUNCTION__);
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    mMessageQueue.remove(MESSAGE_ID_POLL);
    return mMessageQueue.send(&msg, MESSAGE_ID_FLUSH);
}

status_t PreviewStream::flush()
{
    stop();
    mBuffersInDevice.clear();
    return NO_ERROR;
}

status_t PreviewStream::stop()
{
    LOG1("@%s, name:%s", __FUNCTION__, getName());
    status_t status = NO_ERROR;

    mStatus = STOP;
    flushMessage();

    if (mVideoNode->isStarted()) {
        status = mVideoNode->stop();
        if (status != OK)
            LOGW("Failed to stream off the preview device");
    }
    mVideoNode->destroyBufferPool();

    //close the device
    if (mVideoNode->isOpen()) {
        mVideoNode->close();
    }

    mStreamMode = ONLINE;
    mFakeBuffers.clear();
    return NO_ERROR;
}

/**
 * Convert the device notification into stream notification
 */
void PreviewStream::notifyPolledDataToListeners(struct v4l2_buffer_info &v4l2Info,
                                                   Camera3Request* request)
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());

    ICssIspListener::IspMessage outMsg;
    outMsg.id = ICssIspListener::ISP_MESSAGE_ID_EVENT;
    outMsg.data.event.vbuf = v4l2Info.vbuffer;
    outMsg.data.event.timestamp = v4l2Info.vbuffer.timestamp;
    outMsg.data.event.exp_id = v4l2Info.vbuffer.reserved >> 16;
    outMsg.data.event.request_id = (request) ? \
                                    request->getId() : INVALID_REQ_ID;
/*
    if (mRawLockEnabled) {
        outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_RAW_LOCK;
        LOG2("RAW lock: %d/%d", outMsg.data.event.request_id, outMsg.data.event.exp_id);

        notifyListeners(&outMsg);
    }
*/
    sp<CameraBuffer> buff = NULL;
    // Don't pass buffer if it is a fake buffer without request
    if (request) {
        Mutex::Autolock _l(mBufBookKeepingLock);
        if (mBuffersInDevice.size() > 0) {
            // pick up the first buffer in the FIFO
            int lastIndex = mBuffersInDevice.size()-1;
            buff = mBuffersInDevice[lastIndex];
            mBuffersInDevice.removeAt(lastIndex);
        }
        LOG2("received buf [%d], exp_id %d, req_id %d", v4l2Info.vbuffer.index,
            outMsg.data.event.exp_id, outMsg.data.event.request_id);
        if (buff.get()) {
            buff->setTimeStamp(v4l2Info.vbuffer.timestamp);
            outMsg.data.event.request_id = buff->requestId();
        } else {
            LOG2("received fake buf [%d], exp_id %d", v4l2Info.vbuffer.index,
                outMsg.data.event.exp_id);
        }

    }

    outMsg.streamBuffer = buff;
    outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_FRAME;
    notifyListeners(&outMsg);

    if (buff.get() && mConsumer) {
        // dump source YUV image for preview
        if (!strcmp(getName(), PREVIEW_STREAM_NAME))
            buff->dumpImage(CAMERA_DUMP_PREVIEW, PREVIEW_STREAM_NAME);
        // dump source YUV image for video
        if (!strcmp(getName(), VIDEO_STREAM_NAME))
            buff->dumpImage(CAMERA_DUMP_VIDEO, VIDEO_STREAM_NAME);

        if (buff->isLocked()) {
            if (buff->unlock() != NO_ERROR) {
                LOGE("@%s: Could not unlock the buffer", __FUNCTION__);
            }
        }
        mConsumer->captureDone(buff, request);
    }
}

status_t PreviewStream::handleMessagePoll(Message &msg)
{
    int ret = 0;
    struct v4l2_buffer_info v4l2Info;
    CLEAR(v4l2Info);

    if (mStatus != START) {
        LOGW(" Poll when device not start ");
        return NO_ERROR;
    }

    int timeOutCount = CSS_PREVIEW_POLL_TIMEOUT_COUNT;
    LOG2("@%s preview before poll number buffer in device:%d", __FUNCTION__,
            mVideoNode->getBufsInDeviceCount());
    while (timeOutCount-- && ret == 0 && mStatus == START) {
        ret = mVideoNode->poll(CSS_PREVIEW_POLL_TIMEOUT);
    };

    if (ret > 0) {
       LOG2("@%s Entering dequeue : num-of-buffers queued %d", __FUNCTION__, mVideoNode->getBufsInDeviceCount());
       ret = mVideoNode->grabFrame(&v4l2Info);
       if (ret < 0) {
           LOGE("@%s failed to grab a  preview frame  (%d)", __FUNCTION__, ret);
       }
       if (mDebugFPS != NULL)
           mDebugFPS->update();
    } else if (mStatus == START) {
       LOGE("@%s v4l2_poll for preview device failed! (%s)", __FUNCTION__, (ret==0)?"timeout":"error");
    }

    // send fake buffer to ISP to keep preview run
    if (mVideoNode->getBufsInDeviceCount() <= 1 && mStreamMode == OFFLINE_CAPTURE)
        capture(NULL, NULL);
    // drop the fake buffer from ISP
    // but for dvs, we need to notify every frame even it is a fake buffer
    if (v4l2Info.vbuffer.index >= mFakeBufferIndexStart
        && v4l2Info.vbuffer.index < mFakeBufferIndexStart + mMaxNumOfSkipFrames) {
        notifyPolledDataToListeners(v4l2Info, NULL);
        return NO_ERROR;
    }

    notifyPolledDataToListeners(v4l2Info, msg.request);

    PERFORMANCE_TRACES_PREVIEW_SHOWN(v4l2Info.vbuffer.index);

    return NO_ERROR;
}

status_t PreviewStream::handleMessageFlush(Message &msg)
{
    UNUSED(msg);
    LOG1("@%s", __FUNCTION__);
    mMessageQueue.remove(MESSAGE_ID_POLL);
    return NO_ERROR;
}

void PreviewStream::messageThreadLoop(void)
{
    LOG2("@%s, name:%s, Start", __FUNCTION__, getName());

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);
        PERFORMANCE_HAL_ATRACE_PARAM1("msg", msg.id);

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
            break;

        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("    error %d in handling message: %d", status, (int)msg.id);
        mMessageQueue.reply(msg.id, status);
    }
    LOG2("%s: Exit", __FUNCTION__);
}

} /* namespace camera2 */
} /* namespace android */
