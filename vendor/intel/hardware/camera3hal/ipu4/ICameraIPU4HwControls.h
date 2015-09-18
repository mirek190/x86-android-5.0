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

#ifndef CAMERA2600_ICAMERAIPU4HWCONTROLS_H_
#define CAMERA2600_ICAMERAIPU4HWCONTROLS_H_

#include <linux/videodev2.h>

namespace android {
namespace camera2 {

#define CAM_WXH_STR(w,h) STRINGIFY_(w##x##h)
#define CAM_RESO_STR(w,h) CAM_WXH_STR(w,h)  // example: CAM_RESO_STR(VGA_WIDTH,VGA_HEIGHT) -> "640x480"

enum flash_usage { FLASH, TORCH, INDICATOR };

typedef struct {
    //ToDo: add needed content
} pSysInputParameters;

/* Abstraction of HW sensor control interface for 3A support */
class IIPU4HWSensorControl {
public:
    virtual ~IIPU4HWSensorControl() { }

    virtual int getCurrentCameraId(void) = 0;

    virtual int getActivePixelArraySize(int &width, int &height, int &code) = 0;
    virtual int getSensorOutputSize(int &width, int &height, int &code) = 0;
    virtual int getPixelRate(int &pixel_rate) = 0;
    virtual int setExposure(int coarse_exposure, int fine_exposure) = 0;
    virtual int getExposure(int &coarse_exposure, int &fine_exposure) = 0;
    virtual int setGains(int analog_gain, int digital_gain) = 0;
    virtual int getGains(int &analog_gain, int &digital_gain) = 0;
    virtual int setFrameDuration(unsigned int llp, unsigned int fll) = 0;
    virtual int getFrameDuration(unsigned int &llp, unsigned int &fll) = 0;
};

/* Abstraction of HW flash control interface for 3A support */
class IIPU4HWFlashControl {
public:
    virtual ~IIPU4HWFlashControl() { }

    virtual const char* getFlashName(void) = 0;
    virtual int getCurrentCameraId(void) = 0;

    virtual int setFlashStrobeSource(int source) = 0;
    virtual int getFlashStrobeSource(int &source) = 0;
    virtual int FlashStrobe(bool enable) = 0;
    virtual int getFlashStrobeStatus(bool &enabled) = 0;
    virtual int setFlashTimeOut(int timeOut) = 0;
    virtual int setFlashIntensity(flash_usage usage, int intensity) = 0;
    virtual int getFlashIntensity(flash_usage usage, int &intensity) = 0;
    virtual int getFlashFault(int &bitmask) = 0;
    virtual int flashCharge(bool enable) = 0;
    virtual int isFlashReady(bool &status) = 0;
    virtual int setAeFlashMode(v4l2_flash_led_mode mode) = 0;
    virtual int getAeFlashMode(v4l2_flash_led_mode &mode) = 0;
};

/* Abstraction of HW lens control interface for 3A support */
class IIPU4HWLensControl {
public:
    virtual ~IIPU4HWLensControl() { }

    virtual const char* getLensName(void)= 0;
    virtual int getCurrentCameraId(void)= 0;

    // FOCUS
    virtual int moveFocusToPosition(int position) = 0;
    virtual int moveFocusToBySteps(int steps) = 0;
    virtual int getFocusPosition(int &position) = 0;
    virtual int getFocusStatus(int &status) = 0;
    virtual int startAutoFocus(void) = 0;
    virtual int stopAutoFocus(void) = 0;
    virtual int getAutoFocusStatus(int &status) = 0;
    virtual int setAutoFocusRange(int value) = 0;
    virtual int getAutoFocusRange(int &value) = 0;

    // ZOOM
    virtual int moveZoomToPosition(int position) = 0;
    virtual int moveZoomToBySteps(int &steps) = 0;
    virtual int getZoomPosition(int &position) = 0;
    virtual int moveZoomContinuous(int position) = 0;

};

}   // namespace camera2
}   // namespace android
#endif  // CAMERA2600_ICAMERAIPU4HWCONTROLS_H_
