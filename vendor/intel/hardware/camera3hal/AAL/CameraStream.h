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

#ifndef _CAMERA3_HAL_CAMERA_STREAM_H_
#define _CAMERA3_HAL_CAMERA_STREAM_H_

#include <hardware/camera3.h>
#include "CameraStreamNode.h"
#include "CameraBuffer.h"
#include "ICameraHw.h"

namespace android {
namespace camera2 {

// Limited by streams buffers number
#define MAX_REQUEST_IN_PROCESS_NUM 10

enum STREAM_TYPE {
    STREAM_PREVIEW = 1,
    STREAM_CAPTURE = 1 << 1,
    STREAM_VIDEO = 1 << 2
};

class IRequestCallback;
struct Camera3Request;

/**
 * \class CameraStream
 * This class represents a user created stream
 *
 * This class is an HAL internal representation of the user provided stream
 * It is stored in the priv field of the camera3_stream_t passed by the user.
 *
 * It handles the  sequential return of buffers from the Camera HW class
 * Each CameraStream is bound to a HW counter part that will produce the data
 *
 *
 */
class CameraStream : public CameraStreamNode {
public:
    CameraStream(int seqNo, camera3_stream_t * stream, IRequestCallback * callback);
    ~CameraStream();

    void setActive(bool active);
    bool isActive() const {return mActive; }

    void dump(bool dumpBuffers = false) const;

    int seqNo(void) const { return mSeqNo; };
    int width(void) const { return mStream3->width; }
    int height(void) const { return mStream3->height; }
    int format(void) const { return mActualFormat; }
    int usage(void) const { return mStream3->usage; }
    int buffersNum(void) const { return mCamera3Buffers.size(); }
    //override API
    virtual status_t query(FrameInfo *info);
    virtual status_t capture(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);
    virtual status_t captureDone(sp<CameraBuffer> buffer,
                                 Camera3Request* request);
    virtual status_t reprocess(sp<CameraBuffer> buffer,
                             Camera3Request* request);
    //used in camerastream

    status_t processRequest(Camera3Request* request);
    camera3_stream_t * getStream() const { return mStream3;};
    void dump(int fd) const;

    void incOutBuffersInHal() { android_atomic_inc(&mOutputBuffersInHal); }
    void decOutBuffersInHal() { android_atomic_dec(&mOutputBuffersInHal); }
    int32_t outBuffersInHal() { return mOutputBuffersInHal; }

private: /* Methods */
    // CameraStreamNode override API
    virtual status_t configure(void);
    // Lock graphic buffer

private: /* Members */
    bool mActive;   /*!> Tracks the status of the stream during config time*/
    int mSeqNo;     /*!> Index of the stream */
    IRequestCallback * mCallback;
    volatile int32_t mOutputBuffersInHal;

    Vector<sp<CameraBuffer> > mCamera3Buffers;
    camera3_stream_t * mStream3;
    int mActualFormat;
    Vector<Camera3Request*>      mPendingRequests;
    Vector<sp<CameraBuffer> >    mPendingBuffers;
    Mutex mPendingLock;
};

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_CAMERA_STREAM_H_
