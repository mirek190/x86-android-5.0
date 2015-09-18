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

#ifndef _CAMERA3_HAL_PROCESSING_UNIT_H_
#define _CAMERA3_HAL_PROCESSING_UNIT_H_

#include "ICameraIPU4HwControls.h"
#include "MessageThread.h"
#include "MessageQueue.h"
#include "CaptureUnit.h"
#include "ProcUnitTask.h"

namespace android {
namespace camera2 {

typedef struct {
    Vector<camera3_stream_t *> yuvStreams;
    Vector<camera3_stream_t *> rawStreams;
    Vector<camera3_stream_t *> blobStreams;
} StreamConfig;

/**
 * \class ProcessingUnit
 * This class represents the HAL processing unit stage where the captured
 * raw frames are post processed before completing the request.
 *
 * This class creates instances of ProcUnitTask's to do each Task of
 * processing. Based on the stream config the necessary Tasks are instantiated
 * and the start Task is stored. When a completeRequest comes from the Capture
 * unit execution of the stored start Tasks are triggered which propogate to all
 * connected and listening Tasks
 *
 */
class PSysPipeline;
class ProcessingUnit: public IMessageHandler {
public:
    ProcessingUnit(int cameraId, CaptureUnit *aCaptureUnit);
    virtual ~ProcessingUnit();

    //Request handling
    virtual status_t init(void);
    virtual status_t configStreams(Vector<camera3_stream_t*> &activeStreams);
    virtual status_t completeRequest(Camera3Request* request,
                                     CaptureBuffer* rawbuffer,
                                     const pSysInputParameters *pSysInputParams,
                                     int inFlightCount);
    virtual status_t flush();
    virtual void dump(int fd);

private:  /* Types */
    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_EXECUTE_REQ,
        MESSAGE_ID_MAX
    };

    struct Message {
        MessageId id;
        ProcUnitTask::ProcTaskMsg pMsg;
    };

private:  /* Methods */
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);
    status_t handleExecuteReq(Message &msg);
    status_t handleMessageExit(void);
    status_t requestExitAndWait();
    status_t streamConfigAnalyze(StreamConfig activeStreams);

private:
    int mCameraId;
    CaptureUnit *mCaptureUnit;
    StreamConfig  mActiveStreams;
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    Vector<sp<ProcUnitTask> > mStartTask;
    Vector<sp<ProcUnitTask> > mListeningTasks;
}; // class ProcessingUnit

} /* namespace camera2 */
} /* namespace android */
#endif /* _CAMERA3_HAL_PROCESSING_UNIT_H_ */

