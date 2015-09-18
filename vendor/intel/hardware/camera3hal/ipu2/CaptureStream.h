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
#ifndef CAMERA3_HAL_CAPTURE_STREAM_H_
#define CAMERA3_HAL_CAPTURE_STREAM_H_

#include "IPU2HwIsp.h"
#include "HwStream.h"
#include "DebugFrameRate.h"

namespace android {
namespace camera2 {

class V4L2VideoNode;
class IPU2HwSensor;
class IFlashCallback;

class CaptureStream : public HwStream,
                         public ICssIspListener,
                         public IMessageHandler,
                         public ICssBufferMaintainer {
public:
    enum CaptureMode {
        OFFLINE,
        ONLINE,
        FAKE_FOR_SOC
    };

    CaptureStream(sp<V4L2VideoNode> &aCaptureDevice,
                     sp<V4L2VideoNode> &aPostVDevice,
                     sp<IPU2HwIsp> &theIsp,
                     IPU2HwSensor * theSensor,
                     IFlashCallback *flashCallback,
                     int cameraId,
                     const char* streamName);
    virtual ~CaptureStream();
    /* HwStream overloads */
    virtual status_t init();
    virtual status_t setFormat(FrameInfo *aConfig);

    virtual status_t query(FrameInfo *info);
    virtual status_t capture(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);
    virtual status_t captureDone(sp<CameraBuffer> buffer);
    virtual status_t reprocess(sp<CameraBuffer> anInputBuffer);
    void dump(int fd) const;

    /* ICssIspListener overloads */
    virtual bool notifyIspEvent(ICssIspListener::IspMessage *msg);

    virtual status_t start();
    virtual status_t stop();
    virtual status_t flush();

    virtual const char* getName() const { return mStreamName.string(); }

    void setMode(CaptureMode mode);
    void enableCaptureWithoutPreview(bool enable) {
        mCaptureWithoutPreview = enable;
    };

    /* ICssBufferMaintainer overloads */
    void * queryBufferPointer(int requestId);

    void DebugStreamFps(const char *);
    // prevent copy constructor and assignment operator
private:
    CaptureStream(const CaptureStream& other);
    CaptureStream& operator=(const CaptureStream& other);

private:
    virtual status_t configure(void);
    status_t allocateBuffers(int bufsNum);
    status_t freeBuffers(void);

    sp<V4L2VideoNode>   mCaptureDevice;
    sp<V4L2VideoNode>   mPostviewDevice;
    sp<IPU2HwIsp>   mIsp;
    IPU2HwSensor  * mHwSensor;
    sp<DebugFrameRate>  mDebugFPS;
    int mCameraId;
    unsigned int mFakeBufferIndex;

    struct DeviceBuffer {
        sp<CameraBuffer> buffer;
        int bufferIndex;
        bool captureWithoutPreview;
        bool hasInput;
    };

    Mutex               mBufBookKeepingLock;
    /*!< The  mutex above ensures thread safe access to the  buffer book keeping vectors*/
    Vector<DeviceBuffer> mReceivedBuffers;  // received stream buffers for picture (JPEG)
    /* End of scope for mutex mBufBookKeepingLock */

    //Temporary hack until we decide what to do with postview
    FrameInfo             mPostviewConfig;
    Vector<sp<CameraBuffer> > mPostviewBuffers;
    Vector<struct v4l2_buffer> mV4l2PostviewBuffers;

    Vector<sp<CameraBuffer> > mFakeBuffers;
    unsigned int mFakeBufferIndexStart;
    unsigned int mMaxNumOfSkipFrames;

    IFlashCallback *mFlashCallback;
    //Temporary hack until we decide what to do with capture in online mode
    FrameInfo             mCaptureConfig;
    Vector<struct v4l2_buffer> mV4l2CaptureBuffers;

    CaptureMode           mMode;
    bool mRawLockEnabled;
    bool mCaptureWithoutPreview;
    int mLastReqId;
    struct RawLockedInfo {
        int reqId;
        int expId;
    };
    Vector<RawLockedInfo> mRawLockedInfo;

    String8 mStreamName;
private:
    // Poll thread message id's
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

    static const unsigned int REAL_BUF_NUM = 5;
    unsigned int mRealBufferIndex;  // from 0 to (REAL_BUF_NUM - 1)

private:
    status_t flushMessage(void);
    status_t handleMessagePoll(Message &msg);
    status_t handleMessageFlush(Message &msg);
    void notifyPolledDataToListeners(struct v4l2_buffer_info &v4l2Info,
                                     Camera3Request* request);
    status_t triggerCaptureByLockedRaw(int reqId, int expId);
    int getFlashExposedSnapshot(struct v4l2_buffer_info *captureV4l2Info,
                                struct v4l2_buffer_info *postV4l2Info);
    // TODO: use property, like camera.hal.dump, to toggle this debug function on/off.
    void dumpFrame(struct v4l2_buffer_info* buf);
};

} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_CAPTURE_STREAM_H_ */
