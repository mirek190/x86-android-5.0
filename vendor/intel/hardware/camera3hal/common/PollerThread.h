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

#ifndef CAMERA3_HAL_POLLERTHREAD_H_
#define CAMERA3_HAL_POLLERTHREAD_H_

#include "v4l2device.h"
#include "MessageQueue.h"
#include "MessageThread.h"
namespace android {
namespace camera2 {

#define EVENT_POLL_TIMEOUT 100 //100 milliseconds timeout

/**
 * \class IPollEventListener
 *
 * Abstract interface implemented by entities interested on receiving notifications
 * from IPU PollerThread.
 *
 * Notifications are sent whenever the poll returns.
 */
class IPollEventListener {
public:
    enum PollEventMessageId {
        POLL_EVENT_ID_EVENT = 0,
        POLL_EVENT_ID_ERROR
    };

    struct PollEventMessageData {
        const Vector<sp<V4L2DeviceBase> > *activeDevices;
        const Vector<sp<V4L2DeviceBase> > *inactiveDevices;
        Vector<sp<V4L2DeviceBase> > *polledDevices; // NOTE: notified entity is allowed to change this!
        int reqId;
        int pollStatus;
    };

    struct PollEventMessage {
        PollEventMessageId id;
        PollEventMessageData data;
    };

    virtual status_t notifyPollEvent(PollEventMessage *msg) = 0;
    virtual ~IPollEventListener() {};

}; //IPollEventListener

class PollerThread : public RefBase, public IMessageHandler
{
public:
    PollerThread(const char* name,
                        int priority = PRIORITY_HIGHEST);
    ~PollerThread();

    // Public Methods
    status_t init(Vector<sp<V4L2DeviceBase> > &devices, IPollEventListener *observer);
    status_t pollRequest(int reqId, int timeout = EVENT_POLL_TIMEOUT,
                         Vector<sp<V4L2DeviceBase> > *devices = NULL);
    status_t flush();
    status_t requestExitAndWait();

//Private Members
private:
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_INIT,
        MESSAGE_ID_POLL_REQUEST,
        MESSAGE_ID_MAX
    };

    struct MessageInit {
        IPollEventListener *observer;
    };

    struct MessagePollRequest {
        unsigned int reqId;
        unsigned int timeout;
    };

    union MessagePollData {
        MessageInit init;
        MessagePollRequest request;
    };

    struct Message {
        MessageId id;
        MessagePollData data;
        Vector<sp<V4L2DeviceBase> > devices; // for poll and init
    };

    Vector<sp<V4L2DeviceBase> > mPollingDevices;
    Vector<sp<V4L2DeviceBase> > mActiveDevices;
    Vector<sp<V4L2DeviceBase> > mInactiveDevices;

    String8 mName;
    int mPriority;
    bool mThreadRunning;
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThread;
    IPollEventListener* mListener; // one listener per PollerThread
    int mFlushFd[2];    // Flush file descriptor
    pid_t mPid;
    pid_t mTid;

//Private Methods
private:
    /* IMessageHandlerOverload */
    virtual void messageThreadLoop(void);
    status_t handleInit(Message &msg);
    status_t handlePollRequest(Message &msg);
    status_t handleMessageExit(void);

    status_t notifyListener(IPollEventListener::PollEventMessage *msg);

};

}; //namespace camera2
}; //namespace android


#endif /* CAMERA3_HAL_POLLERTHREAD_H_ */
