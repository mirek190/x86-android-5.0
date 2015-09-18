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
//#include "SensorHw.h"
#include "Intel3aPlus.h"
#include "Camera3Request.h"
#include "CaptureBuffer.h"
#include "ItemPool.h"
#include "v4l2device.h"
#include "PollerThread.h"
#include "JpegMaker.h"
#include <utils/KeyedVector.h>
#include <utils/List.h>

/**
 * default value in case static metadata does not define it
 */
static const uint8_t DEFAULT_PIPELINE_DEPTH = 3;


namespace android {
namespace camera2 {

static const uint32_t JPEG_DATA_START_OFFSET = 64 * 1024; // 64K for exif+thumb
static const uint32_t JFIF_HEADER_SIZE = 20;

//class MediaController;
//class SensorHw;
class V4L2VideoNode;
//class CSS2600HwStream;

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
        CAPTURE_EVENT_MAX
    };

    // For MESSAGE_ID_EVENT
    struct CaptureMessageEvent {
        CaptureEventType type;
        struct timeval   timestamp;
        unsigned int     sequence;
        CaptureBuffer*   buffer;
        v4l2_buffer      v4l2buffer;
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

class CaptureUnit : public RefBase,
                    public IMessageHandler,
                    public IBufferOwner,
                    public IPollEventListener
{
public:
    enum CaptureUseCase {
        STILL = 0,
        VIDEO
    };

    CaptureUnit(int camId);
    ~CaptureUnit();

    status_t init();

    status_t flush();
    status_t configStreams(Vector<camera3_stream_t*> &activeStreams);

    status_t doCapture(Camera3Request* request,
                       AiqCaptureSettings &aiqCaptureSettings);

    status_t getSensorData(ia_aiq_exposure_sensor_descriptor* aiqSensorDescriptor);
    status_t getOutputConfig(FrameInfo &info);

    // IBufferOwner interface
    virtual void returnBuffer(CaptureBuffer* buffer);

    /************************************************************
     * Listener management methods
     */
    status_t attachListener(ICaptureEventListener *aListener, ICaptureEventListener::CaptureEventType event);
    status_t detachListener(ICaptureEventListener *aListener, ICaptureEventListener::CaptureEventType event);

    virtual status_t notifyPollEvent(PollEventMessage *msg);

    void dump(int fd);

protected:
    CaptureUseCase figureOutUseCase(Vector<camera3_stream_t*> &activeStreams);

// private methods
private:
    status_t getMediaCtlCamConfig();
    camera3_stream_t* findRawStream();
    status_t findRawBuffer(Camera3Request *request, CaptureBuffer **buf);
    status_t findOverlayBuffer(Camera3Request *request, CaptureBuffer **buf);
    status_t createBufferPool(int numBufs, sp<V4L2VideoNode> videoNode,
                              ItemPool<CaptureBuffer> &itemPool,
                              Vector<CaptureBuffer*> &buffers);
    status_t allocateCaptureBuffers(int numBufs);
    status_t allocateExtraBuffers(); /*!< quirk for the "driver-that-wants-to-keep-one" */
    bool     isBufferForStream(const Vector<camera3_stream_buffer> *buffers, camera3_stream_t *stream);
    status_t freeBuffers();
    status_t applySensorParams(Camera3Request* request, AiqCaptureSettings* aiqCaptureSettings);
    status_t enqueueBuffers(Camera3Request *request);

    // temporary function to help debugging
    int setExposure(unsigned int exposure, unsigned int gain);

    enum ExtraBufferState {
        EXTRA_BUFFER_FREE,
        EXTRA_BUFFER_QUEUED,
        EXTRA_BUFFER_QUEUED_AND_POLLING
    };

    // thread message IDs
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_CAPTURE,
        MESSAGE_ID_NOTIFY,
        MESSAGE_ID_FLUSH,
        MESSAGE_ID_RETURN_BUFFER,
        MESSAGE_ID_ISYS_EVENT,
        MESSAGE_ID_MAX
    };

    struct MessageNotify {
        bool overlay;
        bool capture;
        bool isp;
        // return values (only for debugging, to be removed!)
        bool *overlayDropped;
        bool *captureDropped;
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
        MessageNotify notify;
    };

    struct Message {
        MessageId id;
        MessageData data;
    };

    status_t handleMessageFlush(Message &msg);
    status_t handleMessageCapture(Message &msg);
    status_t handleMessageReturnBuffer(Message &msg);
    status_t handleMessageIsysEvent(Message &msg);
    status_t handleMessageNotify(Message &msg);
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

    status_t handleOverlayPoll();
    status_t handleCapturePoll();
    void handleOverlayExtraBuffer(); // issues the poll if needed
    void handleCaptureExtraBuffer(); // issues the poll if needed
    status_t handleExtraBufferQueuing();
    status_t handleISPPoll() { ALOGE("@%s Not implemented - abort", __FUNCTION__); abort(); return UNKNOWN_ERROR; }

    status_t notifyListeners(ICaptureEventListener::CaptureMessage *msg);

private:
    int     mCameraId;

//    sp<MediaController>    mMediaCtl;
//    sp<InputSystem>        mIsys;
//    sp<SensorHw>           mSensor;
    sp<V4L2VideoNode> mOverlayVideoNode;
    sp<V4L2VideoNode> mCaptureVideoNode;
    sp<V4L2VideoNode> mISPVideoNode;
    static const uint32_t OVERLAY_DEVICE_BIT = 1;
    static const uint32_t CAPTURE_DEVICE_BIT = 1 << 1;
    static const uint32_t ISP_DEVICE_BIT     = 1 << 2;
    Vector<sp<V4L2DeviceBase> > mDevices;
    Vector<sp<V4L2DeviceBase> > mPollDevices;

    Vector<camera3_stream_t *>       mActiveStreams;
    ItemPool<CaptureBuffer>          mCaptureItemsPool;
    ItemPool<CaptureBuffer>          mOverlayItemsPool;
    ItemPool<CaptureBuffer>          mISPItemsPool;
    Vector<CaptureBuffer*>           mRawCaptureBuffers;
    Vector<CaptureBuffer*>           mOverlayCaptureBuffers;
    CaptureBuffer                    mOverlayExtraBuffer; /*!< quirk for the "driver-that-wants-to-keep-one" */
    CaptureBuffer                    mCaptureExtraBuffer; /*!< quirk for the "driver-that-wants-to-keep-one" */

    FrameInfo                        mCaptureConfig;
    FrameInfo                        mOverlayConfig;
//    Vector<MediaCtlConfig>           mConfigList;
    camera3_stream_t*                mRawStream;
    camera3_stream_t*                mOverlayStream;
    camera3_stream_t*                mCaptureStream;

    /* Input system event listeners */
    Mutex   mCaptureEventListenerLock;  /*!< Protects accesses to the Listener management variables */
    KeyedVector<ICaptureEventListener::CaptureEventType, bool> mEventsProvided;
    typedef List< ICaptureEventListener* > listener_list_t;
    KeyedVector<ICaptureEventListener::CaptureEventType, listener_list_t > mListeners;
    /* end of scope for lock mCaptureEventListenerLock */

    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;

    sp<PollerThread> mPollerThread;
}; // class CaptureUnit

} // namespace camera2
} // namespace android

#endif
