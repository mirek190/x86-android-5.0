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

#define LOG_TAG "Camera_Aiq3ARawSensor"

#include "LogHelper.h"
#include <sys/types.h>
#include "camera/CameraMetadata.h"
#include "PlatformData.h"
#include "ia_cmc_parser.h"
#include "Aiq3A.h"
#include "ia_coordinate.h"
#include "ICameraHwControls.h"
#include "MessageThread.h"
#include "MessageQueue.h"
#include "IPU2CameraHw.h"
#include "Aiq3AThread.h"
#include "Aiq3ASetting.h"
#include "Aiq3ARawSensor.h"
#include "BatteryStatus.h"

namespace android {
namespace camera2 {

// check battery status every 10 frames
const int BATTERY_CHECK_INTERVAL_FRAME_UNIT = 10;

Aiq3ARawSensor::Aiq3ARawSensor(HWControlGroup &hwcg, CameraMetadata &staticMeta, I3AControls *sensor):
    mSensor(sensor)
    ,mISP(hwcg.mIspCI)
    ,mSensorCI(hwcg.mSensorCI)
    ,mStaticMeta(staticMeta)
    ,mCurrentAfTriggerId(0)
    ,mRequestAeTriggerState(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE)
    ,mCurrentAeTriggerId(0)
    ,mRequestAfMode(ANDROID_CONTROL_AF_MODE_OFF)
    ,mAfMode(CAM_AF_MODE_NOT_SET)
    ,mAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE)
    ,mAndroidAfMode(ANDROID_CONTROL_AF_MODE_OFF)
    ,mLastAfTrigger(ANDROID_CONTROL_AF_TRIGGER_IDLE)
    ,m3ARunning(false)
    ,mForceAeLock(false)
    ,mAeLockFlash(false)
    ,mForceAwbLock(false)
    ,mStartAf(false)
    ,mFlashMode(CAM_AE_FLASH_MODE_OFF)
    ,mLowBattery(false)
    ,mFrameNum(0)
{
    mRequestAfTriggerState.clear();
    mRequestAfTriggerState.setCapacity(MAX_REQUEST_IN_PROCESS_NUM);
}

Aiq3ARawSensor::~Aiq3ARawSensor()
{
    mRequestAfTriggerState.clear();
}

status_t Aiq3ARawSensor::init3ASetting()
{
    mSensor->set3AColorEffect(CAM_EFFECT_NONE);
    mSensor->setAeFlashMode(CAM_AE_FLASH_MODE_AUTO);
    mSensor->setEv(0);
    mSensor->setAeFlickerMode(CAM_AE_FLICKER_MODE_OFF);
    mSensor->setAwbMode(CAM_AWB_MODE_AUTO);
    mSensor->setAfMode(CAM_AF_MODE_AUTO);
    mSensor->setManualIso(100);
    mSensor->setAeMode(CAM_AE_MODE_AUTO);
    mSensor->setIsoMode(CAM_AE_ISO_MODE_AUTO);
    mSensor->setAeMeteringMode(CAM_AE_METERING_MODE_AUTO);

    // initialize low battery status
    lowPowerMode();

    return NO_ERROR;
}

bool Aiq3ARawSensor::isPerFrameSettingSupported()
{
    return false;
}

status_t Aiq3ARawSensor::perFrameSetting()
{
    return NO_ERROR;
}

status_t Aiq3ARawSensor::startRequestSetting()
{
    return NO_ERROR;
}

status_t Aiq3ARawSensor::apply3ASetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    unsigned int tag = ANDROID_CONTROL_MODE;
    camera_metadata_ro_entry entry = settings.find(tag);
    if (entry.count == 1) {
        if (entry.data.u8[0] == ANDROID_CONTROL_MODE_OFF) {
            m3ARunning = false;
        } else {
            m3ARunning = true;
        }
    }

    if ((status = processSceneSetting(settings)) != OK)
        return status;

    if ((status = processAeSetting(settings)) != OK)
        return status;

    if ((status = processAwbSetting(settings)) != OK)
        return status;

    if ((status = processColorEffectSetting(settings)) != OK)
        return status;

    if ((status = processNREESetting(settings)) != OK)
        return status;

    if ((status = processColorCorrectionSettings(settings)) != OK)
        return status;

    return status;
}

status_t Aiq3ARawSensor::apply3ATrigger(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    unsigned int tag = ANDROID_CONTROL_MODE;
    camera_metadata_ro_entry entry = settings.find(tag);
    if (entry.count == 1) {
        if (entry.data.u8[0] == ANDROID_CONTROL_MODE_OFF) {
            m3ARunning = false;
        } else {
            m3ARunning = true;
        }
    }
    if ((status = processAfTriggerSetting(settings)) != OK)
        return status;

    if ((status = processAePrecaptureTriggerSetting(settings)) != OK)
        return status;

    return status;
}

status_t Aiq3ARawSensor::processSceneSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_SCENE_MODE);
    if (entry.count != 1) {
        return OK;
    }

    uint8_t value = entry.data.u8[0];
    LOG1("value=%d", value);
    SceneMode newValue = \
        (value == ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY) ? CAM_AE_SCENE_MODE_AUTO :
        (value == ANDROID_CONTROL_SCENE_MODE_ACTION) ? CAM_AE_SCENE_MODE_SPORTS :
        (value == ANDROID_CONTROL_SCENE_MODE_PORTRAIT) ? CAM_AE_SCENE_MODE_PORTRAIT :
        (value == ANDROID_CONTROL_SCENE_MODE_LANDSCAPE) ? CAM_AE_SCENE_MODE_LANDSCAPE :
        (value == ANDROID_CONTROL_SCENE_MODE_NIGHT) ? CAM_AE_SCENE_MODE_NIGHT :
        (value == ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT) ? CAM_AE_SCENE_MODE_NIGHT_PORTRAIT :
        (value == ANDROID_CONTROL_SCENE_MODE_THEATRE) ? CAM_AE_SCENE_MODE_AUTO :
        (value == ANDROID_CONTROL_SCENE_MODE_BEACH) ? CAM_AE_SCENE_MODE_BEACH_SNOW :
        (value == ANDROID_CONTROL_SCENE_MODE_SNOW) ? CAM_AE_SCENE_MODE_BEACH_SNOW :
        (value == ANDROID_CONTROL_SCENE_MODE_SUNSET) ? CAM_AE_SCENE_MODE_SUNSET :
        (value == ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO) ? CAM_AE_SCENE_MODE_AUTO :
        (value == ANDROID_CONTROL_SCENE_MODE_FIREWORKS) ? CAM_AE_SCENE_MODE_FIREWORKS :
        (value == ANDROID_CONTROL_SCENE_MODE_SPORTS) ? CAM_AE_SCENE_MODE_SPORTS :
        (value == ANDROID_CONTROL_SCENE_MODE_PARTY) ? CAM_AE_SCENE_MODE_PARTY :
        (value == ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT) ? CAM_AE_SCENE_MODE_CANDLELIGHT :
        (value == ANDROID_CONTROL_SCENE_MODE_BARCODE) ? CAM_AE_SCENE_MODE_TEXT :
             CAM_AE_SCENE_MODE_AUTO;

    status = mSensor->setAeSceneMode(newValue);
    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

/*****************Ae Setting*********************/
status_t Aiq3ARawSensor::processAeSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;

    uint8_t androidAeMode = ANDROID_CONTROL_AE_MODE_OFF;
    AeMode aiqAeMode = CAM_AE_MODE_MANUAL;
    camera_metadata_ro_entry entry = settings.find(ANDROID_CONTROL_AE_MODE);
    if (entry.count == 1) {
        androidAeMode = entry.data.u8[0];
        // ae mode
        aiqAeMode = (androidAeMode == ANDROID_CONTROL_AE_MODE_OFF) ?
                 CAM_AE_MODE_MANUAL : CAM_AE_MODE_AUTO;
    }

    if (m3ARunning == false) // if whole 3A is turned off, it overrides
        aiqAeMode = CAM_AE_MODE_MANUAL;

    status = mSensor->setAeMode(aiqAeMode);
    LOG2("@%s: set AeMode to %d (andr val %d)", __FUNCTION__, aiqAeMode,
         androidAeMode);

    if (status == OK) {
        if (aiqAeMode == CAM_AE_MODE_MANUAL) {
            status |= processSensitivitySetting(settings);
            status |= processExposureTimeSetting(settings);
            status |= processFrameDurationSetting(settings);
        } else {
            status |= processAeLockSetting(settings);
            status |= processEvCompensationSetting(settings);
            status |= processAntibandingSetting(settings);
            status |= processAeRegionsSetting(settings);
        }
        status |= processFlashModeSetting(settings, androidAeMode);
    }

    return status;
}

status_t Aiq3ARawSensor::processSensitivitySetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_SENSOR_SENSITIVITY);
    if (entry.count == 1) {
        int32_t iso = entry.data.i32[0];
        LOG2("@%s: setting ISO to %d", __FUNCTION__, iso);
        status = mSensor->setManualIso(iso);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processExposureTimeSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_SENSOR_EXPOSURE_TIME);
    if (entry.count == 1) {
        // TODO consider changing the AIQ API from float seconds
        uint64_t timeNs = (uint64_t) entry.data.i64[0];
        float expTime = timeNs / 1000000000.0f;
        LOG2("@%s: setting exp time to %f", __FUNCTION__, expTime);
        status = mSensor->setManualShutter(expTime);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processFrameDurationSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_SENSOR_FRAME_DURATION);
    if (entry.count == 1) {
        uint64_t frameDuration = (uint64_t) entry.data.i64[0];
        LOG2("@%s: setting frameDuration to %lld", __FUNCTION__, frameDuration);
        // convert the frameduration(ns) to fps
        int fps = 1000000000/ frameDuration;
        status = mSensor->setFrameRate(fps);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processAePrecaptureTriggerSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);

    if (entry.count == 1) {
        mRequestAeTriggerState = entry.data.u8[0];
        if (mRequestAeTriggerState == ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
            entry = settings.find(ANDROID_CONTROL_AE_PRECAPTURE_ID);
            if (entry.count == 1) {
                int32_t triggerId = entry.data.i32[0];
                LOG2("@%s: trigger id = %d", __FUNCTION__, triggerId);
                mCurrentAeTriggerId = triggerId;
            }
        }
    }
    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::handleLockAe(bool enable)
{
    mForceAeLock = enable;

    // AE/AWB lock is controlled by still AF,
    // Don't set here
    if (!m3ARunning || mStartAf)
        return NO_ERROR;

    mSensor->setAeLock(mForceAeLock);
    if (mForceAeLock)
        mAeLockFlash = mSensor->getAeFlashNecessary();
    else
        mAeLockFlash = false;

    return NO_ERROR;
}

status_t Aiq3ARawSensor::processAeLockSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry = settings.find(ANDROID_CONTROL_AE_LOCK);
    if (entry.count == 1) {
        status = handleLockAe(ANDROID_CONTROL_AE_LOCK_ON == entry.data.u8[0]);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processEvCompensationSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
    if (entry.count == 1) {
        int32_t evCompensation = entry.data.i32[0];
        float step = mSensorCI->getStepEV();
        status = mSensor->setEv(evCompensation * step);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processAntibandingSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
    if (entry.count == 1) {
        uint8_t abMode = entry.data.u8[0];
        FlickerMode newValue = \
                    (abMode == ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ) ?
                            CAM_AE_FLICKER_MODE_50HZ : \
                    (abMode == ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ) ?
                            CAM_AE_FLICKER_MODE_60HZ : \
                    (abMode == ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO) ?
                            CAM_AE_FLICKER_MODE_AUTO : \
                    CAM_AE_FLICKER_MODE_OFF;
        status = mSensor->setAeFlickerMode((FlickerMode)newValue);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

void Aiq3ARawSensor::convertFromAndroidToIaCoordinates(const CameraWindow &srcWindow, CameraWindow &toWindow)
{
    // FIXME these magic values are from ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
    // and they can't be hard-coded like this, they should be fetched here or
    // parameters.. maybe from platform data or cpf
    const ia_coordinate_system androidCoord = {0, 0, 2448, 3264};
    const ia_coordinate_system iaCoord = {IA_COORDINATE_TOP, IA_COORDINATE_LEFT,
            IA_COORDINATE_BOTTOM, IA_COORDINATE_RIGHT};
    ia_coordinate topleft = {0, 0};
    ia_coordinate bottomright = {0, 0};

    topleft.x = srcWindow.x_left;
    topleft.y = srcWindow.y_top;
    bottomright.x = srcWindow.x_right;
    bottomright.y = srcWindow.y_bottom;

    topleft = ia_coordinate_convert(&androidCoord,
                                    &iaCoord, topleft);
    bottomright = ia_coordinate_convert(&androidCoord,
                                        &iaCoord, bottomright);

    toWindow.x_left = topleft.x;
    toWindow.y_top = topleft.y;
    toWindow.x_right = bottomright.x;
    toWindow.y_bottom = bottomright.y;
}

status_t Aiq3ARawSensor::processAeRegionsSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_REGIONS);
    if (entry.count >= 4) {
        CameraWindow src;
        CameraWindow dst;
        src.x_left   = entry.data.i32[0];
        src.y_top    = entry.data.i32[1];
        src.x_right  = entry.data.i32[2];
        src.y_bottom = entry.data.i32[3];
        if (entry.count == 5)
            dst.weight = entry.data.i32[4];  // weight is not converted yet
        else
            dst.weight = 1;

        convertFromAndroidToIaCoordinates(src, dst);

        // fixme - why AIQ needs the mode change for AE window, but not for
        // af window (for which it internally changes mode) ?
        if (src.x_right != 0 || src.y_bottom != 0)
            status = mSensor->setAeMeteringMode(CAM_AE_METERING_MODE_SPOT);
        else
            status = mSensor->setAeMeteringMode(CAM_AE_METERING_MODE_AUTO);

        status |= mSensor->setAeWindow(&dst);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::getBatteryStatusByFs(void)
{
    static const char *CamFlashCtrlFS = "/dev/bcu/camflash_ctrl";

    status_t status;
    char buf[4];
    FILE *fp = NULL;

    LOG1("@%s", __FUNCTION__);
    if (::access(CamFlashCtrlFS, R_OK)) {
        LOG1("@%s, file %s is not readable", __FUNCTION__, CamFlashCtrlFS);
        return BATTERY_STATUS_INVALID;
    }

    fp = ::fopen(CamFlashCtrlFS, "r");
    if (NULL == fp) {
        ALOGW("@%s, file %s open with err:%s", __FUNCTION__, CamFlashCtrlFS, strerror(errno));
        return BATTERY_STATUS_INVALID;
    }
    CLEAR(buf);
    size_t len = ::fread(buf, 1, 1, fp);
    if (len == 0) {
        ALOGW("@%s, fail to read 1 byte from camflash_ctrl", __FUNCTION__);
        ::fclose(fp);
        return BATTERY_STATUS_INVALID;
    }
    ::fclose(fp);

    status = atoi(buf);
    if (status > BATTERY_STATUS_CRITICAL || status < BATTERY_STATUS_NORMAL)
        return BATTERY_STATUS_INVALID;

    if (status > BATTERY_STATUS_NORMAL)
        LOG2("@%s warning battery status:%d", __FUNCTION__, status);

    return status;
}

bool Aiq3ARawSensor::lowPowerMode(void)
{
    LOG2("@%s:", __FUNCTION__);

    // check battery status for every 300ms (about 10 frames).
    if (mFrameNum % BATTERY_CHECK_INTERVAL_FRAME_UNIT != 0) {
        ++mFrameNum;
        return mLowBattery;
    }

    // for platforms on imin_legacy, it still uses the sysFs to control battery status.
    // so we should check /dev/bcu/camflash_ctrl first. if it can be accessed,
    // we check camera.flash.throttling_levels later.
    status_t throttlingLevel = getBatteryStatusByFs();
    if (throttlingLevel == BATTERY_STATUS_INVALID) {
        throttlingLevel = getBatteryStatus();
    }

    LOG2("throttling level is %d", throttlingLevel);

    if (throttlingLevel == BATTERY_STATUS_CRITICAL) {
        mLowBattery = true;
    } else {
        mLowBattery = false;
    }

    ++mFrameNum;
    mFrameNum %= BATTERY_CHECK_INTERVAL_FRAME_UNIT;

    return mLowBattery;
}

status_t Aiq3ARawSensor::processFlashModeSetting(const CameraMetadata &settings, uint8_t aeMode)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    camera_metadata_ro_entry entry = settings.find(ANDROID_FLASH_MODE);
    if (entry.count == 1) {
        flashMode = entry.data.u8[0];
    }

    FlashMode newValue = CAM_AE_FLASH_MODE_OFF;
    if (aeMode == ANDROID_CONTROL_AE_MODE_ON || aeMode == ANDROID_CONTROL_AE_MODE_OFF) {
        if (flashMode == ANDROID_FLASH_MODE_TORCH) {
            newValue = CAM_AE_FLASH_MODE_TORCH;
        } else if (flashMode == ANDROID_FLASH_MODE_SINGLE) {
            newValue = CAM_AE_FLASH_MODE_SINGLE;
        } else {
            newValue = CAM_AE_FLASH_MODE_OFF;
        }
    } else if (aeMode == ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH) {
        newValue = CAM_AE_FLASH_MODE_ON;
    } else if (aeMode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH) {
        newValue = CAM_AE_FLASH_MODE_AUTO;
    } else if (aeMode == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE) {
        newValue = CAM_AE_FLASH_MODE_AUTO;
    }

    mFlashMode = newValue;

    if ((mFlashMode != CAM_AE_FLASH_MODE_OFF) && mLowBattery) {
        if (mSensor->getAeFlashMode() != CAM_AE_FLASH_MODE_OFF)
            LOGD("lower battery, force to set flash mode to OFF");
        newValue = CAM_AE_FLASH_MODE_OFF;
    }

    status = mSensor->setAeFlashMode(newValue);
    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    LOG2("@%s: flashMode=%d, aeMode=%d, newValue=%d", __FUNCTION__, flashMode, aeMode, newValue);
    return status;
}

/*****************AF Setting*********************/
status_t Aiq3ARawSensor::processAfSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry;

    entry = settings.find(ANDROID_CONTROL_AF_MODE);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];
        mRequestAfMode = value;

        LOG2("@%s: af_mode = %d", __FUNCTION__, value);

        AfMode newValue = \
            (value == ANDROID_CONTROL_AF_MODE_OFF) ? CAM_AF_MODE_MANUAL : \
            (value == ANDROID_CONTROL_AF_MODE_MACRO) ? CAM_AF_MODE_MACRO : \
            (value == ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) ?
                    CAM_AF_MODE_CONTINUOUS : \
            (value == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) ?
                    CAM_AF_MODE_CONTINUOUS : \
            CAM_AF_MODE_AUTO;
        if ((AfMode)newValue != mAfMode) {
            // afmode can't be set always, as Aiq3A changes the mode
            // internally during startStillAf. So we only call setAfMode when
            // the setting really changes.
            mAfMode = (AfMode)newValue;
            LOG2("@%s: mode is now %d", __FUNCTION__, mAfMode);
            mSensor->setAfMode(mAfMode);
        }

        // resets if mode changes (trigger etc)
        if (value != mAndroidAfMode) {
            mLastAfTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
            mAndroidAfState = ANDROID_CONTROL_AF_STATE_INACTIVE;
            mAndroidAfMode =
                    (camera_metadata_enum_android_control_af_mode) value;
            mSensor->setAfLock(false);
            mSensor->setAfEnabled(true);
            LOG2("@%s: reset and unlocked af", __FUNCTION__);
        }

        status |= processAfRegionsSetting(settings);
    }

    // handle the focus distance setting
    if (mRequestAfMode == ANDROID_CONTROL_AF_MODE_OFF) {
        float focusDistanceApp = 0.0;
        entry = settings.find(ANDROID_LENS_FOCUS_DISTANCE);
        if (entry.count == 1) {
            int count = 0;
            float *min_focus_distance = (float *)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
                                             TYPE_FLOAT,
                                             &count);
            if (min_focus_distance != NULL && entry.data.f[0] > *min_focus_distance) {
                // less than the min focus distance, not support.
                // set it to min focus distance
                focusDistanceApp = *min_focus_distance;
            }
            else
                focusDistanceApp = entry.data.f[0];
            LOG2("focus_distance from app :%f", focusDistanceApp);

            // unit is mm required by AIQ, default focus distance is 10m
            int focus_distance = 10000;
            // unit from application is 1/m, convert to mm needed by AIQ
            // When the distance is greater than 10 meter, think it as infinity
            if ((focusDistanceApp - 0.0) > 0.1)
                focus_distance = 1000 / focusDistanceApp;
            mSensor->setManualFocusDistance(focus_distance);
        }
    }

    return status;
}

status_t Aiq3ARawSensor::processAfRegionsSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AF_REGIONS);
    if (entry.count >= 4) {
        CameraWindow src;
        CameraWindow dst;
        src.x_left   = entry.data.i32[0];
        src.y_top    = entry.data.i32[1];
        src.x_right  = entry.data.i32[2];
        src.y_bottom = entry.data.i32[3];
        if (entry.count == 5)
            dst.weight = entry.data.i32[4];  // weight is not converted yet
        else
            dst.weight = 1;

        convertFromAndroidToIaCoordinates(src, dst);

        status = mSensor->setAfWindows(&dst, 1);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processAfTriggerSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AF_TRIGGER);
    if (entry.count == 1) {
        LOG2("af trigger count:%d, value:%d", entry.count, entry.data.u8[0]);
        storeRequestAfTriggerState(entry.data.u8[0]);
        if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_START) {
            entry = settings.find(ANDROID_CONTROL_AF_TRIGGER_ID);
            if (entry.count == 1) {
                int32_t triggerId = entry.data.i32[0];
                LOG2("@%s: trigger id = %d", __FUNCTION__, triggerId);
                mCurrentAfTriggerId = triggerId;
            }
        }
    }
    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    if ((status = processAfSetting(settings)) != OK)
        return status;

    return status;
}

uint8_t Aiq3ARawSensor::getRequestAfTriggerState()
{
    uint8_t trigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    //always pick up oldest trgger state
    if (mRequestAfTriggerState.size() > 0)
        trigger = mRequestAfTriggerState.top();
    return trigger;
}

uint8_t Aiq3ARawSensor::popRequestAfTriggerState()
{
    uint8_t trigger = getRequestAfTriggerState();
    mRequestAfTriggerState.pop();
    return trigger;
}

void Aiq3ARawSensor::storeRequestAfTriggerState(uint8_t state)
{
    //If the queue reachs the capacity, remove the oldest one.
    if(mRequestAfTriggerState.size() == mRequestAfTriggerState.capacity())
        mRequestAfTriggerState.pop();
    mRequestAfTriggerState.push_front(state);
}

status_t Aiq3ARawSensor::processColorCorrectionSettings(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    int count = 0;
    uint8_t * aberration_correction_modes = (uint8_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                     ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
                                     TYPE_BYTE,
                                     &count);
    // FOR IPU2 only off mode support
    if (count == 1 && aberration_correction_modes != NULL &&
                      aberration_correction_modes[0] == ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF) {
        camera_metadata_ro_entry entry;
        entry = settings.find(ANDROID_COLOR_CORRECTION_ABERRATION_MODE);
        if (entry.count == 1 && entry.data.u8[0] != ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF)
            status == INVALID_OPERATION;
    }

    return status;
}


/*****************AWB Setting*********************/
status_t Aiq3ARawSensor::handleLockAwb(bool enable)
{
    LOG2("@%s: new lock state: %s", __FUNCTION__, enable ? "enabled" : "disabled");
    mForceAwbLock = enable;

    // AE/AWB lock is controlled by still AF,
    // Don't set here
    if (!m3ARunning || mStartAf)
        return NO_ERROR;

    mSensor->setAwbLock(mForceAwbLock);
    return NO_ERROR;
}

status_t Aiq3ARawSensor::processAwbSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry;

    if ((entry = settings.find(ANDROID_CONTROL_AWB_MODE)).count == 1) {
        uint8_t value = entry.data.u8[0];
        AwbMode newValue = \
            (value == ANDROID_CONTROL_AWB_MODE_INCANDESCENT) ?
                    CAM_AWB_MODE_WARM_INCANDESCENT : \
            (value == ANDROID_CONTROL_AWB_MODE_FLUORESCENT) ?
                    CAM_AWB_MODE_FLUORESCENT : \
            (value == ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT) ?
                    CAM_AWB_MODE_WARM_FLUORESCENT : \
            (value == ANDROID_CONTROL_AWB_MODE_DAYLIGHT) ?
                    CAM_AWB_MODE_DAYLIGHT : \
            (value == ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT) ?
                    CAM_AWB_MODE_CLOUDY : \
            (value == ANDROID_CONTROL_AWB_MODE_TWILIGHT) ?
                    CAM_AWB_MODE_SUNSET : \
            (value == ANDROID_CONTROL_AWB_MODE_SHADE) ?
                    CAM_AWB_MODE_SHADOW : \
            (value == ANDROID_CONTROL_AWB_MODE_OFF) ?
                    CAM_AWB_MODE_OFF : \
            CAM_AWB_MODE_AUTO;
        status = mSensor->setAwbMode(newValue);
    }

    if ((entry = settings.find(ANDROID_CONTROL_AWB_LOCK)).count == 1) {
        handleLockAwb(ANDROID_CONTROL_AWB_LOCK_ON ==
                entry.data.u8[0]);
    }

#ifdef INTEL_ENHANCEMENT
    if ((entry = settings.find(ANDROID_COLOR_CORRECTION_USE_ZERO_HISTORY_MODE)).count == 1) {
        mSensor->setAwbZeroHistoryMode(ANDROID_COLOR_CORRECTION_USE_ZERO_HISTORY_MODE_ON ==
                entry.data.u8[0]);
    }
#endif

    return status;
}

status_t Aiq3ARawSensor::processColorEffectSetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_EFFECT_MODE);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];
        LOG1("value=%d", value);
        EffectMode newValue = \
            (value == ANDROID_CONTROL_EFFECT_MODE_MONO) ?
                    CAM_EFFECT_MONO : \
            (value == ANDROID_CONTROL_EFFECT_MODE_SEPIA) ?
                    CAM_EFFECT_SEPIA : \
            (value == ANDROID_CONTROL_EFFECT_MODE_NEGATIVE) ?
                    CAM_EFFECT_NEGATIVE : \
            CAM_EFFECT_NONE;
        status = mSensor->set3AColorEffect(newValue);
    }

    if (status != OK)
        LOGE("@%s failed processing settings", __FUNCTION__);

    return status;
}

status_t Aiq3ARawSensor::processNREESetting(const CameraMetadata &settings)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry;

    entry = settings.find(ANDROID_NOISE_REDUCTION_MODE);
    if (entry.count == 1 && entry.data.u8[0] == ANDROID_NOISE_REDUCTION_MODE_OFF)
        mSensor->setNRMode(false);
    else
        mSensor->setNRMode(true);

    entry = settings.find(ANDROID_EDGE_MODE);
    if (entry.count == 1 && entry.data.u8[0] == ANDROID_EDGE_MODE_OFF)
        mSensor->setEdgeMode(false);
    else
        mSensor->setEdgeMode(true);

    entry = settings.find(ANDROID_SHADING_MODE);
    if (entry.count == 1 && entry.data.u8[0] == ANDROID_SHADING_MODE_OFF)
        mSensor->setShadingMode(false);
    else
        mSensor->setShadingMode(true);

    entry = settings.find(ANDROID_EDGE_STRENGTH);
    if (entry.count == 1) {
       int ee_strength = entry.data.i32[0];
       if (ee_strength >= 1 && ee_strength <= 10) {
          //AIQ ee strength range[-128, 127], do linear mapping [1, 10] ==> [-128, 127]
          mSensor->setEdgeStrength(-128 + 25.5 * ee_strength);
       }
    }

    return status;
}

status_t Aiq3ARawSensor::fillShutter()
{
    return NO_ERROR;
}

status_t Aiq3ARawSensor::fill3AMetadata()
{
    return NO_ERROR;
}

status_t Aiq3ARawSensor::newFrameDone()
{
    return NO_ERROR;
}

}; // namespace camera2
}; // namespace android

