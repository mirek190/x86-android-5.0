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

#define LOG_TAG "Camera_LensHw"

#include "LensHw.h"
#include "v4l2device.h"
#include "MediaController.h"
#include "MediaEntity.h"
#include "LogHelper.h"
#include "Camera3GFXFormat.h"


namespace android {
namespace camera2 {

LensHw::LensHw(int cameraId, sp<MediaController> mediaCtl):
    mCameraId(cameraId),
    mMediaCtl(mediaCtl) {
    LOG1("@%s", __FUNCTION__);
}

LensHw::~LensHw() {
    LOG1("@%s", __FUNCTION__);
}

status_t LensHw::setLens(sp<MediaEntity> entity)
{
    LOG1("@%s", __FUNCTION__);

    if (entity->getType() != SUBDEV_LENS) {
        LOGE("%s is not sensor subdevice", entity->getName());
        return BAD_VALUE;
    }
    sp<V4L2Subdevice> lens;
    entity->getDevice((sp<V4L2DeviceBase>&) lens);
    mLensSubdev = lens;

    return NO_ERROR;
}

int LensHw::moveFocusToPosition(int position)   // focus with absolute value
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_FOCUS_ABSOLUTE,
                                    position, "focus absolute");
}

int LensHw::moveFocusToBySteps(int steps)      // focus with relative value
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_FOCUS_RELATIVE,
                                    steps, "focus steps");
}

int LensHw::getFocusPosition(int &position)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->getControl(V4L2_CID_FOCUS_ABSOLUTE, &position);
}

int LensHw::getFocusStatus(int &status)
{
    LOG2("@%s", __FUNCTION__);
    // TODO make a v4l2 implementation
    UNUSED(status);
    return NO_ERROR;
}

int LensHw::startAutoFocus(void)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_AUTO_FOCUS_START, 1, "af start");
}

int LensHw::stopAutoFocus(void)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_AUTO_FOCUS_STOP, 0, "af stop");
}

int LensHw::getAutoFocusStatus(unsigned int &status)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->getControl(V4L2_CID_AUTO_FOCUS_STATUS,
                                    reinterpret_cast<int*>(&status));
}

int LensHw::setAutoFocusRange(int value)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_AUTO_FOCUS_RANGE,
                                    value, "set af range");
}

int LensHw::getAutoFocusRange(int &value)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->getControl(V4L2_CID_AUTO_FOCUS_RANGE, &value);
}

// ZOOM
int LensHw::moveZoomToPosition(int position)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_ZOOM_ABSOLUTE,
                                    position, "zoom absolute");
}

int LensHw::moveZoomToBySteps(int steps)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_ZOOM_RELATIVE,
                                    steps, "zoom steps");
}

int LensHw::getZoomPosition(int &position)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->getControl(V4L2_CID_ZOOM_ABSOLUTE, &position);
}

int LensHw::moveZoomContinuous(int position)
{
    LOG2("@%s", __FUNCTION__);
    return mLensSubdev->setControl(V4L2_CID_ZOOM_CONTINUOUS,
                                    position, "continuous zoom");
}

int LensHw::getCurrentCameraId(void)
{
    LOG1("@%s, id: %d", __FUNCTION__, mCameraId);
    return mCameraId;
}

const char* LensHw::getLensName(void)
{
    return mLensInput.name;
}

}   // namespace camera2
}   // namespace android
