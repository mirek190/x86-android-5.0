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

#define LOG_TAG "Camera_CaptureStream"

#include "LogHelper.h"
#include "IPU2CameraHw.h"
#include "CaptureStream.h"
#include "CameraBuffer.h"
#include "PerformanceTraces.h"
#include "ipu2/IPU2HwSensor.h"
#ifdef GRAPHIC_IS_GEN
#include <ufo/graphics.h>
#endif

#define CSS_CAPTURE_POLL_TIMEOUT 500   // 500 ms
#define CSS_CAPTURE_POLL_TIMEOUT_COUNT 6
#define FLASH_TIMEOUT_FRAMES 5

namespace android {
namespace camera2 {

CaptureStream::CaptureStream(sp<V4L2VideoNode> &aCaptureDevice,
                                   sp<V4L2VideoNode> &aPostViewDevice,
                                   sp<IPU2HwIsp> &theIsp,
                                   IPU2HwSensor * theSensor,
                                   IFlashCallback *flashCallback,
                                   int cameraId,
                                   const char* streamName) :
    mCaptureDevice(aCaptureDevice)
    ,mPostviewDevice(aPostViewDevice)
    ,mIsp(theIsp)
    ,mHwSensor(theSensor)
    ,mDebugFPS(NULL)
    ,mCameraId(cameraId)
    ,mFakeBufferIndex(0)
    ,mFakeBufferIndexStart(0)
    ,mMaxNumOfSkipFrames(0)
    ,mFlashCallback(flashCallback)
    ,mMode(OFFLINE)
    ,mRawLockEnabled(false)
    ,mCaptureWithoutPreview(false)
    ,mLastReqId(INVALID_REQ_ID)
    ,mMessageQueue("CaptureStream", (int)MESSAGE_ID_MAX)
    ,mMessageThead(NULL)
    ,mThreadRunning(true)
    ,mRealBufferIndex(0)
{
    mStreamName.setTo(streamName ? streamName : "unnamed capture");
    LOG1("@%s, name:%s", __FUNCTION__, getName());

    mReceivedBuffers.clear();

    CLEAR(mPostviewConfig);
    mPostviewBuffers.clear();
    mV4l2PostviewBuffers.clear();

    CLEAR(mCaptureConfig);
    mV4l2CaptureBuffers.clear();

    mFakeBuffers.clear();
}

CaptureStream::~CaptureStream()
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

    freeBuffers();
    mReceivedBuffers.clear();
    mFakeBuffers.clear();
}

/**
 * init
 * Second stage constructor with allocations that can fail
 */
status_t CaptureStream::init()
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    status_t status = OK;
    mPostviewConfig.width = 320;
    mPostviewConfig.height = 240;
    mPostviewConfig.originalFormat = mPostviewConfig.format = V4L2_PIX_FMT_NV12;

    mMessageThead = new MessageThread(this, "CaptureStream");
    if (mMessageThead == NULL) {
        LOGE("Error create CaptureStream thread in init");
        return NO_INIT;
    }

    const IPU2CameraCapInfo * capInfo = getIPU2CameraCapInfo(mCameraId);
    if (capInfo)
        mMaxNumOfSkipFrames = capInfo->frameInitialSkip();
    mRawLockedInfo.clear();

    return status;
}

void CaptureStream::dump(int fd) const
{
    String8 message;

    message.appendFormat(LOG_TAG ":@%s name:\"%s\"\n", __FUNCTION__, getName());
    write(fd, message.string(), message.size());
}

void CaptureStream::setMode(CaptureMode mode)
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    mMode = mode;
    mRawLockEnabled = mIsp->isRawLockEnabled();
}

status_t CaptureStream::setFormat(FrameInfo *aConfig)
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    status_t status = NO_ERROR;
    // HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED and HAL_PIXEL_FORMAT_YCbCr_420_888
    // are mapped to NV12, and can be handled by hw streams directly.
#ifdef GRAPHIC_IS_GEN
    if (aConfig->format != HAL_PIXEL_FORMAT_YCbCr_422_I &&
            aConfig->format != HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL)
#else
    if (aConfig->format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED &&
            aConfig->format != HAL_PIXEL_FORMAT_YCbCr_420_888)
#endif
    {
        return BAD_VALUE;
    }

    LOG2("@%s, fmt:%d,w:%d,h:%d,stride:%d,size:%d, ori_format: %d", __FUNCTION__, aConfig->format, aConfig->width, aConfig->height, aConfig->stride, aConfig->size, aConfig->originalFormat);

    mConfig = *aConfig;
    mCaptureConfig = *aConfig;
    mCaptureConfig.originalFormat = mCaptureConfig.format = GFXFmt2V4l2Fmt(aConfig->format, mCameraId);
    mCaptureConfig.size = mCaptureConfig.width * mCaptureConfig.height * 3 / 2; // todo stride, format etc

    // Set the postview config
    // Get the first postview size that has the same ratio as the capture size
    int count = 0;
    const IPU2CameraCapInfo * capInfo = getIPU2CameraCapInfo(mCameraId);
    if (!capInfo)
        return UNKNOWN_ERROR;
    int32_t * sizes = (int32_t*)capInfo->getPostViewSizes(&count);
    float ratio = (float)mCaptureConfig.width / mCaptureConfig.height;
    for (int i = 0; i < count-1; i += 2) {
        if (fabs((float)sizes[i] / sizes[i+1] - ratio) < 0.001) {
            mPostviewConfig.width  = sizes[i];
            mPostviewConfig.height = sizes[i + 1];
            break;
        }
    }
    LOG2("capture size [%d, %d], postview size [%d, %d]",
                            mCaptureConfig.width, mCaptureConfig.height,
                            mPostviewConfig.width, mPostviewConfig.height);

    if (!mCaptureDevice->isOpen()) {
       status = mCaptureDevice->open();
       LOG1("@%s, line:%d, status:%d", __FUNCTION__, __LINE__, status);
    }

    status = mCaptureDevice->setFormat(mCaptureConfig);
    if (status != NO_ERROR) {
       LOGE("Failed to set capture format: status(%d)",status);
       return status;
    }
    PERFORMANCE_TRACES_BREAKDOWN_STEP("CaptureDevice");

    if (!mPostviewDevice->isOpen()) {
       status = mPostviewDevice->open();
       LOG1("@%s, line:%d, status:%d", __FUNCTION__, __LINE__, status);
    }
    if (status != NO_ERROR) {
       LOGE("Could not open the capture devices: status(%d)",status);
       return status;
    }

    status = mPostviewDevice->setFormat(mPostviewConfig);
    if (status != NO_ERROR) {
       LOGE("Failed to set postview format: status(%d)",status);
    }
    PERFORMANCE_TRACES_BREAKDOWN_STEP("PostviewDevice");
    return status;
}

status_t CaptureStream::configure(void)
{
    if (mCaptureDevice->isStarted()) {
        LOGE("%s: Device has been started, can't registerBuffers!", __FUNCTION__);
        return INVALID_OPERATION;
    }

    LOG2("@%s, name:%s", __FUNCTION__, getName());
    status_t status = NO_ERROR;

    Mutex::Autolock _l(mBufBookKeepingLock);

    LOG2("@%s, buffers number: real:%d, skip:%d", __FUNCTION__, REAL_BUF_NUM, mMaxNumOfSkipFrames);
    // create fake buffer for frame skipping
    mFakeBufferIndexStart = mFakeBufferIndex = REAL_BUF_NUM;
    mFakeBuffers.clear();
    uint32_t bufCount = REAL_BUF_NUM + mMaxNumOfSkipFrames;
    mRealBufferIndex = 0;

    // Allocate internal buffers
    status = allocateBuffers(bufCount);
    if (status != NO_ERROR) {
        return status;
    }

    status = mCaptureDevice->setBufferPool(mV4l2CaptureBuffers, false);
    status |= mPostviewDevice->setBufferPool(mV4l2PostviewBuffers, false);
    if (status != NO_ERROR) {
        LOGE("Error registering capture buffers ret=%d", status);
        return status;
    }

    PERFORMANCE_TRACES_BREAKDOWN_STEP("Capture");
    return status;
}

status_t CaptureStream::capture(sp<CameraBuffer> aBuffer,
                                   Camera3Request* request)
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    status_t status = OK;
    int index = 0;

    PERFORMANCE_TRACES_SHOT2SHOT_TAKE_PICTURE_HANDLE();
    // If the input params are null, means CameraHw requires to inject
    // fake buffer.
    if (aBuffer == NULL && request == NULL) {
        LOG2("inject a fake buffer, index:%d", mFakeBufferIndex);
        if (mFakeBufferIndex == 0 ||
            mFakeBufferIndex >= mFakeBufferIndexStart + mMaxNumOfSkipFrames) {
            return UNKNOWN_ERROR;
        }

        if (mV4l2CaptureBuffers[mFakeBufferIndex].m.userptr == 0) {
            LOG2("fake buffer is NULL, allocate heap buffer for it");
            sp<CameraBuffer> buf =
                MemoryUtils::allocateHeapBuffer(mConfig.width, mConfig.height,
                    widthToStride(GFXFmt2V4l2Fmt(mConfig.format, mCameraId), mConfig.width),
                    GFXFmt2V4l2Fmt(mConfig.format, mCameraId), mCameraId);
            if (buf.get()) {
                mFakeBuffers.push_front(buf);
                mV4l2CaptureBuffers.editItemAt(mFakeBufferIndex).m.userptr =
                                                (unsigned long int)buf->data();
            } else {
                LOGE("%s: no memory for fake buffer!", __FUNCTION__);
            }
        }
        mV4l2PostviewBuffers.editItemAt(mFakeBufferIndex).m.userptr =
                (unsigned long int)mPostviewBuffers[mFakeBufferIndex]->data();

        index = mFakeBufferIndex;
        mFakeBufferIndex++;
        if (mFakeBufferIndex == mFakeBufferIndexStart + mMaxNumOfSkipFrames) {
            mFakeBufferIndex = mFakeBufferIndexStart;
        }
        mHwSensor->setExpectedCaptureExpId(-1, this);
    } else if (aBuffer != NULL && request != NULL) {
        if (!aBuffer->isLocked())
            aBuffer->lock();

        index = mRealBufferIndex++;
        mRealBufferIndex = (mRealBufferIndex >= REAL_BUF_NUM) ? 0 : mRealBufferIndex;

        mV4l2CaptureBuffers.editItemAt(index).m.userptr =
            (unsigned long int)aBuffer->data();
        mV4l2PostviewBuffers.editItemAt(index).m.userptr =
            (unsigned long int)mPostviewBuffers[index]->data();
        const camera3_capture_request *req3 = request->getUserRequest();
        if (req3->settings ||
            mIsp->isIspPerframeSettingsEnabled()) {
            mV4l2CaptureBuffers.editItemAt(index).reserved2 =
                ATOMISP_BUFFER_HAS_PER_FRAME_SETTING | (request->getId()+1);
            mV4l2PostviewBuffers.editItemAt(index).reserved2 =
                ATOMISP_BUFFER_HAS_PER_FRAME_SETTING | (request->getId()+1);
        } else {
            mV4l2CaptureBuffers.editItemAt(index).reserved2 = 0;
            mV4l2PostviewBuffers.editItemAt(index).reserved2 = 0;
        }

        {
        Mutex::Autolock _l(mBufBookKeepingLock);
        aBuffer->setRequestId(request->getId());
        DeviceBuffer tmpBuffer = {aBuffer, index, \
            mCaptureWithoutPreview, (request->getNumberInputBufs()>0)};
        mReceivedBuffers.push_back(tmpBuffer); // Need to push like this to simulate FIFO behavior
        }

        mHwSensor->setExpectedCaptureExpId(request->getId(), this);
        // Capture stream need to:
        // 1. Which SOF can be used for shutter notification
        // 2. Which RAW buffer can be used to trigger capture
        //   (if it is not zsl capture)
        if (mMode == OFFLINE && mCaptureWithoutPreview)
            mHwSensor->findFrameForCapture(request->getId(), mRawLockEnabled);
        LOG2("###==%s: request_id=%d, output_frame = %p", __FUNCTION__,
                                        aBuffer->requestId(), aBuffer->data());
        mLastReqId = request->getId();
    } else {
        LOGW("%s: aBuffer or request is NULL", __FUNCTION__);
    }

    status = mCaptureDevice->putFrame(&mV4l2CaptureBuffers[index]);
    status |= mPostviewDevice->putFrame(&mV4l2PostviewBuffers[index]);
    if (status != NO_ERROR) {
        LOGE("Failed to queue a picture buffer!");
        return UNKNOWN_ERROR;
    }

    if (!mCaptureDevice->isStarted()) {
        if (mMode == OFFLINE) {
            status  = mIsp->requestContCapture(1, 1, 0);
        }

        mCaptureDevice->start(0);
        mPostviewDevice->start(0);
        status = mIsp->start();
        PERFORMANCE_TRACES_BREAKDOWN_STEP("startDevice");
    }

    if (mFlashCallback && mFlashCallback->isPreFlashUsed()) {
        mIsp->setFlash(1);
    }

    Message msg;
    msg.id = MESSAGE_ID_POLL;
    msg.request = request;
    mMessageQueue.send(&msg);

    return status;
}

status_t CaptureStream::reprocess(sp<CameraBuffer> anInputBuffer)
{
    UNUSED(anInputBuffer);
    LOG2("@%s, name:%s", __FUNCTION__, getName());

#if 0
    status_t status = OK;

    mBuffersInDevice.push_back(anInputBuffer);

    status  = mIsp->requestContCapture(1,-1,0);
    if (status != NO_ERROR) {
        LOGE("@%s: Could not trigger the capture- isp failed", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (!anInputBuffer->isLocked()) {
        status = anInputBuffer->lock();
        if (status != NO_ERROR) {
            LOGE("@%s: Could not lock the buffer", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
    }
    LOG1("queuing buffer %d ptr %lx",anInputBuffer->getV4L2Buff()->index, anInputBuffer->getV4L2Buff()->m.userptr);
    status = mCaptureDevice->putFrame(anInputBuffer->getV4L2Buff());
    if (status != NO_ERROR) {
        LOGE("Failed to queue a capture buffer!");
        return UNKNOWN_ERROR;
    }

    LOG1("queuing pv buffer %d ptr %lx length %d",mPostviewBuffer.vbuffer.index, mPostviewBuffer.vbuffer.m.userptr, mPostviewBuffer.vbuffer.length);
    status = mPostviewDevice->putFrame(&mPostviewBuffer.vbuffer);
    if (status != NO_ERROR) {
        LOGE("Failed to queue a postview buffer!");
        return UNKNOWN_ERROR;
    }

    mCaptureDevice->start(0);
    mPostviewDevice->start(0);

    status |= mDataPoller->poll(mDataPollingSubject, this);
#endif
    return OK;

}

status_t CaptureStream::allocateBuffers(int bufsNum)
{
    if (mPostviewBuffers.size()) {
        LOGW("%s: Already have buffers!", __FUNCTION__);
        return NO_ERROR;
    }

    // Fill up the empty v4L2 buffers
    for (int i = 0 ; i < bufsNum; i++) {
        struct v4l2_buffer v4l2Buf;
        CLEAR(v4l2Buf);
        mV4l2PostviewBuffers.push(v4l2Buf);
        mV4l2CaptureBuffers.push(v4l2Buf);
    }

    // Allocate internal buffers for postview
    for (int i = 0; i < bufsNum; i++) {
        sp<CameraBuffer> buf = MemoryUtils::allocateHeapBuffer(
                                      mPostviewConfig.width,
                                      mPostviewConfig.height,
                                      mPostviewConfig.stride,
                                      PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, mCameraId),
                                      mCameraId);
        if (buf.get()) {
            mPostviewBuffers.push_back(buf);
            mV4l2PostviewBuffers.editItemAt(i).m.userptr = (unsigned long int)buf->data();
        } else {
            LOGE("%s: no memory for postview!", __FUNCTION__);
            freeBuffers();
            return UNKNOWN_ERROR;
        }
    }
    return NO_ERROR;
}

status_t CaptureStream::freeBuffers()
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    if (!mPostviewBuffers.size()) {
        LOGD("%s: no buffers!", __FUNCTION__);
        return NO_ERROR;
    }

    mPostviewBuffers.clear();
    mV4l2CaptureBuffers.clear();
    mV4l2PostviewBuffers.clear();
    return NO_ERROR;
}

status_t CaptureStream::start()
{
    return NO_ERROR;
}

status_t CaptureStream::flushMessage(void)
{
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    mMessageQueue.remove(MESSAGE_ID_POLL);
    return mMessageQueue.send(&msg, MESSAGE_ID_FLUSH);
}

status_t CaptureStream::flush()
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());

    stop();

    mFakeBuffers.clear();
    mReceivedBuffers.clear();
    return NO_ERROR;
}

status_t CaptureStream::stop()
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    status_t status = NO_ERROR;

    flushMessage();

    // usually the devices are stopped already (see notifyPolledEvent) but just in case..
    if (mCaptureDevice->isStarted()) {
        status = mCaptureDevice->stop(false);
        if (status != OK)
            LOGW("@%s Failed to stream off the capture device", __FUNCTION__);
        else
            LOG1("@%s stopped capture device", __FUNCTION__);
    }
    if (mPostviewDevice->isStarted()) {
        status = mPostviewDevice->stop(false);
        if (status != OK)
            LOGW("@%s Failed to stream off the postview device", __FUNCTION__);
        else
            LOG1("@%s stopped postview device", __FUNCTION__);
    }

    // pools need to be destroyed, or to be exact, the VIDIOC_REQBUFS needs to be sent with 0. See V4L2 spec.
    mCaptureDevice->destroyBufferPool();
    mPostviewDevice->destroyBufferPool();
    mPostviewDevice->close(); // <- crucial. Reason unknown, but please verify still snapshot functionality if you attempt to remove this

    freeBuffers();

    mLastReqId = INVALID_REQ_ID;
    mRawLockEnabled = false;
    mCaptureWithoutPreview = false;
    return status;
}

status_t CaptureStream::triggerCaptureByLockedRaw(
                                                   int reqId, int expId)
{
    status_t status = INVALID_OPERATION;
    if (!mReceivedBuffers.size() || reqId > mLastReqId)
        return status;

    /*
     * Lock buffer with INVALID_REQ_ID because of capture without preview:
     */
    bool getExpected = false;
    if (reqId == INVALID_REQ_ID) {
        reqId = mHwSensor->getExpectedReqId(expId);
        getExpected = true;
    }
    if (reqId != INVALID_REQ_ID) {
        RawLockedInfo info = {reqId, expId};
        Mutex::Autolock _l(mBufBookKeepingLock);
        bool found = false;
        for (unsigned int i = 0; i< mReceivedBuffers.size(); i++) {
            if (mReceivedBuffers[i].buffer->requestId() == reqId) {
                // Should not use expected req id if this request has input
                found = !(mReceivedBuffers[i].hasInput && getExpected);
                break;
            }
        }
        if (!found)
            return status;

        status = NO_ERROR;
        LOG2("%s: Trigger %d/%d", __FUNCTION__, reqId, expId);
        if (mRawLockedInfo.size()) {
            mRawLockedInfo.push_back(info);
            return status;
        }
        if (mIsp->triggerCaptureByLockedRaw(expId) < 0) {
            LOG2("%s: error %d %s", __FUNCTION__, errno, strerror(errno));
            if (errno == EBUSY) {
                mRawLockedInfo.push_back(info);
                status = NO_ERROR;
            }
        }
    }
    return status;
}

bool CaptureStream::notifyIspEvent(ICssIspListener::IspMessage *msg)
{
    if (!mRawLockEnabled || !msg)
        return true;

    // Capture is triggered by:
    // 1. RAW lock event (zsl capture: repocess request)
    // 2. Frame event (normal capture)
    if (msg->data.event.type != ICssIspListener::ISP_EVENT_TYPE_RAW_LOCK
        && msg->data.event.type != ICssIspListener::ISP_EVENT_TYPE_FRAME)
        return true;

    LOG2("@%s, name:%s, capture %d/%d", __FUNCTION__, getName(), msg->data.event.request_id, msg->data.event.exp_id);

    int expId = msg->data.event.exp_id;
    int reqId = msg->data.event.request_id;
    if (expId != INVALID_EXP_ID) {
        // Unlock RAW buffer if it is not for capture
        if (NO_ERROR != triggerCaptureByLockedRaw(reqId, expId)) {
            mIsp->unlockRawBuffer(expId);
        }
    }
    return true;
}

status_t CaptureStream::query(FrameInfo *info)
{
    info->width = mConfig.width;
    info->height = mConfig.height;
    info->format = mConfig.format;
    info->originalFormat = mConfig.originalFormat;
    return true;
}

void * CaptureStream::queryBufferPointer(int requestId)
{
    Mutex::Autolock _l(mBufBookKeepingLock);
    LOG2("%s, name:%s, requestId = %d", __FUNCTION__, getName(), requestId);
    for (unsigned int i = 0; i < mReceivedBuffers.size(); i++) {
        if (requestId == mReceivedBuffers.itemAt(i).buffer->requestId())
            return mReceivedBuffers.itemAt(i).buffer->data();
    }
    return NULL;
}

void CaptureStream::DebugStreamFps(const char *streamName)
{
    mDebugFPS = new DebugFrameRate(streamName);
    mDebugFPS->run();
}

status_t CaptureStream::captureDone(sp<CameraBuffer> buffer)
{
    UNUSED(buffer);
    return true;
}

/**
 * Gets a snapshot/postview frame pair from ISP when using flash
 *
 * To ensure flash sync, the function fetches frames in a loop
 * until a properly exposed frame is available.
 */
int CaptureStream::getFlashExposedSnapshot(struct v4l2_buffer_info *captureV4l2Info,
                                                struct v4l2_buffer_info *postV4l2Info)
{
    LOG2("@%s:", __FUNCTION__);
    int ret = 0;
    int frameState = 0;
    for (int cnt = 0;;) {
        ret = mCaptureDevice->grabFrame(captureV4l2Info);
        ret |= mPostviewDevice->grabFrame(postV4l2Info);
        if (ret < 0) {
            LOGE("@%s failed to grab capture frame  (%d)", __FUNCTION__, ret);
            break;
        }

        frameState = (FrameBufferStatus)(captureV4l2Info->vbuffer.reserved & FRAME_STATUS_MASK);
        LOG2("%s: flash flashstate = %d", __FUNCTION__, frameState);

        // drop the fake buffer from ISP
        if (captureV4l2Info->vbuffer.index >= mFakeBufferIndexStart
            && captureV4l2Info->vbuffer.index < mFakeBufferIndexStart + mMaxNumOfSkipFrames) {
            LOG2("%s: flash fake flashstate = %d", __FUNCTION__, frameState);
            break;
        }

        if (frameState == FRAME_STATUS_FLASH_EXPOSED) {
            LOG2("%s: flash exposed, frame %d", __FUNCTION__, cnt);
            break;
        } else if (frameState == FRAME_STATUS_FLASH_FAILED) {
            LOGE("%s: flash fail, frame %d", __FUNCTION__, cnt);
            break;
        }

        if (cnt++ == FLASH_TIMEOUT_FRAMES) {
            LOGE("%s: unexpected flash timeout, frame %d", __FUNCTION__, cnt);
            break;
        }

        mCaptureDevice->putFrame(&(captureV4l2Info->vbuffer));
        mPostviewDevice->putFrame(&(postV4l2Info->vbuffer));
    }

    return ret;
}

/**
 * Convert the device notification into stream notification
 */
void CaptureStream::notifyPolledDataToListeners(struct v4l2_buffer_info &v4l2Info,
                                                   Camera3Request* request)
{
    LOG2("@%s, name:%s", __FUNCTION__, getName());
    ICssIspListener::IspMessage outMsg;
    sp<CameraBuffer> receivedBuf;
    int index;
    bool captureWithoutPreview;

    {
    Mutex::Autolock _l(mBufBookKeepingLock);
    // pick up the first buffer in the FIFO
    receivedBuf = mReceivedBuffers[0].buffer;
    index = mReceivedBuffers[0].bufferIndex;
    captureWithoutPreview = mReceivedBuffers[0].captureWithoutPreview;
    mReceivedBuffers.removeAt(0);

    if (mRawLockedInfo.size()) {
        RawLockedInfo info = mRawLockedInfo[0];
        if (mIsp->triggerCaptureByLockedRaw(info.expId) == 0)
            mRawLockedInfo.removeAt(0);
    }
    }

    //assert((uint32_t)index == v4l2Info.vbuffer.index);
    LOG2("received buffer [%d], should be %d", v4l2Info.vbuffer.index, index);

    outMsg.id = ICssIspListener::ISP_MESSAGE_ID_EVENT;
    outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_FRAME;
    outMsg.data.event.vbuf = v4l2Info.vbuffer;
    outMsg.data.event.timestamp = v4l2Info.vbuffer.timestamp;
    outMsg.data.event.exp_id = v4l2Info.vbuffer.reserved >> 16;
    outMsg.data.event.request_id = receivedBuf->requestId();
    receivedBuf->setTimeStamp(v4l2Info.vbuffer.timestamp);
    outMsg.streamBuffer = receivedBuf;

    if (request) {
        /*
         * TODO:
         * For reprocess request(), the epxId of output frame is decided
         * by app (RAW lock/unlock enabled) or ISP (RAW lock/unlock disabled),
         * It might not match with estimated exp id.
         *
         * Need mark this case.
         */
        if (request->getNumberInputBufs() > 0) {
            LOGW("Wrong expId %d for reprocess req %d",
                outMsg.data.event.exp_id, request->getId());
        }
    }

     if (captureWithoutPreview) {
         notifyListeners(&outMsg);
    }
    // Unlock RAW buffer
    if (mRawLockEnabled) {
        mIsp->unlockRawBuffer(outMsg.data.event.exp_id);
    }

    if (mConsumer) {
        // dump source YUV image for snapshot
        receivedBuf->dumpImage(CAMERA_DUMP_SNAPSHOT, CAPTURE_STREAM_NAME);

        if (receivedBuf->isLocked()) {
            if (receivedBuf->unlock() != NO_ERROR) {
                LOGE("@%s: Could not unlock the buffer", __FUNCTION__);
            }
        }
        mConsumer->captureDone(receivedBuf, request);
    }
}

// Used for debug only. Sampling dump a frame to check if it is a black frame.
// TODO: use property, like camera.hal.dump, to toggle this debug function on/off.
void CaptureStream::dumpFrame(struct v4l2_buffer_info* buf)
{
    struct v4l2_buffer *v4l2_buf = &buf->vbuffer;
    LOG2("@%s: userptr: %p, length: %d, reserved: %d.", __FUNCTION__, (void*)(v4l2_buf->m.userptr), v4l2_buf->length, v4l2_buf->reserved);

    if (v4l2_buf->length < 10020){
        LOG2("@%s: too short frame, no sampling dump.", __FUNCTION__);
        return;
    }

    unsigned int* ptr = (unsigned int*)(v4l2_buf->m.userptr);

    for (int i = 10000; i < 10020; i++){
        LOG2("@%s: 0x%x", __FUNCTION__, *(ptr + i));
    }
}

status_t CaptureStream::handleMessagePoll(Message &msg)
{
    int ret = 0;
    struct v4l2_buffer_info captureV4l2Info;
    struct v4l2_buffer_info postV4l2Info;
    CLEAR(captureV4l2Info);
    CLEAR(postV4l2Info);

    int timeOutCount = CSS_CAPTURE_POLL_TIMEOUT_COUNT;
    LOG2("@%s capture before poll number buffer in device:%d", __FUNCTION__,
            mCaptureDevice->getBufsInDeviceCount());
    while (timeOutCount-- && ret == 0) {
         ret = mCaptureDevice->poll(CSS_CAPTURE_POLL_TIMEOUT);
    }

    if (ret > 0) {
        LOG2("@%s Entering dequeue : num-of-buffers queued %d",
             __FUNCTION__, mCaptureDevice->getBufsInDeviceCount());
             if (mFlashCallback && mFlashCallback->isPreFlashUsed()) {
                 ret = getFlashExposedSnapshot(&captureV4l2Info, &postV4l2Info);
             } else {
                 ret = mCaptureDevice->grabFrame(&captureV4l2Info);
                 ret |= mPostviewDevice->grabFrame(&postV4l2Info);
             }
        if (ret < 0) {
            LOGE("@%s failed to grab capture frame  (%d)", __FUNCTION__, ret);
        }
        if (mDebugFPS != NULL)
            mDebugFPS->update();
        PERFORMANCE_TRACES_BREAKDOWN_STEP_PARAM("grapFrame", captureV4l2Info.vbuffer.index);
    } else if (ret < 0) {
        LOGE("@%s v4l2_poll for capture device error! %d", __FUNCTION__, ret);
    } else {
        LOGE("@%s v4l2_poll for capture device timeout!", __FUNCTION__);
    }

    // drop the fake buffer from ISP
    if (captureV4l2Info.vbuffer.index >= mFakeBufferIndexStart
        && captureV4l2Info.vbuffer.index < mFakeBufferIndexStart + mMaxNumOfSkipFrames) {
        return NO_ERROR;
    }

    if (mFlashCallback && mFlashCallback->isPreFlashUsed()) {
        mFlashCallback->exitPreFlashSequence();
        mIsp->setFlash(0);
    }

    notifyPolledDataToListeners(captureV4l2Info, msg.request);

    if (mMode == OFFLINE) {
        Mutex::Autolock _l(mBufBookKeepingLock);
        if (!mReceivedBuffers.size()) {
            mCaptureDevice->stop(true);
            mPostviewDevice->stop(true);
            mLastReqId = INVALID_REQ_ID;
            PERFORMANCE_TRACES_BREAKDOWN_STEP("stopDevice");
        }
    }

    return NO_ERROR;
}

status_t CaptureStream::handleMessageFlush(Message &msg)
{
    status_t status = NO_ERROR;
    UNUSED(msg);
    mMessageQueue.reply(MESSAGE_ID_FLUSH, status);
    return status;
}


void CaptureStream::messageThreadLoop(void)
{
    LOG2("@%s, name:%s, Start", __FUNCTION__, getName());

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


} /* namespace camera2 */
} /* namespace android */
