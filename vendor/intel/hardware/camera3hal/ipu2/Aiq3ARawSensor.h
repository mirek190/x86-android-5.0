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

#ifndef _CAMERA3_RAW_SENSOR_SETTTING_H_
#define _CAMERA3_RAW_SENSOR_SETTTING_H_

#include "Aiq3A.h"
#include "Aiq3ASetting.h"

namespace android {
namespace camera2 {

class Aiq3ARawSensor : public AAASetting {
public:
    /* TODO, The sensor parameter is temporary added. Because if we new sensor in Aiq3ARawSensor,
     * some class member will re-init to NULL, that causes some unexpected errors. This parameter
     * will be removed in furture.
     */
    Aiq3ARawSensor(HWControlGroup &hwcg, CameraMetadata &staticMeta, I3AControls *sensor);
    ~Aiq3ARawSensor();

    bool lowPowerMode(void);
    FlashMode getFlashMode(void) { return mFlashMode; }

    status_t init3ASetting();
    status_t apply3ASetting(const CameraMetadata &settings);
    status_t apply3ATrigger(const CameraMetadata &settings);

    status_t newFrameDone();

    bool isPerFrameSettingSupported();
    status_t perFrameSetting();
    status_t startRequestSetting();

    status_t fill3AMetadata();
    status_t fillShutter();

    /* TODO: all the follow class member access functions are temporary. Because Aiq3AThread also needs to reference
     * Aiq3ARawSensor internal member temporarily, These will be removed when finish AAASetting implemention. */
    uint8_t getRequestAeTriggerState() {return mRequestAeTriggerState;}
    void setRequestAeTriggerState(uint8_t value) {mRequestAeTriggerState = value;}

    int32_t getCurrentAeTriggerId() {return mCurrentAeTriggerId;}
    void setCurrentAeTriggerId(int32_t value) {mCurrentAeTriggerId = value;}

    int32_t getCurrentAfTriggerId() {return mCurrentAfTriggerId;}
    void setCurrentAfTriggerId(int32_t id) {mCurrentAfTriggerId = id;}

    uint8_t getRequestAfTriggerState();
    uint8_t popRequestAfTriggerState();
    void storeRequestAfTriggerState(uint8_t state);

    uint8_t getRequestAfMode() {return mRequestAfMode;}
    void setRequestAfMode(uint8_t value) {mRequestAfMode = value;}

    AfMode getAfMode() {return mAfMode;}
    void setAfMode(AfMode value) {mAfMode = value;}

    camera_metadata_enum_android_control_af_state getAndroidAfState() {return mAndroidAfState;}
    void setAndroidAfState (camera_metadata_enum_android_control_af_state value) {mAndroidAfState = value;}

    camera_metadata_enum_android_control_af_trigger getLastAfTrigger() {return mLastAfTrigger;}
    void setLastAfTrigger(camera_metadata_enum_android_control_af_trigger value) {mLastAfTrigger = value;}

    bool get3ARunning() {return m3ARunning;}
    void set3ARunning(bool enable) {m3ARunning = enable;}

    bool getStartAf() {return mStartAf;}
    void setStartAf(bool enable) {mStartAf = enable;}

    bool getForceAeLock() {return mForceAeLock;}
    void setForceAeLock(bool enable) {mForceAeLock = enable;}

    bool getAeLockFlash() {return mAeLockFlash;}
    void setAeLockFlash(bool enable) {mAeLockFlash = enable;}

    bool getForceAwbLock() {return mForceAwbLock;}
    void setForceAwbLock(bool enable) {mForceAwbLock = enable;}

private:
    status_t processSceneSetting(const CameraMetadata &settings);

    /*****************Ae Setting*********************/
    status_t processAeSetting(const CameraMetadata &settings);
    status_t processSensitivitySetting(const CameraMetadata &settings);
    status_t processExposureTimeSetting(const CameraMetadata &settings);
    status_t processFrameDurationSetting(const CameraMetadata &settings);
    status_t processAePrecaptureTriggerSetting(const CameraMetadata &settings);
    status_t handleLockAe(bool enable);
    status_t processAeLockSetting(const CameraMetadata &settings);
    status_t processEvCompensationSetting(const CameraMetadata &settings);
    status_t processAntibandingSetting(const CameraMetadata &settings);
    void convertFromAndroidToIaCoordinates(const CameraWindow &srcWindow, CameraWindow &toWindow);
    status_t processAeRegionsSetting(const CameraMetadata &settings);
    status_t processFlashModeSetting(const CameraMetadata &settings, uint8_t aeMode);

    /*****************AF Setting*********************/
    status_t processAfSetting(const CameraMetadata &settings);
    status_t processAfRegionsSetting(const CameraMetadata &settings);
    status_t processAfTriggerSetting(const CameraMetadata &settings);

    /*****************AWB Setting*********************/
    status_t handleLockAwb(bool enable);
    status_t processAwbSetting(const CameraMetadata &settings);

    status_t processColorEffectSetting(const CameraMetadata &settings);

    status_t processNREESetting(const CameraMetadata &settings);
    status_t processColorCorrectionSettings(const CameraMetadata &settings);

    status_t getBatteryStatusByFs(void);

private:
    I3AControls *mSensor;
    IHWIspControl *mISP;
    IHWSensorControl *mSensorCI;
    CameraMetadata mStaticMeta;

    Vector<uint8_t> mRequestAfTriggerState;
    int32_t mCurrentAfTriggerId;
    uint8_t mRequestAeTriggerState;
    int32_t mCurrentAeTriggerId;

    uint8_t mRequestAfMode;
    AfMode mAfMode;

    camera_metadata_enum_android_control_af_state mAndroidAfState;
    camera_metadata_enum_android_control_af_mode mAndroidAfMode;
    camera_metadata_enum_android_control_af_trigger mLastAfTrigger;

    bool m3ARunning;
    bool mForceAeLock;
    bool mAeLockFlash;

    bool mForceAwbLock;

    bool mStartAf;

    FlashMode mFlashMode;
    bool mLowBattery;
    int mFrameNum;

}; // class Aiq3ARawSensor

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_RAW_SENSOR_SETTTING_H_
