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
#define LOG_TAG "Fake_Stream"

#include <sys/mman.h>
#include <sys/stat.h>

#include "LogHelper.h"
#include "FakeStreamManager.h"

namespace android {
namespace camera2 {

FakeStreamManager::FakeStreamManager() :
    mFakeStream(NULL)
    ,mNextStream(NULL)
    ,mStarted(false)
{
}

FakeStreamManager::~FakeStreamManager()
{
    mFakeStream.clear();
    mNextStream.clear();
}

CameraStreamNode * FakeStreamManager::getFakeStream(
                                          FakeStreamType type,
                                          FrameInfo * info,
                                          int bufsNum,
                                          int cameraId)
{
    LOG2("@%s", __FUNCTION__);

    if (mFakeStream.get() && isStreamMatched(mFakeStream.get(), type, info)) {
        LOG1("@%s: reuse current fake stream", __FUNCTION__);
        mNextStream = mFakeStream;
        return mFakeStream.get();
    }
    if (mNextStream.get() && !isStreamMatched(mNextStream.get(), type, info)) {
        mNextStream.clear();
    }
    if (!mNextStream.get()) {
        LOG1("@%s: create new fake stream %d, bufsNum:%d", __FUNCTION__, type, bufsNum);
        mNextStream = new FakeStream(type, bufsNum, cameraId, info);
    }

    return mNextStream.get();
}

void FakeStreamManager::clearFakeStream()
{
    LOG2("@%s", __FUNCTION__);
    stop();
    mFakeStream.clear();
    mNextStream.clear();
}

bool FakeStreamManager::isStreamMatched(
                                          FakeStream * stream,
                                          FakeStreamType type,
                                          FrameInfo * info)
{
    LOG2("@%s", __FUNCTION__);

    if (!stream)
        return false;

    if (type != stream->type())
        return false;

    if (info) {
        FrameInfo oldInfo;
        stream->query(&oldInfo);
        // Don't need to check format, because none use fake image
        return ((oldInfo.width == info->width)
             && (oldInfo.height == info->height));
    }
    return true;
}

status_t FakeStreamManager::start(CameraStreamNode * fakeStream)
{
    LOG2("@%s", __FUNCTION__);

    if (!fakeStream)
        return NO_ERROR;

    if (fakeStream != mNextStream.get()) {
        LOGE("%s: bad stream ptr!", __FUNCTION__);
        return BAD_VALUE;
    }

    if (!mStarted && mNextStream.get()) {
        LOG2("@%s", __FUNCTION__);
        mFakeStream = mNextStream;
        mNextStream.clear();

        if (mFakeStream->allocateBuffers() != NO_ERROR)
            return NO_MEMORY;

        mStarted = true;
    }

    return NO_ERROR;
}

status_t FakeStreamManager::stop()
{
    LOG2("@%s", __FUNCTION__);

    if (mStarted) {
        LOG2("@%s", __FUNCTION__);
        mFakeStream->stop();
        //mFakeStream->freeBuffers();
        mFakeStream.clear();
        mStarted = false;
    }
    return NO_ERROR;
}

status_t FakeStreamManager::processRequest(Camera3Request* request)
{
    LOG2("@%s", __FUNCTION__);

    if (mStarted)
        mFakeStream->processRequest(request);
    return NO_ERROR;
}

FakeStreamManager::FakeStream::FakeStream(FakeStreamType type,
                                          int bufsNum,
                                          int cameraId, FrameInfo *info) :
    mType(type)
    ,mBufsNum(bufsNum + 1)
    ,mCameraId(cameraId)
{
    LOG2("@%s", __FUNCTION__);
    if (info)
        mConfig = *info;
    else {
        mConfig.width = 1280;
        mConfig.height = 720;
        mConfig.originalFormat = mConfig.format = v4L2Fmt2GFXFmt(
                    PlatformData::getV4L2PixelFmtForGfxHalFmt(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, mCameraId),
                    mCameraId);
        mConfig.maxWidth = 2560;
        mConfig.maxHeight = 1920;
        mConfig.bufsNum = mBufsNum;
        mConfig.stride = widthToStride(mConfig.format, mConfig.width);
        mConfig.size = frameSize(mConfig.format, mConfig.stride, mConfig.height);
    }
}

FakeStreamManager::FakeStream::~FakeStream()
{
    LOG2("@%s", __FUNCTION__);
    stop();
    mBuffers.clear();
    mBuffersInDevice.clear();
}

status_t FakeStreamManager::FakeStream::query(FrameInfo * info)
{
    LOG2("@%s", __FUNCTION__);

    if (info)
        *info = mConfig;
    return NO_ERROR;
}

status_t FakeStreamManager::FakeStream::allocateBuffers()
{
    LOG2("@%s", __FUNCTION__);

    Mutex::Autolock _l(mBuffersLock);
    if (mBuffers.size() || mBuffersInDevice.size()) {
        LOGW("%s: buffers are allocated!", __FUNCTION__);
        return NO_ERROR;
    }

    for (int i = 0; i < mBufsNum; i++) {
        sp<CameraBuffer> buf = MemoryUtils::allocateHeapBuffer(
                                      mConfig.width,
                                      mConfig.height,
                                      widthToStride(GFXFmt2V4l2Fmt(mConfig.format, mCameraId), mConfig.width),
                                      GFXFmt2V4l2Fmt(mConfig.format, mCameraId), mCameraId);
        if (buf.get()) {
            mBuffers.push_back(buf);
        } else {
            LOGE("%s: no memory for fake stream %d!", __FUNCTION__, mType);
            freeBuffers();
            return UNKNOWN_ERROR;
        }
    }
    return NO_ERROR;
}

status_t FakeStreamManager::FakeStream::freeBuffers()
{
    LOG2("@%s: %d %d", __FUNCTION__, mBuffers.size(), mBuffersInDevice.size());
    Mutex::Autolock _l(mBuffersLock);
    if (mBuffersInDevice.size())
        LOGW("%s: buffers in use!", __FUNCTION__);

    if (!mBuffers.size()) {
        LOGW("%s: no memory!", __FUNCTION__);
    }

    unsigned int num = mBuffers.size();
    for(unsigned int i = 0; i < num; i++) {
        sp<CameraBuffer> buf = mBuffers[0];
        mBuffers.removeAt(0);
        buf.clear();
    }
    num = mBuffersInDevice.size();
    for(unsigned int i = 0; i < num; i++) {
        sp<CameraBuffer> buf = mBuffersInDevice[0];
        mBuffersInDevice.removeAt(0);
        buf.clear();
    }

    mBuffers.clear();
    mBuffersInDevice.clear();
    return NO_ERROR;
}

status_t FakeStreamManager::FakeStream::capture(sp<CameraBuffer> buf,
                                                Camera3Request* request)
{
    LOGE("@%s: invalid operation for FakeStream!", __FUNCTION__);
    UNUSED(buf);
    UNUSED(request);
    return INVALID_OPERATION;
}

status_t FakeStreamManager::FakeStream::captureDone(sp<CameraBuffer> buf,
                                                    Camera3Request* request)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(request);

    Mutex::Autolock _l(mBuffersLock);

    if (mBuffersInDevice.size() > 0) {
        if (mBuffersInDevice[0].get() == buf.get()) {
            mBuffers.push_back(buf);
            mBuffersInDevice.removeAt(0);
        } else {
            LOGE("%s: buffers out of order!", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
     }

    return NO_ERROR;
}

status_t FakeStreamManager::FakeStream::reprocess(sp<CameraBuffer> inBuf,
                                                  Camera3Request* request)
{
    LOGE("@%s: invalid operation for FakeStream!", __FUNCTION__);
    UNUSED(inBuf);
    UNUSED(request);
    return INVALID_OPERATION;
}

void FakeStreamManager::FakeStream::dump(int fd) const
{
    UNUSED(fd);
}

status_t FakeStreamManager::FakeStream::configure(void)
{
    LOG2("@%s", __FUNCTION__);
    if (!mProducer) {
        LOGE("mProducer = NULL");
        return INVALID_OPERATION;
    }

    FrameInfo info;
    mProducer->query(&info);
    if ((info.width == mConfig.width) && (info.height == mConfig.height) && (info.format == mConfig.format)) {
        Mutex::Autolock _l(mBuffersLock);
    } else {
        LOGE("configure error, not match\n");
    }
    return NO_ERROR;
}

status_t FakeStreamManager::FakeStream::stop()
{
    LOG2("@%s", __FUNCTION__);

    freeBuffers();
    return NO_ERROR;
}

status_t FakeStreamManager::FakeStream::processRequest(Camera3Request* request)
{
    LOG2("@%s", __FUNCTION__);
    if (mProducer == NULL) {
        LOGE("ERROR @%s: mProducer is NULL", __FUNCTION__);
        return BAD_VALUE;
    }
    if (!mBuffers.size()) {
        LOGE("ERROR @%s: No available buffer!", __FUNCTION__);
        return INVALID_OPERATION;
    }

    sp<CameraBuffer> buffer = NULL;
    {
    Mutex::Autolock _l(mBuffersLock);
    buffer = mBuffers.editItemAt(0);
    mBuffersInDevice.push_back(buffer);
    mBuffers.removeAt(0);
    }
    mProducer->capture(buffer, request);
    return NO_ERROR;
}


}; // namespace camera2
} // namespace android
