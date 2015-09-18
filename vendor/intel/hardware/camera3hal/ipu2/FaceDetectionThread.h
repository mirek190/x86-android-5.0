/*
 * Copyright (c) 2014 Intel Corporation.
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

#ifndef _CAMERA3_HAL_FACEDETECTION_THREAD_H_
#define _CAMERA3_HAL_FACEDETECTION_THREAD_H_

#include <utils/threads.h>
#include <gui/SensorEventQueue.h>
#include <time.h>

#include "FaceDetector.h"
#include "MessageQueue.h"
#include "ipu2/HwStream.h"
#include "MessageThread.h"
#include "CameraBuffer.h"

#define FACE_MAX_FRAME_DATA  2

namespace android {
namespace camera2 {

class CameraBuffer;
class ICssIspListener;
class IFaceDetectCallback;


class FaceDetectionThread : public ICssIspListener, public IMessageHandler {
 public:
    FaceDetectionThread(const CameraMetadata &staticMeta,
                              IFaceDetectCallback * callback, int cameraId);
    virtual ~FaceDetectionThread();

    status_t requestExitAndWait();

    /**********************************************************************
     * APIs
     */
    status_t init();
    status_t processRequest(Camera3Request* request);
    void dump(int fd);
    /**********************************************************************
     * ICssIspListener override
     */
    bool notifyIspEvent(ICssIspListener::IspMessage *msg);

 public:
    virtual void startFaceDetection();
    virtual void stopFaceDetection(bool wait = false);
    virtual int sendFrame(sp<CameraBuffer> img);

 private:
/**********************************************************************
 * Thread methods
 */
// thread message id's
enum MessageId {
    MESSAGE_ID_EXIT = 0,

    MESSAGE_ID_NEW_REQUEST,
    MESSAGE_ID_NEW_IMAGE,
    MESSAGE_ID_START_FACE_DETECTION,
    MESSAGE_ID_STOP_FACE_DETECTION,

    MESSAGE_ID_MAX
};

struct MessageFrame {
     sp<CameraBuffer> img;
};


// union of all message data
struct MessageData {
    MessageFrame frame;
};

// message id and message data
struct Message {
    MessageId id;
    MessageData data;
    Camera3Request* request;
};

// private data
 private:
    int mCameraId;
    IFaceDetectCallback *mCallbacks;
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    FaceDetector* mFaceDetector;
    int mLastReportedNumberOfFaces;
    bool mThreadRunning;
    bool mFaceDetectionRunning;
    int mActiveArrayWidth;     // array size from sensor
    int mActiveArrayHeight;

    int mRotation;
    int mCameraOrientation;
    int mSensorLayout;
    sp<SensorEventQueue> mSensorEventQueue;
    int mFrameCount;
    Vector<ia_frame* > mPendingFrameData;
    int mNextWriteProcessFrame;
    int mNextReadProcessFrame;
    Mutex mPendingQueueLock;


// private methods
 private:
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);
    status_t handleNewRequest(Message &msg);
    status_t processSettings(const CameraMetadata &settings);
    status_t handleMessageExit(void);
    status_t handleFrame(MessageFrame frame);
    status_t handleExit();
    status_t handleMessageStartFaceDetection();
    status_t handleMessageStopFaceDetection();

    void detectRotationEvent(void);
    status_t yChannelCopy(sp<CameraBuffer> img, ia_frame* dst);
    status_t frameDataCopy(sp<CameraBuffer> img);
};
}  // namespace camera2
}  // namespace android

#endif  //  _CAMERA3_HAL_FACEDETECTION_THREAD_H_

