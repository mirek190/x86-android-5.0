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

#define LOG_TAG "Camera_V4L2DevBase"

#include "LogHelper.h"
#include "v4l2device.h"
#include "UtilityMacros.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>


namespace android {
namespace camera2 {

////////////////////////////////////////////////////////////////////
//                          PUBLIC METHODS
////////////////////////////////////////////////////////////////////

V4L2DeviceBase::V4L2DeviceBase(const char *name): mName(name),
                                                  mFd(-1)
{
}

V4L2DeviceBase::~V4L2DeviceBase()
{
    LOG1("@%s", __FUNCTION__);
    if (mFd != -1) {
        LOGW("Destroying a device object not closed, closing first");
        close();
    }
}

status_t V4L2DeviceBase::open()
{
    LOG1("@%s %s", __FUNCTION__, mName.string());
    status_t ret = NO_ERROR;
    struct stat st;

    if (mFd != -1) {
        LOGE("Trying to open a device already open");
        return NO_ERROR; //INVALID_OPERATION;
    }

    if (stat (mName.string(), &st) == -1) {
        LOGE("Error stat video device %s: %s",
             mName.string(), strerror(errno));
        return UNKNOWN_ERROR;
    }

    if (!S_ISCHR (st.st_mode)) {
        LOGE("%s is not a device", mName.string());
        return UNKNOWN_ERROR;
    }

    mFd = popen(mName.string(), O_RDWR);

    if (mFd < 0) {
        LOGE("Error opening video device %s: %s",
              mName.string(), strerror(errno));
        return UNKNOWN_ERROR;
    }

    return ret;
}

status_t V4L2DeviceBase::close()
{
    LOG1("@%s device : %s", __FUNCTION__, mName.string());

    if (mFd == -1) {
        LOGW("Device not opened!");
        return INVALID_OPERATION;
    }

    if (pclose(mFd) < 0) {
        LOGE("Close video device failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    mFd = -1;
    return NO_ERROR;
}

int V4L2DeviceBase::xioctl(int request, void *arg) const
{
    int ret(0);
    if (mFd == -1) {
        LOGE("%s invalid device closed!",__FUNCTION__);
        return INVALID_OPERATION;
    }

    do {
        ret = ioctl (mFd, request, arg);
    } while (-1 == ret && EINTR == errno);

    if (ret < 0)
        LOGW ("%s: Request 0x%x failed: %s", __FUNCTION__, request, strerror(errno));

    return ret;
}

/**
 * Waits for frame data to be available
 *
 * \param timeout time to wait for data (in ms), timeout of -1
 *        means to wait indefinitely for data
 *
 * \return 0: timeout, -1: error happened, positive number: success
 */
int V4L2DeviceBase::poll(int timeout)
{
    LOG2("@%s", __FUNCTION__);
    struct pollfd pfd = {0,0,0};
    int ret(0);

    if (mFd == -1) {
        LOG1("Device %s already closed. Do nothing.", mName.string());
        return -1;
    }

    pfd.fd = mFd;
    pfd.events = POLLPRI | POLLIN | POLLERR;

    ret = perfpoll(&pfd, 1, timeout);

    if (pfd.revents & POLLERR) {
        LOG1("%s received POLLERR", __FUNCTION__);
        return -1;
    }

    return ret;
}

int V4L2DeviceBase::subscribeEvent(int event)
{
    LOG1("@%s", __FUNCTION__);
    int ret(0);
    struct v4l2_event_subscription sub;

    if (mFd == -1) {
        LOG1("Device %s already closed. cannot subscribe.", mName.string());
        return -1;
    }

    CLEAR(sub);
    sub.type = event;

    ret = pioctl(mFd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    if (ret < 0) {
        LOGE("error subscribing event %x: %s", event, strerror(errno));
        return ret;
    }

    return ret;
}

int V4L2DeviceBase::unsubscribeEvent(int event)
{
    LOG1("@%s", __FUNCTION__);
    int ret(0);
    struct v4l2_event_subscription sub;

    if (mFd == -1) {
        LOG1("Device %s closed. cannot unsubscribe.", mName.string());
        return -1;
    }

    CLEAR(sub);
    sub.type = event;

    ret = pioctl(mFd, VIDIOC_UNSUBSCRIBE_EVENT, &sub);
    if (ret < 0) {
        LOGE("error unsubscribing event %x :%s",event,strerror(errno));
        return ret;
    }

    return ret;
}

int V4L2DeviceBase::dequeueEvent(struct v4l2_event *event)
{
    LOG2("@%s", __FUNCTION__);
    int ret(0);

    if (mFd == -1) {
        LOG1("Device %s closed. cannot dequeue event.", mName.string());
        return -1;
    }

    ret = pioctl(mFd, VIDIOC_DQEVENT, event);
    if (ret < 0) {
        LOGE("error dequeuing event");
        return ret;
    }

    return ret;
}

status_t V4L2DeviceBase::setControl(int aControlNum, const int value, const char *name)
{
    LOG2("@%s", __FUNCTION__);

    struct v4l2_control control;
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control ext_control;

    CLEAR(control);
    CLEAR(controls);
    CLEAR(ext_control);

    LOG1("setting attribute [%s] to %d", name, value);

    if (mFd == -1) {
        LOGE("%s: Invalid device state (CLOSED)", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    control.id = aControlNum;
    control.value = value;
    controls.ctrl_class = V4L2_CTRL_ID2CLASS(control.id);
    controls.count = 1;
    controls.controls = &ext_control;
    ext_control.id = aControlNum;
    ext_control.value = value;

    if (pioctl(mFd, VIDIOC_S_EXT_CTRLS, &controls) == 0)
        return NO_ERROR;

    if (pioctl(mFd, VIDIOC_S_CTRL, &control) == 0)
        return NO_ERROR;

    LOGE("Failed to set value %d for control %s (%d) on device '%s', %s",
        value, name, aControlNum, mName.string(), strerror(errno));

    return UNKNOWN_ERROR;
}

status_t V4L2DeviceBase::getControl(int aControlNum, int *value)
{
    LOG2("@%s", __FUNCTION__);

    struct v4l2_control control;
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control ext_control;

    CLEAR(control);
    CLEAR(controls);
    CLEAR(ext_control);

    if (mFd == -1) {
        LOGE("%s: Invalid state device (CLOSED)", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    control.id = aControlNum;
    controls.ctrl_class = V4L2_CTRL_ID2CLASS(control.id);
    controls.count = 1;
    controls.controls = &ext_control;
    ext_control.id = aControlNum;

    if (pioctl(mFd, VIDIOC_G_EXT_CTRLS, &controls) == 0) {
       *value = ext_control.value;
       return NO_ERROR;
    }

    if (pioctl(mFd, VIDIOC_G_CTRL, &control) == 0) {
       *value = control.value;
       return NO_ERROR;
    }

    LOGE("Failed to get value for control (%d) on device '%s', %s",
            aControlNum, mName.string(), strerror(errno));
    return UNKNOWN_ERROR;
}

status_t V4L2DeviceBase::queryMenu(v4l2_querymenu &menu)
{
    LOG2("@%s", __FUNCTION__);

    if (mFd == -1) {
        LOGE("%s: Invalid state device (CLOSED)", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (pioctl(mFd, VIDIOC_QUERYMENU, &menu) == 0) {
       return NO_ERROR;
    }

    LOGE("Failed to get values for query menu (%d) on device '%s', %s",
            menu.id, mName.string(), strerror(errno));
    return UNKNOWN_ERROR;
}

status_t V4L2DeviceBase::queryControl(v4l2_queryctrl &control)
{
    LOG2("@%s", __FUNCTION__);

    if (mFd == -1) {
        LOGE("%s: Invalid state device (CLOSED)", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (pioctl(mFd, VIDIOC_QUERYCTRL, &control) == 0) {
       return NO_ERROR;
    }
    LOGE("Failed to get values for query control (%d) on device '%s', %s",
            control.id, mName.string(), strerror(errno));
    return UNKNOWN_ERROR;
}

int V4L2DeviceBase::pollDevices(const Vector<sp<V4L2DeviceBase> > &devices,
                                Vector<sp<V4L2DeviceBase> > &activeDevices,
                                Vector<sp<V4L2DeviceBase> > &inactiveDevices,
                                int timeOut, int flushFd)
{
    LOG2("@%s", __FUNCTION__);
    int numFds = devices.size();
    int totalNumFds = (flushFd != -1) ? numFds + 1 : numFds; //adding one more fd if flushfd given.
    struct pollfd pollFds[totalNumFds];
    int ret = 0;

    for (int i = 0; i < numFds; i++) {
        pollFds[i].fd = devices[i]->mFd;
        pollFds[i].events = POLLPRI | POLLIN | POLLERR;
        pollFds[i].revents = 0;
    }

   if (flushFd != -1) {
        pollFds[numFds].fd = flushFd;
        pollFds[numFds].events = POLLPRI | POLLIN;
        pollFds[numFds].revents = 0;
    }

    ret = perfpoll(pollFds, totalNumFds, timeOut);
    if (ret <= 0) {
        LOGE("Device poll failed (%s)", (ret == 0) ? "timeout" : "error");
        for (uint32_t i = 0; i < devices.size(); i++) {
            if (pollFds[i].revents & POLLERR) {
                LOGE("%s: device %s received POLLERR", __FUNCTION__, devices[i]->name());
            }
        }
        return ret;
    }

    activeDevices.clear();
    inactiveDevices.clear();

    //check first the flush
    if (flushFd != -1) {
        if (pollFds[numFds].revents & POLLIN || pollFds[numFds].revents & POLLPRI) {
            LOG1("%s: Poll returning from flush", __FUNCTION__);
            return ret;
        }
    }

    // check other active devices.
    for (int i = 0; i < numFds; i++) {
        if (pollFds[i].revents & POLLERR) {
            LOGE("%s: received POLLERR", __FUNCTION__);
            return -1;
        }
        // return nodes that have data available
        if (pollFds[i].revents & POLLPRI || pollFds[i].revents & POLLIN) {
            activeDevices.add(devices[i]);
        } else
            inactiveDevices.add(devices[i]);
    }
    return ret;
}


////////////////////////////////////////////////////////////////////
//                          PRIVATE METHODS
////////////////////////////////////////////////////////////////////

} // namespace camera2
} // namespace android



