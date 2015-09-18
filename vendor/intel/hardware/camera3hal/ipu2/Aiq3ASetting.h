/*
 * Copyright (c) 2013 Intel Corporation.
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

#ifndef _CAMERA3_HAL_AIQ_3A_SETTTING_H_
#define _CAMERA3_HAL_AIQ_3A_SETTTING_H_

namespace android {
namespace camera2 {

class AAASetting {
public:
    virtual ~AAASetting() {}

    virtual status_t init3ASetting() {return NO_ERROR;}
    virtual status_t apply3ASetting(const CameraMetadata &settings) {
        UNUSED(settings);
        return NO_ERROR;
    }

    virtual status_t apply3ATrigger(const CameraMetadata &settings) {
        UNUSED(settings);
        return NO_ERROR;
    }

    virtual status_t newFrameDone() {return NO_ERROR;}

    virtual bool isPerFrameSettingSupported() {return false;}
    virtual status_t perFrameSetting() {return NO_ERROR;}
    virtual status_t startRequestSetting() {return NO_ERROR;}

    virtual status_t fill3AMetadata() {return NO_ERROR;}
    virtual status_t fillShutter() {return NO_ERROR;}

    /* TODO: all the follow class member access functions are temporary. Because Aiq3AThread needs to access
     * Aiq3ARawSensor member temporarily, These will be removed when finish AAASetting implemention. */
    virtual uint8_t getRequestAeTriggerState() = 0;
    virtual void setRequestAeTriggerState(uint8_t value) = 0;

    virtual int32_t getCurrentAeTriggerId() = 0;
    virtual void setCurrentAeTriggerId(int32_t value) = 0;

    virtual int32_t getCurrentAfTriggerId() = 0;
    virtual void setCurrentAfTriggerId(int32_t id) = 0;

    virtual uint8_t getRequestAfTriggerState() = 0;
    virtual uint8_t popRequestAfTriggerState() = 0;
    virtual void storeRequestAfTriggerState(uint8_t state) = 0;

    virtual uint8_t getRequestAfMode() = 0;
    virtual void setRequestAfMode(uint8_t value) = 0;

    virtual AfMode getAfMode() = 0;
    virtual void setAfMode(AfMode value) = 0;

    virtual camera_metadata_enum_android_control_af_state getAndroidAfState() = 0;
    virtual void setAndroidAfState (camera_metadata_enum_android_control_af_state value) = 0;

    virtual camera_metadata_enum_android_control_af_trigger getLastAfTrigger() = 0;
    virtual void setLastAfTrigger(camera_metadata_enum_android_control_af_trigger value) = 0;

    virtual bool get3ARunning() = 0;
    virtual void set3ARunning(bool enable) = 0;

    virtual bool getStartAf() = 0;
    virtual void setStartAf(bool enable) = 0;

    virtual bool getForceAeLock() = 0;
    virtual void setForceAeLock(bool enable) = 0;

    virtual bool getAeLockFlash() = 0;
    virtual void setAeLockFlash(bool enable) = 0;

    virtual bool lowPowerMode(void) = 0;
    virtual FlashMode getFlashMode(void) = 0;

    virtual bool getForceAwbLock() = 0;
    virtual void setForceAwbLock(bool enable) = 0;
}; // class AAASetting

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_AIQ_3A_SETTTING_H_
