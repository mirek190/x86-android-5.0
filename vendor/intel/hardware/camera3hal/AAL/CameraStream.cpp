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

#define LOG_TAG "Camera_Stream"
#include <system/graphics.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#ifdef GRAPHIC_IS_GEN
#include <ufo/graphics.h>
#endif

#include "LogHelper.h"

#include "CameraStream.h"
#include "PlatformData.h"
#include "v4l2dev/v4l2device.h" // Only for FrameInfo, TODO: clean this

namespace android {
namespace camera2 {

CameraStream::CameraStream(int seqNo, camera3_stream_t * stream,
                           IRequestCallback * callback) : mActive(false),
                                                          mSeqNo(seqNo),
                                                          mCallback(callback),
                                                          mOutputBuffersInHal(0),
                                                          mStream3(stream)
{
    mConsumer = NULL;
    mProducer = NULL;
    mActualFormat = getActualGFXFmt(mStream3->usage, mStream3->format);
}

CameraStream::~CameraStream()
{
    LOG2("%s, pending request size=%d", __FUNCTION__, mPendingRequests.size());

    mPendingBuffers.clear();
    mPendingRequests.clear();
    mCamera3Buffers.clear();
}

void CameraStream::setActive(bool active)
{
    LOG1("CameraStream [%d] set %s", mSeqNo, active? " Active":" Inactive");
    mActive = active;
}

void CameraStream::dump(bool dumpBuffers) const
{
    LOG1("Stream %d (IO type %d) dump: -----", mSeqNo, mStream3->stream_type);
    LOG1("    %dx%d, fmt%d usage %x, buffers num %d (available %d)",
        mStream3->width, mStream3->height,
        mActualFormat,
        mStream3->usage,
        mStream3->max_buffers, mCamera3Buffers.size());
    if (dumpBuffers)
        for (unsigned int i = 0; i < mCamera3Buffers.size(); i++) {
            LOG1("        %d: handle %p, dataPtr %p", i,
                mCamera3Buffers[i]->getBufferHandle(), mCamera3Buffers[i]->data());
        }
}

status_t CameraStream::query(FrameInfo * info)
{
    LOG1("%s",__FUNCTION__);
    info->width= mStream3->width;
    info->height= mStream3->height;
    info->format =  mActualFormat;
    info->originalFormat =  mStream3->format;
    return NO_ERROR;
}

status_t CameraStream::capture(sp<CameraBuffer> aBuffer,
                               Camera3Request* request)
{
    LOGE("ERROR @%s: this is consumer node is NULL", __FUNCTION__);
    UNUSED(aBuffer);
    UNUSED(request);
    return NO_ERROR;
}

status_t CameraStream::captureDone(sp<CameraBuffer> buffer,
                                   Camera3Request* request)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    UNUSED(request);
    mPendingLock.lock();
    if (mPendingRequests.size() > 0) {
        Camera3Request* theRequest;
        theRequest = mPendingRequests.editItemAt(0);
        mPendingRequests.removeAt(0);
        mCallback->bufferDone(theRequest, buffer);
     }
     mPendingLock.unlock();


    return NO_ERROR;
}

status_t CameraStream::reprocess(sp<CameraBuffer> aBuffer,
                                 Camera3Request* request)
{
    LOGW("@%s: not implemented", __FUNCTION__);
    UNUSED(aBuffer);
    UNUSED(request);
    return NO_ERROR;
}

status_t CameraStream::processRequest(Camera3Request* request)
{
    LOG2("@%s %d, capture mProducer:%p, mConsumer:%p", __FUNCTION__, mSeqNo, mProducer, mConsumer);
    sp<CameraBuffer> buffer;
    if (mProducer == NULL) {
        LOGE("ERROR @%s: mProducer is NULL", __FUNCTION__);
        return BAD_VALUE;
    }

    mPendingLock.lock();
    mPendingRequests.push_back(request);
    mPendingLock.unlock();
    buffer = request->findBuffer(this);
    if CC_UNLIKELY(buffer == NULL)
        return NO_MEMORY;
    mProducer->capture(buffer, request);
    /*} else if (inBuf) {
        buffer = request->findBuffer(this);
        if (buffer == NULL)
            return NO_MEMORY;
        mProducer->reprocess(buffer, request);
        // workaround: return input buffer
        mCallback->bufferDone(request, buffer);
    }*/

    return NO_ERROR;
}

status_t CameraStream::configure(void)
{
    LOG2("@%s, %d, mProducer:%p  (%p)", __FUNCTION__, mSeqNo, mProducer, this);
    if (!mProducer) {
        LOGE("mProducer = NULL");
        return BAD_VALUE;
    }

    FrameInfo info;
    mProducer->query(&info);
    if ((info.width == (int)mStream3->width) && (info.height == (int)mStream3->height) && (info.format == mActualFormat)) {
        return NO_ERROR;
    }

    LOGE("configure error, not match\n");
    return UNKNOWN_ERROR;
}

void CameraStream::dump(int fd) const
{
    LOG2("@%s", __FUNCTION__);

    String8 message;
    message.appendFormat(LOG_TAG ":@%s producer:%p\n", __FUNCTION__, mProducer);
    message.appendFormat(LOG_TAG
            ":@%s requests in queue:%d\n",
            __FUNCTION__, mPendingRequests.size());
    write(fd, message.string(), message.size());
    if (mProducer != NULL)
        mProducer->dump(fd);
}

}; // namespace camera2
}; // namespace android
