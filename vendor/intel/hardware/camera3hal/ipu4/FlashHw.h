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

#ifndef CAMERA2600_FLASHHW_H_
#define CAMERA2600_FLASHHW_H_

#include "ICameraIPU4HwControls.h"
#include "PlatformData.h"

namespace android {
namespace camera2 {

class V4L2Subdevice;
class MediaController;
class MediaEntity;
/**
 * \class FlashHw
 * This class adds the methods that are needed
 * to drive the camera flash using v4l2 commands and custom ioctl.
 *
 */
class FlashHw : public IIPU4HWFlashControl, public RefBase {
public:
    FlashHw(int cameraId, sp<MediaController> mediaCtl);
    ~FlashHw();

    status_t setFlash(sp<MediaEntity> entity);

    /* IHWFlashControl overloads */

    virtual const char* getFlashName(void);
    virtual int getCurrentCameraId(void);

    virtual int setFlashStrobeSource(int source);
    virtual int getFlashStrobeSource(int &source);
    virtual int flashStrobe(bool enable);
    virtual int getFlashStrobeStatus(bool &enabled);
    virtual int setFlashTimeOut(int timeOut);
    virtual int setFlashIntensity(flash_usage usage, int intensity);
    virtual int getFlashIntensity(flash_usage usage, int &intensity);
    virtual int getFlashFault(unsigned int &bitmask);
    virtual int flashCharge(bool enable);
    virtual int isFlashReady(bool &status);

    virtual int setAeFlashMode(v4l2_flash_led_mode mode);
    virtual int getAeFlashMode(v4l2_flash_led_mode &mode);

private:
    static const int MAX_FLASH_NAME_LENGTH = 32;

    struct flashInfo {
        uint32_t index;      //!< V4L2 index
        char name[MAX_FLASH_NAME_LENGTH];
    };

private:
    int mCameraId;
    sp<MediaController> mMediaCtl;
    sp<V4L2Subdevice> mFlashSubdev;
    struct flashInfo mFlashInput;
};  // class FlashHW

};  // namespace camera2
};  // namespace android

#endif  // CAMERA2600_FLASHHW_H_
