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

#ifndef _CAMERA3_REQUESTTHREAD_H_
#define _CAMERA3_REQUESTTHREAD_H_

#include "MessageThread.h"
#include "MessageQueue.h"
#include "ResultProcessor.h"
#include "ItemPool.h"
#include <hardware/camera3.h>


namespace android {
namespace camera2 {

/**
 * \class RequestThread
 * Active object in charge of request management
 *
 * The RequestThread  is the in charge of controlling the flow of request from
 * the client to the HW class.
 */
class RequestThread: public IMessageHandler,
                     public MessageThread {
public:
    RequestThread(int cameraId, ICameraHw *aCameraHW);
    virtual ~RequestThread();

    status_t init(const camera3_callback_ops_t *callback_ops);
    status_t deinit(void);

    status_t configureStreams(camera3_stream_configuration_t *stream_list);
    status_t constructDefaultRequest(int type, camera_metadata_t** reques_settings);
    status_t processCaptureRequest(camera3_capture_request_t *request);
    status_t flush();

    /* IMessageHandler override */
    void messageThreadLoop(void);

    int returnRequest(Camera3Request* request);

    void dump(int fd);

private:  /* types */

    enum MessageId {
        MESSAGE_ID_EXIT = 0,            // call requestExitAndWait

        MESSAGE_ID_REQUEST_DONE,
        MESSAGE_ID_FLUSH,

        // For HAL API
        MESSAGE_ID_CONFIGURE_STREAMS,
        MESSAGE_ID_CONSTRUCT_DEFAULT_REQUEST,
        MESSAGE_ID_PROCESS_CAPTURE_REQUEST,

        MESSAGE_ID_MAX
    };

    struct MessageConfigureStreams {
        camera3_stream_configuration_t * list;
    };

    struct MessageRegisterStreamBuffers {
        const camera3_stream_buffer_set_t * set;
    };

    struct MessageConstructDefaultRequest {
        int type;
        camera_metadata_t ** request;
    };

    struct MessageProcessCaptureRequest {
        camera3_capture_request * request3;
    };

    struct MessageShutter {
        int reqId; // TODO: remove
        int64_t time;
    };

    struct MessageCaptureDone {
        int reqId; // TODO: remove
        //FrameBufferStatus frameStatus;   TODO: make this platform independent
        int64_t time;
        struct timeval timestamp;
    };

    struct MessageStreamOutDone {
        int reqId; // TODO: remove
        int finished;
        status_t status;
    };

    // union of all message data
    union MessageData {
        MessageConfigureStreams streams;
        MessageRegisterStreamBuffers buffers;
        MessageConstructDefaultRequest defaultRequest;
        MessageProcessCaptureRequest request3;
        MessageShutter shutter;
        MessageCaptureDone capture;
        MessageStreamOutDone streamOut;
    };

     // message id and message data
    struct Message {
        MessageId id;
        MessageData data;
        Camera3Request *request;
    };

private:  /* methods */

    status_t handleConfigureStreams(Message & msg);
    status_t handleConstructDefaultRequest(Message & msg);
    status_t handleProcessCaptureRequest(Message & msg);
    int handleReturnRequest(Message & msg);

    status_t registerInRequest(Camera3Request* request,
                             camera3_capture_request * request3);
    status_t registerOutRequest(Camera3Request* request,
                             camera3_capture_request * request3);
    status_t registerRequest(Camera3Request* request,
                             camera3_capture_request * request3);
    status_t captureRequest(Camera3Request* request);
    bool isRequestQFull(void) const {
        return (mRequestsInHAL >= mMaxReqInProcess);
    }
    bool isStreamValid(camera3_capture_request * request3);
    bool isRequestValid(camera3_capture_request * request3);

    bool areAllStreamsUnderMaxBuffers() const;

private:  /* members */
    int mCameraId;
    ICameraHw   *mCameraHw;
    MessageQueue<Message, MessageId> mMessageQueue;
    ItemPool<Camera3Request> mRequestsPool;

    int mRequestsInHAL;
    bool mFlushing;
    bool mRequestBlocked;  /*!< Block request handling, need waiting for HW
                                reconfiguration or available request in Q */
    Camera3Request* mWaitingRequest;  /*!< storage during need to wait for
                                             captures to be finished */
    CameraMetadata mLastSettings;
    int mMaxReqInProcess;

    bool mInitialized;  /*!< tracking the status of the RequestThread */
    /* *********************************************************************
     * Stream info
     */
    ResultProcessor* mResultProcessor;
    Vector<camera3_stream_t *> mStreams;
    Vector<CameraStream *> mLocalStreams;
    unsigned int mStreamSeqNo;

};

} /* namespace camera2 */
} /* namespace android */
#endif /* _CAMERA3_REQUESTTHREAD_H_ */
