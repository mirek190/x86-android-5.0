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

#ifndef CAMERA3_HAL_CONTROLUNIT_H_
#define CAMERA3_HAL_CONTROLUNIT_H_

#include "MessageQueue.h"
#include "MessageThread.h"
#include "ProcessingUnit.h"
#include "CaptureUnit.h"
#include "Intel3aPlus.h"
#include "ItemPool.h"

namespace android {
namespace camera2 {

/**
 * \class ControlUnit
 *
 * ControlUnit class control the request flow between Capture Unit and
 * Processing Unit. It uses the Intel3Aplus to process 3A settings for
 * each request and to run the 3A algorithms.
 *
 */
class ControlUnit : public IMessageHandler, public ICaptureEventListener {
public:
    explicit ControlUnit(ProcessingUnit *thePU, CaptureUnit *theCU, int CameraId);
    virtual ~ControlUnit();

    status_t init();
    status_t configStreamsDone(bool configChanged);

    status_t processRequest(Camera3Request* request);

    /* ICaptureEventListener interface*/
    bool notifyCaptureEvent(CaptureMessage *captureMsg);

public:  /* types */
    /**
     * \enum AlgorithmState
     * Describes the state for all the camera control algorithms in the Control
     * Unit, this is AE, AF and AWB
     */
    typedef enum {
        ALGORITHM_NOT_CONFIG,   /* init state  */
        ALGORITHM_CONFIGURED,   /* request is analyzed AIQ is configured  */
        ALGORITHM_READY,        /* input parameters READY */
        ALGORITHM_RUN           /* algorithm run already, output settings
                                   available */
    }AlgorithmState;

    /**
     * \struct RequestCtrlState
     * Contains the AIQ configuration derived from analyzing the user request
     * settings. This configuration will be applied before running 3A algorithms
     * It also tracks the status of each algortihtm for this request
     */
    typedef struct {
        void init(Camera3Request *req);
        CameraMetadata* ctrlUnitResult; /*!< metadata results written in the
                                             context of the ControlUnit */
        Camera3Request* request;     /*!< user request associated to this AIQ
                                      *   configuration */
        bool controlModeAuto;        /*!< true if 3A algorithms run in auto
                                      *   or semi-auto mode. False if 3A
                                      *   algorithms are completely by passed */
        AiqInputParams      aiqInputParams;
        AiqCaptureSettings  captureSettings; /*!< Results from 3A calculations */

        AlgorithmState  afState;
        AlgorithmState  aeState;
        AlgorithmState  awbState;

        //TODO add here the ProcessingUnit parameters for all pipelines
    } RequestCtrlState;

private:  /* private types */
    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,

        MESSAGE_ID_NEW_REQUEST,
        MESSAGE_ID_NEW_IMAGE,
        MESSAGE_ID_NEW_AF_STAT,
        MESSAGE_ID_NEW_2A_STAT,
        MESSAGE_ID_NEW_SENSOR_METADATA,
        MESSAGE_ID_NEW_SENSOR_DESCRIPTOR,
        MESSAGE_ID_NEW_SHUTTER,
        MESSAGE_ID_MAX
    };

    struct MessageGeneric {
        bool enable;
    };

    struct MessageImage {
        unsigned int frame_number;
        CaptureBuffer *buffer;
    };

    struct MessageRequest {
        unsigned int frame_number;
        RequestCtrlState *state;
    };

    struct MessageStats {
            unsigned int frame_number;  // exposure id
            void* aaStats;          // TODO: add pointer to 2A stats structure
            // TODO: add pointer to AF stats structure
    };

    struct MessageShutter {
        int requestId;
        int64_t tv_sec;
        int64_t tv_usec;
    };

    // union of all message data
    union MessageData {
        MessageGeneric generic;
        MessageImage image;
        MessageRequest request;
        MessageStats stats;
        MessageShutter shutter;
    };

    // message id and message data
    struct Message {
        MessageId id;
        MessageData data;
        Camera3Request* request;
    };

private:  /* Constants*/
    /**
     *  Maximum number of dynamic tags that the Control Unit writes as part of
     *   a result. This constant is used for the allocation of m3AResult
     *
     */
    static const int MAX_CONTROL_TAG_COUNT = 200;

private:  /* Methods */
    status_t requestExitAndWait();
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);
    status_t handleMessageExit(void);
    status_t handleNewRequest(Message &msg);
    status_t handleNewImage(Message &msg);
    status_t handleNewStat(Message &msg);
    status_t handleNewSensorMetadata(void);
    status_t handleNewSensorDescriptor(void);
    status_t handleNewShutter(Message &msg);

    /* Parameter processing methods */
    status_t processRequestSettings(const CameraMetadata  &settings,
                                    RequestCtrlState &aiqCfg);
    status_t processAwbSettings(const CameraMetadata  &settings,
                                RequestCtrlState &reqAiqCfg);
    status_t processAfSettings(const CameraMetadata  &settings,
                               RequestCtrlState &reqAiqCfg);
    status_t processAeSettings(const CameraMetadata  &settings,
                               RequestCtrlState &reqAiqCfg);
    void processAeStateMachine(const CameraMetadata &settings,
                               RequestCtrlState &reqAiqCfg);
    void processAutoAeStates(camera_metadata_enum_android_control_ae_state androidAeState);

    void writeSensorMetadata(RequestCtrlState* reqState);
    void writeAwbMetadata(RequestCtrlState* reqState);
    status_t run2AandCapture(RequestCtrlState *reqState);
    bool statisticsAvailable(RequestCtrlState* reqState, int* index);

private:  /* Members */
    ItemPool<RequestCtrlState>  mRequestStatePool;
    KeyedVector<int, RequestCtrlState*> mWaitingForStats;
    KeyedVector<int, RequestCtrlState*> mWaitingForImage;
    KeyedVector<int, void*> mAvailableStatistics;
    ProcessingUnit *mProcessingUnit;
    CaptureUnit    *mCaptureUnit;
    Intel3aPlus    *m3aWrapper;
    int             mCameraId;
    int             mCurrentAeTriggerId;
    bool            mForceAeLock;
    bool            mFirstRequest;  /*!< the next request is the first after
                                         streamconfig*/
    /**
     * Thread control members
     */
    bool mThreadRunning;
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThread;
    ia_aiq_exposure_sensor_descriptor mSensorDescriptor;
};  // class ControlUnit

};  // namespace camera2
};  // namespace android

#endif  // CAMERA3_HAL_CONTROLUNIT_H_
