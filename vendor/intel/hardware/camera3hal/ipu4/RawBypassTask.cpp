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

#include "RawBypassTask.h"
#include "LogHelper.h"
#include "CameraStream.h"

namespace android {
namespace camera2 {

RawBypassTask::RawBypassTask():
    ProcUnitTask("RawBypassTask")
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

RawBypassTask::~RawBypassTask()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

void RawBypassTask::getTaskOutputType(void)
{
}

status_t
RawBypassTask::executeTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    ProcTaskMsg pMsg;
    pMsg.id = MESSAGE_ID_EXECUTE_TASK;
    pMsg.data = msg.data;
    mMessageQueue.send(&pMsg, MESSAGE_ID_EXECUTE_TASK);

    return NO_ERROR;
}

status_t RawBypassTask::handleExecuteTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    CameraStream *stream = NULL;
    IPUTaskListener::PUTaskMessage outMsg;

    Camera3Request* request = msg.data.request;
    const Vector<camera3_stream_buffer>* outBufs = request->getOutputBuffers();
    for (unsigned int i = 0; i < outBufs->size(); i++) {
        camera3_stream_buffer outputBuffer = outBufs->itemAt(i);
        stream = reinterpret_cast<CameraStream *>(outputBuffer.stream->priv);
        sp<CameraBuffer> buffer = request->findBuffer(stream);
        if (!buffer->isLocked()) {
            status = buffer->lock();
            if (status != NO_ERROR) {
                LOGE("@%s: Could not lock the buffer", __FUNCTION__);
                return UNKNOWN_ERROR;
            }
        }

        status = buffer->waitOnAcquireFence();
        if (status != NO_ERROR) {
            LOGW("Wait on fence for buffer %p timed out", buffer.get());
        }

        if (buffer->format() == HAL_PIXEL_FORMAT_RAW_SENSOR) {
            // Pass through the Raw buffers
            outMsg.id = IPUTaskListener::PU_TASK_MSG_ID_EVENT;
            outMsg.event.type  = IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE;
            outMsg.event.buffer = buffer;
            outMsg.event.request = msg.data.request;
            notifyListeners(&outMsg);

         }
    }

    return status;
}

}  // namespace camera2
}  // namespace android
