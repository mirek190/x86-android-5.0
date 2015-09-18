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
#define LOG_TAG "Camera_PostProcessStream"

#include <utils/Errors.h>
#include <linux/videodev2.h>
#include "ICameraHwControls.h"

#include "LogHelper.h"
#include "CameraBuffer.h"
#ifdef GRAPHIC_IS_GEN
#include <ufo/graphics.h>
#endif

#include "PostProcessStream.h"
#include "ColorConverter.h"
#include "ImageScaler.h"
#include "PerformanceTraces.h"

namespace android {
namespace camera2 {

#define OFFSET_OF_BUF_INDEX 0
#define OFFSET_OF_EXP_ID 1
#define OFFSET_OF_REQUEST_ID 2

PostProcessStream::PostProcessStream(sp<ImgEncoder> imgEncoder
                                           , JpegMaker *jpegMaker
                                           , I3AControls *i3AControls
                                           , IPU2HwIsp * hwIsp
                                           , IPU2HwSensor * hwSensor
                                           , int cameraid)
    :mHw(NULL)
    ,mMode(POSTPROCESSMODE_ZSL)
    ,mRawLockEnabled(false)
    ,mInternalBufferIndex(0)
    ,mBuffersNumInDevice(0)
    ,mMessageQueue("PostProcessStream", (int)MESSAGE_ID_MAX)
    ,mMessageThead(NULL)
    ,mThreadRunning(true)
    ,mImgEncoder(imgEncoder)
    ,mJpegMaker(jpegMaker)
    ,m3AControls(i3AControls)
    ,mIsp(hwIsp)
    ,mSensor(hwSensor)
    ,mCameraId(cameraid)
{

#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
    miVPCtxValid = false;
#endif
    mReceivedBuffers.clear();
    mReceivedRequests.clear();
    mRawLockInfo.clear();

    mInternalBuffers.clear();
    mScaledBuf.clear();
    init();
    mReprocessInfo.exp_id = INVALID_EXP_ID;
    mReprocessInfo.request_id = INVALID_REQ_ID;

    mProducer = NULL;
    mConsumer = NULL;

#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
    // Width and height are not important for us, hence the 1, 1
    if (iVP_create_context(&miVPCtx, 1, 1, 0) == IVP_STATUS_SUCCESS) {
        miVPCtxValid = true;
    } else {
        ALOGE("Failed to create iVP context");
        miVPCtxValid = false;
    }
#endif
}

/**
 * init
 * Second stage constructor with creation of the objects that poll for data
 * and events
 */
status_t PostProcessStream::init()
{
    LOG2("@%s", __FUNCTION__);
    mMessageThead = new MessageThread(this, "PostProcessStream");
    if (mMessageThead == NULL) {
        LOGE("Error create StreamThread in init");
        return NO_INIT;
    }
    // get information for jpeg exif
    status_t status;
    // get ISP makernote
    CLEAR(ispMakerNote);
    status = mIsp->getMakerNote(&ispMakerNote);
    if (status != NO_ERROR)
        LOGW("Could not get ISP makernote information!");

    CameraMetadata staticMeta;
    staticMeta = PlatformData::getStaticMetadata(mCameraId);
    int count = 0;
    float* lensAperture = (float*)MetadataHelper::getMetadataValues(staticMeta,
                                             ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                                             TYPE_FLOAT,
                                             &count);
    if (lensAperture != NULL && count >= 1) {
        uint32_t den = 10;
        uint32_t num = (uint32_t)(*lensAperture * den);
        // reset f_number_curr by the aperture from Metadata
        ispMakerNote.f_number_curr = (num << 16) + (den & 0xffff);
    }

    return NO_ERROR;
}

PostProcessStream::~PostProcessStream()
{
    LOG2("@%s%d", __FUNCTION__, mMode);

    if (mMessageThead != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }
    detachListener();

    mReceivedBuffers.clear();
    mReceivedRequests.clear();
    mRawLockInfo.clear();
    freeBuffers();
    mVideoNode.clear();

#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
    if (miVPCtxValid)
        iVP_destroy_context(&miVPCtx);
#endif

}

void PostProcessStream::dump(int fd) const
{
    String8 message;

    message.appendFormat(LOG_TAG ":@%s name:\"%s\"\n", __FUNCTION__, getName());
    write(fd, message.string(), message.size());
}

void PostProcessStream::setMode(int mode)
{
    LOG2("@%s, mode:%d", __FUNCTION__, mode);
    mMode = mode;

    if (mMode == POSTPROCESSMODE_JPEG_ENCODING && !mImgEncoder.get())
        LOGE("%s: JPEG postProcess stream has no Jpeg encoder!", __FUNCTION__);

    if (mMode == POSTPROCESSMODE_ZSL)
        mRawLockEnabled = mIsp->isRawLockEnabled();
}

status_t PostProcessStream::setFormat(FrameInfo *aConfig)
{
    LOG2("@%s, mode:%d", __FUNCTION__, mMode);
    status_t status = NO_ERROR;

    mConfig = *aConfig;
    return status;
}

/**
 * Configure producer stream node.
 * Register allocated buffers to producer.
 */
status_t PostProcessStream::configure(void)
{
    LOG2("@%s, mode:%d", __FUNCTION__, mMode);
    if (!mProducer) {
        LOGD("%s: mProducer of %s is NULL", __FUNCTION__, getName());
        return NO_ERROR;
    }
    return allocateBuffers(REAL_BUF_NUM);
}

/**
 * Send a buffer for capture.
 * Should be called by consumer.
 */
status_t PostProcessStream::capture(sp<CameraBuffer> aBuffer,
                                       Camera3Request* request)
{
    LOG2("@%s, mode:%d", __FUNCTION__, mMode);
    status_t status = NO_ERROR;
    Mutex::Autolock _l(mBufBookKeepingLock);
    uint32_t i = 0;

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

    if(request->isInputBuffer(aBuffer)) {
        status = reprocess(aBuffer, request);
    } else {
        if (mRawLockEnabled) {
            int * data = (int *)aBuffer->data();
            *(data + OFFSET_OF_BUF_INDEX) = i;
            if (mRawLockInfo[i] != INVALID_EXP_ID) {
                mIsp->unlockRawBuffer(mRawLockInfo[i]);
                mRawLockInfo.editItemAt(i) = INVALID_EXP_ID;
            }
        }

        if (mProducer) {
            mProducer->capture(getOneInternalBuffers(), request);
        }
    }

    mReceivedLock.lock();
    mReceivedBuffers.push_back(aBuffer);
    mReceivedRequests.push_back(request);
    mReceivedLock.unlock();

    return status;
}

/**
 * Notify consumer the capture finished.
 * Should be called by consumer.

 * \param buffer the filled buffer of consumer.
 */
status_t PostProcessStream::notifyCaptureDone(sp<CameraBuffer> buffer,
                                                 Camera3Request* request)
{
    status_t status = NO_ERROR;
    if (buffer->isLocked()) {
        if (buffer->unlock() != NO_ERROR) {
            LOGE("@%s: Could not unlock the buffer", __FUNCTION__);
            status = UNKNOWN_ERROR;
        }
    }
    mConsumer->captureDone(buffer, request);
    return status;
}

/**
 * Process consumer buffer and return
 * It is workaroud for ZSL_CAPTURE:
 *    Fill exp_id in zsl stream buffer
 */
status_t PostProcessStream::captureDoneForZsl(
                                           ICssIspListener::IspMessage *msg,
                                           Camera3Request* request)
{
    sp<CameraBuffer> receivedBuffer = NULL;

    mReceivedLock.lock();
    if (mReceivedBuffers.size() > 0) {
        receivedBuffer = mReceivedBuffers.editItemAt(0);
        mReceivedBuffers.removeAt(0);
    }
    mReceivedLock.unlock();

    if (receivedBuffer == NULL) {
        LOGE("@%s: receivedBuffer is NULL", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (!request->isInputBuffer(receivedBuffer)) {
        receivedBuffer->setTimeStamp(msg->data.event.timestamp);
        if (mRawLockEnabled) {
           if (mIsp->isRawLockReadyForUse()) {
                // Fill exp_id for zsl stream buffer:
                int * data = (int*)receivedBuffer->data();
                *(data + OFFSET_OF_EXP_ID) = msg->data.event.exp_id;
                *(data + OFFSET_OF_REQUEST_ID) = msg->data.event.request_id;

                LOG2("@%s mode:%d keeps exp id:%d lock in group index %d", __FUNCTION__,  mMode,
                        msg->data.event.exp_id, *(data+OFFSET_OF_BUF_INDEX));
                mRawLockInfo.editItemAt(*(data+OFFSET_OF_BUF_INDEX)) = \
                    msg->data.event.exp_id;
            } else {
                mIsp->unlockRawBuffer(msg->data.event.exp_id);
            }
        }
    }

    return notifyCaptureDone(receivedBuffer, request);
}

/**
 * Process consumer buffer and return
 */
status_t PostProcessStream::captureDone(sp<CameraBuffer> buffer,
                                           Camera3Request* request)
{
    LOG2("@%s, mode:%d", __FUNCTION__, mMode);

    sp<CameraBuffer> receivedBuffer = NULL;

    mReceivedLock.lock();
    if (mReceivedBuffers.size() > 0) {
        receivedBuffer = mReceivedBuffers.editItemAt(0);
        mReceivedBuffers.removeAt(0);
        releaseOneInternalBuffers();
    }
    mReceivedLock.unlock();

    if (receivedBuffer == NULL) {
        LOGE("@%s: receivedBuffer is NULL", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    receivedBuffer->setTimeStamp(buffer->timeStamp());

    if (mMode == POSTPROCESSMODE_NONE)
        return notifyCaptureDone(receivedBuffer, request);

    if (mMode == POSTPROCESSMODE_COPY) {
        memcpy(buffer->data(), receivedBuffer->data(), buffer->size());
        return notifyCaptureDone(receivedBuffer, request);
    }
    if (mMode == POSTPROCESSMODE_DOWNSCALING_AND_COLOER_CONVERSION) {
        return handleDownscalingColorConversion(buffer, receivedBuffer,
                                                request);
    }

    if (mMode == POSTPROCESSMODE_JPEG_ENCODING
        && receivedBuffer->format() == HAL_PIXEL_FORMAT_BLOB) {
        if (!receivedBuffer->isLocked()) {
            if (receivedBuffer->lock() != NO_ERROR) {
                LOGE("@%s: Could not lock the buffer", __FUNCTION__);
            }
        }
        LOG1("begin jpeg encoder");

        ImgEncoder::EncodePackage package;
        CLEAR(package);
        package.jpegOut = receivedBuffer;
        package.main = buffer;
        package.thumb = NULL;

        package.settings = &(ReadRequest(request).mMembers.mSettings);
        if (package.settings == NULL) {
            LOGE("No settings in in request");
            return UNKNOWN_ERROR;
        }

        Message msg;
        msg.id = MESSAGE_ID_ENCODE;
        msg.package = package;
        msg.request = request;
        return mMessageQueue.send(&msg);
    }

    LOGE("@%s: BAD mode %d!", __FUNCTION__, mMode);
    return UNKNOWN_ERROR;
}

status_t PostProcessStream::reprocess(sp<CameraBuffer> inBuf,
                             Camera3Request* request)
{
    if (mMode != POSTPROCESSMODE_ZSL || !mRawLockEnabled) {
        LOGW("%s: poststream %d does not support it now! %d", __FUNCTION__,
            mMode, mRawLockEnabled);
        return INVALID_OPERATION;
    }

    // Do not trigger capture immediately because the previous preview requests
    // are not finished. Please see workaround in checkRawLockInfo()
    Mutex::Autolock _l(mReceivedLock);
    if (inBuf.get() && request) {
        int * data = (int*)inBuf->data();
        int buf_index = *(data + OFFSET_OF_BUF_INDEX);
        int exp_id = *(data + OFFSET_OF_EXP_ID);
        if (exp_id != INVALID_EXP_ID) {
            mReprocessInfo.exp_id = exp_id;
            mReprocessInfo.request_id = request->getId();
            LOG2("@%s: <Request %d> exp_id %d", __FUNCTION__,
                request->getId(), exp_id);
            // Erase exp_id because captureHwStream will unlock it
            mRawLockInfo.editItemAt(buf_index) = INVALID_EXP_ID;

            // why 2? please refer to the comments in checkRawLockInfo
            if (mReceivedBuffers.size() < 2) {
                ICssIspListener::IspMessage outMsg;
                outMsg.id = ICssIspListener::ISP_MESSAGE_ID_EVENT;
                outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_RAW_LOCK;
                outMsg.data.event.exp_id = mReprocessInfo.exp_id;
                outMsg.data.event.request_id = mReprocessInfo.request_id;
                notifyListeners(&outMsg);

                mReprocessInfo.exp_id = INVALID_EXP_ID;
                mReprocessInfo.request_id = INVALID_REQ_ID;
            }
        } else {
            LOGE("%s: can't get reprocess info!", __FUNCTION__);
        }
    }

    return NO_ERROR;
}

bool PostProcessStream::notifyIspEvent(ICssIspListener::IspMessage *msg)
{
    LOG2("@%s, mode:%d event type:%d", __FUNCTION__, mMode, msg->data.event.type);

    // Only handle frame event
    if (!msg || msg->data.event.type != ICssIspListener::ISP_EVENT_TYPE_FRAME)
        return true;

    // Trigger capture with saved reprocess input frame
    if (mRawLockEnabled && mMode == POSTPROCESSMODE_ZSL)
        checkRawLockInfo(msg);

    // Process consumer buffers
    if (mProducer) {
        LOGE("%s: %s has own producer, should not receive frame event!",
            __FUNCTION__, getName());
        return false;
    }

    /*
     * Return frame to producer if req id is matched
     * For ZSL stream (RAW lock/unlock is enabled): should pass down event
     *  if req id is not matched (To support normal capture, please refer to:
     *    IPU2CameraHw::bindStreamsForContinuous() )
     */
    int reqId = INVALID_REQ_ID;
    {
        Mutex::Autolock _l(mReceivedLock);
        if (mReceivedRequests.size()) {
            reqId = mReceivedRequests[0]->getId();
        }
    }
    if (reqId != INVALID_REQ_ID && msg->data.event.request_id == reqId) {
        mReceivedLock.lock();
        Camera3Request * request= mReceivedRequests.editItemAt(0);
        mReceivedRequests.removeAt(0);
        mReceivedLock.unlock();
        if (mMode == POSTPROCESSMODE_ZSL) {
            captureDoneForZsl(msg, request);
        } else {
            captureDone(msg->streamBuffer, request);
        }
    } else if (mRawLockEnabled && mMode == POSTPROCESSMODE_ZSL) {
        notifyListeners(msg);
    }

    return true;
}

bool PostProcessStream::checkRawLockInfo(ICssIspListener::IspMessage *msg)
{
    if (mReprocessInfo.request_id == INVALID_REQ_ID)
        return true;

    /*
     *Find the appropiate time to trigger capture for reprocess:
     * ZSL stream should postpone reprocess request processing to make sure
     * reprocess request is returned after previous preview requests

     * Try to trigger reprocess capture when: (TBD)
     * 1) one preview request is in-flight, for small capture size (<=1080P).
     * 2) two preview requests are in-flight, for big capture size (>1080P).
     */
    int triggerAhead = 2; //(mConfig.width * mConfig.height > 1920 * 1080) ? 2 : 1;
    Mutex::Autolock _l(mReceivedLock);
    if (msg->data.event.request_id == INVALID_REQ_ID ||
        mReprocessInfo.request_id <= msg->data.event.request_id + triggerAhead) {
        LOG2("@%s: trigger zsl capture: %d/%d by (%d/%d)", __FUNCTION__,
            mReprocessInfo.request_id, mReprocessInfo.exp_id,
            msg->data.event.request_id, msg->data.event.exp_id);

        ICssIspListener::IspMessage outMsg;
        outMsg.id = ICssIspListener::ISP_MESSAGE_ID_EVENT;
        outMsg.data.event.type = ICssIspListener::ISP_EVENT_TYPE_RAW_LOCK;
        outMsg.data.event.exp_id = mReprocessInfo.exp_id;
        outMsg.data.event.request_id = mReprocessInfo.request_id;
        notifyListeners(&outMsg);

        mReprocessInfo.exp_id = INVALID_EXP_ID;
        mReprocessInfo.request_id = INVALID_REQ_ID;
    }
    return true;
}

status_t PostProcessStream::flush()
{
    LOG2("@%s, mode:%d", __FUNCTION__, mMode);
    status_t status = NO_ERROR;

    stop();

    mReceivedLock.lock();
    mReceivedBuffers.clear();
    mReceivedRequests.clear();
    mReceivedLock.unlock();
    return status;
}

status_t PostProcessStream::stop()
{
    LOG2("@%s, mode:%d", __FUNCTION__, mMode);
    status_t status = NO_ERROR;
    detachListener();
    freeBuffers();
    mReprocessInfo.exp_id = INVALID_EXP_ID;
    mReprocessInfo.request_id = INVALID_REQ_ID;

    mRawLockEnabled = false;
    mRawLockInfo.clear();

    return status;
}

void PostProcessStream::messageThreadLoop(void)
{
    LOG2("@%s: Start", __FUNCTION__);

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

        case MESSAGE_ID_ENCODE:
            status = handleJpegEncode(msg);
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

status_t PostProcessStream::handleJpegEncode(Message &msg)
{
    LOG2("%s", __FUNCTION__);
    status_t status = NO_ERROR;

    sp<ExifMetaData> metadata = new ExifMetaData;
    // get metadata info from platform and 3A control
    // TODO figure out how to get accurate metadata from HW and 3a. Keeping
    // the current implementation for now
    int requestId = msg.request->getId();
    unsigned int exp_id = mSensor->getExpIdForRequest(requestId);
    LOG2("%s: exp_id = %d", __FUNCTION__, exp_id);
    fillPicMetaDataFromHw(metadata, exp_id);
    fillPicMetaDataFrom3A(metadata, requestId);
    calcuAecApexTv(metadata, msg.package);

    status = mJpegMaker->setupExifWithMetaData(msg.package, metadata);
    // Do sw or HW encoding. Also create Thumb buffer if needed
    status = mImgEncoder->encodeSync(msg.package, metadata);
    if (msg.package.thumbOut == NULL) {
        LOGE("%s: No thumb in EXIF", __FUNCTION__);
    }
    // Create a full JPEG image with exif metadata
    status = mJpegMaker->makeJpeg(msg.package, msg.package.jpegOut);
    if (status != NO_ERROR) {
        LOGE("%s: Make Jpeg Failed !", __FUNCTION__);
    }

    // dump JPEG data to file system
    msg.package.jpegOut->dumpImage(CAMERA_DUMP_JPEG, "jpeg");

    notifyCaptureDone(msg.package.jpegOut, msg.request);
    metadata.clear();
    PERFORMANCE_TRACES_BREAKDOWN_STEP("Done");
    return status;
}
#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
// The iVP layer must be zeroed and contain valid rects!
status_t PostProcessStream::cameraBuffer2iVPLayer(sp<CameraBuffer> cameraBuffer,
        iVP_layer_t *iVPLayer)
{
    iVPLayer->srcRect->left = iVPLayer->destRect->left = 0;
    iVPLayer->srcRect->top = iVPLayer->destRect->top = 0;
    iVPLayer->srcRect->width = iVPLayer->destRect->width = cameraBuffer->width();
    iVPLayer->srcRect->height = iVPLayer->destRect->height = cameraBuffer->height();
    iVPLayer->bufferType = IVP_GRALLOC_HANDLE;
    buffer_handle_t *gralloc_handle = cameraBuffer->getBufferHandle();
    if (gralloc_handle == NULL) {
        LOGE("Trying to send non-gralloc buffer to iVP, that does not work, aborting color conversion");
        return INVALID_OPERATION;
    }
    iVPLayer->gralloc_handle = *gralloc_handle;

    return NO_ERROR;
}

status_t PostProcessStream::iVPColorConversion(sp<CameraBuffer> srcBuf,
        sp<CameraBuffer> dstBuf)
{
    if (!miVPCtxValid)
        return UNKNOWN_ERROR;

    iVP_rect_t srcSrcRect, srcDstRect, dstSrcRect, dstDstRect;
    iVP_layer_t src, dst;
    CLEAR(src);
    CLEAR(dst);
    src.srcRect = &srcSrcRect;
    src.destRect = &srcDstRect;
    dst.srcRect = &dstSrcRect;
    dst.destRect = &dstDstRect;
    status_t status = cameraBuffer2iVPLayer(srcBuf, &src);
    if (status != NO_ERROR)
        return status;
    status = cameraBuffer2iVPLayer(dstBuf, &dst);
    if (status != NO_ERROR)
        return status;
    // Src dst rect is the operations dst rect
    srcDstRect = dstDstRect;

    iVP_status iVPstatus = iVP_exec(&miVPCtx, &src, NULL, 0, &dst, true);
    if (iVPstatus != IVP_STATUS_SUCCESS)
        return UNKNOWN_ERROR;

    return NO_ERROR;
}
#endif

status_t PostProcessStream::handleDownscalingColorConversion(
                                sp<CameraBuffer> srcBuf,
                                sp<CameraBuffer> destBuf,
                                Camera3Request* request)
{
    LOG2("%s: srcBuf->format()=%d, destBuf->format()=%d", __FUNCTION__, srcBuf->format(), destBuf->format());
#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
    /* need to convert from YUYV ot NV12T/NV12_Linear format for video stream on BYT, NV12_Linear
     * is just for preview callback.
     */
    if (((destBuf->getOwner())->format() == HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL)
        ||((destBuf->getOwner())->format() == HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL)) {

        // Downscale before color conversion
        sp<CameraBuffer> scaledSrcBuf = srcBuf;
        if ((scaledSrcBuf->width() != destBuf->width())
            || (scaledSrcBuf->height() != destBuf->height())) {
            LOG2("%s: downscaling for color converstion %dx%d -> %dx%d", __FUNCTION__,
                srcBuf->width(), srcBuf->height(),
                destBuf->width(), destBuf->height());
            if (!mScaledBuf.get()) {
                mScaledBuf = MemoryUtils::allocateHeapBuffer(
                                              destBuf->width(),
                                              destBuf->height(),
                                              widthToStride(GFXFmt2V4l2Fmt(srcBuf->format(), mCameraId), destBuf->width()),
                                              GFXFmt2V4l2Fmt(srcBuf->format(), mCameraId), mCameraId);
            }
            ImageScaler::downScaleImage(srcBuf.get(), mScaledBuf.get(), 0, 0);
            scaledSrcBuf = mScaledBuf;
        }
        if (iVPColorConversion(scaledSrcBuf, destBuf) != NO_ERROR) {
            LOGE("%s: not implement for color conversion 0x%x -> 0x%x!",
                    __FUNCTION__, scaledSrcBuf->format(), destBuf->format());
        }
        return notifyCaptureDone(destBuf, request);
    }
#endif

    // Only need downscaling
    if (GFXFmt2V4l2Fmt(srcBuf->format(), mCameraId) == GFXFmt2V4l2Fmt(destBuf->format(), mCameraId)) {
        if(srcBuf->width() != destBuf->width()
           || srcBuf->height() != destBuf->height()) {
           LOG2("%s: downscaling %dx%d -> %dx%d", __FUNCTION__,
               srcBuf->width(), srcBuf->height(),
               destBuf->width(), destBuf->height());
           ImageScaler::downScaleImage(srcBuf.get(), destBuf.get(), 0, 0);
        } else {
           LOG2("only need copy for the same resolution");
           memcpy(destBuf->data(), srcBuf->data(), srcBuf->size());
        }
        return notifyCaptureDone(destBuf, request);
    }

    // Downscale before color conversion
    sp<CameraBuffer> scaledSrcBuf = srcBuf;
    if ((scaledSrcBuf->width() != destBuf->width())
        || (scaledSrcBuf->height() != destBuf->height())) {
        LOG2("%s: downscaling for color converstion %dx%d -> %dx%d", __FUNCTION__,
            srcBuf->width(), srcBuf->height(),
            destBuf->width(), destBuf->height());
        ImageScaler::downScaleImage(srcBuf.get(), mScaledBuf.get(), 0, 0);
        scaledSrcBuf = mScaledBuf;
    }

    // Do color conversion
    int srcV4l2Fmt = GFXFmt2V4l2Fmt(scaledSrcBuf->format(), mCameraId);
    //int destV4l2Fmt = GFXFmt2V4l2Fmt(destBuf->format());
    LOG2("%s: Color converstion 0x%x -> 0x%x", __FUNCTION__,
        scaledSrcBuf->format(), destBuf->format());
    switch (destBuf->format()) {
    case HAL_PIXEL_FORMAT_YV12:
        // XXX -> YV12
        convertBuftoYV12(srcV4l2Fmt, scaledSrcBuf->width(),
                         scaledSrcBuf->height(), scaledSrcBuf->stride(),
                         destBuf->stride(), scaledSrcBuf->data(), destBuf->data());
        break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        // XXX -> NV21
        convertBuftoNV21(srcV4l2Fmt, scaledSrcBuf->width(),
                         scaledSrcBuf->height(), scaledSrcBuf->stride(),
                         destBuf->stride(), scaledSrcBuf->data(), destBuf->data());
        break;
    default:
        LOGE("%s: not implement for color conversion 0x%x -> 0x%x!",
            __FUNCTION__, srcBuf->format(), destBuf->format());
    }

    notifyCaptureDone(destBuf, request);
    return NO_ERROR;
}

status_t PostProcessStream::allocateBuffers(int bufsNum)
{
    LOG2("@%s: bufsNum %d", __FUNCTION__, bufsNum);
    if (mInternalBuffers.size()) {
        LOGE("%s: Already have buffers!", __FUNCTION__);
        return INVALID_OPERATION;
    }

    FrameInfo info;
    mProducer->query(&info);
    for (int i = 0; i < bufsNum; i++) {
#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
        /*
         * Using graphic buffer here as this buffer could be used by iVP
         * in a scale and/or color conversion use case. iVP only support
         * gralloc buffers.
         */
        sp<CameraBuffer> buf = MemoryUtils::allocateGraphicBuffer(
                                      info.width,
                                      info.height,
                                      info.format,mCameraId, 0);
#else
        sp<CameraBuffer> buf = MemoryUtils::allocateHeapBuffer(
                                      info.width,
                                      info.height,
                                      widthToStride(GFXFmt2V4l2Fmt(info.format, mCameraId), info.width),
                                      GFXFmt2V4l2Fmt(info.format, mCameraId), mCameraId);
#endif
        if (buf.get()) {
            mInternalBuffers.push_back(buf);
        } else {
            LOGE("%s: no memory for %s!", __FUNCTION__, getName());
            freeBuffers();
            return UNKNOWN_ERROR;
        }
    }

    mScaledBuf.clear();
    // For producer's buffer downscaling
    if (mMode == POSTPROCESSMODE_DOWNSCALING_AND_COLOER_CONVERSION) {
        mScaledBuf = MemoryUtils::allocateHeapBuffer(
                                      mConfig.width,
                                      mConfig.height,
                                      widthToStride(GFXFmt2V4l2Fmt(info.format, mCameraId), mConfig.width),
                                      GFXFmt2V4l2Fmt(info.format, mCameraId), mCameraId);
        if (!mScaledBuf.get()) {
            LOGE("%s: no memory for %s scaled !", __FUNCTION__, getName());
            freeBuffers();
            return UNKNOWN_ERROR;
        }
    }
    return NO_ERROR;
}

status_t PostProcessStream::freeBuffers()
{
    LOG2("@%s: mode %d", __FUNCTION__, mMode);
    if (mInternalBuffers.size()) {
        mInternalBuffers.clear();
        mScaledBuf.clear();
        mInternalBufferIndex = 0;
        mBuffersNumInDevice = 0;
    }

    return NO_ERROR;
}

sp<CameraBuffer> PostProcessStream::getOneInternalBuffers()
{
    LOG2("@%s: mode %d", __FUNCTION__, mMode);
    if (!mInternalBuffers.size() || mBuffersNumInDevice >= mInternalBuffers.size()) {
        LOGE("@%s, mInternalBuffers.size() is %d, mInternalBufferIndex:%d, mBuffersNumInDevice:%d", __FUNCTION__, mInternalBuffers.size(), mInternalBufferIndex, mBuffersNumInDevice);
        return NULL;
    }

    sp<CameraBuffer> buffer = NULL;

    if (mInternalBuffers.size()) {
        mInternalBufferIndex = mInternalBufferIndex % mInternalBuffers.size();
        buffer = mInternalBuffers.editItemAt(mInternalBufferIndex);
        mInternalBufferIndex++;
        mBuffersNumInDevice++;
    }
    return buffer;
}

void PostProcessStream::releaseOneInternalBuffers()
{
    LOG2("@%s: mode %d, mInternalBufferIndex:%d, mBuffersNumInDevice:%d", __FUNCTION__, mMode, mInternalBufferIndex, mBuffersNumInDevice);
    mBuffersNumInDevice--;
}

status_t PostProcessStream::query(FrameInfo *info)
{
    info->width = mConfig.width;
    info->height = mConfig.height;
    info->format = mConfig.format;
    info->originalFormat = mConfig.originalFormat;
    return NO_ERROR;
}

int PostProcessStream::selectPostProcessMode(FrameInfo & outInfo, FrameInfo & inInfo)
{
    int postMode = POSTPROCESSMODE_NONE;
    if (inInfo.format != outInfo.format) {
        if (outInfo.format == HAL_PIXEL_FORMAT_BLOB)
            postMode = POSTPROCESSMODE_JPEG_ENCODING;
        else
            postMode = POSTPROCESSMODE_DOWNSCALING_AND_COLOER_CONVERSION;
    } else if (inInfo.width > outInfo.width || inInfo.height > outInfo.height) {
        postMode = POSTPROCESSMODE_DOWNSCALING_AND_COLOER_CONVERSION;
    } else if (inInfo.width == outInfo.width && inInfo.height == outInfo.height){
        postMode = POSTPROCESSMODE_COPY;
    }

    return postMode;
}

/*
*   get metadata from hw
*/
status_t PostProcessStream::fillPicMetaDataFromHw(sp<ExifMetaData> metadata,
                                                  unsigned int exp_id)
{
    LOG1("@%s: ", __FUNCTION__);
    status_t status = NO_ERROR;

    // Fill exposure time
    SensorAeConfig *aeConfig = new SensorAeConfig;
    if (aeConfig == NULL) {
        LOGE("No memory for aeConfig allocation");
        return NO_MEMORY;
    } else {
        CLEAR(*aeConfig);
    }
    if (mIsp->isSensorEmbeddedMetaDataSupported()) {
        ia_aiq_exposure_sensor_parameters sensorExposure;
        ia_aiq_exposure_parameters exposure;
        CLEAR(sensorExposure);
        CLEAR(exposure);

        status_t status = mIsp->getDecodedExposureParams(&sensorExposure, &exposure, exp_id);
        if (status == NO_ERROR) {
            aeConfig->expTime = sensorExposure.coarse_integration_time;
        } else {
            LOGE("%s: fail to get exposure for %d", __FUNCTION__, exp_id);
        }
    }
    if (aeConfig->expTime == 0) {
        mSensor->getExposureTime(&aeConfig->expTime);
    }

    // note: the following may be null, if info not available
    metadata->aeConfig = aeConfig;

    metadata->ispMkNote = new atomisp_makernote_info;
    if (metadata->ispMkNote == NULL) {
        LOGE("there isn't enough memory");
        return NO_MEMORY;
    }
    memcpy((void*)metadata->ispMkNote, (void*)&ispMakerNote, sizeof(atomisp_makernote_info));

     //Get flash status from 3A
     bool flashFired = m3AControls->getAeFlashStatus();
     LOG2("The flash status is: %s", flashFired ? "ON" : "OFF");
     metadata->flashFired = flashFired;
    // Request mirroring for snapshot and postview buffers
    // (only for front camera)
    // Do mirroring only in still capture mode
    // TODO: need to consider the current work mode
    metadata->saveMirrored = (PlatformData::facing(mCameraId)== CAMERA_FACING_FRONT);
    metadata->cameraOrientation = PlatformData::orientation(mCameraId);
    return status;
}

/*
*   get metadata from 3A
*/
status_t PostProcessStream::fillPicMetaDataFrom3A(sp<ExifMetaData> metadata, int requestId)
{
    LOG1("@%s: ", __FUNCTION__);
    status_t status = NO_ERROR;

    aaaMetadataInfo *aaaMetadata = new aaaMetadataInfo;
    if (aaaMetadata == NULL) {
        LOGE("not enough memory");
        return NO_MEMORY;
    }
    CLEAR(*aaaMetadata);
    aaaMetadata->needMakeNote = true;
    status = mIsp->getMetadata(requestId, aaaMetadata);
    metadata->mIa3ASetting.brightness = aaaMetadata->brightness;
    metadata->mIa3ASetting.isoSpeed = aaaMetadata->isoSpeed;
    metadata->mIa3ASetting.aeMode = aaaMetadata->aeMode;
    metadata->mIa3ASetting.meteringMode = aaaMetadata->meteringMode;
    metadata->mIa3ASetting.lightSource = aaaMetadata->lightSource;
    metadata->ia3AMkNote = aaaMetadata->aaaMkNote;

    if (metadata->aeConfig != NULL) {
        metadata->aeConfig->aecApexTv = aaaMetadata->aeConfig.aecApexTv;
        metadata->aeConfig->aecApexAv = aaaMetadata->aeConfig.aecApexAv;
    }
    delete aaaMetadata;
    aaaMetadata = NULL;
    LOG1("brightness = %f,isoSpeed=%d,meteringMode=%d,lightSource=%d",
          metadata->mIa3ASetting.brightness, metadata->mIa3ASetting.isoSpeed, metadata->mIa3ASetting.meteringMode,
          metadata->mIa3ASetting.lightSource);
     return status;
}

status_t PostProcessStream::calcuAecApexTv(sp<ExifMetaData> metadata, ImgEncoder::EncodePackage & package)
{
     camera_metadata_ro_entry entry;
     int64_t exposureTimeUs = 0;
     entry = package.settings->find(ANDROID_SENSOR_EXPOSURE_TIME);
     if (entry.count == 1)
         exposureTimeUs = (int64_t)(entry.data.i64[0] / 1000);

     if (metadata->aeConfig != NULL && exposureTimeUs != 0)
         metadata->aeConfig->aecApexTv = -1.0 * (log10((double)exposureTimeUs/1000000) / log10(2.0)) * 65536;

     return NO_ERROR;
}

} /* namespace camera2 */
} /* namespace android */
