/*
 * Copyright (C) 2013 Intel Corporation
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

#ifndef  _CAMERA3_HAL_RESULT_PROCESSOR_H_
#define  _CAMERA3_HAL_RESULT_PROCESSOR_H_

#include <utils/threads.h>
#include <utils/List.h>
#include <hardware/camera3.h>

#include "LogHelper.h"
#include "CameraStream.h"
#include "MessageQueue.h"
#include "MessageThread.h"
#include "ItemPool.h"


namespace android {
namespace camera2 {

/**
 * Forward declarations to avoid circular references of  header files
 */
class RequestThread;
class Camera3Request;


/**
 * \class ResultProcessor
 * This class is responsible for managing the return of completed requests to
 * the HAL client.
 *
 * PSL implementations may return shutter notification, buffers and metadata
 * in any order. The ResultProcessor is in charge of ensuring the
 * callbacks follow the correct order or return the corresponding error.
 *
 * The result processor tracks the relevant events in the life-cycle of a
 * request, these are:
 * - shutter notification
 * - buffer return
 * - partial metadata return
 *
 * The current implementation abides to the rules set in the device API v3.1
 * Some significant changes related to the callback order coming in 3.2 mean
 * that parts of this class will need to be modified. The PSL should remain
 * unaffected.
 * To track the parts affected by the API version search for <v3.1 behavior>
 */
class ResultProcessor : public IRequestCallback, public IMessageHandler {
public:
    ResultProcessor(RequestThread * aReqThread,
                    const camera3_callback_ops_t * callbackOps);
    virtual ~ResultProcessor();
    status_t requestExitAndWait();
    status_t registerRequest(Camera3Request *request);

    //IRequestCallback implementation
    virtual status_t shutterDone(Camera3Request* request, int64_t timestamp);
    virtual status_t metadataDone(Camera3Request* request,
                                  int resultIndex = -1);
    virtual status_t bufferDone(Camera3Request* request,
                               sp<CameraBuffer> buffer);

private:  /* types  and constants */
    /**
     * \struct RequestState_t
     * This struct is used to track the life cycle of a request.
     * ResultProcessor keeps a Vector with the states of the requests currently
     * in the PSL.
     * As the PSL reports partial completion using the IRequestCallback interface
     * the values in this structure are updated.
     * Always in the context of the ResultProcesssor Thread, avoiding the need
     * of mutex
     * locking
     **/
    typedef struct  {
        int reqId;
        int nextReqId;
        Camera3Request *request;

        /**
         * Shutter control variables
         */
        bool isShutterDone;     /*!> from AAL to client */
        bool shutterReceived;   /*!> from PSL to AAL */
        int64_t shutterTime;
        /**
         * Metadata result control variables
         */
        unsigned int partialResultReturned;  /*!> from AAL to client */
        Vector<CameraMetadata*> pendingPartialResults;
        /**
         * Output buffers control variables
         */
        unsigned int buffersReturned;  /*!> from AAL to client */
        unsigned int buffersToReturn;  /*!> total output buffer count of request */
        Vector<sp<CameraBuffer> > pendingBuffers; /*!> where we store the
                                                              buffers received
                                                              from PSL*/

        void init(Camera3Request* req) {
            pendingBuffers.clear();
            pendingPartialResults.clear();
            reqId = req->getId();
            nextReqId = reqId + 1;
            shutterTime = 0;
            shutterReceived = false;
            isShutterDone = false;
            partialResultReturned = 0;
            buffersReturned = 0;
            buffersToReturn = req->getNumberOutputBufs() + req->getNumberInputBufs();
            request = req;
        }
    } RequestState_t;

    static const int MAX_REQUEST_IN_TRANSIT = 10;    /*!> worst case number used
                                                         for pool allocation */

    enum MessageId {
        MESSAGE_ID_EXIT = 0,            // call requestExitAndWait
        MESSAGE_ID_SHUTTER_DONE,
        MESSAGE_ID_METADATA_DONE,       // partial metadata
        MESSAGE_ID_BUFFER_DONE,
        MESSAGE_ID_REGISTER_REQUEST,
        // max number of messages
        MESSAGE_ID_MAX
    };

    struct MessageMetadataDone {
        int resultIndex;    /*!> Index from the partial result array that is
                                 being returned */
     };

    struct MessageShutterDone {
        int64_t time;
     };

    union MessageData {
        MessageMetadataDone meta;
        MessageShutterDone shutter;
    };

    /**
     * \struct
     * Result processor message structure
     */
    struct Message {
        MessageId id;
        MessageData data;
        // common, more complex fields, that can't be put in a union like MessageData
        Camera3Request* request;  // any sent request
        sp<CameraBuffer> buffer;
    };

private:  /* methods */
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);
    status_t handleMessageExit();
    status_t handleShutterDone(Message &msg);
    status_t handleMetadataDone(Message &msg);
    status_t handleBufferDone(Message & msg);
    status_t handleRegisterRequest(Message &msg);
    status_t recycleRequest(Camera3Request *req);
    void returnPendingBuffers(RequestState_t *reqState);
    void returnPendingPartials(RequestState_t *reqState);
    void returnShutterDone(RequestState_t *reqState);
    status_t returnStoredPartials(void);
    status_t returnResult(RequestState_t* reqState, int returnIndex);

private:  /* members */
    RequestThread *mRequestThread;
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThread;
    const camera3_callback_ops_t *mCallbackOps;
    bool mThreadRunning;
    ItemPool<RequestState_t> mReqStatePool;

    KeyedVector<int, RequestState_t*>  mRequestsInTransit;
    unsigned int mPartialResultCount;
    int mNextRequestId;     /*!> used to ensure shutter callbacks are sequential*/
    SortedVector<int> mRequestsPendingMetaReturn; /*!> Sorted List of request id's that have metadata
                                                       ready for return. The metadata for that request
                                                       id should be present in the mRequestInTransit vector. */
};

}; // namespace camera2
}; // namespace android

#endif //  _CAMERA3_HAL_RESULT_PROCESSOR_H_
