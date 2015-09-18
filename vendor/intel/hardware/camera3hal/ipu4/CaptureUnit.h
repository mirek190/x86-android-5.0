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

#ifndef CAMERA3_HAL_CAPTUREUNIT_H_
#define CAMERA3_HAL_CAPTUREUNIT_H_


#include "MessageQueue.h"
#include "MessageThread.h"
#include "SensorHw.h"
#include "Camera3Request.h"
#include "MediaCtlPipeConfig.h"
#include "CaptureBuffer.h"
#include "ItemPool.h"
#include "InputSystem.h"
#include <utils/KeyedVector.h>
#include <utils/List.h>

/**
 * default value in case static metadata does not define it
 */
#define DEFAULT_PIPELINE_DEPTH 4


namespace android {
namespace camera2 {

class MediaController;
class SensorHw;
class V4L2VideoNode;
class CSS2600HwStream;

/**
 * \struct AiqCaptureSettings
 * Contains the results from AIQ (3A + AIC) algorithms
 * These settings are sent through CaptureUnit to sensor, lens and ISP.
 */
typedef struct {
    bool aeEnabled; // FIXME: Temporary to fix AE when not ready and enable Manual exposure.
    AiqResults aiqResults;
    // ToDo: ia_isp_xx results once available
} AiqCaptureSettings;

/**
 * \class ICaptureEventListener
 *
 * Abstract interface implemented by entities interested on receiving notifications
 * from the IPU4 input system.
 *
 * Notifications are sent for AF/2A statistics, histogram and RAW frames.
 */
class ICaptureEventListener {
public:

    enum CaptureMessageId {
        CAPTURE_MESSAGE_ID_EVENT = 0,
        CAPTURE_MESSAGE_ID_ERROR
    };

    enum CaptureEventType {
        CAPTURE_EVENT_MIPI_COMPRESSED = 0,
        CAPTURE_EVENT_MIPI_UNCOMPRESSED,
        CAPTURE_EVENT_RAW_BAYER,
        CAPTURE_EVENT_RAW_BAYER_SCALED,
        CAPTURE_EVENT_AF_STATISTICS,
        CAPTURE_EVENT_2A_STATISTICS,
        CAPTURE_EVENT_AE_HISTOGRAM,
        CAPTURE_EVENT_NEW_SENSOR_DESCRIPTOR,
        CAPTURE_EVENT_SHUTTER,
        CAPTURE_EVENT_MAX
    };

    // For MESSAGE_ID_EVENT
    struct CaptureMessageEvent {
        CaptureEventType type;
        struct timeval   timestamp;
        unsigned int     sequence;
        CaptureBuffer*   buffer;
    };

    // For MESSAGE_ID_ERROR
    struct CaptureMessageError {
        status_t code;
    };

    union CaptureMessageData {
        CaptureMessageEvent event;
        CaptureMessageError error;
    };

    struct CaptureMessage {
        CaptureMessageId   id;
        CaptureMessageData data;
    };

    virtual bool notifyCaptureEvent(CaptureMessage *msg) = 0;
    virtual ~ICaptureEventListener() {};

}; // ICaptureEventListener

class CaptureUnit : public RefBase, public IMessageHandler, public IBufferOwner, public IISysObserver
{
public:
    CaptureUnit(int camId);
    ~CaptureUnit();

    status_t init();

    status_t flush();
    status_t configStreams(Vector<camera3_stream_t*> &activeStreams, bool &configChanged);

    status_t doCapture(Camera3Request* request,
                       AiqCaptureSettings &aiqCaptureSettings);

    status_t getSensorData(ia_aiq_exposure_sensor_descriptor* aiqSensorDescriptor);
    status_t getOutputConfig(FrameInfo &info);
    int getSensorSettingDelay() { return sSensorSettingsDelay; }

    // IBufferOwner interface
    virtual void returnBuffer(CaptureBuffer* buffer);

    // IISysObserver interface
    virtual void notifyIsysEvent(IsysMessage &msg);

    /************************************************************
     * Listener management methods
     */
    status_t attachListener(ICaptureEventListener *aListener, ICaptureEventListener::CaptureEventType event);
    status_t detachListener(ICaptureEventListener *aListener, ICaptureEventListener::CaptureEventType event);


// private methods
private:
    status_t getMediaCtlCamConfig(bool &needIsysRestart);
    camera3_stream_t* findRawStream();
    status_t findRawBuffer(Camera3Request *request, CaptureBuffer **buf);
    status_t createBufferPools(int numBufs);
    status_t createCaptureBufferPool(int numBufs, VideoNodeType nodetype);
    status_t allocateCaptureBuffers(int numBufs);
    status_t freeBuffers();
    status_t applySensorParams(Camera3Request* request, AiqCaptureSettings* aiqCaptureSettings);
    status_t enqueueBuffers(Camera3Request *request);
    void initStaticMetadata(void);

    // thread message IDs
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_CAPTURE,
        MESSAGE_ID_FLUSH,
        MESSAGE_ID_RETURN_BUFFER,
        MESSAGE_ID_ISYS_EVENT,
        MESSAGE_ID_MAX
    };

    struct MessageRequest {
        Camera3Request* request;
        AiqCaptureSettings* settings;
    };

    struct MessageBuffer {
        CaptureBuffer* captureBufPtr;
        struct v4l2_buffer_info v4l2Buf;
        VideoNodeType nodeType;
        int requestId;
    };

    union MessageData {
        MessageRequest request;
        MessageBuffer  buffer;
    };

    struct Message {
        MessageId id;
        MessageData data;
    };

    status_t handleMessageFlush(Message &msg);
    status_t handleMessageCapture(Message &msg);
    status_t handleMessageReturnBuffer(Message &msg);
    status_t handleMessageIsysEvent(Message &msg);
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

    status_t notifyListeners(ICaptureEventListener::CaptureMessage *msg);

private:
    int     mCameraId;

    sp<MediaController>    mMediaCtl;
    sp<InputSystem>        mIsys;
    sp<SensorHw>           mSensor;

    Vector<camera3_stream_t *>       mActiveStreams;
    ItemPool<CaptureBuffer>          mCaptureItemsPool;     /*!< Pool of V4L2 buffers registered to the device. This pool
                                                                 also maintains the relationship between the V4L2 buffers
                                                                 and CameraBuffers that are used by other IPU4 classes. */
    Vector<CaptureBuffer*>           mQueuedCaptureBuffers; /*!< Buffers that are currently queued to the V4L2 device. */

    ItemPool<sp<CameraBuffer> >      mRawBufPool;           /*!< Pool of internally allocated buffers. */
    Vector<sp<CameraBuffer>* >       mQueuedRawBuffers;     /*!< Internally allocated buffers that are currently queued
                                                                 to the V4L2 device. */

    FrameInfo                        mCaptureConfig;
    Vector<MediaCtlConfig>           mConfigList;
    camera3_stream_t*                mRawStream;
    int mLastReqIDReturned;     /*!> Used to keep request ID continuity in case
                                     of stream re-configuration*/

    /* Input system event listeners */
    Mutex   mCaptureEventListenerLock;  /*!< Protects accesses to the Listener management variables */
    KeyedVector<ICaptureEventListener::CaptureEventType, bool> mEventsProvided;
    typedef List< ICaptureEventListener* > listener_list_t;
    KeyedVector<ICaptureEventListener::CaptureEventType, listener_list_t > mListeners;
    /* end of scope for lock mCaptureEventListenerLock */

    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    bool mStreamingStarted;

    /* cached static metadata values  */
    int sPipelineDepth;
    int sBufferPoolSize;
    int sSensorSettingsDelay;
}; // class CaptureUnit

}; // namespace camera2
}; // namespace android

#endif
