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

#define LOG_TAG "Camera_ProcUnit"

#include "ProcessingUnit.h"
#include "LogHelper.h"
#include "CameraStream.h"
#include "RawtoYUVTask.h"
#include "StreamOutputTask.h"
#include "JpegEncodeTask.h"
#include "RawBypassTask.h"

namespace android {
namespace camera2 {

ProcessingUnit::ProcessingUnit(int cameraId, CaptureUnit *aCaptureUnit) :
    mCameraId(cameraId),
    mCaptureUnit(aCaptureUnit),
    mMessageQueue("ProcUnitThread", static_cast<int>(MESSAGE_ID_MAX)),
    mMessageThead(NULL),
    mThreadRunning(false)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    mActiveStreams.blobStreams.clear();
    mActiveStreams.rawStreams.clear();
    mActiveStreams.yuvStreams.clear();
    mStartTask.clear();
    mListeningTasks.clear();
}

ProcessingUnit::~ProcessingUnit()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status;

    mActiveStreams.blobStreams.clear();
    mActiveStreams.rawStreams.clear();
    mActiveStreams.yuvStreams.clear();
    mStartTask.clear();
    mListeningTasks.clear();

    status = requestExitAndWait();
    if (status == NO_ERROR && mMessageThead != NULL) {
        mMessageThead.clear();
        mMessageThead = NULL;
    }
}

status_t
ProcessingUnit::init(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    mMessageThead = new MessageThread(this, "PUThread");
    if (mMessageThead == NULL) {
        LOGE("Error creating Processng Unit Thread in init");
        return NO_MEMORY;
    }

    return NO_ERROR;
}

status_t ProcessingUnit::configStreams(Vector<camera3_stream_t*> &activeStreams)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    mActiveStreams.blobStreams.clear();
    mActiveStreams.rawStreams.clear();
    mActiveStreams.yuvStreams.clear();

    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        switch (activeStreams.editItemAt(i)->format) {
        case HAL_PIXEL_FORMAT_BLOB:
             mActiveStreams.blobStreams.push_back(activeStreams.editItemAt(i));
             break;
        case HAL_PIXEL_FORMAT_RAW_SENSOR:
             mActiveStreams.rawStreams.push_back(activeStreams.editItemAt(i));
             break;
#ifdef GRAPHIC_IS_GEN
        // TODO: Add more supported formats
        case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
#else
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
#endif
             mActiveStreams.yuvStreams.push_back(activeStreams.editItemAt(i));
             break;
        default:
            LOGW("Unsupported stream format %d",
                 activeStreams.editItemAt(i)->format);
            break;
        }
    }
    streamConfigAnalyze(mActiveStreams);
    return status;
}

/**
 * streamConfigAnalyze
 *
 * Analyze the Capture unit input type and streams in the streamconfig
 * to create start stages for various processing. The various start stages are
 * RawtoYUV, streamout, jpegEncode, RawBypass, and YUVtoYUV
 *
 * \param activeStreams [IN] StreamConfig struct filled during configStreams
 */
status_t
ProcessingUnit::streamConfigAnalyze(StreamConfig activeStreams)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    status_t status = NO_ERROR;
    FrameInfo cuFInfo;
    sp<JpegEncodeTask> jpegTask = NULL;
    sp<RawtoYUVTask> raw2yuvTask = NULL;
    sp<RawBypassTask> rawBypassTask = NULL;

    mStartTask.clear();
    mListeningTasks.clear();

    status = mCaptureUnit->getOutputConfig(cuFInfo);
    if (status != NO_ERROR) {
        LOGE("Invalid config from CaptureUnit");
        return BAD_VALUE;
    }

    switch (v4L2Fmt2GFXFmt(cuFInfo.format, mCameraId)) {
    case HAL_PIXEL_FORMAT_RAW_SENSOR:
    {
        if (activeStreams.yuvStreams.size() > 0 || activeStreams.blobStreams.size() > 0) {
            // Need RAW to YUV conversion, create the task
            raw2yuvTask  = new RawtoYUVTask(cuFInfo,
                                            activeStreams.yuvStreams,
                                            activeStreams.blobStreams,
                                            mCameraId);
            status = raw2yuvTask->init();
            if (status != NO_ERROR) {
                LOGE("Failed to init Task");
                return NO_INIT;
            }
            // vector of start Tasks to trigger a buffer flow
            mStartTask.add(raw2yuvTask);

            // Create JPEG encode Task
            if (activeStreams.blobStreams.size() > 0) {
                jpegTask = new JpegEncodeTask(mCameraId);
                jpegTask->init();
                mListeningTasks.push_back(jpegTask);
                raw2yuvTask->attachListener(jpegTask.get(),
                                            IPUTaskListener::PU_TASK_EVENT_JPEG_BUFFER_COMPLETE);
            }

            // Create stream out Task for the streams
            sp<StreamOutputTask> outTask;
            outTask = new StreamOutputTask();
            outTask->init();
            mListeningTasks.push_back(outTask);

            if (raw2yuvTask != NULL)
                raw2yuvTask->attachListener(outTask.get(),
                                            IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE);
            if (jpegTask != NULL)
                jpegTask->attachListener(outTask.get(),
                                         IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE);
        }
        if (activeStreams.rawStreams.size() > 0) {
            rawBypassTask  = new RawBypassTask();
            status = rawBypassTask->init();
            if (status != NO_ERROR) {
                LOGE("Failed to init Task");
                return NO_INIT;
            }
            // vector of start Tasks to trigger a buffer flow
            mStartTask.add(rawBypassTask);

            // Create stream out Task for the streams
            sp<StreamOutputTask> outTask;
            outTask = new StreamOutputTask();
            outTask->init();
            mListeningTasks.push_back(outTask);

            if (rawBypassTask != NULL)
                rawBypassTask->attachListener(outTask.get(),
                                              IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE);
        }
    }
        break;
    // TODO Add YUV to YUV Task
    default:
         LOGE("@%s: Unsupported CaptureUnit format", __FUNCTION__);
        break;
    }

    return status;
}

status_t
ProcessingUnit::completeRequest(Camera3Request* request,
                                CaptureBuffer* rawbuffer,
                                const pSysInputParameters *pSysInputParams,
                                int inFlightCount)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    UNUSED(pSysInputParams);
    UNUSED(inFlightCount);

    const Vector<camera3_stream_buffer>* outBufs = request->getOutputBuffers();
    const Vector<camera3_stream_buffer>* inBufs = request->getInputBuffers();
    int reqId = request->getId();
    LOG2("@%s: Req id %d,  Num outbufs %d Num inbufs %d Rawcapture buffer %p",
         __FUNCTION__, reqId, outBufs->size(), inBufs->size(),
         (void*)rawbuffer->v4l2Buf.m.userptr);

    ProcUnitTask::ProcTaskMsg procMsg;
    procMsg.data.rawCapture = rawbuffer;
    procMsg.data.request = request;

    Message msg;
    msg.id = MESSAGE_EXECUTE_REQ;
    msg.pMsg = procMsg;
    mMessageQueue.send(&msg);

    return NO_ERROR;
}

status_t
ProcessingUnit::handleExecuteReq(Message &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    LOG2("@%s:handleExecuteReq for Req id %d, ", __FUNCTION__, msg.pMsg.data.request->getId());
    for (unsigned int i = 0; i < mStartTask.size(); i++) {
        sp<ProcUnitTask> task = mStartTask.editItemAt(i);
        task->executeTask(msg.pMsg);
        task->executeNextTask(msg.pMsg);
    }

    return status;
}

status_t
ProcessingUnit::flush()
{
    return NO_ERROR;
}

void
ProcessingUnit::dump(int fd)
{
    UNUSED(fd);
}

void
ProcessingUnit::messageThreadLoop(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);

        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            mThreadRunning = false;
            status = NO_ERROR;
            break;
        case MESSAGE_EXECUTE_REQ:
            handleExecuteReq(msg);
            break;
        default:
            LOGE("ERROR @%s: Unknown message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("error %d in handling message: %d",
                 status, static_cast<int>(msg.id));
        mMessageQueue.reply(msg.id, status);
    }
    LOG2("%s: Exit", __FUNCTION__);
}

status_t
ProcessingUnit::requestExitAndWait(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    return mMessageQueue.send(&msg, MESSAGE_ID_EXIT);
}

status_t
ProcessingUnit::handleMessageExit(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    mThreadRunning = false;
    return NO_ERROR;
}

}  // namespace camera2
}  // namespace android
