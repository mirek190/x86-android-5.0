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

#define LOG_TAG "JpegEncode_Task"

#include "JpegEncodeTask.h"
#include "LogHelper.h"
#include "CameraStream.h"

namespace android {
namespace camera2 {

JpegEncodeTask::JpegEncodeTask(int cameraId):
    ProcUnitTask("JpegEncodeTask"),
    mCameraId(cameraId)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    jpegTaskInit();
}

JpegEncodeTask::~JpegEncodeTask()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    if (mImgEncoder != NULL) {
        mImgEncoder.clear();
    }

    if (mJpegMaker != NULL) {
        delete mJpegMaker;
    }
}

status_t
JpegEncodeTask::jpegTaskInit()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    mImgEncoder = new ImgEncoder(mCameraId);
    if (mImgEncoder == NULL) {
        LOGE("Error creating ImgEncoder");
        return NO_INIT;
    }
    status = mImgEncoder->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init ImgEncoder!");
        mImgEncoder.clear();
        return NO_INIT;
    }

    mJpegMaker = new JpegMaker(mCameraId);
    if (mJpegMaker == NULL) {
        LOGE("Error creating JpegMaker");
        return NO_INIT;
    }
    status = mJpegMaker->init();
    if (status != NO_ERROR) {
        LOGE("Failed to init JpegMaker!");
        delete mJpegMaker;
        mImgEncoder.clear();
        return NO_INIT;
    }

    return status;
}

void JpegEncodeTask::getTaskOutputType(void)
{
}

status_t
JpegEncodeTask::executeTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    ProcTaskMsg pMsg;
    pMsg.id = MESSAGE_ID_EXECUTE_TASK;
    pMsg.data = msg.data;
    mMessageQueue.send(&pMsg, MESSAGE_ID_EXECUTE_TASK);

    return NO_ERROR;
}

status_t
JpegEncodeTask::handleExecuteTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    UNUSED(msg);
    return NO_ERROR;
}

status_t
JpegEncodeTask::doJPEGEncode(PUTaskMessage *procmsg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    IPUTaskListener::PUTaskMessage outMsg;

    LOG1("begin jpeg encoder");
    ImgEncoder::EncodePackage package;
    package.jpegOut = procmsg->event.buffer;
    package.main = procmsg->event.jpegInputbuffer;
    package.thumb = NULL;

    package.settings = procmsg->event.request->getSettings();
    sp<ExifMetaData> metadata = new ExifMetaData;

    // get metadata info from platform and 3A control
    // TODO figure out how to get accurate metadata from HW and 3a.

    status = mJpegMaker->setupExifWithMetaData(package, metadata);
    // Do sw or HW encoding. Also create Thumb buffer if needed
    status = mImgEncoder->encodeSync(package, metadata);
    if (package.thumbOut == NULL) {
        LOGE("%s: No thumb in EXIF", __FUNCTION__);
    }
    // Create a full JPEG image with exif metadata
    status = mJpegMaker->makeJpeg(package, package.jpegOut);
    if (status != NO_ERROR) {
        LOGE("%s: Make Jpeg Failed !", __FUNCTION__);
    }

    // Notify listeners, first fill the observer message
    outMsg.id = IPUTaskListener::PU_TASK_MSG_ID_EVENT;
    outMsg.event.type  = IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE;
    outMsg.event.buffer = package.jpegOut.get();
    outMsg.event.request = procmsg->event.request;
    notifyListeners(&outMsg);

    CameraStream *stream = procmsg->event.jpegInputbuffer->getOwner();
    if (stream != NULL) {
        camera3_stream_t *stream3 = stream->getStream();
        if (stream3->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
            // Return the input buffer
            outMsg.id = IPUTaskListener::PU_TASK_MSG_ID_EVENT;
            outMsg.event.type  = IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE;
            outMsg.event.buffer = procmsg->event.jpegInputbuffer;
            outMsg.event.request = procmsg->event.request;
            notifyListeners(&outMsg);
        }
    }

    return status;
}

bool
JpegEncodeTask::notifyPUTaskEvent(PUTaskMessage *procmsg)
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
    case PU_TASK_EVENT_JPEG_BUFFER_COMPLETE:
    {
        doJPEGEncode(procmsg);
        break;
    }
    default:
        LOGW("Unsupported ProcUnit Task event ");
        break;
    }

    return true;
}

} //  namespace camera2
} //  namespace android
