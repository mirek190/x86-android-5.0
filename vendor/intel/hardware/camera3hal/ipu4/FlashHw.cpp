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

#define LOG_TAG "Camera_FlashHw"

#include "FlashHw.h"
#include "v4l2device.h"
#include "MediaController.h"
#include "MediaEntity.h"
#include "LogHelper.h"


namespace android {
namespace camera2 {

FlashHw::FlashHw(int cameraId, sp<MediaController> mediaCtl):
    mCameraId(cameraId),
    mMediaCtl(mediaCtl) {
    LOG1("@%s", __FUNCTION__);
}

FlashHw::~FlashHw() {
    LOG1("@%s", __FUNCTION__);
}

status_t FlashHw::setFlash(sp<MediaEntity> entity)
{
    LOG1("@%s", __FUNCTION__);

    if (entity->getType() != SUBDEV_FLASH) {
        LOGE("%s is not flash subdevice", entity->getName());
        return BAD_VALUE;
    }
    sp<V4L2Subdevice> flash;
    entity->getDevice((sp<V4L2DeviceBase>&) flash);
    mFlashSubdev = flash;

    return NO_ERROR;
}

int FlashHw::setFlashStrobeSource(int source)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->setControl(V4L2_CID_FLASH_STROBE_SOURCE,
                                    source, "strobe source");
}

int FlashHw::getFlashStrobeSource(int &source)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->getControl(V4L2_CID_FLASH_STROBE_SOURCE, &source);
}

int FlashHw::flashStrobe(bool enable)
{
    LOG2("@%s, enabled = %s", __FUNCTION__, enable?"true":"false");

    if (enable)
        return mFlashSubdev->setControl(V4L2_CID_FLASH_STROBE,
                                        1, "set strobe");
    else
        return mFlashSubdev->setControl(V4L2_CID_FLASH_STROBE_STOP,
                                        0, "set strobe stop");
}

int FlashHw::getFlashStrobeStatus(bool &status)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->getControl(V4L2_CID_FLASH_STROBE_STATUS,
                                    reinterpret_cast<int*>(&status));
}

int FlashHw::setFlashTimeOut(int timeOut)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->setControl(V4L2_CID_FLASH_TIMEOUT,
                                    timeOut, "set strobe timeout");
}

int FlashHw::setFlashIntensity(flash_usage usage, int intensity)
{
    LOG2("@%s", __FUNCTION__);

    switch (usage) {
    case FLASH:
        return mFlashSubdev->setControl(V4L2_CID_FLASH_INTENSITY,
                                        intensity, "set flash intensity");
    case TORCH:
        return mFlashSubdev->setControl(V4L2_CID_FLASH_TORCH_INTENSITY,
                                        intensity, "set torch intensity");
    case INDICATOR:
        return mFlashSubdev->setControl(V4L2_CID_FLASH_INDICATOR_INTENSITY,
                                        intensity, "set indicator intensity");
    default:
        LOGE("%s: unknown flash type (%d)", __FUNCTION__, usage);
        return BAD_VALUE;
    }
}

int FlashHw::getFlashIntensity(flash_usage usage, int &intensity)
{
    LOG2("@%s", __FUNCTION__);

    switch (usage) {
    case FLASH:
        return mFlashSubdev->getControl(V4L2_CID_FLASH_INTENSITY,
                                        &intensity);
    case TORCH:
        return mFlashSubdev->getControl(V4L2_CID_FLASH_TORCH_INTENSITY,
                                        &intensity);
    case INDICATOR:
        return mFlashSubdev->getControl(V4L2_CID_FLASH_INDICATOR_INTENSITY,
                                        &intensity);
    default:
        LOGE("%s: unknown flash type (%d)", __FUNCTION__, usage);
        return BAD_VALUE;
    }
}

int FlashHw::getFlashFault(unsigned int &bitmask)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->getControl(V4L2_CID_FLASH_FAULT,
                                    reinterpret_cast<int*>(&bitmask));
}

int FlashHw::flashCharge(bool enable)
{
    LOG2("@%s, enabled = %s", __FUNCTION__, enable?"true":"false");

    if (enable)
        return mFlashSubdev->setControl(V4L2_CID_FLASH_CHARGE,
                                        1, "flash charge");
    else
        return mFlashSubdev->setControl(V4L2_CID_FLASH_CHARGE,
                                        0, "flash charge");
}

int FlashHw::isFlashReady(bool &status)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->getControl(V4L2_CID_FLASH_READY,
                                    reinterpret_cast<int*>(&status));
}

int FlashHw::setAeFlashMode(v4l2_flash_led_mode mode)
{
    LOG2("@%s: %d", __FUNCTION__, mode);
    return mFlashSubdev->setControl(V4L2_CID_FLASH_LED_MODE,
                                    mode, "Flash mode");
}

int FlashHw::getAeFlashMode(v4l2_flash_led_mode &mode)
{
    LOG2("@%s", __FUNCTION__);
    return mFlashSubdev->getControl(V4L2_CID_FLASH_LED_MODE,
                                    reinterpret_cast<int*>(&mode));
}

int FlashHw::getCurrentCameraId(void)
{
    LOG1("@%s, id: %d", __FUNCTION__, mCameraId);
    return mCameraId;
}

const char* FlashHw::getFlashName(void)
{
    return mFlashInput.name;
}

}   // namespace camera2
}   // namespace android
