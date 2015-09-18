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

#define LOG_TAG "Camera3_RequestThread"

#include "RequestThread.h"
#include "ResultProcessor.h"
#include "CameraMetadataHelper.h"
#include "PlatformData.h"
#include "PerformanceTraces.h"

namespace android {
namespace camera2 {


RequestThread::RequestThread(int cameraId, ICameraHw *aCameraHW) :
    MessageThread(this, "Cam3ReqThread"),
    mCameraId(cameraId),
    mCameraHw(aCameraHW),
    mMessageQueue("RequestThread", static_cast<int>(MESSAGE_ID_MAX)),
    mRequestsInHAL(0),
    mFlushing(false),
    mRequestBlocked(false),
    mWaitingRequest(NULL),
    mMaxReqInProcess(MAX_REQUEST_IN_PROCESS_NUM),
    mInitialized(false),
    mResultProcessor(NULL),
    mStreamSeqNo(0)
{
    LOG1("@%s", __FUNCTION__);
    mStreams.clear();
    mLocalStreams.clear();
}

RequestThread::~RequestThread()
{
    if(mInitialized)
        deinit();
}
status_t
RequestThread::init(const camera3_callback_ops_t *callback_ops)
{
    LOG1("@%s", __FUNCTION__);

    status_t status = NO_ERROR;
    status = mRequestsPool.init(MAX_REQUEST_IN_PROCESS_NUM);
    if (status != NO_ERROR) {
        LOGE("Error creating RequestPool: %d", status);
        return status;
    }

    mResultProcessor = new ResultProcessor(this, callback_ops);
    if (mResultProcessor == NULL) {
        LOGE("Error creating ResultProcessor: No memory");
        return NO_MEMORY;
    }
    mInitialized = true;
    return NO_ERROR;
}

status_t
RequestThread::deinit()
{
    if (mResultProcessor!= NULL) {
        mResultProcessor->requestExitAndWait();
        delete mResultProcessor;
        mResultProcessor = NULL;
    }

    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    mMessageQueue.send(&msg);
    requestExitAndWait();

    // Delete all streams
    for (unsigned int i = 0; i < mLocalStreams.size(); i++) {
        CameraStream *s = mLocalStreams.editItemAt(i);
        delete s;
    }

    mStreams.clear();
    mLocalStreams.clear();

    mWaitingRequest = NULL;
    if (mInitialized) {
        mRequestsPool.deInit();
        mInitialized = false;
    }
    return NO_ERROR;
}

status_t
RequestThread::configureStreams(camera3_stream_configuration_t *stream_list)
{
    Message msg;
    msg.id = MESSAGE_ID_CONFIGURE_STREAMS;
    msg.data.streams.list = stream_list;
    return mMessageQueue.send(&msg, MESSAGE_ID_CONFIGURE_STREAMS);
}

bool RequestThread::areAllStreamsUnderMaxBuffers() const
{
    for (Vector<CameraStream *>::const_iterator it = mLocalStreams.begin(); it != mLocalStreams.end(); ++it) {
        if ((*it)->outBuffersInHal() == (int32_t) (*it)->getStream()->max_buffers)
            return false;
    }
    return true;
}

status_t
RequestThread::handleConfigureStreams(Message & msg)
{
    LOG1("@%s", __FUNCTION__);

    status_t status = NO_ERROR;
    mLastSettings.clear();
    mWaitingRequest = NULL;

    uint32_t streamsNum = msg.data.streams.list->num_streams;
    int inStreamsNum = 0;
    int outStreamsNum = 0;
    camera3_stream_t *stream = NULL;
    CameraStream * s = NULL;
    LOG1("Received %d streams :", streamsNum);
    // Check number and type of streams
   for (uint32_t i = 0; i < streamsNum; i++) {
        stream = msg.data.streams.list->streams[i];
        LOG1("Config stream (%s): %dx%d, fmt %s, max buffers:%d, priv %p",
                METAID2STR(android_scaler_availableStreamConfigurations_values,stream->stream_type),
                stream->width, stream->height,
                METAID2STR(android_scaler_availableFormats_values, stream->format),
                stream->max_buffers, stream->priv);
        if (stream->stream_type == CAMERA3_STREAM_OUTPUT)
            outStreamsNum++;
        else if (stream->stream_type == CAMERA3_STREAM_INPUT)
            inStreamsNum++;
        else if (stream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
            inStreamsNum++;
            outStreamsNum++;
        } else {
            LOGE("Unknown stream type %d!", stream->stream_type);
            return BAD_VALUE;
        }
        if (inStreamsNum > 1) {
            LOGE("Too many input streams!");
            return BAD_VALUE;
        }
    }

    if (!outStreamsNum) {
        LOGE("No output streams!");
        return BAD_VALUE;
    }

    // Mark all streams as NOT active
    for (unsigned int i = 0; i < mStreams.size(); i++) {
        s = (CameraStream *)(mStreams.editItemAt(i)->priv);
        s->setActive(false);
    }

    // Create for new streams
    for (uint32_t i = 0; i < streamsNum; i++) {
        stream = msg.data.streams.list->streams[i];
        if (!stream->priv) {
            s = new CameraStream(mStreamSeqNo, stream,
                    reinterpret_cast<IRequestCallback*>(mResultProcessor));
            s->setActive(true);
            stream->priv = s;
            mStreams.push_back(stream);
            mStreamSeqNo++;
            mLocalStreams.push_back(s);
        } else {
            s = (CameraStream *)(stream->priv);
            s->setActive(true);
        }
     }

    // Delete inalive streams
    for (unsigned int i=0; i < mStreams.size();) {
        s = (CameraStream *)(mStreams.editItemAt(i)->priv);
        if (!s->isActive()) {
            delete s;
            s = NULL;
            mStreams.editItemAt(i)->priv = NULL;
            mLocalStreams.removeItemsAt(i);
            i = mStreams.removeItemsAt(i);
        } else {
            i++;
        }
    }

    status = mCameraHw->configStreams(mStreams);
    if (status != NO_ERROR) {
        LOGE("Error configuring the streams @%s:%d", __FUNCTION__, __LINE__);
        return status;
    }

    Vector<CameraStreamNode *> activeStreams;
    mMaxReqInProcess = MAX_REQUEST_IN_PROCESS_NUM;
    for (unsigned int i = 0; i < mStreams.size(); i++)
        activeStreams.push_back((CameraStreamNode *)(mStreams.editItemAt(i)->priv));

    status = mCameraHw->bindStreams(activeStreams);
    PERFORMANCE_TRACES_BREAKDOWN_STEP("Done");

    return status;

}

status_t
RequestThread::constructDefaultRequest(int type,
                                            camera_metadata_t** meta)
{
    Message msg;
    msg.id = MESSAGE_ID_CONSTRUCT_DEFAULT_REQUEST;
    msg.data.defaultRequest.type= type;
    msg.data.defaultRequest.request = meta;
    return mMessageQueue.send(&msg, MESSAGE_ID_CONSTRUCT_DEFAULT_REQUEST);

}

status_t
RequestThread::handleConstructDefaultRequest(Message & msg)
{
    LOG2("@%s", __FUNCTION__);
    int requestType = msg.data.defaultRequest.type;
    const camera_metadata_t* defaultRequest;
    defaultRequest = mCameraHw->getDefaultRequestSettings(requestType);
    *(msg.data.defaultRequest.request) = (camera_metadata_t*)defaultRequest;

    PERFORMANCE_TRACES_BREAKDOWN_STEP("Done");
    return (*(msg.data.defaultRequest.request)) ? NO_ERROR : NO_MEMORY;
}

status_t
RequestThread::processCaptureRequest(camera3_capture_request_t *request)
{
    Message msg;
    msg.id = MESSAGE_ID_PROCESS_CAPTURE_REQUEST;
    msg.data.request3.request3 = request;

    return mMessageQueue.send(&msg, MESSAGE_ID_PROCESS_CAPTURE_REQUEST);
}

// NO_ERROR: request process is OK (waiting for ISP mode change or shutter)
// BAD_VALUE: request is not correct
// else: request process failed due to device error
status_t
RequestThread::handleProcessCaptureRequest(Message & msg)
{
    LOG2("%s:", __FUNCTION__);
    status_t status = BAD_VALUE;

    /**
     * If our request queue capacity is N, the Nth request would be blocked util
     * one request is finished. The #N+1 request should never come according to spec
     */
    if(isRequestQFull()) {
        LOGE("Request coming when processing queue is full, this should never happen");
        return status;
    }

    Camera3Request *request;
    status = mRequestsPool.acquireItem(&request);
    if (status != NO_ERROR) {
        LOGE("Failed to acquire empty  Request from the pool (%d)", status);
        return status;
    }
    // Request counter
    mRequestsInHAL++;

    /**
     * Settings may be NULL in repeating requests but not in the first one
     * check that now.
     */
    if (msg.data.request3.request3->settings) {
        MetadataHelper::dumpMetadata(msg.data.request3.request3->settings);
        // This assignment implies a memcopy.
        // mLastSettings has a copy of the current settings
        mLastSettings = msg.data.request3.request3->settings;
    } else if (mLastSettings.isEmpty()) {
        status = BAD_VALUE;
        LOGE("ERROR: NULL settings for the first request!");
        goto badRequest;
    }

    status = request->init(msg.data.request3.request3,
                           mResultProcessor,
                           mLastSettings, mCameraId);
    if (status != NO_ERROR) {
        LOGE("Failed to initialize Request (%d)", status);
        goto badRequest;
    }

    // HAL should block user to send this new request when:
    //   1. Request Q is full
    //   2. When the request requires reconfiguring the ISP in a manner which
    //      requires stopping the pipeline and emptying the driver from buffers
    //   3. when any of the streams has all buffers in HAL

    // Send for capture
    status = captureRequest(request);
    if (status == WOULD_BLOCK) {
        // Need ISP reconfiguration
        mRequestBlocked = true;
        mWaitingRequest = request;
        return NO_ERROR;
    } else if (status != NO_ERROR) {
        status =  UNKNOWN_ERROR;
        goto badRequest;
    }

    if (isRequestQFull() || !areAllStreamsUnderMaxBuffers()) {
        // Request Q is full
        mRequestBlocked = true;
    }
    return NO_ERROR;

badRequest:
    request->deInit();
    mRequestsPool.releaseItem(request);
    mRequestsInHAL--;
    return status;
}

int
RequestThread::returnRequest(Camera3Request* req)
{
    Message msg;
    msg.id = MESSAGE_ID_REQUEST_DONE;
    msg.request = req;
    msg.data.streamOut.reqId = req->getId();
    mMessageQueue.send(&msg);

    return 0;
}
int
RequestThread::handleReturnRequest(Message & msg)
{
    Camera3Request* request = msg.request;
    status_t status = NO_ERROR;

    request->deInit();
    mRequestsPool.releaseItem(request);
    mRequestsInHAL--;
    // Check blocked request
    if (mRequestBlocked) {
        if (mWaitingRequest != NULL && mRequestsInHAL == 1) {
            status = captureRequest(mWaitingRequest);
            if (status != NO_ERROR) {
                mWaitingRequest->deInit();
                mRequestsPool.releaseItem(mWaitingRequest);
                mRequestsInHAL--;
            }
            mWaitingRequest = NULL;
        }
        if (mWaitingRequest == NULL) {
            if (areAllStreamsUnderMaxBuffers()) {
                mRequestBlocked = false;
                mMessageQueue.reply(MESSAGE_ID_PROCESS_CAPTURE_REQUEST, status);
            }
        }
    }

    if (mFlushing && !mRequestsInHAL) {
        mMessageQueue.reply(MESSAGE_ID_FLUSH, NO_ERROR);
        mFlushing = false;
    }

    return 0;
}

status_t RequestThread::flush(void)
{
    // if hal version >= CAMERA_DEVICE_API_VERSION_3_1, we need to support flush()
    // this is the implement for the dummy flush, it will wait all the request to finish and then return.
    // flush() should only return when there are no more outstanding buffers or requests left in the HAL.
    // flush() must return within 1000ms
    // TODO: write one full version flush()

    nsecs_t startTime = systemTime();
    nsecs_t interval = 0;
    while (mRequestsInHAL > 0 && interval / 1000 <= 1000000) { // wait 1000ms at most
        usleep(10000); // wait 10ms
        interval = systemTime() - startTime;
    }

    LOG2("@%s, line:%d, mRequestsInHAL:%d, time spend:%lldus", __FUNCTION__, __LINE__, mRequestsInHAL, interval / 1000);

    if (interval / 1000 > 1000000) {
        LOGE("@%s, the flush() > 1000ms, time spend:%lldus", __FUNCTION__, interval / 1000);
        return -ENODEV;
    }

    return 0;
}

void
RequestThread::messageThreadLoop(void)
{
    LOG2("%s: Start", __func__);
    while (1) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);
        PERFORMANCE_HAL_ATRACE_PARAM1("msg", msg.id);
        if (msg.id == MESSAGE_ID_EXIT) {
            if (mRequestBlocked) {
                mRequestBlocked = false;
                LOG1("%s: exit - replying", __FUNCTION__);
                mMessageQueue.reply(MESSAGE_ID_PROCESS_CAPTURE_REQUEST, NO_INIT);
            }
            LOG1("%s: EXIT", __FUNCTION__);
            break;
        }

        if (mFlushing && msg.id != MESSAGE_ID_REQUEST_DONE) {
             mMessageQueue.reply(msg.id, INVALID_OPERATION);
        }

        bool replyImmediately = true;
        switch (msg.id) {
        case MESSAGE_ID_CONFIGURE_STREAMS:
            status = handleConfigureStreams(msg);
            break;
        case MESSAGE_ID_CONSTRUCT_DEFAULT_REQUEST:
            status = handleConstructDefaultRequest(msg);
            break;
        case MESSAGE_ID_PROCESS_CAPTURE_REQUEST:
            status = handleProcessCaptureRequest(msg);
            replyImmediately = !mRequestBlocked;
            break;
        case MESSAGE_ID_REQUEST_DONE:
            status = handleReturnRequest(msg);
            break;
        case MESSAGE_ID_FLUSH:
            break;
        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("    error %d in handling message: %d", status, (int)msg.id);
        if (replyImmediately)
            mMessageQueue.reply(msg.id, status);

    }

    LOG2("%s: Exit", __FUNCTION__);
}

status_t
RequestThread::captureRequest(Camera3Request* request)
{
    status_t status;
    CameraStream *stream = NULL;

    status = mResultProcessor->registerRequest(request);
    if (status != NO_ERROR) {
        LOGE("Error registering request to result Processor- bug");
        return status;
    }

    status = mCameraHw->processRequest(request,mRequestsInHAL);
    if (status ==  WOULD_BLOCK) {
        return status;
    }

    // handle output buffers

    const Vector<CameraStreamNode*>* outStreams = request->getOutputStreams();
    if (CC_UNLIKELY(outStreams == NULL)) {
        LOGE("there is no output streams. this should not happen");
        return BAD_VALUE;
    }
    CameraStreamNode* streamNode = NULL;
    for (unsigned int i = 0; i < outStreams->size(); i++) {
        streamNode = outStreams->itemAt(i);
        stream = reinterpret_cast<CameraStream *>(streamNode);
        stream->processRequest(request);
    }

    const Vector<CameraStreamNode*>* inStreams = request->getInputStreams();
    if (inStreams != NULL) {
        for (unsigned int i = 0; i < inStreams->size(); i++) {
            streamNode = inStreams->itemAt(i);
            stream = reinterpret_cast<CameraStream *>(streamNode);
            stream->processRequest(request);
        }
    }

    return status;
}

void RequestThread::dump(int fd)
{
    LOG2("@%s", __FUNCTION__);

    String8 message;
    message.appendFormat(LOG_TAG ":@%s\n", __FUNCTION__);
    message.appendFormat(LOG_TAG
            ":@%s requests in hal:%d blocked requests:%d\n",
            __FUNCTION__, mRequestsInHAL, mWaitingRequest == NULL ? 0 : 1);
    write(fd, message.string(), message.size());

}

} /* namespace camera2 */
} /* namespace android */
