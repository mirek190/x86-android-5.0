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

#ifndef _CAMERA3_HAL_CSS2400_FAKE_STREAM_H_
#define _CAMERA3_HAL_CSS2400_FAKE_STREAM_H_

#include <utils/Errors.h>

#include "Camera3Request.h"
#include "CameraStreamNode.h"
#include "HwStream.h"

namespace android {
namespace camera2 {

class FakeStreamManager {
public:
    FakeStreamManager();
    ~FakeStreamManager();

    enum FakeStreamType {
        FAKE_NONE,
        // Fake video stream, for the video request with only one YUV stream
        FAKE_VIDEO_NO_PREVIEW,
    };

    CameraStreamNode * getFakeStream(
                                    FakeStreamType type,
                                    FrameInfo * info,
                                    int bufsNum,
                                    int cameraId);
    void clearFakeStream();

    status_t processRequest(Camera3Request* request);

    status_t start(CameraStreamNode * fakeStream);
    status_t stop();

private:
    class FakeStream : public CameraStreamNode{
    public:
        FakeStream(FakeStreamType type, int bufsNum, int cameraId, FrameInfo * info = NULL);
        virtual ~FakeStream();
        int type() const { return mType; };

        // CameraStreamNode override
        virtual status_t query(FrameInfo * info);
        virtual status_t capture(sp<CameraBuffer> aBuffer,
                                 Camera3Request* request);
        virtual status_t captureDone(sp<CameraBuffer> buffer,
                                     Camera3Request* request);
        virtual status_t reprocess(sp<CameraBuffer> buffer,
                                   Camera3Request* request);
        virtual void dump(int fd) const;

        status_t processRequest(Camera3Request* request);
        status_t allocateBuffers();
        status_t freeBuffers();
        status_t stop();

    private:
        virtual status_t configure(void);

    private:
        Vector<sp<CameraBuffer> > mBuffers;
        Vector<sp<CameraBuffer> > mBuffersInDevice;
        Mutex mBuffersLock;
        FakeStreamType mType;
        FrameInfo mConfig;
        int mBufsNum;
        int mCameraId;
    };

private:
    bool isStreamMatched(FakeStream * stream,
                         FakeStreamType type,
                         FrameInfo * info);

private:
    sp<FakeStream> mFakeStream;
    sp<FakeStream> mNextStream;
    bool mStarted;
    int mBufsNum;
};

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_CSS2400_FAKE_STREAM_H_
