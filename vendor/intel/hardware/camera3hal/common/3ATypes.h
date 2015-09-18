/*
 * Copyright (C) 2012,2013 Intel Corporation
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

#ifndef _HAL_3A_TYPES_H_
#define _HAL_3A_TYPES_H_

#include <ia_mkn_types.h>
#include <ia_aiq_types.h>

namespace android {
namespace camera2 {

// DetermineFlash: returns true if flash should be determined according to current exposure
#define DetermineFlash(x) (x == CAM_AE_FLASH_MODE_AUTO || \
                           x == CAM_AE_FLASH_MODE_DAY_SYNC || \
                           x == CAM_AE_FLASH_MODE_SLOW_SYNC) \

#define EV_LOWER_BOUND         -100
#define EV_UPPER_BOUND          100

#define GAMMA_LUT_LOWER_BOUND  0
#define GAMMA_LUT_UPPER_BOUND  255

enum EffectMode
{
    CAM_EFFECT_NONE = 0,
    CAM_EFFECT_MONO,
    CAM_EFFECT_SEPIA,
    CAM_EFFECT_NEGATIVE,
    CAM_EFFECT_SKY_BLUE,
    CAM_EFFECT_GRASS_GREEN,
    CAM_EFFECT_SKIN_WHITEN_LOW,
    CAM_EFFECT_SKIN_WHITEN,
    CAM_EFFECT_SKIN_WHITEN_HIGH,
    CAM_EFFECT_VIVID,
};


enum AeMode
{
    CAM_AE_MODE_NOT_SET = -1,
    CAM_AE_MODE_AUTO,
    CAM_AE_MODE_MANUAL,
    CAM_AE_MODE_SHUTTER_PRIORITY,
    CAM_AE_MODE_APERTURE_PRIORITY
};

enum AfStatus
{
    CAM_AF_STATUS_FAIL = -1,
    CAM_AF_STATUS_IDLE,
    CAM_AF_STATUS_BUSY,
    CAM_AF_STATUS_SUCCESS
};

enum SceneMode
{
    CAM_AE_SCENE_MODE_NOT_SET = -1,
    CAM_AE_SCENE_MODE_AUTO,
    CAM_AE_SCENE_MODE_PORTRAIT,
    CAM_AE_SCENE_MODE_SPORTS,
    CAM_AE_SCENE_MODE_LANDSCAPE,
    CAM_AE_SCENE_MODE_NIGHT,
    CAM_AE_SCENE_MODE_NIGHT_PORTRAIT,
    CAM_AE_SCENE_MODE_FIREWORKS,
    CAM_AE_SCENE_MODE_TEXT,
    CAM_AE_SCENE_MODE_SUNSET,
    CAM_AE_SCENE_MODE_PARTY,
    CAM_AE_SCENE_MODE_CANDLELIGHT,
    CAM_AE_SCENE_MODE_BEACH_SNOW,
    CAM_AE_SCENE_MODE_DAWN_DUSK,
    CAM_AE_SCENE_MODE_FALL_COLORS,
    CAM_AE_SCENE_MODE_BACKLIGHT
};

enum AwbMode
{
    CAM_AWB_MODE_NOT_SET = -1,
    CAM_AWB_MODE_OFF,
    CAM_AWB_MODE_AUTO,
    CAM_AWB_MODE_MANUAL_INPUT,
    CAM_AWB_MODE_DAYLIGHT,
    CAM_AWB_MODE_SUNSET,
    CAM_AWB_MODE_CLOUDY,
    CAM_AWB_MODE_TUNGSTEN,
    CAM_AWB_MODE_FLUORESCENT,
    CAM_AWB_MODE_WARM_FLUORESCENT,
    CAM_AWB_MODE_SHADOW,
    CAM_AWB_MODE_WARM_INCANDESCENT
};

enum MeteringMode
{
    CAM_AE_METERING_MODE_NOT_SET = -1,
    CAM_AE_METERING_MODE_AUTO,
    CAM_AE_METERING_MODE_SPOT,
    CAM_AE_METERING_MODE_CENTER,
    CAM_AE_METERING_MODE_CUSTOMIZED
};

/** add Iso mode*/
/* ISO control mode setting */
enum IsoMode {
    CAM_AE_ISO_MODE_NOT_SET = -1,
    CAM_AE_ISO_MODE_AUTO,   /* Automatic */
    CAM_AE_ISO_MODE_MANUAL  /* Manual */
};

enum AfMode
{
    CAM_AF_MODE_NOT_SET = -1,
    CAM_AF_MODE_AUTO,
    CAM_AF_MODE_MACRO,
    CAM_AF_MODE_INFINITY,
    CAM_AF_MODE_FIXED,
    CAM_AF_MODE_TOUCH,
    CAM_AF_MODE_MANUAL,
    CAM_AF_MODE_FACE,
    CAM_AF_MODE_CONTINUOUS
};

struct SensorAeConfig
{
    float evBias;
    int expTime;
    short unsigned int aperture_num;
    short unsigned int aperture_denum;
    int aecApexTv;
    int aecApexSv;
    int aecApexAv;
    float digitalGain;
    float totalGain;
};

enum FlashMode
{
    CAM_AE_FLASH_MODE_NOT_SET = -1,
    CAM_AE_FLASH_MODE_AUTO,
    CAM_AE_FLASH_MODE_OFF,
    CAM_AE_FLASH_MODE_ON,
    CAM_AE_FLASH_MODE_DAY_SYNC,
    CAM_AE_FLASH_MODE_SLOW_SYNC,
    CAM_AE_FLASH_MODE_TORCH,
    CAM_AE_FLASH_MODE_SINGLE
};

enum FlashStage
{
    CAM_FLASH_STAGE_NOT_SET = -1,
    CAM_FLASH_STAGE_NONE,
    CAM_FLASH_STAGE_PRE,
    CAM_FLASH_STAGE_MAIN
};

enum FlickerMode
{
    CAM_AE_FLICKER_MODE_NOT_SET = -1,
    CAM_AE_FLICKER_MODE_OFF,
    CAM_AE_FLICKER_MODE_50HZ,
    CAM_AE_FLICKER_MODE_60HZ,
    CAM_AE_FLICKER_MODE_AUTO
};

enum AFBracketingMode
{
    CAM_AF_BRACKETING_MODE_SYMMETRIC,
    CAM_AF_BRACKETING_MODE_TOWARDS_NEAR,
    CAM_AF_BRACKETING_MODE_TOWARDS_FAR,
};

struct AAAWindowInfo {
    unsigned width;
    unsigned height;
};

}
}

#endif // _HAL_3A_TYPES_H_
