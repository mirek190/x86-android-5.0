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

#ifndef _CAMERA3_HAL_CAMERA_BUFFER_H_
#define _CAMERA3_HAL_CAMERA_BUFFER_H_

#include <utils/threads.h>
#include <ui/GraphicBuffer.h>
#include <hardware/camera3.h>
#include "UtilityMacros.h"

namespace android {
namespace camera2 {

// Forward declaration to  avoid extra include
class CameraStream;

/**
 * \class CameraBuffer
 *
 * This class is the buffer abstraction in the HAL. It can store buffers
 * provided by the framework or buffers allocated by the HAL.
 * Allocation in the HAL can be done via gralloc, malloc or mmap
 * in case of mmap the memory cannot be freed
 */
class CameraBuffer: public RefBase {
public:
    enum BufferType {
        BUF_TYPE_HANDLE,
        BUF_TYPE_GFX,
        BUF_TYPE_MALLOC,
        BUF_TYPE_MMAP,
    };

public:
    /**
     * default constructor
     * Used for buffers coming from the framework. The wrapper is initialized
     * using the method init
     */
    CameraBuffer();

    /**
     * constructor for the HAL-allocated buffer
     * These are used via the utility methods in the MemoryUtils namespace
     */
    CameraBuffer(int w, int h, int s, int format, GraphicBuffer *gfxBuf, void * ptr, int cameraId);
    CameraBuffer(int w, int h, int s, int v4l2fmt, void* usrptr, int cameraId);
    CameraBuffer(int fd, int length, int v4l2fmt, int offset, int prot, int flags);
    /**
     * initialization for the wrapper around the framework buffers
     */
    status_t init(const camera3_stream_buffer *aBuffer, int cameraId);

    void* data() { return mDataPtr; };

    status_t lock();
    status_t unlock();

    bool isLocked() const { return mLocked; };
    buffer_handle_t * getBufferHandle() const;
    status_t waitOnAcquireFence();
    GraphicBuffer * getGfxBuffer() const {
        return (mType == BUF_TYPE_GFX) ? mBuffer.gfx : NULL;
    }

    void dump();
    void dumpImage(const int type, const char *name) const;
    void dumpImage(const char *name) const;
    void dumpImage(const void *data, const int size, int width, int height,
                    const char *name) const;
    CameraStream * getOwner() const { return mOwner; }
    int width() {return mWidth; }
    int height() {return mHeight; }
    int stride() {return mStride; }
    unsigned int size() {return mSize; }
    int format() {return mFormat; }
    int v4l2Fmt() {return mV4L2Fmt; }
    struct timeval timeStamp() {return mTimestamp; }
    int64_t timeStampNano() { return TIMEVAL2NSECS(&mTimestamp); }
    void setTimeStamp(struct timeval timestamp) {mTimestamp = timestamp; }
    void setRequestId(int requestId) {mRequestID = requestId; }
    int requestId() {return mRequestID; }

private:  /* methods */
    /**
     * no need to delete a buffer since it is RefBase'd. Buffer will be deleted
     * when no reference to it exist.
     */
    virtual ~CameraBuffer();

private:
    camera3_stream_buffer_t mUserBuffer; /*!< Original structure passed by request */
    int             mWidth;
    int             mHeight;
    unsigned int    mSize;           /*!< size in bytes, this is filled when we
                                           lock the buffer */
    int             mFormat;         /*!<  HAL PIXEL fmt */
    int             mV4L2Fmt;        /*!< fourcc format code */
    int             mStride;
    struct timeval  mTimestamp;
    bool            mInit;           /*!< Boolean to check the integrity of the
                                          buffer when it is created*/
    bool            mLocked;         /*!< Use to track the lock status */

    BufferType mType;
    union {
        buffer_handle_t *handle;     /*!< Structure provided by FrameWork */
        GraphicBuffer * gfx;
    } mBuffer;
    CameraStream *mOwner;             /*!< Stream this buffer belongs to */
    void*         mDataPtr;           /*!< if locked, here is the vaddr */
    int           mRequestID;         /*!< this is filled by hw streams after
                                          calling putframe */
    int mCameraId;
};

namespace MemoryUtils {

sp<CameraBuffer>
allocateGraphicBuffer(int w, int h, int gfxFmt, int cameraId, int usage = 0);

sp<CameraBuffer>
allocateHeapBuffer(int w, int h, int s, int v4l2Fmt, int cameraId);

};


}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_CAMERA_BUFFER_H_
