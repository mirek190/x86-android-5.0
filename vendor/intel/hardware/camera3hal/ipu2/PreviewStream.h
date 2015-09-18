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
#ifndef CAMERA3_HAL_PREVIEW_STREAM_H_
#define CAMERA3_HAL_PREVIEW_STREAM_H_
#include <utils/threads.h>
#include "MessageQueue.h"
#include "HwStream.h"
#include "IPU2CameraHw.h"
#include "MessageThread.h"
#include "DebugFrameRate.h"
#include "PerformanceTraces.h"

namespace android {
namespace camera2 {

class PreviewStream : public HwStream,
                            public IMessageHandler,
                            public ICssBufferMaintainer {
#define CSS_MAX_NUM_SKIP_FRAME 3

public:
    PreviewStream(sp<V4L2VideoNode> &aPreviewDevice,
                             sp<IPU2HwIsp> &theIsp,
                             IPU2HwSensor * theSensor,
                             int cameraId,
                             const char* streamName);
    virtual ~PreviewStream();
    /* HwStream overloads */
    virtual status_t init();
    virtual status_t setFormat(FrameInfo *aConfig);
    virtual status_t capture(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);
    void dump(int fd) const;

    virtual status_t stop();
    virtual status_t flush();
    virtual const char* getName() const { return mStreamName.string(); }

    void setMode(IspMode mode);

    /* ICssBufferMaintainer overloads */
    void * queryBufferPointer(int requestId);

    void DebugStreamFps(const char *);
// prevent copy constructor and assignment operator
private:
    PreviewStream(const PreviewStream& other);
    PreviewStream& operator=(const PreviewStream& other);

private:
    sp<V4L2VideoNode>   mVideoNode;
    sp<IPU2HwIsp>        mIsp;
    IPU2HwSensor * mHwSensor;
    sp<DebugFrameRate>  mDebugFPS;
    int mCameraId;
    IspMode mMode;
    bool mRawLockEnabled;
    unsigned int mFakeBufferIndex;

    Vector<struct v4l2_buffer> mDeviceBuffers;

    Mutex               mBufBookKeepingLock;
    /*!< The  mutex above ensures thread safe access to the  buffer book keeping vectors*/
    Vector<sp<CameraBuffer> > mBuffersInDevice;
    Vector<sp<CameraBuffer> > mFakeBuffers;
    unsigned int mFakeBufferIndexStart;
    unsigned int mMaxNumOfSkipFrames;
    /* End of scope for mutex mBufBookKeepingLock */

    FrameInfo mVideoNodeConfig;

    String8 mStreamName;
public:
    enum StreamMode {
        ONLINE,
        OFFLINE_CAPTURE,
        OFFLINE_VIDEO
    };
    void setMode(StreamMode mode);
    /* CameraStreamNode overloads */
    virtual status_t query(FrameInfo *info);
    virtual status_t captureDone(sp<CameraBuffer> buffer) {UNUSED(buffer); return true;}

private:
    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_POLL,
        MESSAGE_ID_FLUSH,
        MESSAGE_ID_MAX
    };

    union MessageData {
        int data;
    };

    struct Message {
        MessageId id;
        MessageData data;
        Camera3Request* request;
    };

    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

    enum StreamStatus {
        STOP = 0,
        START,
    };
    // indicate the current status
    StreamStatus mStatus;
    Mutex mStatusLock;
    StreamMode mStreamMode;
    static const unsigned int REAL_BUF_NUM = 6;
    unsigned int mRealBufferIndex;  // from 0 to (REAL_BUF_NUMs - 1)

private:
    virtual status_t configure(void);
    status_t flushMessage();
    status_t handleMessagePoll(Message &msg);
    status_t handleMessageFlush(Message &msg);
    void notifyPolledDataToListeners(struct v4l2_buffer_info &v4l2Info,
                                     Camera3Request* request);

};

} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_PREVIEW_STREAM_H_ */
