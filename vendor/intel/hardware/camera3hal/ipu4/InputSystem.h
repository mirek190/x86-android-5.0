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

#ifndef CAMERA3_HAL_CSS2600ISYS_H_
#define CAMERA3_HAL_CSS2600ISYS_H_

#include "MessageQueue.h"
#include "MessageThread.h"
#include "PollerThread.h"
#include "v4l2device.h"
#include "MediaCtlPipeConfig.h"
#include <utils/KeyedVector.h>
#include <linux/media.h>
#include <poll.h>

namespace android {
namespace camera2 {

class MediaController;
class MediaEntity;

class IISysObserver {
public:
    enum IsysMessageId {
        ISYS_MESSAGE_ID_EVENT = 0,
        ISYS_MESSAGE_ID_ERROR
    };

    // For MESSAGE_ID_EVENT
    struct IsysMessageEvent {
        int requestId;
        VideoNodeType nodeType;
        struct v4l2_buffer_info *buffer;
    };

    // For MESSAGE_ID_ERROR
    struct IsysMessageError {
        status_t status;
    };

    union IsysMessageData {
        IsysMessageEvent event;
        IsysMessageError error;
    };

    struct IsysMessage {
        IsysMessageId   id;
        IsysMessageData data;
    };

    virtual void notifyIsysEvent(IsysMessage &msg) = 0;
    virtual ~IISysObserver() {};
};

class InputSystem : public RefBase, public IMessageHandler, public IPollEventListener
{

public:
    InputSystem(int cameraId, IISysObserver *observer, sp<MediaController> mediaCtl);
    ~InputSystem();

    status_t init();
    status_t configure(int width, int height, int &format);

    status_t start();
    status_t stop();
    bool isStarted();

    status_t putFrame(VideoNodeType type, const struct v4l2_buffer *buf);
    status_t setBufferPool(VideoNodeType type, Vector<struct v4l2_buffer> &pool, bool cached);
    status_t getOutputNodes(Vector<sp<V4L2VideoNode> > **nodes, int &nodeCount);
    status_t capture(int requestId);
    status_t flush();

    // IPollEvenListener
    virtual status_t notifyPollEvent(PollEventMessage *msg);

private: /* methods */
    status_t openVideoNodes();
    status_t closeVideoNodes();
    status_t openVideoNode(const char *entityName, VideoNodeType nodeType);
    status_t grabFrame(VideoNodeType type, struct v4l2_buffer_info *buf);

    // thread message IDs
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_CONFIGURE,
        MESSAGE_ID_START,
        MESSAGE_ID_STOP,
        MESSAGE_ID_IS_STARTED,
        MESSAGE_ID_PUT_FRAME,
        MESSAGE_ID_SET_BUFFER_POOL,
        MESSAGE_ID_GET_NODES,
        MESSAGE_ID_CAPTURE,
        MESSAGE_ID_FLUSH,
        MESSAGE_ID_POLL,
        MESSAGE_ID_MAX
    };

    struct MessageConfigure {
        int width;
        int height;
    };

    struct MessageFrame {
        VideoNodeType type;
        const struct v4l2_buffer *buf;
    };

    struct MessageBufferPool {
        VideoNodeType type;
        Vector<struct v4l2_buffer> *pool;
        bool cached;
    };

    struct MessageNodes {
        Vector<sp<V4L2VideoNode> > **nodes;
        int *nodeCount;
    };

    struct MessageCapture {
        int requestId;
    };

    struct MessagePollEvent {
        int requestId;
        sp<V4L2VideoNode> *activeDevices;
        int numDevices;
    };

    union MessageData {
        MessageFrame      frame;
        MessageBufferPool bufferPool;
        MessageNodes      nodes;
        MessageConfigure  config;
        MessageCapture    capture;
        MessagePollEvent  pollEvent;
    };

    struct Message {
        MessageId id;
        MessageData data;
    };

    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

    status_t handleMessageConfigure(Message &msg);
    status_t handleMessageStart();
    status_t handleMessageStop();
    status_t handleMessageIsStarted();
    status_t handleMessagePutFrame(Message &msg);
    status_t handleMessageSetBufferPool(Message &msg);
    status_t handleMessageGetOutputNodes(Message &msg);
    status_t handleMessageCapture(Message &msg);
    status_t handleMessagePollEvent(Message &msg);
    status_t handleMessageFlush();

private: /* members */
    int                 mCameraId;
    IISysObserver*      mObserver;
    sp<MediaController> mMediaCtl;
    bool                mStarted;

    Vector<sp<V4L2VideoNode> >                     mConfiguredNodes;        /*!< Configured video nodes */
    KeyedVector<VideoNodeType, sp<V4L2VideoNode> > mConfiguredNodesPerType; /*!< Configured video nodes, Key: node type */
    const MediaCtlConfig *mMediaCtlConfig;

    int                   mFormat;
    int                   mBuffersReceived;
    uint32_t              mBufferSeqNbr;

    /**
     * Thread control members
     */
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThread;
    bool mThreadRunning;

    sp<PollerThread> mPollerThread;
}; // class InputSystem

}; // namespace camera2
}; // namespace android

#endif
