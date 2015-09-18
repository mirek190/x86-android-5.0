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

#define LOG_TAG "Camera_HWStream"

#include "LogHelper.h"
#include "HwStream.h"



namespace android {
namespace camera2 {

const char *HwStream::State2String[] = { "STATE_STOPPED",
                                         "STATE_PAUSED",
                                         "STATE_RUNNING"};
HwStream::HwStream()
{
    mFps = 0;
    CLEAR(mConfig);
    mListeners.clear();
}

HwStream::~HwStream()
{

}

status_t HwStream::query(FrameInfo * info)
{
    UNUSED(info);
    return NO_ERROR;
}

status_t HwStream::setFormat(FrameInfo *aConfig)
{
    LOG1("@%s", __FUNCTION__);
    UNUSED(aConfig);
    return NO_ERROR;
}

status_t HwStream::capture(sp<CameraBuffer> aBuffer,
                                 Camera3Request* request)
{
    LOG1("@%s", __FUNCTION__);
    UNUSED(aBuffer);
    UNUSED(request);
    return NO_ERROR;
}

status_t HwStream::captureDone(sp<CameraBuffer> aBuffer,
                                     Camera3Request* request)
{
    LOG1("@%s", __FUNCTION__);
    UNUSED(aBuffer);
    UNUSED(request);
    return NO_ERROR;
}

status_t HwStream::reprocess(sp<CameraBuffer> anInputBuffer,
                                   Camera3Request* request)
{
    LOG1("@%s capture stream", __FUNCTION__);
    UNUSED(anInputBuffer);
    UNUSED(request);
    return NO_ERROR;
}

void HwStream::dump(int fd) const
{
    String8 message;

    message.appendFormat(LOG_TAG ":@%s name:\"%s\" - this is the base-class virtual dump function, nothing to see here yet\n", __FUNCTION__, getName());
    write(fd, message.string(), message.size());
}

status_t HwStream::start()
{
    LOG1("@%s", __FUNCTION__);
    return NO_ERROR;
}

status_t HwStream::stop()
{
    LOG1("@%s", __FUNCTION__);
    return NO_ERROR;
}

status_t HwStream::flush()
{
    LOG1("@%s", __FUNCTION__);
    return NO_ERROR;
}

/**
 * Attach a Listening client to a particular event
 *
 *
 * ObserverThread for distinct ObserverOperation exists. For new operations,
 * new ObserverThread gets created and started into paused state.
 *
 * @param observer interface pointer to attach
 * @param event concrete event to listen to
 */
status_t
HwStream::attachListener(ICssIspListener *observer,
                               ICssIspListener::IspEventType event)
{
    LOG1("@%s:%s  %p to event type %d", __FUNCTION__, getName(), observer, event);
    status_t status = NO_ERROR;
    if (observer == NULL)
        return BAD_VALUE;

    Mutex::Autolock _l(mListenerLock);
    // Check if we have any listener registered to this event
    int index = mListeners.indexOfKey(event);
    if (index == NAME_NOT_FOUND) {
        // First time someone registers for this event
        listener_list_t theList;
        theList.push_back(observer);
        mListeners.add(event, theList);
        return NO_ERROR;
    }

    // Now we will have more than one listener to this event
    listener_list_t &theList = mListeners.editValueAt(index);

    List<ICssIspListener*>::iterator it = theList.begin();
    for (;it != theList.end(); ++it)
        if (*it == observer)
            return ALREADY_EXISTS;

    theList.push_back(observer);

    mListeners.replaceValueFor(event,theList);
    return status;
}

status_t HwStream::notifyListeners(ICssIspListener::IspMessage *msg)
{
    LOG2("@%s:%s", getName(), __FUNCTION__);
    bool ret = false;
    Mutex::Autolock _l(mListenerLock);
    int index = mListeners.indexOfKey(msg->data.event.type);
    if (index != NAME_NOT_FOUND) {
        listener_list_t theList = mListeners.valueAt(index);
        for (List<ICssIspListener*>::iterator it = theList.begin();it != theList.end(); it++) {
            ret |= (*it)->notifyIspEvent((ICssIspListener::IspMessage*)msg);
        }
    }

    return NO_ERROR;
}

status_t
HwStream::detachListener()
{
    LOG1("@%s:%s", __FUNCTION__,getName());

    Mutex::Autolock _l(mListenerLock);
    status_t status = NO_ERROR;
    while (mListeners.size() > 0) {
        listener_list_t &theList = mListeners.editValueAt(0);
        LOG1("@%s:%s removing a list of listeners", __FUNCTION__,getName());
        theList.clear();
        mListeners.removeItemsAt(0);
    }

    return status;
}

void HwStream::dump(bool dumpBuffers) const
{
    UNUSED(dumpBuffers);
}


}   // namespace camera2
}   // namespace android
