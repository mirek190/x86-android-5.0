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

#define LOG_TAG "Camera_StreamBuffer"
#include "LogHelper.h"
#include <sys/mman.h>
#include <ui/Fence.h>
#include <ui/GraphicBufferMapper.h>
#include "CameraBuffer.h"
#include "CameraStream.h"
#include "Camera3GFXFormat.h"

#include <utils/Trace.h>


namespace android {
namespace camera2 {

////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////

/**
 * CameraBuffer
 *
 * Default constructor
 * This constructor is used when we pre-allocate the CameraBuffer object
 * The initialization will be done as a second stage wit the method
 * init(), where we initialize the wrapper with the gralloc buffer provided by
 * the framework
 */
CameraBuffer::CameraBuffer() :  mWidth(0),
                                mHeight(0),
                                mSize(0),
                                mFormat(0),
                                mV4L2Fmt(0),
                                mStride(0),
                                mInit(false),
                                mLocked(false),
                                mType(BUF_TYPE_HANDLE),
                                mOwner(NULL),
                                mDataPtr(NULL),
                                mRequestID(0),
                                mCameraId(0)
{
    LOG1("%s default constructor for buf %p", __FUNCTION__, this);
    CLEAR(mUserBuffer);
    CLEAR(mTimestamp);
    CLEAR(mBuffer);
}

/**
 * CameraBuffer
 *
 * Constructor for buffers allocated using MemoryUtils::allocateGraphicBuffer
 *
 * \param w [IN] width
 * \param h [IN] height
 * \param s [IN] stride
 * \param format [IN] Gfx format
 * \param gfxBuf [IN] gralloc buffer
 * \param ptr [IN] mapperPointer
 */
CameraBuffer::CameraBuffer(int w, int h, int s, int format, GraphicBuffer *gfxBuf, void * ptr, int cameraId) :
    mWidth(w)
    ,mHeight(h)
    ,mSize(0)
    ,mFormat(format)
    ,mV4L2Fmt(0)
    ,mStride(s)
    ,mInit(false)
    ,mLocked(true)
    ,mType(BUF_TYPE_GFX)
    ,mOwner(NULL)
    ,mDataPtr(ptr)
    ,mRequestID(0)
    ,mCameraId(cameraId)
{
    CLEAR(mUserBuffer);
    CLEAR(mTimestamp);
    CLEAR(mBuffer);
    mUserBuffer.release_fence = -1;
    mUserBuffer.acquire_fence = -1;

    mBuffer.gfx = gfxBuf;
    if (gfxBuf && ptr) {
        mV4L2Fmt = GFXFmt2V4l2Fmt(mFormat, mCameraId);
        mSize = frameSize(mV4L2Fmt, mStride, mHeight);
        mInit= true;
    } else
        LOGE("%s: NULL input pointer!", __FUNCTION__);
}

/**
 * CameraBuffer
 *
 * Constructor for buffers allocated using MemoryUtils::allocateHeapBuffer
 *
 * \param w [IN] width
 * \param h [IN] height
 * \param s [IN] stride
 * \param v4l2fmt [IN] V4l2 format
 * \param usrPtr [IN] Data pointer
 */
CameraBuffer::CameraBuffer(int w, int h, int s, int v4l2fmt, void* usrPtr, int cameraId):
        mWidth(w),
        mHeight(h),
        mSize(0),
        mFormat(0),
        mV4L2Fmt(v4l2fmt),
        mStride(s),
        mInit(false),
        mLocked(true),
        mType(BUF_TYPE_MALLOC),
        mOwner(NULL),
        mDataPtr(NULL),
        mRequestID(0),
        mCameraId(cameraId)
{
    if (usrPtr != NULL) {
        mDataPtr = usrPtr;
        mInit = true;
        mSize = frameSize(mV4L2Fmt, mStride, mHeight);
        mFormat = v4L2Fmt2GFXFmt(v4l2fmt, mCameraId);
    } else {
        LOGE("Tried to initialize a buffer with NULL ptr!!");
    }
    CLEAR(mUserBuffer);
    CLEAR(mTimestamp);
    CLEAR(mBuffer);
    mUserBuffer.release_fence = -1;
    mUserBuffer.acquire_fence = -1;
}

/**
 * CameraBuffer
 *
 * Constructor for buffers allocated using mmap
 *
 * \param fd [IN] File descriptor to map
 * \param length [IN] amount of data to map
 * \param v4l2fmt [IN] Pixel format in V4L2 enum
 * \param offset [IN] offset from the begining of the file (mmap param)
 * \param prot [IN] memory protection (mmap param)
 * \param flags [IN] flags (mmap param)
 *
 * Success of the mmap can be queried by checking the size of the resulting
 * buffer
 */
CameraBuffer::CameraBuffer(int fd, int length, int v4l2fmt, int offset,
                           int prot, int flags):
        mWidth(1),
        mHeight(length),
        mSize(0),
        mFormat(0),
        mV4L2Fmt(v4l2fmt),
        mStride(1),
        mInit(false),
        mLocked(false),
        mType(BUF_TYPE_MMAP),
        mOwner(NULL),
        mDataPtr(NULL),
        mRequestID(0)
{

    mDataPtr = mmap(NULL, length, prot, flags, fd, offset);
    if (CC_UNLIKELY(mDataPtr == MAP_FAILED)) {
        LOGE("Failed to MMAP the buffer %s", strerror(errno));
        mDataPtr = NULL;
        return;
    }
    mLocked = true;
    mInit = true;
    mSize = length;
    CLEAR(mUserBuffer);
    mUserBuffer.release_fence = -1;
    mUserBuffer.acquire_fence = -1;
    LOG1("mmaped address for  %p length %d", mDataPtr, mSize);
}

/**
 * init
 *
 * Constructor to wrap a camera3_stream_buffer
 *
 * \param aBuffer [IN] camera3_stream_buffer buffer
 */
status_t CameraBuffer::init(const camera3_stream_buffer *aBuffer, int cameraId)
{
    mType = BUF_TYPE_HANDLE;
    mWidth = aBuffer->stream->width;
    mHeight = aBuffer->stream->height;
    mFormat = getActualGFXFmt(aBuffer->stream->usage, aBuffer->stream->format);
    mV4L2Fmt = GFXFmt2V4l2Fmt(mFormat, cameraId);
    // Use actual width from platform native handle for stride
    if (getNativeHandleStride(aBuffer->buffer))
       mStride =  getNativeHandleStride(aBuffer->buffer);
    else
       mStride = widthToStride(mV4L2Fmt, mWidth);
    mSize = 0;
    mLocked = false;
    mOwner = static_cast<CameraStream*>(aBuffer->stream->priv);
    mInit = true;
    mDataPtr = NULL;
    mUserBuffer = *aBuffer;
    mUserBuffer.release_fence = -1;
    mBuffer.handle = mUserBuffer.buffer;
    mCameraId = cameraId;
    /* TODO: add some consistency checks here and return an error */
    return NO_ERROR;
}

CameraBuffer::~CameraBuffer()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    if (mInit) {
        switch(mType) {
        case BUF_TYPE_GFX:
            mBuffer.gfx->unlock();
            mBuffer.gfx->decStrong(this);
            mBuffer.gfx = NULL;
            break;
        case BUF_TYPE_MALLOC:
            free(mDataPtr);
            mDataPtr = NULL;
            break;
        case BUF_TYPE_MMAP:
            if (mDataPtr != NULL)
                munmap(mDataPtr, mSize);
            mDataPtr = NULL;
            mSize = 0;
            break;
        default:
            //silent default
            break;
        }
    }
    LOG1("%s destroying buf %p", __FUNCTION__, this);
}

status_t CameraBuffer::waitOnAcquireFence()
{
    LOG2("%s: Fence in HAL is %d", __FUNCTION__, mUserBuffer.acquire_fence);

    sp<Fence> bufferAcquireFence = new Fence(mUserBuffer.acquire_fence);
    int res = bufferAcquireFence->wait(2000);  // 2s
    if (res == TIMED_OUT) {
        LOGE("%s: Buffer %p: Fence timed out after %d ms",
                __FUNCTION__, this, 2000);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t CameraBuffer::lock()
{
    if (!mInit) {
        LOGE("@%s: Error: Cannot lock now this buffer, not initialized",__FUNCTION__);
        return INVALID_OPERATION;
    }

    if (mType == BUF_TYPE_MALLOC) {
         mLocked = true;
         return NO_ERROR;
    }

    if (mLocked && mType == BUF_TYPE_GFX)
         return NO_ERROR;

    if (mLocked) {
        LOGE("@%s: Error: Cannot lock buffer from stream(%d), already locked", __FUNCTION__,mOwner->seqNo());
        return INVALID_OPERATION;
    }

    status_t status;

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    int lockMode = mOwner->usage();

    void * data = NULL;
    unsigned int size;

    if (mFormat == HAL_PIXEL_FORMAT_BLOB) {
        // for jpeg IMG gives resolution as width x 1, buffer size is right the width
        size = getNativeHandleWidth(mBuffer.handle);
        const Rect bounds(size, 1);
        status = mapper.lock(*mBuffer.handle, lockMode, bounds, &data);
    } else {
        const Rect bounds(mWidth, mHeight);
        size = frameSize(mV4L2Fmt,mStride,mHeight);
        status = mapper.lock(*mBuffer.handle, lockMode, bounds, &data);
    }
    if (status != NO_ERROR) {
       LOGE("ERROR @%s: Failed to lock GraphicBufferMapper! %d", __FUNCTION__, status);
       mapper.unlock(*mBuffer.handle);
    } else {
        mLocked = true;
        mDataPtr = data;
        mSize = size;
    }

    if (mType == BUF_TYPE_HANDLE)
        checkGrallocBuffer(mWidth, mV4L2Fmt, mBuffer.handle);

   return status;
}

status_t CameraBuffer::unlock()
{
    if (mLocked && mType == BUF_TYPE_MALLOC) {
         mLocked = false;
         return NO_ERROR;
    }

    if (mLocked && mType == BUF_TYPE_GFX)
         return NO_ERROR;

    if (mLocked) {
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        mapper.unlock(*mBuffer.handle);
        mLocked = false;
        return NO_ERROR;
    }
    LOGW("@%s:trying to unlock a buffer that is not locked", __FUNCTION__);
    return INVALID_OPERATION;
}

buffer_handle_t *CameraBuffer::getBufferHandle() const
{
    if (mType == BUF_TYPE_HANDLE) {
        return mBuffer.handle;
    } else if (mType == BUF_TYPE_GFX) {
        return &mBuffer.gfx->getNativeBuffer()->handle;
    }
    return NULL;
}

void CameraBuffer::dump()
{
    if (mInit) {
        LOG1("Buffer dump: gralloc handle %p: locked :%d: dataPtr:%p",
            (void*)mBuffer.handle, mLocked, mDataPtr);
    } else {
        LOG1("Buffer dump: Buffer not initialized");
    }
}

void CameraBuffer::dumpImage(const int type, const char *name) const
{
    if (LogHelper::isDumpImageTypeEnable(type)) {
        dumpImage(name);
    }
}

void CameraBuffer::dumpImage(const char *name) const
{
    dumpImage(mDataPtr, mSize, mWidth, mHeight, name);
}

void CameraBuffer::dumpImage(const void *data, const int size, int width, int height, const char *name) const
{
    char filename[80];
    static unsigned int count = 0;
    FILE *fp;

    LOG2("%s filename is %s", __func__, name);

    {
        snprintf(filename, sizeof(filename), "%sdump_%d_%d_%03u_%s_%d", CAMERA_OPERATION_FOLDER, width,
                 height, count, name, mRequestID);
    }
    count++;

    fp = fopen (filename, "w+");
    if (fp == NULL) {
        LOGE("open file failed");
        return;
    }
    LOG1("Begin write image %s", filename);

    if ((fwrite(data, size, 1, fp)) != 1)
        LOGW("Error or short count writing %d bytes to %s", size, filename);
    fclose (fp);
}

/**
 * Utility methods to allocate CameraBuffers from HEAP or Gfx memory
 *
 * Note that there is no free/delete, this is because CameraBuffer is RefBase'd
 * and therefore the destructor will be called once no reference exist
 * It is then mandatory the use of strong smart pointers to hold references
 * to this items (sp<>)
 */
namespace MemoryUtils {

sp<CameraBuffer>
allocateGraphicBuffer(int w, int h, int gfxFmt, int cameraId, int usage)
{
    LOG1("@%s", __FUNCTION__);
    UNUSED(usage);
    status_t status = OK;

    void * mapperPointer = NULL;

    int lockMode = GRALLOC_USAGE_SW_READ_OFTEN |
                GRALLOC_USAGE_SW_WRITE_NEVER |
                GRALLOC_USAGE_HW_CAMERA_READ |
                GRALLOC_USAGE_HW_CAMERA_WRITE |
                GRALLOC_USAGE_HW_COMPOSER;

    LOG1("%s with these properties: (%dx%d) gfx format %d", __FUNCTION__,
            w, h, gfxFmt);

    GraphicBuffer *gfxBuffer = new GraphicBuffer
        (w, h, gfxFmt,
        GraphicBuffer::USAGE_HW_RENDER | GraphicBuffer::USAGE_SW_WRITE_OFTEN | \
        GraphicBuffer::USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_CAMERA_WRITE);

    if (!gfxBuffer) {
        LOGE("No memory to allocate graphic buffer");
        return NULL;
    }

    ANativeWindowBuffer *nativeWinBuffer = gfxBuffer->getNativeBuffer();

    status = gfxBuffer->lock(lockMode, &mapperPointer);
    if (status != NO_ERROR) {
        LOGE("@%s: Failed to lock GraphicBuffer! %d", __FUNCTION__, status);
        return NULL;
    }

    if (w != nativeWinBuffer->stride) {
        LOG1("%s: potential bpl problem requested %d, Gfx requries %d",
            __FUNCTION__, w, nativeWinBuffer->stride);
    } else {
        LOG1("%s bpl from Gfx is %d", __FUNCTION__, nativeWinBuffer->stride);
    }

    CameraBuffer * buf =
        new CameraBuffer(w, h, nativeWinBuffer->stride, gfxFmt, gfxBuffer, mapperPointer, cameraId);
    gfxBuffer->incStrong(buf);

    return buf;
}

/**
 * Allocates the memory needed to store the image described by the parameters
 * passed during construction
 */
sp<CameraBuffer>
allocateHeapBuffer(int w, int h, int s, int v4l2Fmt, int cameraId)
{
    void* dataPtr;

    int dataSize = frameSize(v4l2Fmt, s, h);
    LOG1("@%s, dataSize:%d", __FUNCTION__, dataSize);

    int ret = posix_memalign(&dataPtr, PAGESIZE, dataSize);
    if (dataPtr == NULL || ret != 0) {
        LOGE("Could not allocate heap camera buffer of size %d", dataSize);
        return NULL;
    }

    CameraBuffer * buf = new CameraBuffer(w, h, s, v4l2Fmt, dataPtr, cameraId);

    return buf;
}

}; // namespace MemoryUtils

}; // namespace camera2
}; // namespace android
