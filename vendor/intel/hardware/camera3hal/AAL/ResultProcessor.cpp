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

#define LOG_TAG "Camera_ResultProcessor"

#include "ResultProcessor.h"
#include "Camera3Request.h"
#include "RequestThread.h"
#include "PlatformData.h"
#include "PerformanceTraces.h"

namespace android {
namespace camera2 {

ResultProcessor::ResultProcessor(RequestThread * aReqThread,
                                 const camera3_callback_ops_t * cbOps) :
    mRequestThread(aReqThread),
    mMessageQueue("ResultProcessor", MESSAGE_ID_MAX),
    mCallbackOps(cbOps),
    mThreadRunning(true),
    mPartialResultCount(0),
    mNextRequestId(0)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    // TODO: Move to 2 phase constructor.
    mMessageThread = new MessageThread(this,"ResultProcessor");
    if (mMessageThread == NULL) {
        LOGE("Error create StreamThread");
    }
    mReqStatePool.init(MAX_REQUEST_IN_TRANSIT);
    mRequestsPendingMetaReturn.setCapacity(MAX_REQUEST_IN_TRANSIT);
}

ResultProcessor::~ResultProcessor()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    if (mMessageThread != NULL) {
        mMessageThread->requestExitAndWait();
        mMessageThread.clear();
        mMessageThread = NULL;
    }
    mRequestsPendingMetaReturn.clear();
    mRequestsInTransit.clear();
}

/**********************************************************************
 * Public methods
 */
/**********************************************************************
 * Thread methods
 */
status_t ResultProcessor::requestExitAndWait(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    mMessageQueue.send(&msg);
    return NO_ERROR;
}
status_t ResultProcessor::handleMessageExit(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mThreadRunning = false;
    return NO_ERROR;
}

/**
 * registerRequest
 *
 * Present a request to the ResultProcessor.
 * This call is used to inform the result processor that a new request
 * has been sent to the PSL. REquestThread uses this method
 * ResultProcessor will store its state in an internal vector to track
 * the different events during the lifetime of the request.
 *
 * Once the request has been completed ResultProcessor returns the request
 * to the RequestThread for recycling, using the method:
 * RequestThread::returnRequest();
 *
 * \param request [IN] item to register
 * \return NO_ERROR
 */
status_t ResultProcessor::registerRequest(Camera3Request *request)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE);
    Message msg;
    msg.id = MESSAGE_ID_REGISTER_REQUEST;
    msg.request = request;
    return mMessageQueue.send(&msg, MESSAGE_ID_REGISTER_REQUEST);
}

status_t ResultProcessor::handleRegisterRequest(Message &msg)
{
    status_t status = NO_ERROR;
    RequestState_t* reqState;
    int reqId = msg.request->getId();
    /**
     * check if the request was not already register. we may receive registration
     * request duplicated in case of request that are held by the PSL
     */
    if(mRequestsInTransit.indexOfKey(reqId) != NAME_NOT_FOUND) {
        return NO_ERROR;
    }

    status = mReqStatePool.acquireItem(&reqState);
    if (status != NO_ERROR) {
        LOGE("Could not acquire an empty reqState from the pool");
        return status;
    }

    reqState->init(msg.request);
    mRequestsInTransit.add(reqState->reqId, reqState);
    LOGR("<request %d> registered @ ResultProcessor", reqState->reqId);
    /**
     * get the number of partial results the request may return, this is not
     *  going to change once the camera is open, so do it only once.
     *  We initialize the value to 0, the minimum value should be 1
     */
    if (CC_UNLIKELY(mPartialResultCount == 0)) {
        mPartialResultCount = msg.request->getpartialResultCount();
    }
    return status;
}

void ResultProcessor::messageThreadLoop(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;
        Message msg;
        mMessageQueue.receive(&msg);
        PERFORMANCE_HAL_ATRACE_PARAM1("msg", msg.id);
        Camera3Request* request = msg.request;
        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            status = handleMessageExit();
            break;
       case MESSAGE_ID_SHUTTER_DONE:
           status = handleShutterDone(msg);
           break;
       case MESSAGE_ID_METADATA_DONE:
           status = handleMetadataDone(msg);
           break;
       case MESSAGE_ID_BUFFER_DONE:
           status = handleBufferDone(msg);
           break;
       case MESSAGE_ID_REGISTER_REQUEST:
           status = handleRegisterRequest(msg);
           break;
        default:
           status = BAD_VALUE;
           break;
        }
        mMessageQueue.reply(msg.id, status);
    }
}

status_t ResultProcessor::shutterDone(Camera3Request* request,
                                      int64_t timestamp)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE);
    Message msg;
    msg.id = MESSAGE_ID_SHUTTER_DONE;
    msg.request = request;
    msg.data.shutter.time = timestamp;

    return mMessageQueue.send(&msg);
}

status_t ResultProcessor::handleShutterDone(Message &msg)
{
    status_t status = NO_ERROR;
    int reqId = 0;
    Camera3Request* request = msg.request;

    reqId = request->getId();
    LOGR("%s for <Request : %d", __FUNCTION__, reqId);
    int index = mRequestsInTransit.indexOfKey(reqId);
    if (index == NAME_NOT_FOUND) {
        LOGE("Request %d was not registered find the bug", reqId);
        return BAD_VALUE;
    }
    RequestState_t *reqState = mRequestsInTransit.valueAt(index);

    reqState->shutterTime = msg.data.shutter.time;
    if (mNextRequestId != reqId) {
        LOGW("shutter done received ahead of time, expecting %d got %d Or discontinuities requests received.",
                mNextRequestId, reqId);
        reqState->shutterReceived = true;
    }

    returnShutterDone(reqState);

    /**
     *  <v3.1 behavior>
     *  if buffers came already they should be in pending buffers
     *  the order of return is :
     *  - shutter callback
     *  - buffers/meta (any order)
     */
    if (!reqState->pendingBuffers.isEmpty()) {
        returnPendingBuffers(reqState);
    }
    /**
     *  <v3.1 behavior>
     *  V3.1 only supports returning the result metadata in one piece. Check
     *  for that and return it if possible.
     */
    unsigned int resultsReceived = reqState->pendingPartialResults.size();
    bool allMetaReceived = (resultsReceived == mPartialResultCount);

    if (allMetaReceived) {
        returnPendingPartials(reqState);
    }

    bool allMetaDone = (reqState->partialResultReturned == mPartialResultCount);
    bool allBuffersDone = (reqState->buffersReturned == reqState->buffersToReturn);
    if (allBuffersDone && allMetaDone) {
        status = recycleRequest(request);
    }

    return status;
}

/**
 * returnShutterDone
 * Signal to the client that shutter event was received
 * \param reqState [IN/OUT] state of the request
 */
void ResultProcessor::returnShutterDone(RequestState_t* reqState)
{
    if (reqState->isShutterDone)
        return;

    camera3_notify_msg shutter;
    shutter.type = CAMERA3_MSG_SHUTTER;
    shutter.message.shutter.frame_number = reqState->reqId;
    shutter.message.shutter.timestamp =reqState->shutterTime;
    mCallbackOps->notify(mCallbackOps, &shutter);
    reqState->isShutterDone = true;
    mNextRequestId = reqState->nextReqId;
    LOGR("<Request %d> shutter done", reqState->reqId);
}

status_t ResultProcessor::metadataDone(Camera3Request* request,
                                       int resultIndex)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE);
    Message msg;
    msg.id = MESSAGE_ID_METADATA_DONE;
    msg.request = request;
    msg.data.meta.resultIndex = resultIndex;

    return mMessageQueue.send(&msg);
}

status_t ResultProcessor::handleMetadataDone(Message &msg)
{
    status_t status = NO_ERROR;
    Camera3Request* request = msg.request;
    int reqId = request->getId();
    LOGR("%s for <Request %d>", __FUNCTION__, reqId);

    int index = mRequestsInTransit.indexOfKey(reqId);
    if (index == NAME_NOT_FOUND) {
        LOGE("Request %d was not registered:find the bug", reqId);
        return BAD_VALUE;
    }
    RequestState_t *reqState = mRequestsInTransit.valueAt(index);

    if (msg.data.meta.resultIndex >= 0) {
        /**
         * New Partial metadata result path. The result buffer is not the
         * settings but a separate buffer stored in the request.
         * The resultIndex indicates which one.
         * This can be returned straight away now that we have declared 3.2
         * device version. No need to enforce the order between shutter events
         * result and buffers. We do not need to store the partials either.
         * we can return them directly
         */
        status = returnResult(reqState, msg.data.meta.resultIndex);
        return status;
    }
    /**
     * FROM THIS POINT TO THE END OF THIS METHOD IS OLD CODE PATH
     * This code path will be clean up once all PSL implement the return of
     * metadata results using a different buffer than the settings buffer
     */
    ReadWriteRequest rwRequest(request);
    reqState->pendingPartialResults.add(&rwRequest.mMembers.mSettings);
    LOGR(" <Request %d> Metadata arrived %d/%d", reqId,
            reqState->pendingPartialResults.size(),mPartialResultCount);
    /**
     * <v3.1 behavior>
     * metadata is only returned to client if shutter notification has already
     * been given to client as well. Leave now as we have already stored it
     * to the pending partial vector.
     */
    if (!reqState->isShutterDone) {
        LOGR("metadata arrived before shutter, storing");
        return NO_ERROR;
    }

    /**
     * <v3.1 behavior>
     * Return metadata when all partial results have been accumulated. This
     * is only 1 currently . No partial results possible yet
     */
    unsigned int resultsReceived = reqState->pendingPartialResults.size();
    bool allMetaReceived = (resultsReceived == mPartialResultCount);

    if (allMetaReceived) {
        returnPendingPartials(reqState);
    }

    bool allMetadataDone = (reqState->partialResultReturned == mPartialResultCount);
    bool allBuffersDone = (reqState->buffersReturned == reqState->buffersToReturn);
    if (allBuffersDone && allMetadataDone) {
        status = recycleRequest(request);
    }

    /**
     * if the metadata done for the next request is available then send it.
     *
     */
    if (allMetadataDone) {
        returnStoredPartials();
    }

    return status;
}

/**
 * returnStoredPartials
 * return the all stored pending metadata
 */
status_t ResultProcessor::returnStoredPartials()
{
    status_t status = NO_ERROR;
    while (mRequestsPendingMetaReturn.size() > 0) {
        LOGR("stored metadata req size:%d, first reqid:%d", mRequestsPendingMetaReturn.size(), mRequestsPendingMetaReturn[0]);
        int index = mRequestsInTransit.indexOfKey(mRequestsPendingMetaReturn[0]);
        if (index == NAME_NOT_FOUND) {
            LOGE("Request %d was not registered:find the bug", mRequestsPendingMetaReturn[0]);
            mRequestsPendingMetaReturn.removeAt(0);
            return BAD_VALUE;
        }

        RequestState_t * reqState = mRequestsInTransit.valueAt(index);
        returnPendingPartials(reqState);
        bool allMetadataDone = (reqState->partialResultReturned == mPartialResultCount);
        bool allBuffersDone = (reqState->buffersReturned == reqState->buffersToReturn);
        if (allBuffersDone && allMetadataDone) {
            status = recycleRequest(reqState->request);
        }

        mRequestsPendingMetaReturn.removeAt(0);
    }
    return status;
}


status_t ResultProcessor::bufferDone(Camera3Request* request,
                                     sp<CameraBuffer> buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE);
    Message msg;
    msg.id = MESSAGE_ID_BUFFER_DONE;
    msg.request = request;
    msg.buffer = buffer;

    return  mMessageQueue.send(&msg);
}

/**
 * handleBufferDone
 *
 * Try to return the buffer provided by PSL to client
 * This method checks whether we can return the buffer straight to client or
 * we need to hold it until shutter event has been received.
 * \param msg [IN] Contains the buffer produced by PSL
 */
status_t ResultProcessor::handleBufferDone(Message &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE);
    status_t status = NO_ERROR;
    Camera3Request* request = msg.request;
    sp<CameraBuffer> buffer = msg.buffer;

    if (buffer.get() && buffer->isLocked())
        buffer->unlock();

    int reqId = request->getId();
    int index = mRequestsInTransit.indexOfKey(reqId);
    if (index == NAME_NOT_FOUND) {
        LOGE("Request %d was not registered find the bug", reqId);
        return BAD_VALUE;
    }
    RequestState_t *reqState = mRequestsInTransit.valueAt(index);

    LOGR("<Request %d> buffer received from PSL", reqId);
    reqState->pendingBuffers.add(buffer);
    /**
     * <v3.1 behavior>
     * buffers can only be returned when shutter has been triggered.
     * if not the case then store them for later.
     */
    if (!reqState->isShutterDone) {
        LOGR("Buffer arrived before shutter req %d, queue it",reqId);
        return NO_ERROR;
    }

    returnPendingBuffers(reqState);

    /**
     *  <v3.1 behavior>
     *  if at least one buffers have been returned means shutter is done
     *  therefore we can also return metadata if it was received from PSL
     */

    if (!reqState->pendingPartialResults.isEmpty()) {
        returnPendingPartials(reqState);
    }

    bool allMetaDone = (reqState->partialResultReturned == mPartialResultCount);
    bool allBuffersDone = (reqState->buffersReturned == reqState->buffersToReturn);
    if (allBuffersDone && allMetaDone) {
        status = recycleRequest(request);
    }
    return status;
}

void ResultProcessor::returnPendingBuffers(RequestState_t* reqState)
{
    LOGR("@%s - req-%d  %d buffs", __FUNCTION__, reqState->reqId,
                                  reqState->pendingBuffers.size());
    unsigned int i;
    camera3_capture_result_t result;
    camera3_stream_buffer_t buf;
    sp<CameraBuffer> pendingBuf;
    Camera3Request* request = reqState->request;

    /**
     * protection against duplicated calls when all buffers have been returned
     */
    if(reqState->buffersReturned == reqState->buffersToReturn) {
        LOGW("trying to return buffers twice. Check PSL implementation");
        return;
    }

    for (i = 0; i < reqState->pendingBuffers.size(); i++) {
        CLEAR(buf);
        CLEAR(result);

        pendingBuf = reqState->pendingBuffers[i];
        if (!request->isInputBuffer(pendingBuf)) {
            result.num_output_buffers = 1;
        }
        result.frame_number = reqState->reqId;
        /**
         * TODO: in this class we should get already a camera3_stream_buffer
         * we cannot hardcode the fences, that is not a decision that the AAL
         * can take. The PSL should do that
         */
        buf.status = OK;
        buf.acquire_fence = -1;
        buf.release_fence = -1;
        buf.stream =  pendingBuf->getOwner()->getStream();
        buf.buffer =  pendingBuf->getBufferHandle();

        result.result = NULL;
        if (request->isInputBuffer(pendingBuf)) {
            result.input_buffer = &buf;
            LOGR(" <Request %d> return an input buffer", reqState->reqId);
        } else {
            result.output_buffers = &buf;
        }

        mCallbackOps->process_capture_result(mCallbackOps, &result);
        pendingBuf->getOwner()->decOutBuffersInHal();
        reqState->buffersReturned += 1;
        LOGR(" <Request %d> buffer done %d/%d ", reqState->reqId,
                 reqState->buffersReturned,reqState->buffersToReturn);
    }

    reqState->pendingBuffers.clear();
}

/**
 * Returns the single partial result stored in the vector.
 * In the future we will have more than one.
 */
void ResultProcessor::returnPendingPartials(RequestState_t* reqState)
{
    camera3_capture_result result;
    CLEAR(result);

    // it must be 1 for >= CAMERA_DEVICE_API_VERSION_3_2 if we don't support partial metadata
    result.partial_result = mPartialResultCount;

    //TODO: combine them all in one metadata buffer and return
    result.frame_number = reqState->reqId;
    // check if metadata result of the previous request is returned
    int pre_reqId = reqState->reqId - 1;
    int index = mRequestsInTransit.indexOfKey(pre_reqId);
    if (index != NAME_NOT_FOUND) {
        RequestState_t *pre_reqState = mRequestsInTransit.valueAt(index);
        if (pre_reqState->partialResultReturned == 0) {
            LOGR("wait the metadata of the previous request return");
            mRequestsPendingMetaReturn.add(reqState->reqId);
            return;
        }
    }

    CameraMetadata * settings = reqState->pendingPartialResults[0];

    result.result = settings->getAndLock();
    result.num_output_buffers = 0;

    mCallbackOps->process_capture_result(mCallbackOps, &result);

    settings->unlock(result.result);

    reqState->partialResultReturned += 1;
    LOGR("<Request %d> result cb done",reqState->reqId);
    reqState->pendingPartialResults.clear();
}

/**
 * returnResult
 *
 * Returns a partial result metadata buffer, just one.
 *
 * \param reqState[IN]: Request State control structure
 * \param returnIndex[IN]: index of the result buffer in the array of result
 *                         buffers stored in the request
 */
status_t ResultProcessor::returnResult(RequestState_t* reqState, int returnIndex)
{
    status_t status = NO_ERROR;
    camera3_capture_result result;
    CameraMetadata *resultMetadata;
    CLEAR(result);
    resultMetadata = reqState->request->getPartialResultBuffer(returnIndex);
    if (resultMetadata == NULL) {
        LOGE("Cannot get partial result buffer");
        return UNKNOWN_ERROR;
    }
    // This value should be between 1 and android.request.partialResultCount
    // The index goes between 0-partialResultCount -1
    result.partial_result = returnIndex + 1;
    result.frame_number = reqState->reqId;
    result.result = resultMetadata->getAndLock();
    result.num_output_buffers = 0;

    mCallbackOps->process_capture_result(mCallbackOps, &result);

    resultMetadata->unlock(result.result);

    reqState->partialResultReturned += 1;
    LOGR("<Request %d> result cb done", reqState->reqId);
    return status;
}

/**
 * Request is fully processed
 * send the request object back to RequestThread for recycling
 * return the request-state struct to the pool
 */
status_t ResultProcessor::recycleRequest(Camera3Request *req)
{
    status_t status = NO_ERROR;
    int id = req->getId();
    RequestState_t *reqState = mRequestsInTransit.valueFor(id);
    status = mReqStatePool.releaseItem(reqState);
    if (status != NO_ERROR) {
        LOGE("Request State pool failure, recycling is broken!!");
    }

    mRequestsInTransit.removeItem(id);
    mRequestThread->returnRequest(req);
    LOGR("<Request %d> OUT from ResultProcessor",id);
    return status;
}
//----------------------------------------------------------------------------
}; // namespace camera2
}; // namespace android
