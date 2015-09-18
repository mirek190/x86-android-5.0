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

#define LOG_TAG "StreamOut_Task"

#include "StreamOutputTask.h"
#include "LogHelper.h"
#include "CameraStream.h"

namespace android {
namespace camera2 {

StreamOutputTask::StreamOutputTask():
    ProcUnitTask("StreamOutputTask"),
    mCaptureDoneCount(0)

{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

StreamOutputTask::~StreamOutputTask()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

void StreamOutputTask::getTaskOutputType(void)
{
}

bool
StreamOutputTask::notifyPUTaskEvent(PUTaskMessage *procmsg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    if (procmsg == NULL) {
        return false;
    }

    if (procmsg->id == PU_TASK_MSG_ID_ERROR) {
        // ProcUnit Task error
        return true;
    }

    switch (procmsg->event.type) {
    case PU_TASK_EVENT_BUFFER_COMPLETE:
    {
        // call capturedone for the stream of the buffer
        sp<CameraBuffer> buffer = procmsg->event.buffer;
        Camera3Request* request = procmsg->event.request;
        CameraStream *stream = buffer->getOwner();
        stream->captureDone(buffer, request);

        mCaptureDoneCount++;
        break;
    }
    default:
        LOGW("Unsupported ProcUnit Task event ");
        break;
    }
    return true;
}

status_t
StreamOutputTask::executeTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    ProcTaskMsg pMsg;
    pMsg.id = MESSAGE_ID_EXECUTE_TASK;
    pMsg.data = msg.data;
    mMessageQueue.send(&pMsg, MESSAGE_ID_EXECUTE_TASK);

    return NO_ERROR;
}

status_t StreamOutputTask::handleExecuteTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    UNUSED(msg);

    if (mCaptureDoneCount) {
        LOG1("@%s: captureDone called for %d buffers", __FUNCTION__, mCaptureDoneCount);
        mCaptureDoneCount = 0;
    } else {
        LOGE("@%s: captureDone not called", __FUNCTION__);
    }


    return status;
}

}  // namespace camera2
}  // namespace android
