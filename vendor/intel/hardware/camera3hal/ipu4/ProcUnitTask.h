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

#ifndef CAMERA3_HAL_PROCUNITTASK_H_
#define CAMERA3_HAL_PROCUNITTASK_H_

#include "ICameraIPU4HwControls.h"
#include "CaptureBuffer.h"
#include "Camera3Request.h"
#include "MessageQueue.h"
#include "MessageThread.h"
#include <utils/KeyedVector.h>
#include <utils/List.h>

namespace android {
namespace camera2 {

/**
 * \class IPUTaskListener
 *
 * Abstract interface implemented by tasks interested on receiving notifications
 * from the previous Task. A task now has 2 ways to get executed.
 * 1. Listening to the previous task for buffer completion
 * 2. By executeNextTask()
 * Each task can use either of the method for execution
 *
 * Add event for buffer complete.
 */
class IPUTaskListener {
public:
    enum PUTaskId {
        PU_TASK_MSG_ID_EVENT = 0,
        PU_TASK_MSG_ID_ERROR
    };

    enum PUTaskEventType {
        PU_TASK_EVENT_BUFFER_COMPLETE = 0,
        PU_TASK_EVENT_JPEG_BUFFER_COMPLETE,
        PU_TASK_EVENT_MAX
    };

    // For MESSAGE_ID_EVENT
    struct PUTaskEvent {
        PUTaskEventType type;
        sp<CameraBuffer> buffer;
        Camera3Request* request;
        sp<CameraBuffer> jpegInputbuffer;
    };

    // For MESSAGE_ID_ERROR
    struct PUTaskError {
        status_t code;
    };

    struct PUTaskMessage {
        PUTaskId   id;
        PUTaskEvent event;
        PUTaskError error;
    };

    // Impelmented by listener
    virtual bool notifyPUTaskEvent(PUTaskMessage *msg) = 0;
    virtual ~IPUTaskListener() {}
};

/**
 * \class ProcUnitTask
 * Base class of all Processing unit Tasks.Defines how Tasks
 * behave and communicate with other Tasks
 *
 */
class ProcUnitTask : public IMessageHandler, public RefBase {
public:
    struct TaskInput {
        CaptureBuffer*   rawCapture;
        sp<CameraBuffer> paramBuffer;
        Camera3Request* request;
    };

    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_EXECUTE_TASK,
        MESSAGE_ID_MAX
    };

    struct MessageData {
        CaptureBuffer*   rawCapture;
        Camera3Request* request;
        sp<CameraBuffer> paramBuffer;
    };

    struct ProcTaskMsg {
        MessageId id;
        MessageData data;
    };

    ProcUnitTask(const char* name, int priority = PRIORITY_HIGHEST);
    virtual ~ProcUnitTask();

    virtual status_t executeTask(ProcTaskMsg &msg) = 0;
    virtual status_t handleExecuteTask(ProcTaskMsg &msg) = 0;
    virtual void getTaskOutputType(void) = 0;

    status_t init();
    void executeNextTask(ProcTaskMsg &msg);
    void registerNextTask(sp<ProcUnitTask> Task);
    void setTaskParams(const pSysInputParameters *pSysInputParams) {mPSysInputParams = pSysInputParams;}

    status_t attachListener(IPUTaskListener *aListener,
                            IPUTaskListener::PUTaskEventType event);
    status_t detachListener(IPUTaskListener *aListener,
                            IPUTaskListener::PUTaskEventType event);
    status_t notifyListeners(IPUTaskListener::PUTaskMessage *msg);

private:
    status_t deInit();
    status_t handleMessageExit(void);
    status_t requestExitAndWait();
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

private:
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    String8 mName;
    int mPriority;
    Mutex   mListenerLock;  /*!< Protects accesses to the Listener management variables */
    KeyedVector<IPUTaskListener::PUTaskEventType, bool> mEventsProvided;
    typedef List< IPUTaskListener* > listener_list_t;
    KeyedVector<IPUTaskListener::PUTaskEventType, listener_list_t > mListeners;

protected:
    MessageQueue<ProcTaskMsg, MessageId> mMessageQueue;
    Vector<sp<ProcUnitTask> > mNextTask;
    const pSysInputParameters * mPSysInputParams;
};

}  // namespace camera2
}  // namespace android

#endif  // CAMERA3_HAL_PROCUNITTASK_H_
