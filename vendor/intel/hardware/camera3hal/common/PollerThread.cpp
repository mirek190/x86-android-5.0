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

#define LOG_TAG "Camera_PollerThread"

#include <fcntl.h>
#include <SchedulingPolicyService.h>
#include "PollerThread.h"

namespace android {
namespace camera2 {

static const int POLLER_THREAD_PRIORITY_LEVEL = 1;

PollerThread::PollerThread(const char* name,
                            int priority):
    mName(name),
    mPriority(priority),
    mThreadRunning(false),
    mMessageQueue("PollThread", static_cast<int>(MESSAGE_ID_MAX)),
    mListener (NULL)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    mMessageThread = new MessageThread(this, mName.string(), mPriority);
    mFlushFd[0] = -1;
    mFlushFd[1] = -1;
    mPid = getpid();
    mTid = gettid();
}

PollerThread::~PollerThread()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    close(mFlushFd[0]);
    close(mFlushFd[1]);
    // detach Listener
    mListener = NULL;

    if (mMessageThread != NULL) {
        mMessageThread->requestExitAndWait();
        mMessageThread.clear();
        mMessageThread = NULL;
    }
}

/**
 * init()
 * initialize flush file descriptors
 * start message thread with parameters
 * params: devices to poll.
 * params: observer
 *
 */
status_t PollerThread::init(Vector<sp<V4L2DeviceBase> > &devices, IPollEventListener *observer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;

    msg.id = MESSAGE_ID_INIT;
    msg.devices = devices; // copy the vector
    msg.data.init.observer = observer;

    return mMessageQueue.send(&msg);
}

status_t PollerThread::handleInit(Message &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;
    status = pipe(mFlushFd);

    // Change Thread Priority, to level threadPriorityPoller
    // change request asynchronously
    status = requestPriority(mPid, mTid, POLLER_THREAD_PRIORITY_LEVEL);
    if (status != NO_ERROR) {
        LOGW("%s: Policy SCHED_FIFO priority %d is not available for pid %d, tid %d; err = %d",
             __FUNCTION__, POLLER_THREAD_PRIORITY_LEVEL, mPid, mTid, status);
    }

    if (msg.devices.size() == 0) {
        LOGE("%s, No devices provided", __FUNCTION__);
        return BAD_VALUE;
    }

    if (msg.data.init.observer == NULL)
    {
        LOGE("%s, No observer provided", __FUNCTION__);
        return BAD_VALUE;
    }

    mPollingDevices = msg.devices;

    //attach listener.
    mListener = msg.data.init.observer;
    return status;
}

/**
 * pollRequest()
 * this method enqueue the poll request.
 * params: request ID
 *
 */
status_t PollerThread::pollRequest(int reqId, int timeout,
                                   Vector<sp<V4L2DeviceBase> > *devices)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    Message msg;
    msg.id = MESSAGE_ID_POLL_REQUEST;
    msg.data.request.reqId = reqId;
    msg.data.request.timeout = timeout;
    if (devices)
        msg.devices = *devices;

    return mMessageQueue.send(&msg);
}

status_t PollerThread::handlePollRequest(Message &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    int ret;
    int readbuf = 0;
    IPollEventListener::PollEventMessage outMsg;

    if (msg.devices.size() > 0)
        mPollingDevices = msg.devices;

    do {
        ret = V4L2DeviceBase::pollDevices(mPollingDevices, mActiveDevices,
                                          mInactiveDevices,
                                          msg.data.request.timeout, mFlushFd[0]);
        if (ret <= 0) {
            outMsg.id = IPollEventListener::POLL_EVENT_ID_ERROR;
        } else {
            outMsg.id = IPollEventListener::POLL_EVENT_ID_EVENT;
            // if no active devices selected and success of poll,
            // then the poll returned on a flush
            if (mActiveDevices.size() == 0) {
                read(mFlushFd[0], (void*) &readbuf, sizeof(int));
            }
        }
        outMsg.data.reqId = msg.data.request.reqId;
        outMsg.data.activeDevices = &mActiveDevices;
        outMsg.data.inactiveDevices = &mInactiveDevices;
        outMsg.data.polledDevices = &mPollingDevices;
        outMsg.data.pollStatus = ret;
        status = notifyListener(&outMsg);
    } while (status == -EAGAIN);
    return status;
}

/**
 * flush()
 * this method is done to interrupt the polling.
 * a value is written to a polled fd, which will make the poll returning
 *
 */
status_t PollerThread::flush()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    int buf = 0xf;  // random value to write to flush fd.

    mMessageQueue.remove(MESSAGE_ID_POLL_REQUEST);
    write(mFlushFd[1], &buf, sizeof(int));
    return NO_ERROR;
}

void PollerThread::messageThreadLoop()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mThreadRunning = true;

    while (mThreadRunning) {
        status_t status = NO_ERROR;
        Message msg;

        mMessageQueue.receive(&msg);

        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            status = handleMessageExit();
            break;

        case MESSAGE_ID_INIT:
            status = handleInit(msg);
            break;

        case MESSAGE_ID_POLL_REQUEST:
            status = handlePollRequest(msg);
            break;

        default:
            LOGE("error in handling message: %d, unknown message",
                static_cast<int>(msg.id));
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("error %d in handling message: %d", status, (int)msg.id);
    }
}

status_t PollerThread::requestExitAndWait(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    msg.data.request.reqId = 0;
    return mMessageQueue.send(&msg, MESSAGE_ID_EXIT);
}

status_t PollerThread::handleMessageExit(void)
{
    status_t status = NO_ERROR;
    mThreadRunning = false;
    mMessageQueue.reply(MESSAGE_ID_EXIT, status);
    return status;
}

/** Listener Methods **/
status_t PollerThread::notifyListener(IPollEventListener::PollEventMessage *msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = OK;

    if (mListener == NULL)
        return BAD_VALUE;

    status = mListener->notifyPollEvent((IPollEventListener::PollEventMessage*)msg);

    return status;
}

}; //namespace camera2
}; //namespace android
