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

#define LOG_TAG "ProcUnit_Task"

#include "ProcUnitTask.h"
#include "LogHelper.h"

namespace android {
namespace camera2 {

ProcUnitTask::ProcUnitTask(const char *name, int priority):
    mThreadRunning(false),
    mName(name),
    mPriority(priority),
    mMessageQueue(mName.string(), static_cast<int>(MESSAGE_ID_MAX))
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

ProcUnitTask::~ProcUnitTask()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mNextTask.clear();
    deInit();
}

status_t
ProcUnitTask::init()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;

    String8 threadName = mName + String8("Thread");
    mMessageThead = new MessageThread(this, threadName.string(), mPriority);
    if (mMessageThead == NULL) {
        LOGE("Error creating %s Thread in init", threadName.string());
        deInit();
        return NO_MEMORY;
    }
    return status;
}

status_t
ProcUnitTask::deInit()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    if (mThreadRunning) {
        status = requestExitAndWait();
        if (status == NO_ERROR && mMessageThead != NULL) {
            mMessageThead.clear();
            mMessageThead = NULL;
        }
    }
    return status;
}

void
ProcUnitTask::registerNextTask(sp<ProcUnitTask> Task)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    mNextTask.push_back(Task);
}

void
ProcUnitTask::executeNextTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    for (unsigned int i = 0; i < mNextTask.size(); i++) {
        sp<ProcUnitTask> Task = mNextTask.editItemAt(i);
        Task->executeTask(msg);
        Task->executeNextTask(msg);
    }
}

void
ProcUnitTask::messageThreadLoop(void)
{
    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        ProcTaskMsg msg;
        mMessageQueue.receive(&msg);

        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            mThreadRunning = false;
            status = NO_ERROR;
            break;
        case MESSAGE_ID_EXECUTE_TASK:
            status = handleExecuteTask(msg);
            break;
        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("error %d in handling message: %d",
                 status, static_cast<int>(msg.id));
        mMessageQueue.reply(msg.id, status);
    }
}

status_t
ProcUnitTask::requestExitAndWait(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    ProcTaskMsg msg;
    msg.id = MESSAGE_ID_EXIT;
    return mMessageQueue.send(&msg, MESSAGE_ID_EXIT);
}

/**
 * Attach a Listening client to a particular event
 *
 * @param observer interface pointer to attach
 * @param event concrete event to listen to
 */
status_t
ProcUnitTask::attachListener(IPUTaskListener *observer,
                             IPUTaskListener::PUTaskEventType event)
{
    LOG1("@%s: %p to event type %d", __FUNCTION__, observer, event);
    status_t status = NO_ERROR;
    if (observer == NULL)
        return BAD_VALUE;
    Mutex::Autolock _l(mListenerLock);
    // Check event to be in the range of allowed events.
    if ((event < IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE ) ||
        (event > IPUTaskListener::PU_TASK_EVENT_MAX)) {
        LOGE("Event is outside the range of allowed events.");
        return BAD_VALUE;
    }

    // Check if we have any listener registered to this event
    int index = mListeners.indexOfKey(event);
    if (index == NAME_NOT_FOUND) {
        // First time someone registers for this event
        listener_list_t theList;
        theList.push_back(observer);
        mListeners.add(event, theList);
        return NO_ERROR;
    }

    // Now we will have more than one listener to this event
    listener_list_t &theList = mListeners.editValueAt(index);

    List<IPUTaskListener*>::iterator it = theList.begin();
    for (; it != theList.end(); ++it)
        if (*it == observer) {
            LOGW("listener previously added, ignoring");
            return ALREADY_EXISTS;
        }

    theList.push_back(observer);

    mListeners.replaceValueFor(event, theList);
    return status;
}

/**
 * Detach observer interface
 *
 * @param observer interface pointer to detach
 * @param event identifier for event type to detach
 */
status_t
ProcUnitTask::detachListener(IPUTaskListener *listener,
                             IPUTaskListener::PUTaskEventType event)
{
    LOG1("@%s:%d", __FUNCTION__, event);

    if (listener == NULL)
        return BAD_VALUE;

    Mutex::Autolock _l(mListenerLock);
    status_t status = BAD_VALUE;
    int index = mListeners.indexOfKey(event);
    if (index == NAME_NOT_FOUND) {
       LOGW("No listener register for this event, ignoring");
       return NO_ERROR;
    }

    listener_list_t &theList = mListeners.editValueAt(index);
    List<IPUTaskListener*>::iterator it = theList.begin();
    for (; it != theList.end(); ++it) {
        if (*it == listener) {
            theList.erase(it);
            if (theList.empty()) {
                mListeners.removeItemsAt(index);
            }
            status = NO_ERROR;
            break;
        }
    }
    return status;
}

status_t
ProcUnitTask::notifyListeners(IPUTaskListener::PUTaskMessage *msg)
{
    LOG2("@%s", __FUNCTION__);
    bool ret = false;
    Mutex::Autolock _l(mListenerLock);
    listener_list_t &theList = mListeners.editValueFor(msg->event.type);
    List<IPUTaskListener*>::iterator it = theList.begin();
    for (; it != theList.end(); ++it)
        ret |= (*it)->notifyPUTaskEvent((IPUTaskListener::PUTaskMessage*)msg);

    return ret;
}

}  // namespace camera2
}  // namespace android
