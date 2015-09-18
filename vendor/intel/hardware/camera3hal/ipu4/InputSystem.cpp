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

#define LOG_TAG "Camera_InputSystem"

#include "Camera3GFXFormat.h"
#include "InputSystem.h"
#include "MediaController.h"
#include "PlatformData.h"
#include "MediaEntity.h"
#include "LogHelper.h"
#include "IPU4CameraCapInfo.h"

#define CSS_EVENT_POLL_TIMEOUT 100

namespace android {
namespace camera2 {

InputSystem::InputSystem(int cameraId, IISysObserver *observer, sp<MediaController> mediaCtl):
    mCameraId(cameraId),
    mObserver(observer),
    mMediaCtl(mediaCtl),
    mStarted(false),
    mMediaCtlConfig(NULL),
    mFormat(0),
    mBuffersReceived(0),
    mBufferSeqNbr(0),
    mMessageQueue("Camera_InputSystem", (int)MESSAGE_ID_MAX),
    mMessageThread(NULL),
    mThreadRunning(false)
{
    LOG1("@%s", __FUNCTION__);
}

InputSystem::~InputSystem()
{
    LOG1("@%s", __FUNCTION__);

    if (mPollerThread != NULL) {
        mPollerThread->requestExitAndWait();
        mPollerThread.clear();
        mPollerThread = NULL;
    }

    if (mMessageThread != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThread->requestExitAndWait();
        mMessageThread.clear();
        mMessageThread = NULL;
    }
    closeVideoNodes();
}

status_t InputSystem::init()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mMessageThread = new MessageThread(this, "InputSystem");
    if (mMessageThread == NULL) {
        LOGE("@%s: Error creating message thread", __FUNCTION__);
        return NO_INIT;
    }

    mPollerThread = new PollerThread("IsysPollerThread");
    if (mMessageThread == NULL) {
        LOGE("@%s: Error creating poller thread", __FUNCTION__);
        return NO_INIT;
    }

    return status;
}

status_t InputSystem::configure(int width, int height, int &format)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_CONFIGURE;
    msg.data.config.width = width;
    msg.data.config.height = height;
    status = mMessageQueue.send(&msg, MESSAGE_ID_CONFIGURE);
    format = mFormat;
    return status;
}

status_t InputSystem::handleMessageConfigure(Message &msg)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    const IPU4CameraCapInfo *cap = NULL;

    int width = msg.data.config.width;
    int height = msg.data.config.height;

    status = closeVideoNodes();
    if (status != NO_ERROR) {
        LOGE("%s: failed to close video nodes (ret = %d)", __FUNCTION__, status);
        goto exit;
    }

    cap = getIPU4CameraCapInfo(mCameraId);
    mMediaCtlConfig = cap->getMediaCtlConfig(width, height);
    if (mMediaCtlConfig == NULL) {
        LOGE("%s:Not able to pick up config %dx%d",__FUNCTION__, width, height);
        status = BAD_VALUE;
        goto exit;
    }

    status = mMediaCtl->resetLinks();
    if (status != NO_ERROR) {
        LOGE("%s: cannot reset MediaCtl links", __FUNCTION__);
        goto exit;
    }

    // setting all the Link necessary for the media controller.
    for (unsigned int i = 0; i < mMediaCtlConfig->mLinkParams.size(); i++) {
        MediaCtlLinkParams pipeLink = mMediaCtlConfig->mLinkParams[i];
        status = mMediaCtl->configureLink(pipeLink.srcName.string(), pipeLink.srcPad,
                                          pipeLink.sinkName.string(), pipeLink.sinkPad,
                                          pipeLink.enable);
        if (status != NO_ERROR) {
            LOGE("%s: cannot set MediaCtl links (ret = %d)", __FUNCTION__, status);
            goto exit;
        }
    }

    // setting all the format necessary for the media controller entities
    for (unsigned int i = 0; i < mMediaCtlConfig->mFormatParams.size(); i++) {
        MediaCtlFormatParams pipeFormat = mMediaCtlConfig->mFormatParams[i];
        status = mMediaCtl->setFormat(pipeFormat.entityName.string(), pipeFormat.pad,
                                      pipeFormat.width, pipeFormat.height,
                                      pipeFormat.formatCode);
        if (status != NO_ERROR) {
            LOGE("%s: cannot set MediaCtl format (ret = %d)", __FUNCTION__, status);
            goto exit;
        }

        // get the capture pipe output format
        sp<MediaEntity> entity = NULL;
        status = mMediaCtl->getMediaEntity(entity, pipeFormat.entityName.string());
        if (status != NO_ERROR) {
            LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, pipeFormat.entityName.string());
            goto exit;
        }
        if (entity->getType() == DEVICE_VIDEO) {
            mFormat = pipeFormat.formatCode;
            LOG1("%s: capture pipe output format: %s", __FUNCTION__, v4l2Fmt2Str(mFormat));
        }
    }

    // setting all the selection rectangle necessary for the media controller entities
    for (unsigned int i = 0; i < mMediaCtlConfig->mSelectionParams.size(); i++) {
        MediaCtlSelectionParams pipeSelection = mMediaCtlConfig->mSelectionParams[i];
        status = mMediaCtl->setSelection(pipeSelection.entityName.string(),
                                         pipeSelection.pad,
                                         pipeSelection.target,
                                         pipeSelection.top, pipeSelection.left,
                                         pipeSelection.width, pipeSelection.height);
        if (status != NO_ERROR) {
            LOGE("%s: cannot set MediaCtl selection (ret = %d)", __FUNCTION__, status);
            goto exit;
        }
    }

    // setting all the basic controls necessary for the media controller entities
    for (unsigned int i = 0; i < mMediaCtlConfig->mControlParams.size(); i++) {
        MediaCtlControlParams pipeControl = mMediaCtlConfig->mControlParams[i];
        status = mMediaCtl->setControl(pipeControl.entityName.string(),
                                       pipeControl.controlId, pipeControl.value,
                                       pipeControl.controlName.string());
        if (status != NO_ERROR) {
            LOGE("%s: cannot set MediaCtl control (ret = %d)", __FUNCTION__, status);
            goto exit;
        }
    }

    status = openVideoNodes();
    if (status != NO_ERROR) {
        LOGE("%s: failed to open video nodes (ret = %d)", __FUNCTION__, status);
        goto exit;
    }

    status = mPollerThread->init((Vector<sp<V4L2DeviceBase> >&) mConfiguredNodes, this);
    if (status != NO_ERROR) {
       LOGE("%s: PollerThread init failed (ret = %d)", __FUNCTION__, status);
       goto exit;
    }

exit:
    mMessageQueue.reply(MESSAGE_ID_CONFIGURE, status);
    return status;
}

status_t InputSystem::openVideoNodes()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = UNKNOWN_ERROR;

    // Open video nodes that are listed in the current config
    for (unsigned int i = 0; i < mMediaCtlConfig->mVideoNodes.size(); i++) {
        MediaCtlElement element = mMediaCtlConfig->mVideoNodes[i];
        VideoNodeType nodeType = (VideoNodeType) element.videoNodeType;

        status = openVideoNode(element.name.string(), nodeType);
        if (status != NO_ERROR) {
            LOGE("%s:Cannot open video node (status = 0x%X)", __FUNCTION__, status);
            return status;
        }
    }

    return NO_ERROR;
}

status_t InputSystem::openVideoNode(const char *entityName, VideoNodeType nodeType)
{
    LOG1("@%s: %s, type: %d", __FUNCTION__, entityName, nodeType);
    status_t status = UNKNOWN_ERROR;
    sp<MediaEntity> entity = NULL;
    sp<V4L2VideoNode> videoNode = NULL;

    if (entityName != NULL) {
        status = mMediaCtl->getMediaEntity(entity, entityName);
        if (status != NO_ERROR) {
            LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, entityName);
            return status;
        }
        status = entity->getDevice((sp<V4L2DeviceBase>&) videoNode);
        if (status != NO_ERROR) {
            LOGE("@%s: error opening device \"%s\"", __FUNCTION__, entityName);
            return status;
        }
        videoNode->setType(nodeType);

        mConfiguredNodes.add(videoNode);
        mConfiguredNodesPerType.add(nodeType, videoNode);
    }

    return status;
}

status_t InputSystem::closeVideoNodes()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    // stop streaming before closing devices
    if (mStarted) {
        status = stop();
        if (status != NO_ERROR)
            LOGW("@%s: error in stopping video nodes", __FUNCTION__);
    }

    for (size_t i = 0; i < mConfiguredNodes.size(); i++) {
        status = mConfiguredNodes[i]->close();
        if (status != NO_ERROR)
            LOGW("@%s error in closing video node (%d)", __FUNCTION__, i);
    }
    mConfiguredNodes.clear();
    mConfiguredNodesPerType.clear();

    return NO_ERROR;
}

status_t InputSystem::start()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_START;
    status = mMessageQueue.send(&msg, MESSAGE_ID_START);

    return status;
}

status_t InputSystem::handleMessageStart()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    int ret = 0;

    for (size_t i = 0; i < mConfiguredNodes.size(); i++) {
        ret = mConfiguredNodes[i]->start(0);
        if (ret < 0) {
            LOGE("@%s failed (%d)", __FUNCTION__, i);
            status = UNKNOWN_ERROR;
            stop();
            break;
        }
    }
    if (status == NO_ERROR)
        mStarted = true;

    mMessageQueue.reply(MESSAGE_ID_START, status);
    return status;
}

status_t InputSystem::stop()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_STOP;
    status = mMessageQueue.send(&msg, MESSAGE_ID_STOP);

    return status;
}

status_t InputSystem::handleMessageStop()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    int ret = 0;

    for (size_t i = 0; i < mConfiguredNodes.size(); i++) {
        if (mConfiguredNodes[i]->isStarted()) {
            ret = mConfiguredNodes[i]->stop();
            if (ret < 0) {
                LOGE("@%s failed (%d)", __FUNCTION__, i);
                status = UNKNOWN_ERROR;
            }
        }
    }
    mStarted = false;

    mMessageQueue.reply(MESSAGE_ID_STOP, status);
    return status;
}

bool InputSystem::isStarted()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_IS_STARTED;
    status = mMessageQueue.send(&msg, MESSAGE_ID_IS_STARTED);

    return mStarted;
}

status_t InputSystem::handleMessageIsStarted()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mMessageQueue.reply(MESSAGE_ID_IS_STARTED, status);
    return status;
}

status_t InputSystem::putFrame(VideoNodeType type, const struct v4l2_buffer *buf)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_PUT_FRAME;
    msg.data.frame.type = type;
    msg.data.frame.buf = buf;
    status = mMessageQueue.send(&msg);

    return status;
}

status_t InputSystem::handleMessagePutFrame(Message &msg)
{
    LOG2("@%s", __FUNCTION__);

    VideoNodeType type = msg.data.frame.type;
    const struct v4l2_buffer *buf = msg.data.frame.buf;

    int index = mConfiguredNodesPerType.indexOfKey(type);
    if (index == NAME_NOT_FOUND) {
        LOGE("@%s: node type (%d) not found!", __FUNCTION__, type);
        return BAD_VALUE;
    }
    sp<V4L2VideoNode> videoNode = mConfiguredNodesPerType[index];
    int ret = videoNode->putFrame(buf);
    if (ret < 0) {
        LOGE("@%s failed", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t InputSystem::grabFrame(VideoNodeType type, struct v4l2_buffer_info *buf)
{
    LOG2("@%s", __FUNCTION__);
    int index = mConfiguredNodesPerType.indexOfKey(type);
    if (index == NAME_NOT_FOUND) {
        LOGE("@%s: node type (%d) not found!", __FUNCTION__, type);
        return BAD_VALUE;
    }
    sp<V4L2VideoNode> videoNode = mConfiguredNodesPerType[index];
    int ret = videoNode->grabFrame(buf);
    if (ret < 0) {
        LOGE("@%s failed", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t InputSystem::setBufferPool(VideoNodeType type, Vector<struct v4l2_buffer> &pool, bool cached)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_SET_BUFFER_POOL;
    msg.data.bufferPool.type = type;
    msg.data.bufferPool.pool = &pool;
    msg.data.bufferPool.cached = cached;
    status = mMessageQueue.send(&msg, MESSAGE_ID_SET_BUFFER_POOL);

    return status;
}

status_t InputSystem::handleMessageSetBufferPool(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    VideoNodeType type = msg.data.bufferPool.type;
    Vector<struct v4l2_buffer> *pool = msg.data.bufferPool.pool;
    bool cached = msg.data.bufferPool.cached;
    sp<V4L2VideoNode> videoNode = NULL;

    int index = mConfiguredNodesPerType.indexOfKey(type);
    if (index == NAME_NOT_FOUND) {
        LOGE("@%s: node type (%d) not found!", __FUNCTION__, type);
        status = BAD_VALUE;
        goto exit;
    }
    videoNode = mConfiguredNodesPerType[index];
    status = videoNode->setBufferPool(*pool, cached);
    if (status != NO_ERROR) {
        LOGE("@%s failed", __FUNCTION__);
        goto exit;
    }

exit:
    mMessageQueue.reply(MESSAGE_ID_SET_BUFFER_POOL, status);
    return NO_ERROR;
}

status_t InputSystem::getOutputNodes(Vector<sp<V4L2VideoNode> > **nodes, int &nodeCount)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_GET_NODES;
    msg.data.nodes.nodes = nodes;
    msg.data.nodes.nodeCount = &nodeCount;
    status = mMessageQueue.send(&msg, MESSAGE_ID_GET_NODES);
    return status;
}

status_t InputSystem::handleMessageGetOutputNodes(Message &msg)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    Vector<sp<V4L2VideoNode> > **nodes = msg.data.nodes.nodes;
    int *nodeCount = msg.data.nodes.nodeCount;

    if (nodes)
        *nodes = &mConfiguredNodes;

    if (nodeCount)
        *nodeCount = mConfiguredNodes.size();

    mMessageQueue.reply(MESSAGE_ID_GET_NODES, status);
    return status;
}

status_t InputSystem::capture(int requestId)
{
    LOG2("@%s: request ID: %d", __FUNCTION__, requestId);
    status_t status = NO_ERROR;

    Message msg;
    msg.id = MESSAGE_ID_CAPTURE;
    msg.data.capture.requestId = requestId;
    status = mMessageQueue.send(&msg);

    return status;
}

status_t InputSystem::handleMessageCapture(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = mPollerThread->pollRequest(msg.data.capture.requestId);

    return status;
}

status_t InputSystem::flush()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    // flush the poll messages
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    mMessageQueue.remove(MESSAGE_ID_CAPTURE);
    status = mMessageQueue.send(&msg, MESSAGE_ID_FLUSH);
    return status;
}

status_t InputSystem::handleMessageFlush()
{
    LOG1("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    mPollerThread->flush();
    mMessageQueue.reply(MESSAGE_ID_FLUSH, status);
    return status;
}

void InputSystem::messageThreadLoop(void)
{
    LOG1("@%s: Start", __FUNCTION__);

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

        case MESSAGE_ID_CONFIGURE:
            status = handleMessageConfigure(msg);
            break;

        case MESSAGE_ID_START:
            status = handleMessageStart();
            break;

        case MESSAGE_ID_STOP:
            status = handleMessageStop();
            break;

        case MESSAGE_ID_IS_STARTED:
            status = handleMessageIsStarted();
            break;

        case MESSAGE_ID_PUT_FRAME:
            status = handleMessagePutFrame(msg);
            break;

        case MESSAGE_ID_SET_BUFFER_POOL:
            status = handleMessageSetBufferPool(msg);
            break;

        case MESSAGE_ID_GET_NODES:
            status = handleMessageGetOutputNodes(msg);
            break;

        case MESSAGE_ID_CAPTURE:
            status = handleMessageCapture(msg);
            break;

        case MESSAGE_ID_FLUSH:
            status = handleMessageFlush();
            break;

        case MESSAGE_ID_POLL:
            status = handleMessagePollEvent(msg);
            break;

        default:
            LOGE("@%s: Unknown message: %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("error %d in handling message: %d", status, (int)msg.id);

    }
    LOG1("%s: Exit", __FUNCTION__);
}

status_t InputSystem::notifyPollEvent(PollEventMessage *pollMsg)
{
    LOG2("@%s", __FUNCTION__);

    if (pollMsg == NULL || pollMsg->data.activeDevices == NULL)
        return BAD_VALUE;

    if (pollMsg->id == POLL_EVENT_ID_EVENT) {
        int numDevices = pollMsg->data.activeDevices->size();
        if (numDevices == 0) {
            LOG1("@%s: devices flushed", __FUNCTION__);
            return OK;
        }
        Message msg;
        msg.id = MESSAGE_ID_POLL;
        msg.data.pollEvent.activeDevices = new sp<V4L2VideoNode>[numDevices];
        for (int i = 0; i < numDevices; i++) {
            msg.data.pollEvent.activeDevices[i] = (sp<V4L2VideoNode>&) pollMsg->data.activeDevices->itemAt(i);
        }
        msg.data.pollEvent.numDevices = numDevices;
        msg.data.pollEvent.requestId = pollMsg->data.reqId;
        mMessageQueue.send(&msg);
    } else {
        LOGE("@%s: device poll failed", __FUNCTION__);
    }

    return OK;
}

status_t InputSystem::handleMessagePollEvent(Message &msg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    struct v4l2_buffer_info outBuf;
    sp<V4L2VideoNode> *activeNodes;
    int nodeCount = mConfiguredNodes.size();
    int activeNodecount = 0;
    int requestId = -1;

    activeNodes = msg.data.pollEvent.activeDevices;
    activeNodecount = msg.data.pollEvent.numDevices;
    requestId = msg.data.pollEvent.requestId;

    LOG2("@%s: received %d / %d buffers for request %d", __FUNCTION__, activeNodecount, nodeCount, requestId);

    for (int i = 0; i < activeNodecount; i++) {
        VideoNodeType nodeType = activeNodes[i]->getType();

        // dequeue the buffer and notify listeners
        status = grabFrame(nodeType, &outBuf);
        if (status != NO_ERROR) {
            LOGE("@%s: Error getting data from video node %d",__FUNCTION__, nodeType);
            // Notify observer
            IISysObserver::IsysMessage isysMsg;
            isysMsg.id = IISysObserver::ISYS_MESSAGE_ID_ERROR;
            isysMsg.data.error.status = status;
            mObserver->notifyIsysEvent(isysMsg);
            return status;
        }

        // When receiving the first buffer for a request,
        // store the sequence number. All buffers should
        // have the same sequence number.
        if (mBufferSeqNbr == 0) {
            mBufferSeqNbr = outBuf.vbuffer.sequence;
        } else if (mBufferSeqNbr != outBuf.vbuffer.sequence) {
            LOGW("@%s: sequence number mismatch, expecting %d but received %d", __FUNCTION__,
                  mBufferSeqNbr, outBuf.vbuffer.sequence);
        }
        mBuffersReceived++;

        // Notify observer
        IISysObserver::IsysMessage isysMsg;
        isysMsg.id = IISysObserver::ISYS_MESSAGE_ID_EVENT;
        isysMsg.data.event.requestId = requestId;
        isysMsg.data.event.nodeType = nodeType;
        isysMsg.data.event.buffer = &outBuf;
        mObserver->notifyIsysEvent(isysMsg);

    }
    delete[] activeNodes;

    if (mBuffersReceived == nodeCount) {
        LOG2("@%s: all buffers received (%d) for request %d",
             __FUNCTION__, mBuffersReceived, requestId);
        mBuffersReceived = 0;
        mBufferSeqNbr = 0;
    } else {
        LOG2("@%s: %d buffers still pending for request %d, sending a new poll request",
             __FUNCTION__, nodeCount - mBuffersReceived, requestId);
        status = mPollerThread->pollRequest(requestId);
    }

    return status;
}

}
}
