/*
 * Copyright (C) 2013 Intel Corporation.
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
#define LOG_TAG "Camera_SOC3A"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <linux/media.h>
#include <linux/atomisp.h>
#include "LogHelper.h"
#include "PlatformData.h"
//#include "PerformanceTraces.h"
#include "Soc3A.h"
#include "ICameraHwControls.h"
//#include "ColorConverter.h"

namespace android {
namespace camera2 {

////////////////////////////////////////////////////////////////////
//                          PUBLIC METHODS
////////////////////////////////////////////////////////////////////

Soc3A::Soc3A(int cameraId, HWControlGroup &hwcg) :
    mCameraId(cameraId)
    ,mISP(hwcg.mIspCI)
    ,mSensorCI(hwcg.mSensorCI)
    ,mFlashCI(hwcg.mFlashCI)
    ,mLensCI(hwcg.mLensCI)
    ,mPublicAeMode(CAM_AE_MODE_AUTO)
    ,mPublicAfMode(CAM_AF_MODE_AUTO)
{
}

Soc3A::Soc3A(int cameraId) :
    mCameraId(cameraId)
    ,mISP(NULL)
    ,mSensorCI(NULL)
    ,mFlashCI(NULL)
    ,mLensCI(NULL)
    ,mPublicAeMode(CAM_AE_MODE_AUTO)
    ,mPublicAfMode(CAM_AF_MODE_AUTO)
{
}

Soc3A::~Soc3A()
{
}

// I3AControls

status_t Soc3A::init3A()
{
    // check if HW control are ready
    if (mISP == NULL || mSensorCI == NULL ||  mFlashCI == NULL || mLensCI == NULL)
        return NO_INIT;
    return NO_ERROR;
}

status_t Soc3A::deinit3A()
{
    return NO_ERROR;
}

status_t Soc3A::attachHwControl(HWControlGroup &hwcg)
{
    LOG1("@%s", __FUNCTION__);
    mISP = hwcg.mIspCI;
    mSensorCI = hwcg.mSensorCI;
    mFlashCI = hwcg.mFlashCI;
    mLensCI = hwcg.mLensCI;
    return NO_ERROR;
}

status_t Soc3A::setAeMode(AeMode mode)
{
    LOG1("@%s: %d", __FUNCTION__, mode);
    status_t status = NO_ERROR;
    v4l2_exposure_auto_type v4lMode;

    switch (mode)
    {
        case CAM_AE_MODE_AUTO:
            v4lMode = V4L2_EXPOSURE_AUTO;
            break;
        case CAM_AE_MODE_MANUAL:
            v4lMode = V4L2_EXPOSURE_MANUAL;
            break;
        case CAM_AE_MODE_SHUTTER_PRIORITY:
            v4lMode = V4L2_EXPOSURE_SHUTTER_PRIORITY;
            break;
        case CAM_AE_MODE_APERTURE_PRIORITY:
            v4lMode = V4L2_EXPOSURE_APERTURE_PRIORITY;
            break;
        default:
            LOGW("Unsupported AE mode (%d), using AUTO", mode);
            v4lMode = V4L2_EXPOSURE_AUTO;
            break;
    }

    int ret = mSensorCI->setExposureMode(v4lMode);
    if (ret != 0) {
        LOGE("Error setting AE mode (%d) in the driver", v4lMode);
        status = UNKNOWN_ERROR;
    }

    return status;
}

AeMode Soc3A::getAeMode()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    AeMode mode = CAM_AE_MODE_AUTO;
    v4l2_exposure_auto_type v4lMode = V4L2_EXPOSURE_AUTO;

    int ret = mSensorCI->getExposureMode(&v4lMode);
    if (ret != 0) {
        LOGE("Error getting AE mode from the driver");
        status = UNKNOWN_ERROR;
    }

    switch (v4lMode)
    {
        case V4L2_EXPOSURE_AUTO:
            mode = CAM_AE_MODE_AUTO;
            break;
        case V4L2_EXPOSURE_MANUAL:
            mode = CAM_AE_MODE_MANUAL;
            break;
        case V4L2_EXPOSURE_SHUTTER_PRIORITY:
            mode = CAM_AE_MODE_SHUTTER_PRIORITY;
            break;
        case V4L2_EXPOSURE_APERTURE_PRIORITY:
            mode = CAM_AE_MODE_APERTURE_PRIORITY;
            break;
        default:
            LOGW("Unsupported AE mode (%d), using AUTO", v4lMode);
            mode = CAM_AE_MODE_AUTO;
            break;
    }

    return mode;
}

status_t Soc3A::setEv(float bias)
{
    status_t status = NO_ERROR;
    int evValue = (int)bias;
    LOG1("@%s: bias: %f, EV value: %d", __FUNCTION__, bias, evValue);

    int ret = mSensorCI->setExposureBias(evValue);
    if (ret != 0) {
        LOGE("Error setting EV in the driver");
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Soc3A::getEv(float *bias)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    int evValue = 0;

    int ret = mSensorCI->getExposureBias(&evValue);
    if (ret != 0) {
        LOGE("Error getting EV from the driver");
        status = UNKNOWN_ERROR;
    }
    *bias = (float)evValue;

    return status;
}

status_t Soc3A::setAeSceneMode(SceneMode mode)
{
    LOG1("@%s: %d", __FUNCTION__, mode);
    status_t status = NO_ERROR;
    v4l2_scene_mode v4lMode;

    switch (mode)
    {
        case CAM_AE_SCENE_MODE_AUTO:
            v4lMode = V4L2_SCENE_MODE_NONE;
            break;
        case CAM_AE_SCENE_MODE_PORTRAIT:
            v4lMode = V4L2_SCENE_MODE_PORTRAIT;
            break;
        case CAM_AE_SCENE_MODE_SPORTS:
            v4lMode = V4L2_SCENE_MODE_SPORTS;
            break;
        case CAM_AE_SCENE_MODE_LANDSCAPE:
            v4lMode = V4L2_SCENE_MODE_LANDSCAPE;
            break;
        case CAM_AE_SCENE_MODE_NIGHT:
            v4lMode = V4L2_SCENE_MODE_NIGHT;
            break;
        case CAM_AE_SCENE_MODE_NIGHT_PORTRAIT:
            v4lMode = V4L2_SCENE_MODE_NIGHT;
            break;
        case CAM_AE_SCENE_MODE_FIREWORKS:
            v4lMode = V4L2_SCENE_MODE_FIREWORKS;
            break;
        case CAM_AE_SCENE_MODE_TEXT:
            v4lMode = V4L2_SCENE_MODE_TEXT;
            break;
        case CAM_AE_SCENE_MODE_SUNSET:
            v4lMode = V4L2_SCENE_MODE_SUNSET;
            break;
        case CAM_AE_SCENE_MODE_PARTY:
            v4lMode = V4L2_SCENE_MODE_PARTY_INDOOR;
            break;
        case CAM_AE_SCENE_MODE_CANDLELIGHT:
            v4lMode = V4L2_SCENE_MODE_CANDLE_LIGHT;
            break;
        case CAM_AE_SCENE_MODE_BEACH_SNOW:
            v4lMode = V4L2_SCENE_MODE_BEACH_SNOW;
            break;
        case CAM_AE_SCENE_MODE_DAWN_DUSK:
            v4lMode = V4L2_SCENE_MODE_DAWN_DUSK;
            break;
        case CAM_AE_SCENE_MODE_FALL_COLORS:
            v4lMode = V4L2_SCENE_MODE_FALL_COLORS;
            break;
        case CAM_AE_SCENE_MODE_BACKLIGHT:
            v4lMode = V4L2_SCENE_MODE_BACKLIGHT;
            break;
        default:
            LOGW("Unsupported scene mode (%d), using NONE", mode);
            v4lMode = V4L2_SCENE_MODE_NONE;
            break;
    }

    int ret = mSensorCI->setSceneMode(v4lMode);
    if (ret != 0) {
        LOGE("Error setting scene mode in the driver");
        status = UNKNOWN_ERROR;
    }

    return status;
}

SceneMode Soc3A::getAeSceneMode()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    SceneMode mode = CAM_AE_SCENE_MODE_AUTO;
    v4l2_scene_mode v4lMode = V4L2_SCENE_MODE_NONE;

    int ret = mSensorCI->getSceneMode(&v4lMode);
    if (ret != 0) {
        LOGE("Error getting scene mode from the driver");
        status = UNKNOWN_ERROR;
    }

    switch (v4lMode)
    {
        case V4L2_SCENE_MODE_NONE:
            mode = CAM_AE_SCENE_MODE_AUTO;
            break;
        case V4L2_SCENE_MODE_PORTRAIT:
            mode = CAM_AE_SCENE_MODE_PORTRAIT;
            break;
        case V4L2_SCENE_MODE_SPORTS:
            mode = CAM_AE_SCENE_MODE_SPORTS;
            break;
        case V4L2_SCENE_MODE_LANDSCAPE:
            mode = CAM_AE_SCENE_MODE_LANDSCAPE;
            break;
        case V4L2_SCENE_MODE_NIGHT:
            mode = CAM_AE_SCENE_MODE_NIGHT;
            break;
        case V4L2_SCENE_MODE_FIREWORKS:
            mode = CAM_AE_SCENE_MODE_FIREWORKS;
            break;
        case V4L2_SCENE_MODE_TEXT:
            mode = CAM_AE_SCENE_MODE_TEXT;
            break;
        case V4L2_SCENE_MODE_SUNSET:
            mode = CAM_AE_SCENE_MODE_SUNSET;
            break;
        case V4L2_SCENE_MODE_PARTY_INDOOR:
            mode = CAM_AE_SCENE_MODE_PARTY;
            break;
        case V4L2_SCENE_MODE_CANDLE_LIGHT:
            mode = CAM_AE_SCENE_MODE_CANDLELIGHT;
            break;
        case V4L2_SCENE_MODE_BEACH_SNOW:
            mode = CAM_AE_SCENE_MODE_BEACH_SNOW;
            break;
        case V4L2_SCENE_MODE_DAWN_DUSK:
            mode = CAM_AE_SCENE_MODE_DAWN_DUSK;
            break;
        case V4L2_SCENE_MODE_FALL_COLORS:
            mode = CAM_AE_SCENE_MODE_FALL_COLORS;
            break;
        case V4L2_SCENE_MODE_BACKLIGHT:
            mode = CAM_AE_SCENE_MODE_BACKLIGHT;
            break;
        default:
            LOGW("Unsupported scene mode (%d), using AUTO", v4lMode);
            mode = CAM_AE_SCENE_MODE_AUTO;
            break;
    }

    return mode;
}

status_t Soc3A::setAwbMode(AwbMode mode)
{
    LOG1("@%s: %d", __FUNCTION__, mode);
    status_t status = NO_ERROR;
    v4l2_auto_n_preset_white_balance v4lMode;

    switch (mode)
    {
        case CAM_AWB_MODE_AUTO:
            v4lMode = V4L2_WHITE_BALANCE_AUTO;
            break;
        case CAM_AWB_MODE_MANUAL_INPUT:
            v4lMode = V4L2_WHITE_BALANCE_MANUAL;
            break;
        case CAM_AWB_MODE_DAYLIGHT:
            v4lMode = V4L2_WHITE_BALANCE_DAYLIGHT;
            break;
        case CAM_AWB_MODE_SUNSET:
            v4lMode = V4L2_WHITE_BALANCE_INCANDESCENT;
            break;
        case CAM_AWB_MODE_CLOUDY:
            v4lMode = V4L2_WHITE_BALANCE_CLOUDY;
            break;
        case CAM_AWB_MODE_TUNGSTEN:
            v4lMode = V4L2_WHITE_BALANCE_INCANDESCENT;
            break;
        case CAM_AWB_MODE_FLUORESCENT:
            v4lMode = V4L2_WHITE_BALANCE_FLUORESCENT;
            break;
        case CAM_AWB_MODE_WARM_FLUORESCENT:
            v4lMode = V4L2_WHITE_BALANCE_FLUORESCENT_H;
            break;
        case CAM_AWB_MODE_SHADOW:
            v4lMode = V4L2_WHITE_BALANCE_SHADE;
            break;
        case CAM_AWB_MODE_WARM_INCANDESCENT:
            v4lMode = V4L2_WHITE_BALANCE_INCANDESCENT;
            break;
        default:
            LOGW("Unsupported AWB mode %d", mode);
            v4lMode = V4L2_WHITE_BALANCE_AUTO;
            break;
    }

    int ret = mSensorCI->setWhiteBalance(v4lMode);
    if (ret != 0) {
        LOGE("Error setting WB mode (%d) in the driver", v4lMode);
        status = UNKNOWN_ERROR;
    }

    return status;
}

AwbMode Soc3A::getAwbMode()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    AwbMode mode = CAM_AWB_MODE_AUTO;
    v4l2_auto_n_preset_white_balance v4lMode = V4L2_WHITE_BALANCE_AUTO;

    int ret = mSensorCI->getWhiteBalance(&v4lMode);
    if (ret != 0) {
        LOGE("Error getting WB mode from the driver");
        status = UNKNOWN_ERROR;
    }

    switch (v4lMode)
    {
        case V4L2_WHITE_BALANCE_AUTO:
            mode = CAM_AWB_MODE_AUTO;
            break;
        case V4L2_WHITE_BALANCE_MANUAL:
            mode = CAM_AWB_MODE_MANUAL_INPUT;
            break;
        case V4L2_WHITE_BALANCE_DAYLIGHT:
            mode = CAM_AWB_MODE_DAYLIGHT;
            break;
        case V4L2_WHITE_BALANCE_CLOUDY:
            mode = CAM_AWB_MODE_CLOUDY;
            break;
        case V4L2_WHITE_BALANCE_INCANDESCENT:
            mode = CAM_AWB_MODE_TUNGSTEN;
            break;
        case V4L2_WHITE_BALANCE_FLUORESCENT:
            mode = CAM_AWB_MODE_FLUORESCENT;
            break;
        case V4L2_WHITE_BALANCE_FLUORESCENT_H:
            mode = CAM_AWB_MODE_WARM_FLUORESCENT;
            break;
        case V4L2_WHITE_BALANCE_SHADE:
            mode = CAM_AWB_MODE_SHADOW;
            break;
        default:
            LOGW("Unsupported AWB mode %d", v4lMode);
            mode = CAM_AWB_MODE_AUTO;
            break;
    }

    return mode;
}

status_t Soc3A::setManualIso(int iso)
{
    LOG1("@%s: ISO: %d", __FUNCTION__, iso);
    status_t status = NO_ERROR;

    int ret = mSensorCI->setIso(iso);
    if (ret != 0) {
        LOGE("Error setting ISO in the driver");
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Soc3A::getManualIso(int *iso)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    int ret = mSensorCI->getIso(iso);
    if (ret != 0) {
        LOGE("Error getting ISO from the driver");
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Soc3A::setIsoMode(IsoMode mode) {
    /*ISO mode not supported for SOC sensor yet.*/
    LOG1("@%s", __FUNCTION__);
    UNUSED(mode);
    status_t status = INVALID_OPERATION;
    return status;
}

IsoMode Soc3A::getIsoMode(void) {
    /*ISO mode not supported for SOC sensor yet.*/
    return CAM_AE_ISO_MODE_NOT_SET;
}

status_t Soc3A::setAeMeteringMode(MeteringMode mode)
{
    LOG1("@%s: %d", __FUNCTION__, mode);
    status_t status = NO_ERROR;
    v4l2_exposure_metering v4lMode;

    switch (mode)
    {
        case CAM_AE_METERING_MODE_AUTO:
            v4lMode = V4L2_EXPOSURE_METERING_AVERAGE;
            break;
        case CAM_AE_METERING_MODE_SPOT:
            v4lMode = V4L2_EXPOSURE_METERING_SPOT;
            break;
        case CAM_AE_METERING_MODE_CENTER:
            v4lMode = V4L2_EXPOSURE_METERING_CENTER_WEIGHTED;
            break;
        default:
            LOGW("Unsupported AE metering mode (%d), using AVERAGE", mode);
            v4lMode = V4L2_EXPOSURE_METERING_AVERAGE;
            break;
    }

    int ret = mSensorCI->setAeMeteringMode(v4lMode);
    if (ret != 0) {
        LOGE("Error setting AE metering mode (%d) in the driver", v4lMode);
        status = UNKNOWN_ERROR;
    }

    return status;
}

MeteringMode Soc3A::getAeMeteringMode()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    MeteringMode mode = CAM_AE_METERING_MODE_NOT_SET;
    v4l2_exposure_metering v4lMode = V4L2_EXPOSURE_METERING_AVERAGE;

    int ret = mSensorCI->getAeMeteringMode(&v4lMode);
    if (ret != 0) {
        LOGE("Error getting AE metering mode from the driver");
        status = UNKNOWN_ERROR;
    }

    switch (v4lMode)
    {
        case V4L2_EXPOSURE_METERING_CENTER_WEIGHTED:
            mode = CAM_AE_METERING_MODE_CENTER;
            break;
        case V4L2_EXPOSURE_METERING_SPOT:
            mode = CAM_AE_METERING_MODE_SPOT;
            break;
        case V4L2_EXPOSURE_METERING_AVERAGE:
            mode = CAM_AE_METERING_MODE_AUTO;
            break;
        default:
            LOGW("Unsupported AE metering mode (%d), using AUTO", v4lMode);
            mode = CAM_AE_METERING_MODE_AUTO;
            break;
    }

    return mode;
}

status_t Soc3A::set3AColorEffect(EffectMode effect)
{
    LOG1("@%s: effect = %d", __FUNCTION__, effect);
    status_t status = NO_ERROR;

    v4l2_colorfx v4l2Effect = V4L2_COLORFX_NONE;
    if (effect == CAM_EFFECT_MONO)
        v4l2Effect = V4L2_COLORFX_BW;
    else if (effect == CAM_EFFECT_NEGATIVE)
        v4l2Effect = V4L2_COLORFX_NEGATIVE;
    else if (effect == CAM_EFFECT_SEPIA)
        v4l2Effect = V4L2_COLORFX_SEPIA;
    else if (effect == CAM_EFFECT_SKY_BLUE)
        v4l2Effect = V4L2_COLORFX_SKY_BLUE;
    else if (effect == CAM_EFFECT_GRASS_GREEN)
        v4l2Effect = V4L2_COLORFX_GRASS_GREEN;
    else if (effect == CAM_EFFECT_SKIN_WHITEN_LOW)
        v4l2Effect = (v4l2_colorfx)V4L2_COLORFX_SKIN_WHITEN_LOW;
    else if (effect == CAM_EFFECT_SKIN_WHITEN)
        v4l2Effect = V4L2_COLORFX_SKIN_WHITEN;
    else if (effect == CAM_EFFECT_SKIN_WHITEN_HIGH)
        v4l2Effect = (v4l2_colorfx)V4L2_COLORFX_SKIN_WHITEN_HIGH;
    else if (effect == CAM_EFFECT_VIVID)
        v4l2Effect = V4L2_COLORFX_VIVID;
    else if (effect == CAM_EFFECT_NONE)
        v4l2Effect = V4L2_COLORFX_NONE;
    else {
        LOGE("Color effect not found.");
        status = -1;
        v4l2Effect= V4L2_COLORFX_NONE;
        // Fall back to the effect NONE
    }

    status = mISP->setColorEffect(v4l2Effect);
    status = mISP->applyColorEffect();
    return status;
}

status_t Soc3A::setAeFlickerMode(FlickerMode flickerMode)
{
    LOG2("@%s: %d", __FUNCTION__, (int)flickerMode);
    status_t status(NO_ERROR);
    v4l2_power_line_frequency theMode(V4L2_CID_POWER_LINE_FREQUENCY_DISABLED);

    switch(flickerMode) {
    case CAM_AE_FLICKER_MODE_50HZ:
        theMode = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
        break;
    case CAM_AE_FLICKER_MODE_60HZ:
        theMode = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
        break;
    case CAM_AE_FLICKER_MODE_OFF:
        theMode = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
        break;
    case CAM_AE_FLICKER_MODE_AUTO:
        theMode = V4L2_CID_POWER_LINE_FREQUENCY_AUTO;
        break;
    default:
        LOGE("unsupported light frequency mode(%d)", (int)flickerMode);
        status = BAD_VALUE;
        return status;
    }

    int ret = mSensorCI->setAeFlickerMode(theMode);
    if (ret != 0) {
        LOGE("Error setting AE flicker mode (%d) in the driver", theMode);
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Soc3A::setAfMode(AfMode mode)
{
    LOG1("@%s: %d", __FUNCTION__, mode);
    status_t status = NO_ERROR;
    v4l2_auto_focus_range v4lMode;

    switch (mode)
    {
        case CAM_AF_MODE_AUTO:
            v4lMode = V4L2_AUTO_FOCUS_RANGE_AUTO;
            break;
        case CAM_AF_MODE_MACRO:
            v4lMode = V4L2_AUTO_FOCUS_RANGE_MACRO;
            break;
        case CAM_AF_MODE_INFINITY:
            v4lMode = V4L2_AUTO_FOCUS_RANGE_INFINITY;
            break;
        default:
            LOGW("Unsupported AF mode (%d), using AUTO", mode);
            v4lMode = V4L2_AUTO_FOCUS_RANGE_AUTO;
            break;
    }

    int ret = mSensorCI->setAfMode(v4lMode);
    if (ret != 0) {
        LOGE("Error setting AF  mode (%d) in the driver", v4lMode);
        status = UNKNOWN_ERROR;
    }

    return status;
}

AfMode Soc3A::getAfMode()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    AfMode mode = CAM_AF_MODE_AUTO;
    v4l2_auto_focus_range v4lMode = V4L2_AUTO_FOCUS_RANGE_AUTO;

    int ret = mSensorCI->getAfMode(&v4lMode);
    if (ret != 0) {
        LOGE("Error getting AF mode from the driver");
        status = UNKNOWN_ERROR;
    }

    switch (v4lMode)
    {
        case V4L2_AUTO_FOCUS_RANGE_AUTO:
            mode = CAM_AF_MODE_AUTO;
            break;
        case CAM_AF_MODE_MACRO:
            mode = CAM_AF_MODE_MACRO;
            break;
        case CAM_AF_MODE_INFINITY:
            mode = CAM_AF_MODE_INFINITY;
            break;
        default:
            LOGW("Unsupported AF mode (%d), using AUTO", v4lMode);
            mode = CAM_AF_MODE_AUTO;
            break;
    }

    return mode;
}

status_t Soc3A::setAfEnabled(bool enable)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    int ret = mSensorCI->setAfEnabled(enable);
    if (ret != 0) {
        LOGE("Error setting Auto Focus (%d) in the driver", enable);
        status = UNKNOWN_ERROR;
    }

    return status;
}

int Soc3A::get3ALock()
{
    LOG1("@%s", __FUNCTION__);
    int aaaLock = 0;

    int ret = mSensorCI->get3ALock(&aaaLock);
    if (ret != 0) {
        LOGE("Error getting 3A Lock setting from the driver");
    }

    return aaaLock;
}

bool Soc3A::getAeLock()
{
    LOG1("@%s", __FUNCTION__);
    bool aeLockEnabled = false;

    int aaaLock = get3ALock();
    if (aaaLock & V4L2_LOCK_EXPOSURE)
        aeLockEnabled = true;

    return aeLockEnabled;
}

status_t Soc3A::setAeLock(bool enable)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    int aaaLock = get3ALock();
    if (enable)
        aaaLock |= V4L2_LOCK_EXPOSURE;
    else
        aaaLock &= ~V4L2_LOCK_EXPOSURE;

    int ret = mSensorCI->set3ALock(aaaLock);
    if (ret != 0) {
        LOGE("Error setting AE lock (%d) in the driver", enable);
        status = UNKNOWN_ERROR;
    }

    return status;
}

bool Soc3A::getAfLock()
{
    LOG1("@%s", __FUNCTION__);
    bool afLockEnabled = false;

    int aaaLock = get3ALock();
    if (aaaLock & V4L2_LOCK_FOCUS)
        afLockEnabled = true;

    return afLockEnabled;
}

status_t Soc3A::setAfLock(bool enable)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    int aaaLock = get3ALock();
    if (enable)
        aaaLock |= V4L2_LOCK_FOCUS;
    else
        aaaLock &= ~V4L2_LOCK_FOCUS;

    int ret = mSensorCI->set3ALock(aaaLock);
    if (ret != 0) {
        LOGE("Error setting AF lock (%d) in the driver", enable);
        status = UNKNOWN_ERROR;
    }

    return status;
}

bool Soc3A::getAwbLock()
{
    LOG1("@%s", __FUNCTION__);
    bool awbLockEnabled = false;

    int aaaLock = get3ALock();
    if (aaaLock & V4L2_LOCK_WHITE_BALANCE)
        awbLockEnabled = true;

    return awbLockEnabled;
}

status_t Soc3A::setAwbLock(bool enable)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    int aaaLock = get3ALock();
    if (enable)
        aaaLock |= V4L2_LOCK_WHITE_BALANCE;
    else
        aaaLock &= ~V4L2_LOCK_WHITE_BALANCE;

    int ret = mSensorCI->set3ALock(aaaLock);
    if (ret != 0) {
        LOGE("Error setting AWB lock (%d) in the driver", enable);
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Soc3A::applyEv(float bias)
{
    LOG1("@%s: bias: %f", __FUNCTION__, bias);
    return setEv(bias);
}

status_t Soc3A::setManualShutter(float expTime)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    int time = expTime / 0.0001; // 100 usec units

    int ret = mSensorCI->setExposureTime(time);
    if (ret != 0) {
        LOGE("Error setting Exposure time (%d) in the driver", time);
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Soc3A::setManualFrameDuration(uint64_t nsFrameDuration)
{
    LOG1("@%s, frame duration %lld ns", __FUNCTION__, nsFrameDuration);
    LOGW("@%s NOT IMPLEMENTED!", __FUNCTION__);
    return OK;
}

status_t Soc3A::setAeFlashMode(FlashMode mode)
{
    LOG1("@%s: %d", __FUNCTION__, mode);
    status_t status = NO_ERROR;
    v4l2_flash_led_mode v4lMode;

    switch (mode)
    {
        case CAM_AE_FLASH_MODE_OFF:
            v4lMode = V4L2_FLASH_LED_MODE_NONE;
            break;
        case CAM_AE_FLASH_MODE_ON:
            v4lMode = V4L2_FLASH_LED_MODE_FLASH;
            break;
        case CAM_AE_FLASH_MODE_TORCH:
            v4lMode = V4L2_FLASH_LED_MODE_TORCH;
            break;
        default:
            LOGW("Unsupported Flash mode (%d), using OFF", mode);
            v4lMode = V4L2_FLASH_LED_MODE_NONE;
            break;
    }

    int ret = mSensorCI->setAeFlashMode(v4lMode);
    if (ret != 0) {
        LOGE("Error setting Flash mode (%d) in the driver", v4lMode);
        status = UNKNOWN_ERROR;
    }

    return status;
}

FlashMode Soc3A::getAeFlashMode()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    FlashMode mode = CAM_AE_FLASH_MODE_OFF;
    v4l2_flash_led_mode v4lMode = V4L2_FLASH_LED_MODE_NONE;

    int ret = mSensorCI->setAeFlashMode(v4lMode);
    if (ret != 0) {
        LOGE("Error getting Flash mode from the driver");
        status = UNKNOWN_ERROR;
    }

    switch (v4lMode)
    {
        case V4L2_FLASH_LED_MODE_NONE:
            mode = CAM_AE_FLASH_MODE_OFF;
            break;
        case V4L2_FLASH_LED_MODE_FLASH:
            mode = CAM_AE_FLASH_MODE_ON;
            break;
        case V4L2_FLASH_LED_MODE_TORCH:
            mode = CAM_AE_FLASH_MODE_TORCH;
            break;
        default:
            LOGW("Unsupported Flash mode (%d), using OFF", v4lMode);
            mode = CAM_AE_FLASH_MODE_OFF;
            break;
    }

    return mode;
}

void Soc3A::setPublicAeMode(AeMode mode)
{
    LOG2("@%s", __FUNCTION__);
    mPublicAeMode = mode;
}

AeMode Soc3A::getPublicAeMode()
{
    LOG2("@%s", __FUNCTION__);
    return mPublicAeMode;
}

void Soc3A::setPublicAfMode(AfMode mode)
{
    LOG2("@%s", __FUNCTION__);
    mPublicAfMode = mode;
}

AfMode Soc3A::getPublicAfMode()
{
    LOG2("@%s", __FUNCTION__);
    return mPublicAfMode;
}

status_t Soc3A::setFlash(int numFrames)
{
    return mFlashCI->setFlash(numFrames);
}

int Soc3A::getCurrentFocusDistance()
{
    LOG3A("@%s", __FUNCTION__);
    return 0;
}

int Soc3A::getOpticalStabilizationMode()
{
    //add interface of OpticalStabilizationmode, default value is OFF.
    return 0;
}

int Soc3A::getnsExposuretime()
{
    LOG3A("@%s", __FUNCTION__);
    return 0;
}

int Soc3A::getnsFrametime()
{
    LOG3A("@%s", __FUNCTION__);
    return 0;
}




/****************************************************************************
 * Stub implementation of stub methods or methods only used by Aiq3A
 * implementation
 *
 ***************************************************************************/
status_t Soc3A::setManualFrameDurationRange(int minFps, int maxFps)
{
    UNUSED(minFps);
    UNUSED(maxFps);
    return INVALID_OPERATION;
}

void Soc3A::FaceDetectCallbackFor3A(ia_face_state *facestate)
{
    UNUSED(facestate);
}

void Soc3A::FaceDetectCallbackForApp(camera_frame_metadata_t *face_metadata)
{
    UNUSED(face_metadata);
}

bool Soc3A::isIntel3A()
{
    return false;
}

status_t Soc3A::getAeManualBrightness(float *ret) INVALID_STUB(ret)

size_t  Soc3A::getAeMaxNumWindows() { return 0; }

size_t   Soc3A::getAfMaxNumWindows() { return 0; }

status_t Soc3A::setAeWindow(const CameraWindow *window) INVALID_STUB(window)

status_t Soc3A::setAfWindows(const CameraWindow *windows, size_t numWindows)
{
    UNUSED(windows);
    UNUSED(numWindows);
    return INVALID_OPERATION;
}

status_t Soc3A::setManualFocusIncrement(int step) INVALID_STUB(step)

status_t Soc3A::initAfBracketing(int stops,  AFBracketingMode mode)
{
    UNUSED(stops);
    UNUSED(mode);
    return INVALID_OPERATION;
}

status_t Soc3A::initAeBracketing() { return INVALID_OPERATION; }

status_t Soc3A::getExposureInfo(SensorAeConfig& sensorAeConfig)
{
    UNUSED(sensorAeConfig);
    return INVALID_OPERATION;
}

bool Soc3A::getAfNeedAssistLight() { return false; }

bool Soc3A::getAeFlashNecessary() { return false; }

AwbMode Soc3A::getLightSource() { return CAM_AWB_MODE_NOT_SET; }

status_t Soc3A::setAeBacklightCorrection(bool en) INVALID_STUB(en)

status_t Soc3A::apply3AProcess(bool read_stats,
                                   struct timeval sof_timestamp,
                                   unsigned int exp_id)
{
    UNUSED(read_stats);
    UNUSED(sof_timestamp);
    UNUSED(exp_id);
    return INVALID_OPERATION;
}

status_t Soc3A::startStillAf() { return INVALID_OPERATION; }

status_t Soc3A::stopStillAf() { return INVALID_OPERATION; }

AfStatus Soc3A::isStillAfComplete() { return CAM_AF_STATUS_FAIL; }

status_t Soc3A::applyPreFlashProcess(struct timeval timestamp, unsigned int exp_id, FlashStage stage)
{
    UNUSED(timestamp);
    UNUSED(exp_id);
    UNUSED(stage);
    return INVALID_OPERATION;
}

status_t Soc3A::getGBCEResults(ia_aiq_gbce_results *gbce_results)
{
    UNUSED(gbce_results);
    return INVALID_OPERATION;
}

void Soc3A::get3aMakerNote(ia_mkn_trg mknMode, ia_binary_data& aaaMkNote)
{
    UNUSED(mknMode);
    UNUSED(aaaMkNote);
}

ia_binary_data *Soc3A::get3aMakerNote(ia_mkn_trg mode)
{
    UNUSED(mode);
    return NULL;
}

void Soc3A::put3aMakerNote(ia_binary_data *mknData) { UNUSED(mknData); }

void Soc3A::reset3aMakerNote(void) { }

int Soc3A::add3aMakerNoteRecord(ia_mkn_dfid mkn_format_id,
                                    ia_mkn_dnid mkn_name_id,
                                    const void *record,
                                    unsigned short record_size)
{
    UNUSED(mkn_format_id);
    UNUSED(mkn_name_id);
    UNUSED(record);
    UNUSED(record_size);
    return -1;
}

status_t Soc3A::convertGeneric2SensorAE(int sensitivity, int expTimeUs,
                     unsigned short *coarse_integration_time,
                     unsigned short *fine_integration_time,
                     int *analog_gain_code, int *digital_gain_code)
{
    UNUSED(sensitivity);
    UNUSED(expTimeUs);
    UNUSED(coarse_integration_time);
    UNUSED(fine_integration_time);
    UNUSED(analog_gain_code);
    UNUSED(digital_gain_code);
    return INVALID_OPERATION;
}

status_t Soc3A::convertSensor2ExposureTime(unsigned short coarse_integration_time,
                     unsigned short fine_integration_time,
                     int analog_gain_code, int digital_gain_code,
                     int *exposure_time, int *iso)
{
    UNUSED(coarse_integration_time);
    UNUSED(fine_integration_time);
    UNUSED(exposure_time);
    UNUSED(analog_gain_code);
    UNUSED(digital_gain_code);
    UNUSED(iso);
    return INVALID_OPERATION;
}

status_t Soc3A::convertSensorExposure2GenericExposure(unsigned short coarse_integration_time,
                                        unsigned short fine_integration_time,
                                        int analog_gain_code, int digital_gain_code,
                                        ia_aiq_exposure_parameters *exposure)
{
    UNUSED(coarse_integration_time);
    UNUSED(fine_integration_time);
    UNUSED(analog_gain_code);
    UNUSED(digital_gain_code);
    UNUSED(exposure);
    return INVALID_OPERATION;
}

status_t Soc3A::getParameters(ispInputParameters *ispParams,
                                  ia_aiq_exposure_parameters *exposure)
{
    UNUSED(ispParams);
    UNUSED(exposure);
    return INVALID_OPERATION;
}

status_t Soc3A::setSmartSceneDetection(bool en)
{
    UNUSED(en);
    return INVALID_OPERATION;
}

bool Soc3A::getSmartSceneDetection() { return false; }

status_t Soc3A::switchModeAndRate(IspMode mode, float fps)
{
    UNUSED(mode);
    UNUSED(fps);
    return INVALID_OPERATION;
}

AfStatus Soc3A::getCAFStatus() { return CAM_AF_STATUS_FAIL; }

status_t Soc3A::getSmartSceneMode(int *sceneMode, bool *sceneHdr)
{
    UNUSED(sceneMode);
    UNUSED(sceneHdr);
    return INVALID_OPERATION;
}

status_t Soc3A::setFaces(const ia_face_state& faceState)
{
    UNUSED(faceState);
    return INVALID_OPERATION;
}

int32_t Soc3A::mapUIISO2RealISO (int32_t iso)
{
    UNUSED(iso);
    return INVALID_OPERATION;
}

int32_t Soc3A::mapRealISO2UIISO (int32_t iso)
{
    UNUSED(iso);
    return INVALID_OPERATION;
}
}; // namespace camera2
} // namespace android
