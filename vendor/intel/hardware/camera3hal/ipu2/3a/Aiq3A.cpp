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

#define LOG_TAG "Camera_Aiq3A"

#include <math.h>
#include <time.h>
#include <dlfcn.h>
#include <utils/String8.h>

#include "LogHelper.h"
#include "ia_cmc_parser.h"
#include "PlatformData.h"
#include "ia_isp_types.h"
#include "IPU2CameraCapInfo.h"
#include "PerformanceTraces.h"

#include "Aiq3A.h"
#include "ia_mkn_encoder.h"
#include "ia_mkn_decoder.h"
#include "ia_exc.h"

#define MAX_EOF_SOF_DIFF 200000
#define DEFAULT_EOF_SOF_DELAY 66000

#define MIN_FRAMES_AE_CONVERGED 6

#define COLD_START_EXPOSURE_NUM 1

/**
 * \define MIN_SOF_DELAY
 * Minimum time in microseconds between latest SOF event reported and 3A running
 * to consider that the SOF event time given to 3A belongs to the same frame as
 * the current stats.
 * This means that if last SOF reported happen less than 20ms before 3A stats
 * we assume that this SOF belongs to the next frame.
 */
#define MIN_SOF_DELAY 20000
#define EPSILON 0.00001
#define RETRY_COUNT 5

namespace android {
namespace camera2 {

#if ENABLE_PROFILING
    #define PERFORMANCE_TRACES_AAA_PROFILER_START() \
        do { \
            PerformanceTraces::AAAProfiler::enable(true); \
            PerformanceTraces::AAAProfiler::start(); \
           } while(0)

    #define PERFORMANCE_TRACES_AAA_PROFILER_STOP() \
        do {\
            PerformanceTraces::AAAProfiler::stop(); \
           } while(0)
#else
     #define PERFORMANCE_TRACES_AAA_PROFILER_START()
     #define PERFORMANCE_TRACES_AAA_PROFILER_STOP()
#endif

#define MAX_STATISTICS_WIDTH 150
#define MAX_STATISTICS_HEIGHT 150
#define IA_AIQ_MAX_NUM_FACES 5

Aiq3A::Aiq3A(HWControlGroup &hwcg)
{
    LOG1("@%s", __FUNCTION__);
    Aiq3A(-1);
    mISP = hwcg.mIspCI;
    mSensorCI = hwcg.mSensorCI;
    mFlashCI = hwcg.mFlashCI;
    mLensCI = hwcg.mLensCI;
}

Aiq3A::Aiq3A(int cameraId ):
    mISP(NULL)
    ,mAfMode(CAM_AF_MODE_NOT_SET)
    ,mStillAfStart(0)
    ,mContinuousAfStart(0)
    ,mFocusPosition(0)
    ,mAfBracketingResult(NULL)
    ,mBracketingStops(0)
    ,mAeMode(CAM_AE_MODE_NOT_SET)
    ,mPublicAeMode(CAM_AE_MODE_NOT_SET)
    ,mAeSceneMode(CAM_AE_SCENE_MODE_NOT_SET)
    ,mAeFlickerMode(CAM_AE_FLICKER_MODE_NOT_SET)
    ,mAeFlashMode(CAM_AE_FLASH_MODE_NOT_SET)
    ,mAeConverged(false)
    ,mStatCount(0)
    ,mFlashStatus(false)
    ,mBracketingRunning(false)
    ,mAEBracketingResult(NULL)
    ,mAwbResults(NULL)
    ,mAwbMode(CAM_AWB_MODE_NOT_SET)
    ,mIsAwbZeroHistoryMode(false)
    ,mAwbLocked(false)
    ,mAwbDisabled(false)
    ,mAwbRunCount(0)
    ,mAwbConverged(false)
    ,mGBCEResults(NULL)
    ,mGBCEDefault(false)
    ,mPaResults(NULL)
    ,mMkn(NULL)
    ,mSensorCI(NULL)
    ,mFlashCI(NULL)
    ,mLensCI(NULL)
    ,mFpsSetting(FPS_SETTING_OFF)
    ,mLastExposuretime(0)
    ,mLastFrameTime(0)
    ,mNoiseReduction(true)
    ,mEdgeEnhancement(true)
    ,mLensShading(true)
    ,mEdgeStrength(0)
    ,mPseudoIso(0)
    ,mBaseIso(0)
    ,mCameraId(cameraId)
    ,mFlashStage(CAM_FLASH_STAGE_NOT_SET)
{
    LOG1("@%s", __FUNCTION__);
    CLEAR(mPrintFunctions);
    CLEAR(m3aState);
    CLEAR(mStatisticsInputParameters);
    CLEAR(mAfInputParameters);
    CLEAR(mAfState);
    CLEAR(mAeInputParameters);
    CLEAR(mAeSensorDescriptor);
    CLEAR(mAeState);
    CLEAR(mAeCoord);
    CLEAR(mAeBracketingInputParameters);
    CLEAR(mAeManualLimits);
    CLEAR(mAwbInputParameters);
    CLEAR(mAwbStoredResults);
    CLEAR(mIspInputParams);
    CLEAR(mDSDInputParameters);
    CLEAR(mDetectedSceneMode);
    CLEAR(mCmcParsedAG);
    CLEAR(mCmcParsedDG);
    CLEAR(mCmcSensitivity);
    CLEAR(mLastExposure);
    CLEAR(mLastSensorExposure);

}

Aiq3A::~Aiq3A()
{
    LOG1("@%s", __FUNCTION__);
}

status_t Aiq3A::init3A()
{
    LOG1("@%s", __FUNCTION__);

    status_t status = _init3A();

    mAeConverged = false;
    mStatCount = 0;
    mAwbConverged = false;
    mFlashStatus = false;
    return status;
}

status_t Aiq3A::_init3A()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    ia_err ret = ia_err_none;

    // check if HW control are ready
    if (mISP == NULL || mSensorCI == NULL ||  mFlashCI == NULL || mLensCI == NULL)
        return NO_INIT;

    ia_binary_data cpfData;
    status = getAiqConfig(&cpfData);
    if (status != NO_ERROR) {
        LOGE("Error retrieving sensor params");
        return status;
    }

    ia_binary_data *aicNvm = NULL;
    mSensorCI->generateNVM(&aicNvm);
    mMkn = ia_mkn_init(ia_mkn_cfg_compression, 32000, 100000);
    if(mMkn == NULL)
        LOGE("Error makernote init");
    ret = ia_mkn_enable(mMkn, true);
    if(ret != ia_err_none)
        LOGE("Error makernote init");

    ia_cmc_t *cmc = NULL;
    if (PlatformData::AiqConfig)
        cmc = PlatformData::AiqConfig.getCMCHandler();
    if (cmc == NULL)
        return NO_INIT;

    // read the aiqd data from file system
    ia_binary_data aiqdData;
    CLEAR(aiqdData);
    const ia_binary_data* pAiqdData = loadAiqdData(&aiqdData, getAiqdFileName()) ? &aiqdData : NULL;

    m3aState.ia_aiq_handle = ia_aiq_init((ia_binary_data*)&(cpfData),
                                         (ia_binary_data*)aicNvm,
                                         pAiqdData,
                                         MAX_STATISTICS_WIDTH,
                                         MAX_STATISTICS_HEIGHT,
                                         NUM_EXPOSURES,
                                         cmc,
                                         mMkn);

    // release the aiqd data which is allocated in the loadAiqdData
    if (aiqdData.data)
        free(aiqdData.data);

    mISP->initIaIspAdaptor((ia_binary_data*)&(cpfData),
                         MAX_STATISTICS_WIDTH,
                         MAX_STATISTICS_HEIGHT,
                         cmc,
                         mMkn);

    if (cmc) {
        if (cmc->cmc_sensitivity) {
            mBaseIso = cmc->cmc_sensitivity->base_iso;
            memcpy(&mCmcSensitivity, cmc->cmc_sensitivity, sizeof(cmc_sensitivity_t));
        }

        // cmc_parsed_analog_gain_conversion
        if (cmc->cmc_parsed_analog_gain_conversion.cmc_analog_gain_conversion) {
            mCmcParsedAG.cmc_analog_gain_conversion = new cmc_analog_gain_conversion_t;
            if (mCmcParsedAG.cmc_analog_gain_conversion)
                memcpy(mCmcParsedAG.cmc_analog_gain_conversion,
                    cmc->cmc_parsed_analog_gain_conversion.cmc_analog_gain_conversion,
                    sizeof(cmc_analog_gain_conversion_t));
        }
        if (cmc->cmc_parsed_analog_gain_conversion.cmc_analog_gain_segments) {
            mCmcParsedAG.cmc_analog_gain_segments = new cmc_analog_gain_segment_t;
            if (mCmcParsedAG.cmc_analog_gain_segments)
                memcpy(mCmcParsedAG.cmc_analog_gain_segments,
                    cmc->cmc_parsed_analog_gain_conversion.cmc_analog_gain_segments,
                    sizeof(cmc_analog_gain_segment_t));
        }
        if (cmc->cmc_parsed_analog_gain_conversion.cmc_analog_gain_pairs) {
            mCmcParsedAG.cmc_analog_gain_pairs = new cmc_analog_gain_pair_t;
            if (mCmcParsedAG.cmc_analog_gain_pairs)
                memcpy(mCmcParsedAG.cmc_analog_gain_pairs,
                    cmc->cmc_parsed_analog_gain_conversion.cmc_analog_gain_pairs,
                    sizeof(cmc_analog_gain_pair_t));
        }

        // cmc_parsed_digital_gain
        if (cmc->cmc_parsed_digital_gain.cmc_digital_gain) {
            mCmcParsedDG.cmc_digital_gain = new cmc_digital_gain_t;
            if (mCmcParsedDG.cmc_digital_gain)
                memcpy(mCmcParsedDG.cmc_digital_gain,
                    cmc->cmc_parsed_digital_gain.cmc_digital_gain,
                    sizeof(cmc_digital_gain_t));
        }
        if (cmc->cmc_parsed_digital_gain.cmc_digital_gain_v102) {
            mCmcParsedDG.cmc_digital_gain_v102 = new cmc_digital_gain_v102_t;
            if (mCmcParsedDG.cmc_digital_gain_v102)
                memcpy(mCmcParsedDG.cmc_digital_gain_v102,
                    cmc->cmc_parsed_digital_gain.cmc_digital_gain_v102,
                    sizeof(cmc_digital_gain_v102_t));
        }
        if (cmc->cmc_parsed_digital_gain.cmc_digital_gain_pairs) {
            mCmcParsedDG.cmc_digital_gain_pairs = new cmc_analog_gain_pair_t;
            if (mCmcParsedDG.cmc_digital_gain_pairs)
                memcpy(mCmcParsedDG.cmc_digital_gain_pairs,
                    cmc->cmc_parsed_digital_gain.cmc_digital_gain_pairs,
                    sizeof(cmc_analog_gain_pair_t));
        }
    }

    if (!m3aState.ia_aiq_handle) {
        mSensorCI->releaseNVM(aicNvm);
        return UNKNOWN_ERROR;
    }

    m3aState.frame_use = ia_aiq_frame_use_preview;
    m3aState.dsd_enabled = false;

    run3aInit();
    mSensorCI->releaseNVM(aicNvm);
    CLEAR(m3aState.results);

    return status;
}

String8 Aiq3A::getAiqdFileName(void)
{
    LOG1("@%s", __FUNCTION__);
    String8 sensorName, aiqdFileName;
    int spacePos;

    sensorName = mSensorCI->getSensorName();
    spacePos = sensorName.find(" ");
    if (spacePos < 0) {
        sensorName = mSensorCI->getSensorName();
    } else {
        sensorName.setTo(sensorName, spacePos);
    }

    aiqdFileName = String8(CAMERA_OPERATION_FOLDER) + sensorName + String8(".aiqd");
    LOG2("@%s, sensor name: %s, aiqd filename: %s", __FUNCTION__, sensorName.string(), aiqdFileName.string());
    return aiqdFileName;
}

bool Aiq3A::loadAiqdData(ia_binary_data* aiqdData, String8 aiqdFileName)
{
    LOG1("@%s", __FUNCTION__);
    struct stat fileStat;

    if (stat(aiqdFileName, &fileStat) != 0) {
        LOG1("@%s, can't read aiqd file or file doesn't exist", __FUNCTION__);
        return false;
    }

    aiqdData->size = fileStat.st_size;
    LOG2("@%s, aiqd file size: %d", __FUNCTION__, aiqdData->size);

    if (aiqdData->size > 0) {
        aiqdData->data = (unsigned char*)malloc(aiqdData->size);
        FILE *f = fopen(aiqdFileName, "rb");
        if (f && aiqdData->data) {
            fread(aiqdData->data, sizeof(unsigned char), aiqdData->size, f);
            fclose(f);
        }
    }

    return true;
}

bool Aiq3A::saveAiqdData(String8 aiqdFileName)
{
    LOG1("@%s", __FUNCTION__);
    ia_binary_data aiqdData;
    ia_err err;

    CLEAR(aiqdData);
    err = ia_aiq_get_aiqd_data(m3aState.ia_aiq_handle, &aiqdData);
    if (err != ia_err_none
        || aiqdData.size == 0
        || aiqdData.data == NULL) {
        LOGE("@%s, call ia_aiq_get_aiqd_data() fail, err:%d, size:%d, data:%p", __FUNCTION__, err, aiqdData.size, aiqdData.data);
        return false;
    }

    FILE *f = fopen(aiqdFileName, "wb");
    if (NULL == f) {
        LOGE("@%s, Can't save aiqd to file!", __FUNCTION__);
        return false;
    }

    fwrite((char *)aiqdData.data, 1, aiqdData.size, f);
    fflush(f);
    fclose(f);

    LOG2("@%s, Save aiqd %d bytes to file: %s", __FUNCTION__, aiqdData.size, aiqdFileName.string());
    return true;
}



status_t Aiq3A::attachHwControl(HWControlGroup &hwcg)
{
    LOG1("@%s", __FUNCTION__);
    mISP = hwcg.mIspCI;
    mSensorCI = hwcg.mSensorCI;
    mFlashCI = hwcg.mFlashCI;
    mLensCI = hwcg.mLensCI;
    return NO_ERROR;
}

status_t Aiq3A::getAiqConfig(ia_binary_data *cpfData)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    if (PlatformData::AiqConfig && cpfData != NULL) {
        cpfData->data = PlatformData::AiqConfig.ptr();
        cpfData->size = PlatformData::AiqConfig.size();
    } else {
        status = UNKNOWN_ERROR;
    }
    return status;
}

status_t Aiq3A::deinit3A()
{
    LOG1("@%s", __FUNCTION__);

    // if app don't close torch, close it.
    if (mAeFlashMode == CAM_AE_FLASH_MODE_TORCH ||
        mAeFlashMode == CAM_AE_FLASH_MODE_SINGLE)
        mFlashCI->setTorch(0);

    // if camera exit after starting CAF, stop it.
    if (mStillAfStart != 0)
        stopStillAf();

    if (mAeState.stored_results) {
        delete mAeState.stored_results;
        mAeState.stored_results = NULL;
    }
    if (m3aState.faces) {
        free(m3aState.faces);
        m3aState.faces = NULL;
    }

    if (mCmcParsedAG.cmc_analog_gain_conversion)
        delete mCmcParsedAG.cmc_analog_gain_conversion;
    if (mCmcParsedAG.cmc_analog_gain_segments)
        delete mCmcParsedAG.cmc_analog_gain_segments;
    if (mCmcParsedAG.cmc_analog_gain_pairs)
        delete mCmcParsedAG.cmc_analog_gain_pairs;
    mCmcParsedAG.cmc_analog_gain_conversion = NULL;
    mCmcParsedAG.cmc_analog_gain_segments = NULL;
    mCmcParsedAG.cmc_analog_gain_pairs = NULL;

    if (mCmcParsedDG.cmc_digital_gain)
        delete mCmcParsedDG.cmc_digital_gain;
    if (mCmcParsedDG.cmc_digital_gain_v102)
        delete mCmcParsedDG.cmc_digital_gain_v102;
    if (mCmcParsedDG.cmc_digital_gain_pairs)
        delete mCmcParsedDG.cmc_digital_gain_pairs;
    mCmcParsedDG.cmc_digital_gain = NULL;
    mCmcParsedDG.cmc_digital_gain_v102 = NULL;
    mCmcParsedDG.cmc_digital_gain_pairs = NULL;

    // store the aiqd to file system
    saveAiqdData(getAiqdFileName());

    ia_aiq_deinit(m3aState.ia_aiq_handle);
    ia_mkn_uninit(mMkn);
    mISP = NULL;
    mAfMode = CAM_AF_MODE_NOT_SET;
    mAwbMode = CAM_AWB_MODE_NOT_SET;
    mFocusPosition = 0;
    return NO_ERROR;
}

status_t Aiq3A::switchModeAndRate(IspMode mode, float fps)
{
    status_t status = NO_ERROR;
    LOG1("@%s: mode = %d", __FUNCTION__, mode);

    ia_aiq_frame_use isp_mode;
    switch (mode) {
    case MODE_PREVIEW:
        isp_mode = ia_aiq_frame_use_preview;
        break;
    case MODE_CAPTURE:
        isp_mode = ia_aiq_frame_use_still;
        break;
    case MODE_VIDEO:
    case MODE_CONTINUOUS_VIDEO:
        isp_mode = ia_aiq_frame_use_video;
        break;
    case MODE_CONTINUOUS_CAPTURE:
        isp_mode = ia_aiq_frame_use_continuous;
        break;
    default:
        isp_mode = ia_aiq_frame_use_preview;
        LOGW("SwitchMode: Wrong sensor mode %d", mode);
        break;
    }

    m3aState.frame_use = isp_mode;
    mAfInputParameters.frame_use = m3aState.frame_use;
    mAfState.previous_sof = 0;
    mAeInputParameters.frame_use = m3aState.frame_use;
    if (mFpsSetting & FPS_SETTING_OFF)
        mAeInputParameters.manual_limits->manual_frame_time_us_min = (long) 1/fps*1000000;

    mFlashStage = CAM_FLASH_STAGE_NOT_SET;
    mAwbInputParameters.frame_use = m3aState.frame_use;

    // In high speed recording or fixed fps case, set the AE operation mode as action to notify AIQ
    if (fps > DEFAULT_RECORDING_FPS) {
        mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_action;
        mFpsSetting = FPS_SETTING_HIGH_SPEED;
    } else if (mAeSceneMode == CAM_AE_SCENE_MODE_NOT_SET || mAeSceneMode == CAM_AE_SCENE_MODE_AUTO) {
        // When the default AE scene mode (AUTO) is used, the application will not set the
        // scene mode when switching between different capture modes. Reset the AE operation
        // mode to default value when not in HS recording mode
        mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_automatic;
    }

    /* usually the grid changes as well when the mode changes. */
    changeSensorMode();
    if (mBracketingRunning) {
        mAeBracketingInputParameters = mAeInputParameters;
        mAeBracketingInputParameters.frame_use =  m3aState.frame_use;
    }

    // Using fixed delay to sync active sensor exposure with AEC input
    // parameters.
    // Note: fixed delay needs to be big enough to cover latencies of
    //       3A process itself (timing of applying compared to frames)
    //       and the pre-defined latencies by sensor
    // TODO: Use API's further to control this setting
    if (mAeInputParameters.frame_use == ia_aiq_frame_use_still)
        mAeState.feedback_delay = 0;
    else {
        mAeState.feedback_delay = mSensorCI->getExposureDelay();
        if (mAeState.feedback_delay == 0)
            mAeState.feedback_delay = AE_DELAY_FRAMES_DEFAULT;
    }

    /* Invalidate and re-run AEC to re-calculate sensor exposure for potential changes
     * in sensor settings */
    mAeState.ae_results = NULL;
    mStatCount = 0;
    int old_exposures_num = mAeInputParameters.num_exposures;
    mAeInputParameters.num_exposures = COLD_START_EXPOSURE_NUM;

    // the 3A has the issue for online still capture, it will set one lower exposure value than the preview to driver.
    // TODO: remove it when the 3A issue has fixed for the online still capture.
    int old_manual_exposure_time_us = mAeInputParameters.manual_exposure_time_us;
    float old_manual_analog_gain = mAeInputParameters.manual_analog_gain;
    if (m3aState.frame_use == ia_aiq_frame_use_still
        && mAeFlashMode == CAM_AE_FLASH_MODE_OFF) {
        LOG3A("@%s, use the last exposure settings", __FUNCTION__);
        if (mAeInputParameters.manual_exposure_time_us == -1)
            mAeInputParameters.manual_exposure_time_us = mLastExposure.exposure_time_us;
        if (mAeInputParameters.manual_analog_gain == -1)
            mAeInputParameters.manual_analog_gain = mLastExposure.analog_gain;
    }

    status = runAeMain();

    mAeInputParameters.num_exposures = old_exposures_num;
    mAeInputParameters.manual_exposure_time_us = old_manual_exposure_time_us;
    mAeInputParameters.manual_analog_gain = old_manual_analog_gain;

    /* Default GBCE is used with flash.
     * We use default GBCE flag to indicate when to restore results from
     * pre-flash start condition.
     * Note:
     * - AWB is run during flash sequence and results are therefore explicitly stored
     * - GBCE is run during flash sequence with default gamma
     *   runAICMain()
     */

    status |= runGBCEMain();

    if (mGBCEDefault && isp_mode != ia_aiq_frame_use_still) {
        LOG1("Restoring AWBresults that preceded flash");
        mGBCEDefault = false;
        mAwbResults = &mAwbStoredResults;
    } else {
        /* run AWB to get initial values */
        runAwbMain();
    }

    // reset mLensShading to true
    mLensShading = true;
    /* Re-run AIC to get new results for new mode. LSC needs to be updated if resolution changes. */
    status |= runAICMain();

    return status;
}

status_t Aiq3A::setAeWindow(const CameraWindow *window)
{

    LOG1("@%s", __FUNCTION__);

    if (window == NULL)
        return BAD_VALUE;
    else
        LOG3A("window = %p (%d,%d,%d,%d,%d)", window,
                                             window->x_left,
                                             window->y_top,
                                             window->x_right,
                                             window->y_bottom,
                                             window->weight);

    /* Calculate center coordinate of window. */
    int width = window->x_right - window->x_left;
    int height = window->y_bottom - window->y_top;
    mAeCoord.x = window->x_left + width/2;
    mAeCoord.y = window->y_top + height/2;

    return NO_ERROR;
}

status_t Aiq3A::setAfWindow(const CameraWindow *window)
{
    if (window == NULL)
        return BAD_VALUE;
    else
        LOG3A("@%s: window = %p (%d,%d,%d,%d,%d)", __FUNCTION__,
            window,
            window->x_left,
            window->y_top,
            window->x_right,
            window->y_bottom,
            window->weight);

    // In case of null-window, the HAL can decide which metering is used. Use "auto".
    if (window[0].x_left == window[0].x_right && window[0].y_top == window[0].y_bottom &&
        window[0].x_left == 0 && window[0].y_top == 0) {
            setAfMeteringMode(ia_aiq_af_metering_mode_auto);
    } else {
        // When window is set, obey the coordinates. Use touch.
        setAfMeteringMode(ia_aiq_af_metering_mode_touch);
        LOG3A("Af window not NULL, metering mode touch");
    }

    mAfInputParameters.focus_rect->left = window[0].x_left;
    mAfInputParameters.focus_rect->top = window[0].y_top;
    mAfInputParameters.focus_rect->width = window[0].x_right - window[0].x_left;
    mAfInputParameters.focus_rect->height = window[0].y_bottom - window[0].y_top;

    //ToDo: Make sure that all coordinates passed to AIQ are in format/range defined in ia_coordinate.h.

    return NO_ERROR;
}

status_t Aiq3A::setAfWindows(const CameraWindow *windows, size_t numWindows)
{
    LOG3A("@%s: windows = %p, num = %u", __FUNCTION__, windows, numWindows);

    // If no windows given, equal to null-window. HAL meters as it wants -> "auto".
    if (numWindows == 0) {
        setAfMeteringMode(ia_aiq_af_metering_mode_auto);
        return NO_ERROR;
    }

    // at the moment we only support one window
    return setAfWindow(windows);
}

// TODO: no manual setting for scene mode, map that into AE/AF operation mode
status_t Aiq3A::setAeSceneMode(SceneMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);

    mAeSceneMode = mode;
    resetAECParams();
    resetAWBParams();
    switch (mode) {
    case CAM_AE_SCENE_MODE_AUTO:
        break;
    case CAM_AE_SCENE_MODE_PORTRAIT:
        break;
    case CAM_AE_SCENE_MODE_SPORTS:
        mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_action;
        break;
    case CAM_AE_SCENE_MODE_LANDSCAPE:
        break;
    case CAM_AE_SCENE_MODE_NIGHT:
        mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_long_exposure;
        mAeInputParameters.flash_mode = ia_aiq_flash_mode_off;
        break;
    case CAM_AE_SCENE_MODE_FIREWORKS:
        mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_fireworks;
        mAwbInputParameters.scene_mode = ia_aiq_awb_operation_mode_manual_cct_range;
        m3aState.cct_range.min_cct = 5500;
        m3aState.cct_range.max_cct = 5500;
        mAwbInputParameters.manual_cct_range = &m3aState.cct_range;
        break;
    default:
        LOGE("Get: invalid AE scene mode (%d).", mode);
        break;
    }
    return NO_ERROR;
}

SceneMode Aiq3A::getAeSceneMode()
{
    return mAeSceneMode;
}

status_t Aiq3A::setAeFlickerMode(FlickerMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);

    mAeFlickerMode = mode;
    switch(mode) {
    case CAM_AE_FLICKER_MODE_50HZ:
        mAeInputParameters.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_50hz;
        break;
    case CAM_AE_FLICKER_MODE_60HZ:
        mAeInputParameters.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_60hz;
        break;
    case CAM_AE_FLICKER_MODE_AUTO:
        mAeInputParameters.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_auto;
        break;
    case CAM_AE_FLICKER_MODE_OFF:
    default:
        mAeInputParameters.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_off;
        break;
    }

    return NO_ERROR;
}

FlickerMode Aiq3A::getAeFlickerMode()
{
    return mAeFlickerMode;
}

// No support for aperture priority, always in shutter priority mode
status_t Aiq3A::setAeMode(AeMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);

    mAeMode = mode;
    switch(mode) {
    case CAM_AE_MODE_MANUAL:
        break;
    case CAM_AE_MODE_AUTO:
    case CAM_AE_MODE_SHUTTER_PRIORITY:
    case CAM_AE_MODE_APERTURE_PRIORITY:
    default:
        mAeInputParameters.manual_analog_gain = -1;
        mAeInputParameters.manual_iso = -1;
        mAeInputParameters.manual_exposure_time_us = -1;
        if (!(mFpsSetting & FPS_SETTING_HIGH_SPEED))
            mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_automatic;
        break;
    }
    return NO_ERROR;
}

AeMode Aiq3A::getAeMode()
{
    return mAeMode;
}

status_t Aiq3A::setAfMode(AfMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);

    //reset the AF action
    if (mAfInputParameters.manual_focus_parameters)
        mAfInputParameters.manual_focus_parameters->manual_focus_action = ia_aiq_manual_focus_action_none;

    switch (mode) {
    case CAM_AF_MODE_CONTINUOUS:
        setAfFocusMode(ia_aiq_af_operation_mode_auto);
        setAfFocusRange(ia_aiq_af_range_normal);
        setAfMeteringMode(ia_aiq_af_metering_mode_auto);
        break;
    case CAM_AF_MODE_AUTO:
        // we use hyperfocal default lens position in hyperfocal mode
        setAfFocusMode(ia_aiq_af_operation_mode_manual);
        setAfFocusRange(ia_aiq_af_range_extended);
        setAfMeteringMode(ia_aiq_af_metering_mode_auto);
        break;
    case CAM_AF_MODE_MACRO:
        setAfFocusMode(ia_aiq_af_operation_mode_manual);
        setAfFocusRange(ia_aiq_af_range_macro);
        setAfMeteringMode(ia_aiq_af_metering_mode_auto);
        break;
    case CAM_AF_MODE_INFINITY:
        setAfFocusMode(ia_aiq_af_operation_mode_infinity);
        setAfFocusRange(ia_aiq_af_range_extended);
        break;
    case CAM_AF_MODE_FIXED:
        setAfFocusMode(ia_aiq_af_operation_mode_hyperfocal);
        setAfFocusRange(ia_aiq_af_range_extended);
        break;
    case CAM_AF_MODE_MANUAL:
        setAfFocusMode(ia_aiq_af_operation_mode_manual);
        setAfFocusRange(ia_aiq_af_range_extended);
        break;
    default:
        LOGE("Set: invalid AF mode: %d. Using AUTO!", mode);
        mode = CAM_AF_MODE_AUTO;
        setAfFocusMode(ia_aiq_af_operation_mode_auto);
        setAfFocusRange(ia_aiq_af_range_normal);
        setAfMeteringMode(ia_aiq_af_metering_mode_auto);
        break;
    }

    mAfMode = mode;

    return NO_ERROR;
}

AfMode Aiq3A::getAfMode()
{
    LOG1("@%s, afMode: %d", __FUNCTION__, mAfMode);
    return mAfMode;
}

status_t Aiq3A::setAeFlashMode(FlashMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);
    // no support for slow sync and day sync flash mode,
    // just use auto flash mode to replace
    ia_aiq_flash_mode wr_val;

    if (mAeFlashMode == CAM_AE_FLASH_MODE_TORCH
        && mode != CAM_AE_FLASH_MODE_TORCH)
        mFlashCI->setTorch(0);
    if (mAeFlashMode == CAM_AE_FLASH_MODE_SINGLE
        && mode != CAM_AE_FLASH_MODE_SINGLE)
        mFlashCI->setTorch(0);
    switch (mode) {
    case CAM_AE_FLASH_MODE_ON:
    case CAM_AE_FLASH_MODE_DAY_SYNC:
    case CAM_AE_FLASH_MODE_SLOW_SYNC:
        wr_val = ia_aiq_flash_mode_on;
        break;
    case CAM_AE_FLASH_MODE_TORCH:
        if (mAeFlashMode != CAM_AE_FLASH_MODE_TORCH)
            mFlashCI->setTorch(TORCH_INTENSITY);
        // Note: intentional omit of break, AEC is run with
        //       ia_aiq_flash_mode_off when torch is used.
    case CAM_AE_FLASH_MODE_OFF:
        wr_val = ia_aiq_flash_mode_off;
        break;
    case CAM_AE_FLASH_MODE_AUTO:
        wr_val = ia_aiq_flash_mode_auto;
        break;
    case CAM_AE_FLASH_MODE_SINGLE:
        wr_val = ia_aiq_flash_mode_on;
        if (mAeFlashMode != CAM_AE_FLASH_MODE_SINGLE) {
            mFlashCI->setTorch(TORCH_INTENSITY);
        }
        break;
    default:
        LOGE("Set: invalid flash mode: %d. Using AUTO!", mode);
        mode = CAM_AE_FLASH_MODE_AUTO;
        wr_val = ia_aiq_flash_mode_auto;
        break;
    }
    mAeFlashMode = mode;
    mAeInputParameters.flash_mode = wr_val;

    return NO_ERROR;
}

FlashMode Aiq3A::getAeFlashMode()
{
    LOG3A("@%s", __FUNCTION__);
    return mAeFlashMode;
}

bool Aiq3A::getAfNeedAssistLight()
{
    LOG1("@%s", __FUNCTION__);
    bool ret = false;
    if (mAfState.assist_light)
        ret = true;
    else if (mAfState.af_results)
        ret = mAfState.af_results->use_af_assist;
    return ret;
}

// ToDo: check if this function is needed or if the info
// could be used directly from AE results
// TODO 2: add support for multiple flash leds
bool Aiq3A::getAeFlashNecessary()
{
    LOG3A("@%s", __FUNCTION__);
    if(mAeState.ae_results)
        return mAeState.ae_results->flashes[0].status;
    else
        return false;
}

status_t Aiq3A::setAwbMode (AwbMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);
    mAwbDisabled = false;
    ia_aiq_awb_operation_mode wr_val;
    switch (mode) {
    case CAM_AWB_MODE_DAYLIGHT:
        wr_val = ia_aiq_awb_operation_mode_daylight;
        break;
    case CAM_AWB_MODE_CLOUDY:
        wr_val = ia_aiq_awb_operation_mode_partly_overcast;
        break;
    case CAM_AWB_MODE_SUNSET:
        wr_val = ia_aiq_awb_operation_mode_sunset;
        break;
    case CAM_AWB_MODE_TUNGSTEN:
        wr_val = ia_aiq_awb_operation_mode_incandescent;
        break;
    case CAM_AWB_MODE_FLUORESCENT:
        wr_val = ia_aiq_awb_operation_mode_fluorescent;
        break;
    case CAM_AWB_MODE_WARM_FLUORESCENT:
        wr_val = ia_aiq_awb_operation_mode_fluorescent;
        break;
    case CAM_AWB_MODE_WARM_INCANDESCENT:
        wr_val = ia_aiq_awb_operation_mode_incandescent;
        break;
    case CAM_AWB_MODE_SHADOW:
        wr_val = ia_aiq_awb_operation_mode_fully_overcast;
        break;
    case CAM_AWB_MODE_MANUAL_INPUT:
        wr_val = ia_aiq_awb_operation_mode_manual_white;
        break;
    case CAM_AWB_MODE_OFF:
        mAwbDisabled = true;
        break;
    case CAM_AWB_MODE_AUTO:
        wr_val = ia_aiq_awb_operation_mode_auto;
        break;
    default:
        LOGE("Set: invalid AWB mode: %d. Using AUTO!", mode);
        mode = CAM_AWB_MODE_AUTO;
        wr_val = ia_aiq_awb_operation_mode_auto;
        break;
    }

    mAwbMode = mode;
    if (mAwbMode != CAM_AWB_MODE_OFF) {
        mAwbInputParameters.scene_mode = wr_val;
        LOG3A("@%s: Intel mode = %d", __FUNCTION__, mAwbInputParameters.scene_mode);
    }
    return NO_ERROR;
}

AwbMode Aiq3A::getAwbMode()
{
    return mAwbMode;
}

status_t Aiq3A::setAwbZeroHistoryMode(bool en)
{
    LOG1("@%s", __FUNCTION__);
    mIsAwbZeroHistoryMode = en;
    return NO_ERROR;
}

AwbMode Aiq3A::getLightSource()
{
    AwbMode mode = getAwbMode();
    return mode;
}

status_t Aiq3A::setAeMeteringMode(MeteringMode mode)
{
    LOG1("@%s: mode = %d", __FUNCTION__, mode);

    /* Don't use exposure coordinate in other than SPOT mode. */
    mAeInputParameters.exposure_coordinate = NULL;

    ia_aiq_ae_metering_mode wr_val;
    switch (mode) {
    case CAM_AE_METERING_MODE_SPOT:
        mAeInputParameters.exposure_coordinate = &mAeCoord;
        wr_val = ia_aiq_ae_metering_mode_evaluative;
        break;
    case CAM_AE_METERING_MODE_CENTER:
        wr_val = ia_aiq_ae_metering_mode_center;
        break;
    case CAM_AE_METERING_MODE_CUSTOMIZED:
    case CAM_AE_METERING_MODE_AUTO:
        wr_val = ia_aiq_ae_metering_mode_evaluative;
        break;
    default:
        LOGE("Set: invalid AE metering mode: %d. Using AUTO!", mode);
        wr_val = ia_aiq_ae_metering_mode_evaluative;
        break;
    }
    mAeInputParameters.metering_mode = wr_val;

    return NO_ERROR;
}

MeteringMode Aiq3A::getAeMeteringMode()
{
    LOG1("@%s", __FUNCTION__);
    MeteringMode mode = CAM_AE_METERING_MODE_NOT_SET;

    ia_aiq_ae_metering_mode rd_val = mAeInputParameters.metering_mode;
    switch (rd_val) {
    case ia_aiq_ae_metering_mode_evaluative:
        /* Handle SPOT mode. */
        if (mAeInputParameters.exposure_coordinate)
            mode = CAM_AE_METERING_MODE_SPOT;
        else
            mode = CAM_AE_METERING_MODE_AUTO;
        break;
    case ia_aiq_ae_metering_mode_center:
        mode = CAM_AE_METERING_MODE_CENTER;
        break;
    default:
        LOGE("Get: invalid AE metering mode: %d. Using SPOT!", rd_val);
        mode = CAM_AE_METERING_MODE_SPOT;
        break;
    }

    return mode;
}

status_t Aiq3A::setNRMode(bool enable)
{
    LOG1("@%s", __FUNCTION__);
    mNoiseReduction = enable;
    return NO_ERROR;
}

status_t Aiq3A::setEdgeMode(bool enable)
{
    LOG1("@%s", __FUNCTION__);
    mEdgeEnhancement = enable;
    return NO_ERROR;
}

status_t Aiq3A::setShadingMode(bool enable)
{
    LOG1("@%s, enable:%d", __FUNCTION__, enable);
    mLensShading = enable;
    return NO_ERROR;
}

status_t Aiq3A::setEdgeStrength(int strength)
{
    LOG1("@%s", __FUNCTION__);
    mEdgeStrength = strength;
    return NO_ERROR;
}

status_t Aiq3A::setAeLock(bool en)
{
    LOG1("@%s: en = %d", __FUNCTION__, en);
    mAeState.ae_locked = en;
    return NO_ERROR;
}

bool Aiq3A::getAeLock()
{
    LOG1("@%s", __FUNCTION__);
    bool ret = mAeState.ae_locked;
    return ret;
}

status_t Aiq3A::setAfLock(bool en)
{
    LOG1("@%s: en = %d", __FUNCTION__, en);
    mAfState.af_locked = en;
    return NO_ERROR;
}

bool Aiq3A::getAfLock()
{
    LOG1("@%s, af_locked: %d ", __FUNCTION__, mAfState.af_locked);
    bool ret = false;
    ret = mAfState.af_locked;
    return ret;
}

AfStatus Aiq3A::getCAFStatus()
{
    LOG3A("@%s", __FUNCTION__);
    AfStatus status = CAM_AF_STATUS_BUSY;
    if (mAfState.af_results != NULL) {
        if (mAfState.af_results->status == ia_aiq_af_status_success && (mAfState.af_results->final_lens_position_reached || mStillAfStart == 0)) {
            status = CAM_AF_STATUS_SUCCESS;
            mContinuousAfStart = 0;
        }
        else if (mAfState.af_results->status == ia_aiq_af_status_fail && (mAfState.af_results->final_lens_position_reached || mStillAfStart == 0)) {
            status  = CAM_AF_STATUS_FAIL;
            mContinuousAfStart = 0;
        }
        else {
            status = CAM_AF_STATUS_BUSY;
            if (mContinuousAfStart == 0)
                mContinuousAfStart = systemTime();
            if (((systemTime() - mContinuousAfStart) / 1000000) > AIQ_MAX_TIME_FOR_AF) {
                LOGW("Auto-focus sequence for continuous capture is taking too long. Cancelling!");
                mContinuousAfStart = 0;
                status = CAM_AF_STATUS_FAIL;
            }
        }
    }

    if (mStillAfStart != 0 && status == CAM_AF_STATUS_BUSY) {
        if (((systemTime() - mStillAfStart) / 1000000) > AIQ_MAX_TIME_FOR_AF) {
            LOGW("Auto-focus sequence for still capture is taking too long. Cancelling!");
            status = CAM_AF_STATUS_FAIL;
        }
    }
    LOG3A("af_results->status:%d", status);
    return status;
}

status_t Aiq3A::setAwbLock(bool en)
{
    LOG1("@%s: en = %d", __FUNCTION__, en);
    mAwbLocked = en;
    return NO_ERROR;
}

bool Aiq3A::getAwbLock()
{
    LOG1("@%s, AsbLocked: %d", __FUNCTION__, mAwbLocked);
    bool ret = mAwbLocked;
    return ret;
}

status_t Aiq3A::set3AColorEffect(EffectMode effect)
{
    LOG1("@%s: effect = %d", __FUNCTION__, effect);
    status_t status = NO_ERROR;

    ia_isp_effect ispEffect = ia_isp_effect_none;
    if (effect == CAM_EFFECT_MONO)
        ispEffect = ia_isp_effect_grayscale;
    else if (effect == CAM_EFFECT_NEGATIVE)
        ispEffect = ia_isp_effect_negative;
    else if (effect == CAM_EFFECT_SEPIA)
        ispEffect = ia_isp_effect_sepia;
    else if (effect == CAM_EFFECT_SKY_BLUE)
        ispEffect = ia_isp_effect_sky_blue;
    else if (effect == CAM_EFFECT_GRASS_GREEN)
        ispEffect = ia_isp_effect_grass_green;
    else if (effect == CAM_EFFECT_SKIN_WHITEN_LOW)
        ispEffect = ia_isp_effect_skin_whiten_low;
    else if (effect == CAM_EFFECT_SKIN_WHITEN)
        ispEffect = ia_isp_effect_skin_whiten;
    else if (effect == CAM_EFFECT_SKIN_WHITEN_HIGH)
        ispEffect = ia_isp_effect_skin_whiten_high;
    else if (effect == CAM_EFFECT_VIVID)
        ispEffect = ia_isp_effect_vivid;
    else if (effect == CAM_EFFECT_NONE)
        ispEffect = ia_isp_effect_none;
    else {
        LOGE("Color effect not found.");
        status = -1;
        // Fall back to the effect NONE
    }
    mIspInputParams.effects = ispEffect;
    return status;
}

void Aiq3A::setPublicAeMode(AeMode mode)
{
    LOG3A("@%s, mPublicAeMode: %d", __FUNCTION__, mode);
    mPublicAeMode = mode;
}

AeMode Aiq3A::getPublicAeMode()
{
    LOG3A("@%s, mPublicAeMode: %d", __FUNCTION__, mPublicAeMode);
    return mPublicAeMode;
}

void Aiq3A::setPublicAfMode(AfMode mode)
{
    LOG3A("@%s, AfMode: %d", __FUNCTION__, mode);
    mAfMode = mode;
}

AfMode Aiq3A::getPublicAfMode()
{
    LOG3A("@%s, AfMode: %d", __FUNCTION__, mAfMode);
    return mAfMode;
}


int Aiq3A::getCurrentFocusDistance()
{
    int focus_distance = 0;
    if (mAfState.af_results == NULL) {
        return focus_distance;
    }

    //workaround: Because the current_focus_distance in af result should take effect after 5 stats,
    //it's too late, it will cause the CTS case failed, so just return the input.
    if (mAfInputParameters.manual_focus_parameters) {
        focus_distance = mAfInputParameters.manual_focus_parameters->manual_focus_distance;
    }
    LOG3A("@%s, current_focus_distance: %d", __FUNCTION__, focus_distance);
    return focus_distance;
}

int Aiq3A::getOpticalStabilizationMode()
{
    //add interface of OpticalStabilizationmode, default value is OFF.
    return 0;
}
int Aiq3A::getnsExposuretime()
{
    if (mAeState.ae_results == NULL)
        return mLastExposuretime;

    int exposuretime;
    exposuretime = mSensorCI->getExposurelinetime() * mAeState.ae_results->exposures->sensor_exposure->coarse_integration_time;
    mLastExposuretime = exposuretime;
    LOG2("@%s, : %d", __FUNCTION__, exposuretime);

    return exposuretime;
}

int Aiq3A::getnsFrametime()
{
    if (mAeState.ae_results == NULL)
        return mLastFrameTime;

    int frametime;
    frametime = mSensorCI->getExposurelinetime() * mAeState.ae_results->exposures->sensor_exposure->frame_length_lines;
    mLastFrameTime = frametime;
    LOG2("@%s, : %d", __FUNCTION__, frametime);
    return frametime;
}


status_t Aiq3A::startStillAf()
{
    LOG1("@%s", __FUNCTION__);
    if (mAeFlashMode != CAM_AE_FLASH_MODE_TORCH
        && mAeFlashMode != CAM_AE_FLASH_MODE_OFF) {
        mAfState.assist_light = getAfNeedAssistLight();
        if (mAfState.assist_light) {
            LOG1("Using AF assist light with auto-focus");
            mFlashCI->setTorch(TORCH_INTENSITY);
        }
    }
    setAfFocusMode(ia_aiq_af_operation_mode_auto);
    //ToDo: Generally "still" frame_use should get best focus accurate,
    //      but seldom get the focus success, so here replace the frame_use
    //      with "continuous", if AIQ algorithm has improvement in the future,
    //      then change to use "still"
    mAfInputParameters.frame_use = ia_aiq_frame_use_continuous;
    mStillAfStart = systemTime();

    return NO_ERROR;
}

status_t Aiq3A::stopStillAf()
{
    LOG1("@%s", __FUNCTION__);
    if (mAfState.assist_light) {
        LOG1("Turning off AF assist light");
        mFlashCI->setTorch(0);
        mAfState.assist_light = false;
    }

    if (mAfMode == CAM_AF_MODE_AUTO || mAfMode == CAM_AF_MODE_MACRO) {
        setAfFocusMode(ia_aiq_af_operation_mode_manual);
    }
    mAfInputParameters.frame_use = m3aState.frame_use;

    mStillAfStart = 0;
    return NO_ERROR;
}

AfStatus Aiq3A::isStillAfComplete()
{
    LOG3A("@%s", __FUNCTION__);
    if (mStillAfStart == 0) {
        // startStillAf wasn't called? return error
        LOGE("Call startStillAf before calling %s!", __FUNCTION__);
        return CAM_AF_STATUS_FAIL;
    }
    AfStatus ret = getCAFStatus();
    return ret;
}

status_t Aiq3A::getExposureInfo(SensorAeConfig& aeConfig)
{
    LOG3A("@%s", __FUNCTION__);

    // evBias not reset, so not using memset
    aeConfig.expTime = 0;
    aeConfig.aperture_num = 0;
    aeConfig.aperture_denum = 1;
    aeConfig.aecApexTv = 0;
    aeConfig.aecApexSv = 0;
    aeConfig.aecApexAv = 0;
    aeConfig.digitalGain = 0;
    getAeExpCfg(&aeConfig.expTime,
            &aeConfig.aperture_num,
            &aeConfig.aperture_denum,
            &aeConfig.aecApexTv,
            &aeConfig.aecApexSv,
            &aeConfig.aecApexAv,
            &aeConfig.digitalGain,
            &aeConfig.totalGain);

    return NO_ERROR;
}

// TODO: it needed by exif, so need AIQ provide
status_t Aiq3A::getAeManualBrightness(float *ret)
{
    LOG1("@%s", __FUNCTION__);
    UNUSED(ret);
    return INVALID_OPERATION;
}

// Focus operations
status_t Aiq3A::initAfBracketing(int stops, AFBracketingMode mode)
{
    LOG1("@%s", __FUNCTION__);
    mBracketingStops = stops;
    ia_aiq_af_bracket_input_params param;
    switch (mode) {
    case CAM_AF_BRACKETING_MODE_SYMMETRIC:
        param.af_bracket_mode = ia_aiq_af_bracket_mode_symmetric;
        break;
    case CAM_AF_BRACKETING_MODE_TOWARDS_NEAR:
        param.af_bracket_mode = ia_aiq_af_bracket_mode_towards_near;
        break;
    case CAM_AF_BRACKETING_MODE_TOWARDS_FAR:
        param.af_bracket_mode = ia_aiq_af_bracket_mode_towards_far;
        break;
    default:
        param.af_bracket_mode = ia_aiq_af_bracket_mode_symmetric;
        break;
    }
    param.focus_positions = (char) stops;
    //first run AF to get the af result
    runAfMain();
    memcpy(&param.af_results, mAfState.af_results, sizeof(ia_aiq_af_results));
    ia_aiq_af_bracket(m3aState.ia_aiq_handle, &param, &mAfBracketingResult);
    for(int i = 0; i < stops; i++)
        LOG3A("i=%d, postion=%d", i, mAfBracketingResult->lens_positions_bracketing[i]);

    return  NO_ERROR;
}

status_t Aiq3A::setManualFocusIncrement(int steps)
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = NO_ERROR;
    if(steps >= 0 && steps < mBracketingStops) {
        int position = mAfBracketingResult->lens_positions_bracketing[steps];
        int focus_moved = mLensCI->moveFocusToPosition(position);
        if(focus_moved != 0)
            ret = UNKNOWN_ERROR;
    }
    return ret;
}

status_t Aiq3A::initAeBracketing()
{
    LOG1("@%s", __FUNCTION__);
    mBracketingRunning = true;
    return NO_ERROR;
}

// Exposure operations
// For exposure bracketing
status_t Aiq3A::applyEv(float bias)
{
    LOG1("@%s: bias=%.2f", __FUNCTION__, bias);
    mAeBracketingInputParameters.ev_shift = bias;
    mAeBracketingInputParameters.flash_mode = ia_aiq_flash_mode_off;
    status_t ret = NO_ERROR;
    if (m3aState.ia_aiq_handle){
        ia_aiq_ae_run(m3aState.ia_aiq_handle, &mAeBracketingInputParameters, &mAEBracketingResult);
    }
    if (mAEBracketingResult != NULL) {

        /* Apply Sensor settings */
        ret |= mSensorCI->setExposure(mAEBracketingResult->exposures[0].sensor_exposure->coarse_integration_time,
                                      mAEBracketingResult->exposures[0].sensor_exposure->fine_integration_time,
                                      mAEBracketingResult->exposures[0].sensor_exposure->analog_gain_code_global,
                                      mAEBracketingResult->exposures[0].sensor_exposure->digital_gain_global);
    }
    return ret;
}

status_t Aiq3A::setEv(float bias)
{
    LOG1("@%s: bias=%.2f", __FUNCTION__, bias);
    if(bias > 4 || bias < -4)
        return BAD_VALUE;
    mAeInputParameters.ev_shift = bias;

    return NO_ERROR;
}

status_t Aiq3A::getEv(float *ret)
{
    LOG3A("@%s", __FUNCTION__);
    *ret = mAeInputParameters.ev_shift;
    return NO_ERROR;
}

status_t Aiq3A::setFrameRate(int fps)
{
    LOG3A("@%s fps:%d", __FUNCTION__, fps);
    if (fps <= 0)
        return INVALID_OPERATION;

    int us = 1000000 / fps;
    mAeInputParameters.manual_limits->manual_frame_time_us_min = us;
    mAeInputParameters.manual_limits->manual_frame_time_us_max = us;
    if (fps > DEFAULT_RECORDING_FPS)
        mFpsSetting = FPS_SETTING_HIGH_SPEED;
    else
        mFpsSetting = FPS_SETTING_FIXED;

    return NO_ERROR;
}

// TODO: need confirm if it's correct.
status_t Aiq3A::setManualShutter(float expTime)
{
    LOG1("@%s, expTime: %f", __FUNCTION__, expTime);
    mAeInputParameters.manual_exposure_time_us = expTime * 1000000;
    return NO_ERROR;
}

status_t Aiq3A::setManualFrameDuration(uint64_t nsFrameDuration)
{
    LOG1("@%s, frame duration %lld ns", __FUNCTION__, nsFrameDuration);
    LOGW("@%s NOT IMPLEMENTED!", __FUNCTION__);
    return OK;
}

status_t Aiq3A::setManualFrameDurationRange(int minFps, int maxFps)
{
    LOG1("@%s, frame duration range(%d, %d)", __FUNCTION__, minFps, maxFps);
    if (minFps > maxFps)
        return UNKNOWN_ERROR;
    if (minFps == maxFps) {
        setFrameRate(minFps);
    } else {
        int us_min = 1000000 / maxFps;
        int us_max = 1000000 / minFps;
        mAeInputParameters.manual_limits->manual_frame_time_us_min = us_min;
        mAeInputParameters.manual_limits->manual_frame_time_us_max = us_max;
        mFpsSetting = FPS_SETTING_RANGE;
    }

    return OK;
}

// params: distance unit is um
status_t Aiq3A::setManualFocusDistance(int distance)
{
    LOG1("@%s - distance:%d", __FUNCTION__, distance);
    if (mAfInputParameters.manual_focus_parameters) {
        mAfInputParameters.focus_mode = ia_aiq_af_operation_mode_manual;
        mAfInputParameters.manual_focus_parameters->manual_focus_distance = distance;
        mAfInputParameters.manual_focus_parameters->manual_focus_action = ia_aiq_manual_focus_action_set_distance;
    }

    return NO_ERROR;
}

status_t Aiq3A::setIsoMode(IsoMode mode)
{
    LOG1("@%s - %d", __FUNCTION__, mode);

    if (mode == CAM_AE_ISO_MODE_AUTO)
        mAeInputParameters.manual_iso = -1;

    return NO_ERROR;
}

status_t Aiq3A::setManualIso(int sensitivity)
{
    LOG1("@%s - %d", __FUNCTION__, sensitivity);
    mAeInputParameters.manual_iso = mapUIISO2RealISO(sensitivity);
    return NO_ERROR;
}

status_t Aiq3A::getManualIso(int *ret)
{
    LOG3A("@%s - %d", __FUNCTION__, mAeInputParameters.manual_iso);

    status_t status = NO_ERROR;

    if (mAeInputParameters.manual_iso > 0) {
        *ret = mapRealISO2UIISO(mAeInputParameters.manual_iso);
    } else if (mAeState.ae_results && mAeState.ae_results->exposures[0].exposure) {
        // in auto iso mode result current real iso values
        *ret = mapRealISO2UIISO(mAeState.ae_results->exposures[0].exposure->iso);
    } else {
        LOGW("no ae result available for ISO value");
        *ret = 0;
        status = UNKNOWN_ERROR;
    }

    return status;
}

status_t Aiq3A::applyPreFlashProcess(struct timeval timestamp, unsigned int exp_id, FlashStage stage)
{
    LOG3A("@%s", __FUNCTION__);

    status_t ret = NO_ERROR;

    // Upper layer is skipping frames for exposure delay,
    // setting feedback delay to 0.
    mAeState.feedback_delay = 0;
    mFlashStage = stage;

    if (stage == CAM_FLASH_STAGE_PRE || stage == CAM_FLASH_STAGE_MAIN)
    {
        /* Store previous state of 3A locks. */
        bool prev_af_lock = getAfLock();
        bool prev_ae_lock = getAeLock();
        bool prev_awb_lock = getAwbLock();
        mGBCEDefault = true;

        /* AF is not run during flash sequence. */
        setAfLock(true);

        /* During flash sequence AE and AWB must be enabled in order to calculate correct parameters for the final image. */
        setAeLock(false);
        setAwbLock(false);

        mAeInputParameters.frame_use = ia_aiq_frame_use_still;

        ret = apply3AProcess(true, timestamp, exp_id);

        mAeInputParameters.frame_use = m3aState.frame_use;

        /* Restore previous state of 3A locks. */
        setAfLock(prev_af_lock);
        setAeLock(prev_ae_lock);
        setAwbLock(prev_awb_lock);
        resetGBCEParams();
    }
    else
    {
        ret = apply3AProcess(true, timestamp, exp_id);
    }

    if (mAeInputParameters.frame_use == ia_aiq_frame_use_still)
        mAeState.feedback_delay = 0;
    else
        mAeState.feedback_delay = mSensorCI->getExposureDelay();

    return ret;
}


status_t Aiq3A::setFlash(int numFrames)
{
    LOG1("@%s: numFrames = %d", __FUNCTION__, numFrames);
    return mFlashCI->setFlash(numFrames);
}

status_t Aiq3A::apply3AProcess(bool read_stats,
                                 struct timeval sof_timestamp,
                                 unsigned int exp_id)
{
    LOG3A("@%s: read_stats = %d exp_id=%d", __FUNCTION__, read_stats, exp_id);
    status_t status = NO_ERROR;
    ia_err iaErr = ia_err_none;

    if (read_stats) {
        int64_t ts = mSensorCI->getShutterTimeByExpId(exp_id);
        if (ts > 0) {
            struct timeval timestamp;
            timestamp.tv_sec = ts / 1000000000LL;
            timestamp.tv_usec = (ts % 1000000000LL) / 1000LL;
            status = getStatistics(&timestamp, exp_id);
        } else {
            status = getStatistics(&sof_timestamp, exp_id);
        }
    }
    else
    {
        // Frame id must be always set.
        m3aState.statistics_input_parameters.frame_timestamp = TIMEVAL2USECS(&sof_timestamp);
        m3aState.statistics_input_parameters.frame_id++;
    }

    if (status == NO_ERROR) {
        iaErr = ia_aiq_statistics_set(m3aState.ia_aiq_handle,
                                      &m3aState.statistics_input_parameters);
        status |= convertError(iaErr);
    }

    mStatCount++;
    PERFORMANCE_HAL_ATRACE_PARAM1(mAeConverged ? "AEC converged after 3A runs":"AEC converging after 3A runs", mStatCount-1);
    status |= run3aMain();

    return status;
}

status_t Aiq3A::setSmartSceneDetection(bool en)
{
    LOG1("@%s: en = %d", __FUNCTION__, en);

    m3aState.dsd_enabled = en;
    return NO_ERROR;
}

bool Aiq3A::getSmartSceneDetection()
{
    LOG3A("@%s", __FUNCTION__);
    return m3aState.dsd_enabled;
}

status_t Aiq3A::getSmartSceneMode(int *sceneMode, bool *sceneHdr)
{
    LOG1("@%s", __FUNCTION__);
    if(sceneMode != NULL && sceneHdr != NULL) {
        *sceneMode = mDetectedSceneMode;
        *sceneHdr = (mAeState.ae_results->multiframe & ia_aiq_bracket_mode_hdr) ? true : false;
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t Aiq3A::setFaces(const ia_face_state& faceState)
{
    LOG3A("@%s", __FUNCTION__);

    m3aState.faces->num_faces = faceState.num_faces;
    if(m3aState.faces->num_faces > IA_AIQ_MAX_NUM_FACES)
        m3aState.faces->num_faces = IA_AIQ_MAX_NUM_FACES;

    /*ia_aiq assumes that the faces are ordered in the order of importance*/
    memcpy(m3aState.faces->faces, faceState.faces, m3aState.faces->num_faces*sizeof(ia_face));

    return NO_ERROR;
}

void Aiq3A::get3aMakerNote(ia_mkn_trg mknMode, ia_binary_data& makerNote)
{
    LOG3A("@%s", __FUNCTION__);
    ia_mkn_trg mknTarget = ia_mkn_trg_section_1;

    // detailed makernote data for RAW images
    if (mknMode == ia_mkn_trg_section_2)
        mknTarget = ia_mkn_trg_section_2;
    makerNote = ia_mkn_prepare(mMkn, mknTarget);

    return;
}

ia_binary_data *Aiq3A::get3aMakerNote(ia_mkn_trg mknMode)
{
    LOG3A("@%s", __FUNCTION__);
    ia_mkn_trg mknTarget = ia_mkn_trg_section_1;

    ia_binary_data *makerNote;
    makerNote = (ia_binary_data *)malloc(sizeof(ia_binary_data));
    if (!makerNote) {
        LOGE("Error allocation memory for mknote!");
        return NULL;
    }
    // detailed makernote data for RAW images
    if(mknMode == ia_mkn_trg_section_2)
        mknTarget = ia_mkn_trg_section_2;
    ia_binary_data mkn_binary_data = ia_mkn_prepare(mMkn, mknTarget);

    makerNote->size = mkn_binary_data.size;
    makerNote->data = (char*)malloc(makerNote->size);
    if (makerNote->data)
    {
        memcpy(makerNote->data, mkn_binary_data.data, makerNote->size);
    } else {
        LOGE("Error allocation memory for mknote data!");
        free(makerNote);
        return NULL;
    }
    return makerNote;
}

void Aiq3A::put3aMakerNote(ia_binary_data *mknData)
{
    LOG3A("@%s", __FUNCTION__);

    if (mknData) {
        if (mknData->data) {
            free(mknData->data);
            mknData->data = NULL;
        }
        free(mknData);
        mknData = NULL;
    }
}

void Aiq3A::reset3aMakerNote(void)
{
    LOG3A("@%s", __FUNCTION__);
    ia_mkn_reset(mMkn);
}

int Aiq3A::add3aMakerNoteRecord(ia_mkn_dfid mkn_format_id,
                                  ia_mkn_dnid mkn_name_id,
                                  const void *record,
                                  unsigned short record_size)
{
    LOG3A("@%s", __FUNCTION__);

    if(record != NULL)
        ia_mkn_add_record(mMkn, mkn_format_id, mkn_name_id, record, record_size, NULL);

    return 0;
}

status_t Aiq3A::convertGeneric2SensorAE(int sensitivity, int expTimeUs,
                                 unsigned short *coarse_integration_time,
                                 unsigned short *fine_integration_time,
                                 int *analog_gain_code, int *digital_gain_code)
{
    float analog_gain = 0;
    float digital_gain = 0;

    // convert exposure time
    ia_exc_exposure_time_to_sensor_units(NULL, &mAeSensorDescriptor, expTimeUs,
                            coarse_integration_time, fine_integration_time);

    // convert iso
    ia_exc_convert_iso_to_gains(&mCmcParsedAG, &mCmcParsedDG, &mCmcSensitivity,
                                mapUIISO2RealISO(sensitivity),
                                &analog_gain, analog_gain_code,
                                &digital_gain, digital_gain_code);

    return UNKNOWN_ERROR;
}

status_t Aiq3A::convertSensor2ExposureTime(unsigned short coarse_integration_time,
                                        unsigned short fine_integration_time,
                                        int analog_gain_code, int digital_gain_code,
                                        int *exposure_time, int *iso)
{
    ia_exc_sensor_units_to_exposure_time(&mAeSensorDescriptor,
                                         coarse_integration_time,
                                         fine_integration_time,
                                         exposure_time);

    ia_exc_convert_gain_codes_to_iso(&mCmcParsedAG, &mCmcParsedDG, &mCmcSensitivity,
                                     analog_gain_code, digital_gain_code, iso);

    return NO_ERROR;
}

status_t Aiq3A::convertSensorExposure2GenericExposure(unsigned short coarse_integration_time,
                                        unsigned short fine_integration_time,
                                        int analog_gain_code, int digital_gain_code,
                                        ia_aiq_exposure_parameters *exposure)
{

    ia_exc_sensor_units_to_exposure_time(&mAeSensorDescriptor,
                                         coarse_integration_time,
                                         fine_integration_time,
                                         &(exposure->exposure_time_us));
    ia_exc_sensor_units_to_digital_gain(&mCmcParsedDG,
                                        digital_gain_code,
                                        &(exposure->digital_gain));
    ia_exc_sensor_units_to_analog_gain(&mCmcParsedAG,
                                       analog_gain_code,
                                       &(exposure->analog_gain));
    ia_exc_convert_gain_codes_to_iso(&mCmcParsedAG, &mCmcParsedDG,
                                     &mCmcSensitivity,
                                     analog_gain_code, digital_gain_code,
                                     &(exposure->iso));
    exposure->total_target_exposure = (long)((float)exposure->exposure_time_us
                                              * exposure->analog_gain
                                              * exposure->digital_gain
                                              + 0.5f);
    return NO_ERROR;
}

/**
 * Helper method to store AE results into mAeState.stored_results
 *
 * mAeState.stored_results is a fixed depth fifo queue. This method
 * pushes new results into fifo and automatically dequeues the oldest
 * when fifo is full. In case updateIdx argument is given, a specific
 * index in fifo is updated instead of normal operation.
 *
 * Why results are stored ?
 *
 * AEC requires userspace synching solution to workaround the fact that
 * lower level cannot reliably map statistics to a frame and that sensor
 * metadata is not available through the pipeline. Nevertheless, AEC needs
 * input parameters containing its own results from history as a feedback.
 * This feedback needs to correspond to parameters active in sensor for a
 * frame wherefrom the statistics get read.
 *
 * AIQ API expects to receive feedback as a compound struct ia_aiq_ae_results,
 * combining 5 result structures (ae_results, weight_grid, exposure,
 * sensor_exposure and flash). New compound struct stored_ae_results is
 * defined to be able to keep the history in a generic container (AtomFifo).
 *
 * Original ae_results structure contains pointers to corresponding
 * weight_grid, exposure, sensor_exposure and flash structures. Helper
 * methods storeAeResults(), peekAeStoredResults() and pickAeFeedbackResults()
 * are provided to consolidate the related memcpy's and pointer operations.
 *
 * Sensor specific latency (currently AE_DELAY_FRAMES) and the timing of
 * applying results will specify the number of history results we need to
 * retain in memory for delayed feedback results (mAeState.feedback_delay).
 *
 * \see #peekAeStoredResults()
 * \see #pickAeFeedbackResults()
 *
 * \param ae_results AE results to be stored
 * \param updateIdx offset to be updated instead of pushing (default -1 == off)
 * \return pointer to stored structure (a copy of ae_results in fifo)
 */
ia_aiq_ae_results* Aiq3A::storeAeResults(ia_aiq_ae_results *ae_results, int updateIdx)
{
    LOG3A("@%s", __FUNCTION__);
    stored_ae_results new_stored_results_element;
    stored_ae_results *new_stored_results = &new_stored_results_element;
    if (mAeState.stored_results == NULL)
        return NULL;

    if (updateIdx >= 0) {
        // instead of dropping and placing new from stack,
        // replace the head when requested
        new_stored_results = mAeState.stored_results->peek(updateIdx);
        if (new_stored_results == NULL)
            return NULL;
    }

    if (ae_results != NULL) {
        ia_aiq_ae_results *store_results = &(new_stored_results->results);
        memcpy(store_results, ae_results, sizeof(ia_aiq_ae_results));
        memcpy(&(new_stored_results->weight_grid), ae_results->weight_grid, sizeof(ia_aiq_hist_weight_grid));
        memcpy(&(new_stored_results->exposures[0]), ae_results->exposures[0].exposure, sizeof(ia_aiq_exposure_parameters));
        memcpy(&(new_stored_results->sensor_exposures[0]), ae_results->exposures[0].sensor_exposure, sizeof(ia_aiq_exposure_sensor_parameters));
        memcpy(&(new_stored_results->flashes), ae_results->flashes, sizeof(ia_aiq_flash_parameters)*NUM_FLASH_LEDS);
        new_stored_results->exposure_result_array[0].distance_from_convergence = ae_results->exposures[0].distance_from_convergence;
        new_stored_results->exposure_result_array[0].converged = ae_results->exposures[0].converged;
        // clean cross-dependencies for container
        store_results->weight_grid = NULL;
        store_results->exposures = NULL;
        store_results->flashes = NULL;
    } else {
        CLEAR(*new_stored_results);
    }

    if (updateIdx < 0) {
        // when fifo is full drop the oldest
        if (mAeState.stored_results->getCount() >= mAeState.stored_results->getDepth())
            mAeState.stored_results->dequeue();

        int ret = mAeState.stored_results->enqueue(*new_stored_results);
        if (ret != 0)
            return NULL;
    }

    return peekAeStoredResults(0);
}

/**
 * Helper method to peek AE results from mAeState.stored_results
 *
 * \see #storeAeResults()
 *
 * \param offset to pick results from history (0 - latest)
 * \return pointer to results structure in fifo
 */
ia_aiq_ae_results* Aiq3A::peekAeStoredResults(unsigned int offset)
{
    LOG3A("@%s, offset %d", __FUNCTION__, offset);
    ia_aiq_ae_results *ae_results;
    if (!mAeState.stored_results)
        return NULL;

    stored_ae_results *stored_results_element = mAeState.stored_results->peek(offset);
    if (stored_results_element == NULL) {
        LOG1("%s: No results with offset %d", __FUNCTION__, offset);
        return NULL;
    }

    ae_results = &stored_results_element->results;

    // restore cross-dependencies
    ae_results->weight_grid = &stored_results_element->weight_grid;
    ae_results->exposures = stored_results_element->exposure_result_array;
    ae_results->exposures[0].exposure_index = 0;
    ae_results->exposures[0].exposure = &stored_results_element->exposures[0];
    ae_results->exposures[0].sensor_exposure = &stored_results_element->sensor_exposures[0];
    ae_results->flashes = stored_results_element->flashes;

    return ae_results;
}

/**
 * Helper method to pick copy of stored AE results from mAeState.stored_results
 * into mAeState.feedback_results passed as feedback with getStatistics() to
 * AEC.
 *
 * \see #storeAeResults()
 * \return pointer to mAeState.feedback_results.results
 */
ia_aiq_ae_results* Aiq3A::pickAeFeedbackResults()
{
    LOG3A("@%s, delay %d", __FUNCTION__, mAeState.feedback_delay);
    ia_aiq_ae_results *feedback_results = &mAeState.feedback_results.results;
    ia_aiq_ae_results *stored_results = peekAeStoredResults(mAeState.feedback_delay);
    if (stored_results == NULL) {
        LOGE("Failed to pick AE results from history, delay %d", mAeState.feedback_delay);
        return NULL;
    } else {
        LOG3A("Picked AE results from history, delay %d, depth %d",
                mAeState.feedback_delay, mAeState.stored_results->getCount());
        LOG3A("Feedback AEC integration_time[0]: %d",
                stored_results->exposures[0].sensor_exposure->coarse_integration_time);
        LOG3A("Feedback AEC integration_time[1]: %d",
                stored_results->exposures[0].sensor_exposure->fine_integration_time);
        LOG3A("Feedback AEC gain[0]: %x",
                stored_results->exposures[0].sensor_exposure->analog_gain_code_global);
        LOG3A("Feedback AEC gain[1]: %x",
                stored_results->exposures[0].sensor_exposure->digital_gain_global);

        memcpy(feedback_results, stored_results, sizeof(ia_aiq_ae_results));
        feedback_results->weight_grid = &mAeState.feedback_results.weight_grid;
        feedback_results->exposures = mAeState.feedback_results.exposure_result_array;
        feedback_results->exposures[0].exposure_index = 0;
        feedback_results->exposures[0].exposure = &mAeState.feedback_results.exposures[0];
        feedback_results->exposures[0].sensor_exposure = &mAeState.feedback_results.sensor_exposures[0];
        feedback_results->flashes = mAeState.feedback_results.flashes;
        feedback_results->num_exposures = NUM_EXPOSURES;
        feedback_results->exposures[0].distance_from_convergence = stored_results->exposures[0].distance_from_convergence;
        feedback_results->exposures[0].converged = stored_results->exposures[0].converged;
        memcpy(feedback_results->weight_grid, stored_results->weight_grid, sizeof(ia_aiq_hist_weight_grid));
        memcpy(feedback_results->exposures[0].exposure, stored_results->exposures[0].exposure, sizeof(ia_aiq_exposure_parameters));
        memcpy(feedback_results->exposures[0].sensor_exposure, stored_results->exposures[0].sensor_exposure, sizeof(ia_aiq_exposure_sensor_parameters));
        memcpy(feedback_results->flashes, stored_results->flashes, sizeof(ia_aiq_flash_parameters)*NUM_FLASH_LEDS);
        return feedback_results;
    }
}

/**
 * Apply exposure parameters
 */
int Aiq3A::applyExposure(ia_aiq_exposure_sensor_parameters *sensor_exposure)
{
    LOG3A("@%s", __FUNCTION__);
    int ret = 0;

    /* Apply Sensor settings as exposure changes*/
    ret = mSensorCI->setExposure(sensor_exposure->coarse_integration_time,
                                 sensor_exposure->fine_integration_time,
                                 sensor_exposure->analog_gain_code_global,
                                 sensor_exposure->digital_gain_global);
    if (ret != 0) {
        LOGE("Exposure applying failed: %d", ret);
    }

    LOG3A("Applyed sensor exposure params: {coarse=%d, analog_gain=%d, digital_gain=%d}",
        sensor_exposure->coarse_integration_time,
        sensor_exposure->analog_gain_code_global,
        sensor_exposure->digital_gain_global);

    return ret;
}

int Aiq3A::run3aInit()
{
    LOG1("@%s", __FUNCTION__);
    CLEAR(m3aState.rgbs_grid);
    CLEAR(m3aState.af_grid);

    m3aState.faces = (ia_face_state*)malloc(sizeof(ia_face_state) + IA_AIQ_MAX_NUM_FACES*sizeof(ia_face));
    if (m3aState.faces) {
        m3aState.faces->num_faces = 0;
        m3aState.faces->faces = (ia_face*)((char*)m3aState.faces + sizeof(ia_face_state));
    }
    else
        return -1;

    resetAFParams();
    mAfState.af_results = NULL;
    CLEAR(mAeCoord);
    CLEAR(mAeState);
    if (mSensorCI == NULL) {
        free(m3aState.faces);
        m3aState.faces = NULL;
        return -1;
    }
    unsigned int store_size = mSensorCI->getExposureDelay() + 1; /* max delay + current results */
    if (store_size == 1) {
        // IHWSensorControl API returned 0 delay this means not set,
        // using default
        store_size += AE_DELAY_FRAMES_DEFAULT;
    }
    mAeState.stored_results = new AtomFifo<stored_ae_results>(store_size);
    if (mAeState.stored_results == NULL) {
        free(m3aState.faces);
        m3aState.faces = NULL;
        return -1;
    }
    LOG1("@%s: keeping %d AE history results stored", __FUNCTION__, store_size);
    resetAECParams();
    resetAWBParams();
    mAwbResults = NULL;
    resetGBCEParams();
    resetDSDParams();

    return 0;
}

/* returns false for error, true for success */
bool Aiq3A::changeSensorMode(void)
{
    LOG1("@%s", __FUNCTION__);
    unsigned int frameWidth = 0;
    unsigned int frameHeight = 0;
    ia_aiq_exposure_sensor_descriptor *sensor_descriptor = &mAeSensorDescriptor;

    mSensorCI->getFrameParameters(&m3aState.sensor_frame_params,
                                  sensor_descriptor,
                                  frameWidth, frameHeight);

    /* Reconfigure 3A grid */
    mISP->sensorModeChanged();
    return true;
}

status_t Aiq3A::getStatistics(const struct timeval *sof_timestamp,
                                 unsigned int exposure_id)
{
    LOG3A("@%s exp_id=%d", __FUNCTION__, exposure_id);
    status_t ret = NO_ERROR;
    CLEAR(m3aState.statistics_input_parameters);

    m3aState.statistics_input_parameters.rgbs_grids = mRgbsGridArray;
    m3aState.statistics_input_parameters.num_rgbs_grids = NUM_EXPOSURES;
    m3aState.statistics_input_parameters.af_grids = mAfGridArray;
    m3aState.statistics_input_parameters.num_af_grids = NUM_EXPOSURES;

    PERFORMANCE_TRACES_AAA_PROFILER_START();
    ret = mISP->getIspStatistics(
            const_cast<ia_aiq_rgbs_grid**>(&m3aState.statistics_input_parameters.rgbs_grids[0]),
            const_cast<ia_aiq_af_grid**>(&m3aState.statistics_input_parameters.af_grids[0]), &exposure_id);
    if (ret < 0 && errno == EAGAIN) {
        LOG1("buffer for isp statistics reallocated according resolution changing\n");
        if (changeSensorMode() == false)
            LOGE("error in calling changeSensorMode()\n");
        ret = mISP->getIspStatistics(
                const_cast<ia_aiq_rgbs_grid**>(&m3aState.statistics_input_parameters.rgbs_grids[0]),
                const_cast<ia_aiq_af_grid**>(&m3aState.statistics_input_parameters.af_grids[0]), &exposure_id);
    }
    PERFORMANCE_TRACES_AAA_PROFILER_STOP();

    if (ret == 0)
    {
        m3aState.statistics_input_parameters.frame_timestamp = TIMEVAL2USECS(sof_timestamp);
        m3aState.statistics_input_parameters.frame_id = exposure_id;

        m3aState.statistics_input_parameters.frame_af_parameters = NULL;
        m3aState.statistics_input_parameters.external_histograms = NULL;
        m3aState.statistics_input_parameters.num_external_histograms = 0;

        if(m3aState.faces)
            m3aState.statistics_input_parameters.faces = m3aState.faces;

        if(mPaResults)
            m3aState.statistics_input_parameters.frame_pa_parameters = mPaResults;

        if (mAeState.ae_results) {
            m3aState.statistics_input_parameters.frame_ae_parameters = pickAeFeedbackResults();
        }
        //update the exposure params with the sensor metadata
        if (m3aState.statistics_input_parameters.frame_ae_parameters) {
            mISP->getDecodedExposureParams(m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].sensor_exposure,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].exposure, exposure_id);
            LOG3A("Exposure params for AIQ: (exp_id %d): {coarse=%d, analog_gain=%d, digital_gain=%d}", exposure_id,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].sensor_exposure->coarse_integration_time,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].sensor_exposure->analog_gain_code_global,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].sensor_exposure->digital_gain_global);

            LOG3A("[Check AEC][exp_id %d] Generic Exposure params for AIQ: : coarse=%d, analog_gain=%.2f, digital_gain=%.2f, iso=%d",
                    exposure_id,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].exposure->exposure_time_us,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].exposure->analog_gain,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].exposure->digital_gain,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].exposure->iso);
            LOG3A("pickup converge distance:%f, converged? : %d", m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].distance_from_convergence,
                    m3aState.statistics_input_parameters.frame_ae_parameters->exposures[0].converged);
        }

        if (mAfState.af_results) {
            // pass AF results as AEC input during still AF, AIQ will
            // internally let AEC to converge to assist light
            m3aState.af_results_feedback = *mAfState.af_results;
            m3aState.af_results_feedback.use_af_assist = mAfState.assist_light;
            m3aState.statistics_input_parameters.frame_af_parameters = &m3aState.af_results_feedback;
            LOG3A("AF assist light %s", (mAfState.assist_light) ? "on":"off");
        }
        // TODO: take into account camera mount orientation. AIQ needs device orientation to handle statistics.
        m3aState.statistics_input_parameters.camera_orientation = ia_aiq_camera_orientation_unknown;
    }

    return ret;
}

void Aiq3A::setAfFocusMode(ia_aiq_af_operation_mode mode)
{
    mAfInputParameters.focus_mode = mode;
}

void Aiq3A::setAfFocusRange(ia_aiq_af_range range)
{
    mAfInputParameters.focus_range = range;
}

void Aiq3A::setAfMeteringMode(ia_aiq_af_metering_mode mode)
{
    mAfInputParameters.focus_metering_mode = mode;
}

void Aiq3A::resetAFParams()
{
    LOG3A("@%s", __FUNCTION__);
    mAfInputParameters.focus_mode = ia_aiq_af_operation_mode_auto;
    mAfInputParameters.focus_range = ia_aiq_af_range_extended;
    mAfInputParameters.focus_metering_mode = ia_aiq_af_metering_mode_auto;
    mAfInputParameters.flash_mode = ia_aiq_flash_mode_auto;

    mAfInputParameters.focus_rect = &mAfState.focus_rect;
    mAfInputParameters.focus_rect->height = 0;
    mAfInputParameters.focus_rect->width = 0;
    mAfInputParameters.focus_rect->left = 0;
    mAfInputParameters.focus_rect->top = 0;
    mAfInputParameters.frame_use = m3aState.frame_use;
    mAfInputParameters.lens_position = 0;

    mAfInputParameters.manual_focus_parameters = &mAfState.focus_parameters;
    mAfInputParameters.manual_focus_parameters->manual_focus_action = ia_aiq_manual_focus_action_none;
    mAfInputParameters.manual_focus_parameters->manual_focus_distance = 500;
    mAfInputParameters.manual_focus_parameters->manual_lens_position = 0;
    mAfInputParameters.trigger_new_search = false;

    mAfState.af_locked = false;
    mAfState.aec_locked = false;
    mAfState.previous_sof = 0;
}

status_t Aiq3A::runAfMain()
{
    LOG3A("@%s", __FUNCTION__);
    status_t ret = NO_ERROR;

    if (mAfState.af_locked)
        return ret;

    ia_err err = ia_err_none;

    LOG3A("@af window = (%d,%d,%d,%d)",mAfInputParameters.focus_rect->height,
                 mAfInputParameters.focus_rect->width,
                 mAfInputParameters.focus_rect->left,
                 mAfInputParameters.focus_rect->top);

    if(m3aState.ia_aiq_handle)
        err = ia_aiq_af_run(m3aState.ia_aiq_handle, &mAfInputParameters, &mAfState.af_results);

    ia_aiq_af_results* af_results_ptr = mAfState.af_results;

    /* Move the lens to the required lens position */
    LOG3A("lens_driver_action:%d, af status:%d, final position:%d", af_results_ptr->lens_driver_action, af_results_ptr->status, af_results_ptr->final_lens_position_reached);
    if (err == ia_err_none && af_results_ptr->lens_driver_action == ia_aiq_lens_driver_action_move_to_unit)
    {
        LOG3A("next lens position:%d", af_results_ptr->next_lens_position);
        ret = mLensCI->moveFocusToPosition(af_results_ptr->next_lens_position);
        if (ret == NO_ERROR)
        {
            clock_gettime(CLOCK_MONOTONIC, &m3aState.lens_timestamp);
            mAfInputParameters.lens_movement_start_timestamp = (unsigned long long)((m3aState.lens_timestamp.tv_sec*1000000000LL + m3aState.lens_timestamp.tv_nsec)/1000LL);
            mAfInputParameters.lens_position = af_results_ptr->next_lens_position; /*Assume that the lens has moved to the requested position*/
        }
    }

    return ret;
}

void Aiq3A::resetAECParams()
{
    LOG3A("@%s", __FUNCTION__);
    mAeMode = CAM_AE_MODE_NOT_SET;

    mAeInputParameters.frame_use = m3aState.frame_use;
    mAeInputParameters.flash_mode = ia_aiq_flash_mode_auto;
    if (!(mFpsSetting & FPS_SETTING_HIGH_SPEED))
        mAeInputParameters.operation_mode = ia_aiq_ae_operation_mode_automatic;
    mAeInputParameters.metering_mode = ia_aiq_ae_metering_mode_evaluative;
    mAeInputParameters.priority_mode = ia_aiq_ae_priority_mode_normal;
    mAeInputParameters.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_auto;
    mAeInputParameters.sensor_descriptor = &mAeSensorDescriptor;
    mAeInputParameters.exposure_window = NULL; // TODO: exposure window should be used with digital zoom.
    mAeInputParameters.exposure_coordinate = NULL;
    mAeInputParameters.ev_shift = 0;
    mAeInputParameters.manual_exposure_time_us = -1;
    mAeInputParameters.manual_analog_gain = -1;
    mAeInputParameters.manual_iso = -1;
    mAeInputParameters.manual_limits = &mAeManualLimits;
    mAeInputParameters.manual_limits->manual_exposure_time_min = -1;
    mAeInputParameters.manual_limits->manual_exposure_time_max = -1;
    if (mFpsSetting & FPS_SETTING_OFF) {
        mAeInputParameters.manual_limits->manual_frame_time_us_min = -1;
        mAeInputParameters.manual_limits->manual_frame_time_us_max = -1;
    }
    mAeInputParameters.manual_limits->manual_iso_min = -1;
    mAeInputParameters.manual_limits->manual_iso_max = -1;
    mAeInputParameters.aec_features = NULL; // Using AEC feature definitions from CPF
    mAeInputParameters.num_exposures = NUM_EXPOSURES;
}

status_t Aiq3A::runAeMain()
{
    LOG3A("@%s", __FUNCTION__);
    status_t ret = NO_ERROR;
    bool invalidated = false;

    if (mAeState.ae_results == NULL)
    {
        LOG3A("AEC invalidated, frame_use %d", mAeInputParameters.frame_use);
        // Note: Invalidation means different behaviour in still and preview
        //  - In preview, invalidation will flush the AEC results history
        //    (mAeState.feedback_results) and fill it with new results.
        //    (this is meant to happen in mode switches)
        //
        //  - In still, runAeMain() keeps updating the latest results only
        //    while previous preview results are kept in history for later
        //    restoring when mode is switched back to preview.
        //
        //  When invalidation occurs with stored history results, stored
        //  results are picked and re-used to re-calculate the corresponding
        //  sensor exposure. (this is meant for re-calculating for changed
        //  sensor settings in mode switch and to always start with previously
        //  known good values)
        invalidated = true;
    }

    // AE locked, do nothing
    if (!invalidated && mAeState.ae_locked)
        return ret;

    // ToDo:
    // More intelligent handling of ae_lock should be implemented:

    ia_err err = ia_err_none;
    ia_aiq_ae_results *new_ae_results = NULL;

    if (mAfState.assist_light) {
        // HACK: This workaround is needed in order to get and restore
        //       sensor exposure for preview after AF with assist light
        //       ends but still before application is informed from focus
        //       done. It is done here because AIQ AEC itself provides the
        //       logic for storing and restoring over the custom AF assist
        //       mode it provides.
        // To restore preview exposure in single run, we need to run AEC
        // according to latest AF results and therefore update the input
        // parameters passed with statistics.
        if (getCAFStatus() != CAM_AF_STATUS_BUSY) {
            m3aState.af_results_feedback.use_af_assist = false;
            m3aState.statistics_input_parameters.frame_af_parameters = &m3aState.af_results_feedback;
            LOG1("AEC: AF with assist light ended, updating af results for AEC run");
            LOG2("%s frame_id = %llu", __FUNCTION__, m3aState.statistics_input_parameters.frame_id);
            ia_aiq_statistics_set(m3aState.ia_aiq_handle, &m3aState.statistics_input_parameters);
        }
    }

    if(m3aState.ia_aiq_handle){
        LOG3A("AEC manual_exposure_time_us: %d manual_analog_gain: %f manual_iso: %d",
              mAeInputParameters.manual_exposure_time_us,
              mAeInputParameters.manual_analog_gain,
              mAeInputParameters.manual_iso);
        LOG3A("AEC sensor_descriptor ->line_periods_per_field: %d", mAeInputParameters.sensor_descriptor->line_periods_per_field);
        LOG3A("AEC mAeInputParameters.frame_use: %d",mAeInputParameters.frame_use);

        err = ia_aiq_ae_run(m3aState.ia_aiq_handle, &mAeInputParameters, &new_ae_results);
        LOG3A("@%s result: %d", __FUNCTION__, err);
    }

    if (new_ae_results != NULL && new_ae_results->exposures && new_ae_results->exposures->sensor_exposure)
    {
        // Saving last ae values when flash stage is CAM_FLASH_STAGE_NONE
        if (mAeInputParameters.frame_use != ia_aiq_frame_use_still && mFlashStage == CAM_FLASH_STAGE_NONE) {
            mLastSensorExposure.coarse_integration_time = new_ae_results->exposures[0].sensor_exposure->coarse_integration_time;
            mLastSensorExposure.fine_integration_time = new_ae_results->exposures[0].sensor_exposure->fine_integration_time;
            mLastSensorExposure.analog_gain_code_global = new_ae_results->exposures[0].sensor_exposure->analog_gain_code_global;
            mLastSensorExposure.digital_gain_global = new_ae_results->exposures[0].sensor_exposure->digital_gain_global;
        }

        // save the last ae result from 3A
        mLastExposure.exposure_time_us = new_ae_results->exposures[0].exposure->exposure_time_us;
        mLastExposure.analog_gain = new_ae_results->exposures[0].exposure->analog_gain;

        for (unsigned int i = 0; i < new_ae_results->num_exposures; i++) {
            mSensorCI->storeAutoExposure((i==0),
                             new_ae_results->exposures[i].sensor_exposure->coarse_integration_time,
                             new_ae_results->exposures[i].sensor_exposure->fine_integration_time,
                             new_ae_results->exposures[i].sensor_exposure->analog_gain_code_global,
                             new_ae_results->exposures[i].sensor_exposure->digital_gain_global);
        }

        // always apply when invalidated
        bool apply_exposure = invalidated;
        bool apply_flash_intensity = invalidated;
        bool update_results_history = (mAeInputParameters.frame_use != ia_aiq_frame_use_still);

        LOG3A("AEC %s", new_ae_results->exposures[0].converged ? "converged":"converging");
        /* Workaround:
         * The result shows converged in two frames after stream on
         * But it is a fake result and should be ignored.
         * Waiting for 3A team to fix it.
         */
        if (mStatCount >= MIN_FRAMES_AE_CONVERGED)
            mAeConverged = new_ae_results->exposures[0].converged;

        // Fill history with these values when invalidated
        if (invalidated && update_results_history)
        {
            /*
             * Fill AE results history with first AE results because there is no AE delay in the beginning OR
             * Fill AE results history with first AE results because there is no AE delay after mode change (handled with 'first_run' flag - see switchModeAndRate()) OR
             * Fill AE results history with flash AE results because flash process skips partially illuminated frames removing the AE delay.
             */
            for (unsigned int i = 0; i < mAeState.stored_results->getDepth(); i++)
                storeAeResults(new_ae_results);
        }

        /* apply frameline for fps control */
        if (mFpsSetting & FPS_SETTING_FIXED) {
            int framelines = new_ae_results->exposures->sensor_exposure->frame_length_lines;
            ret |= mSensorCI->setFramelines(framelines);
        }

        // when there's previous values, compare and apply only if changed
        if (mAeState.ae_results && !invalidated)
        {
            // Compare sensor exposure parameters instead of generic exposure parameters to take into account mode changes when exposure time doesn't change but sensor parameters do change.
            apply_exposure = needApplyExposure(new_ae_results);
            apply_flash_intensity = needApplyFlashIntensity(new_ae_results);
        }

        /* Apply Sensor settings */
        if (apply_exposure && apply_exposure && new_ae_results->exposures) {
            // After preflash, AE is much more lower and preview will be a little dark.
            // Now set last AE value saved before flash was fired to driver to workaround this issue.
            if (mAeInputParameters.frame_use == ia_aiq_frame_use_still && mFlashStage == CAM_FLASH_STAGE_MAIN) {
                ret |= applyExposure(&mLastSensorExposure);
            } else {
                ret |= applyExposure(new_ae_results->exposures[0].sensor_exposure);
            }
        }

        /* Apply Flash settings
         * TODO: add support for multiple leds
         * */
        if (apply_flash_intensity)
            ret |= mFlashCI->setFlashIntensity((int)(new_ae_results->flashes[0]).power_prc);

        if (update_results_history) {
            mAeState.ae_results = storeAeResults(new_ae_results);
        } else {
            // Delaying of results is not needed in still mode, since upper
            // layer sequences are handling exposure delays with skipping.
            // Update the head and keep the rest for restoring in mode switch
            mAeState.ae_results = storeAeResults(new_ae_results, 0);
        }

        if (mAeState.ae_results == NULL) {
           LOGE("Failed to store AE Results");
           mAeState.ae_results = new_ae_results;
        }
    } else {
        LOG3A("%s: no new results", __FUNCTION__);
    }
    return ret;
}

/**
* Helper method to check if the exposure values have changed between the previous setting
* and the current one. In case a change is detected in the exposure value, the boolean to
* apply a new value will be set.
*
* \see #runAeMain()
* \return boolean to either apply a new exposure value or not
*/

bool Aiq3A::needApplyExposure(ia_aiq_ae_results *new_ae_results)
{
    bool apply_exposure;

    ia_aiq_exposure_sensor_parameters *prev_exposure = mAeState.ae_results->exposures[0].sensor_exposure;
    ia_aiq_exposure_sensor_parameters *new_exposure = new_ae_results->exposures[0].sensor_exposure;

    apply_exposure = prev_exposure->coarse_integration_time != new_exposure->coarse_integration_time;
    apply_exposure |= prev_exposure->fine_integration_time != new_exposure->fine_integration_time;
    apply_exposure |= prev_exposure->digital_gain_global != new_exposure->digital_gain_global;
    apply_exposure |= prev_exposure->analog_gain_code_global != new_exposure->analog_gain_code_global;

    return apply_exposure;
}

/**
* Helper method to check if the flash intensity value has changed between the previous setting
* and the current one. In case a change is detected in the flash intensity, the boolean to apply
* a new value will be set.
*
* \see #runAeMain()
* \return boolean to either apply a new flash intensity value or not
*/

bool Aiq3A::needApplyFlashIntensity(ia_aiq_ae_results *new_ae_results) {

    bool apply_flash_intensity;

    ia_aiq_flash_parameters *prev_flash = &mAeState.ae_results->flashes[0];
    ia_aiq_flash_parameters *new_flash = &new_ae_results->flashes[0];
    apply_flash_intensity = prev_flash->power_prc != new_flash->power_prc;

    return apply_flash_intensity;
}

void Aiq3A::resetAWBParams()
{
    LOG3A("@%s", __FUNCTION__);
    mAwbInputParameters.frame_use = m3aState.frame_use;
    mAwbInputParameters.scene_mode = ia_aiq_awb_operation_mode_auto;
    mAwbInputParameters.manual_cct_range = NULL;
    mAwbInputParameters.manual_white_coordinate = NULL;
}

void Aiq3A::runAwbMain()
{
    LOG3A("@%s", __FUNCTION__);

    if (mAwbLocked || mAwbDisabled)
        return;
    ia_err ret = ia_err_none;

    if(m3aState.ia_aiq_handle)
    {
        //mAwbInputParameters.scene_mode = ia_aiq_awb_operation_mode_auto;
        LOG3A("before ia_aiq_awb_run() param-- frame_use:%d scene_mode:%d", mAwbInputParameters.frame_use, mAwbInputParameters.scene_mode);
        if (mIsAwbZeroHistoryMode) {
            mAwbInputParameters.frame_use = ia_aiq_frame_use_still;
        } else {
            mAwbInputParameters.frame_use = m3aState.frame_use;
        }
        ret = ia_aiq_awb_run(m3aState.ia_aiq_handle, &mAwbInputParameters, &mAwbResults);
        LOG3A("@%s result: %d", __FUNCTION__, ret);
        mAwbConverged = (mAwbResults->distance_from_convergence < 0.001);
    }
}

void Aiq3A::resetGBCEParams()
{
    mGBCEDefault = false;
    mGBCEResults = NULL;
}

status_t Aiq3A::runGBCEMain()
{
    LOG3A("@%s", __FUNCTION__);
    if (m3aState.ia_aiq_handle) {
        ia_aiq_gbce_input_params gbce_input_params;
        if (mGBCEDefault)
        {
            gbce_input_params.gbce_level = ia_aiq_gbce_level_bypass;
        } else
        {
            gbce_input_params.gbce_level = ia_aiq_gbce_level_use_tuning;
        }
        gbce_input_params.frame_use = m3aState.frame_use;
        getEv(&gbce_input_params.ev_shift);
        ia_err err = ia_aiq_gbce_run(m3aState.ia_aiq_handle, &gbce_input_params, &mGBCEResults);
        if(err == ia_err_none)
            LOG3A("@%s success", __FUNCTION__);
    } else {
        mGBCEResults = NULL;
    }
    return NO_ERROR;
}

status_t Aiq3A::getGBCEResults(ia_aiq_gbce_results *gbce_results)
{
    LOG3A("@%s", __FUNCTION__);

    if (!gbce_results)
        return BAD_VALUE;

    if (mGBCEResults)
        *gbce_results = *mGBCEResults;
    else
        return INVALID_OPERATION;

    return NO_ERROR;
}

void Aiq3A::resetDSDParams()
{
    m3aState.dsd_enabled = false;
}

status_t Aiq3A::runDSDMain()
{
    LOG3A("@%s", __FUNCTION__);
    if (m3aState.ia_aiq_handle && m3aState.dsd_enabled)
    {
        mDSDInputParameters.af_results = mAfState.af_results;
        mDSDInputParameters.scene_modes_selection = (ia_aiq_scene_mode)
            (ia_aiq_scene_mode_close_up_portrait |
            ia_aiq_scene_mode_portrait |
            ia_aiq_scene_mode_lowlight_portrait |
            ia_aiq_scene_mode_low_light |
            ia_aiq_scene_mode_action |
            ia_aiq_scene_mode_backlight |
            ia_aiq_scene_mode_landscape |
            ia_aiq_scene_mode_firework |
            ia_aiq_scene_mode_lowlight_action);

        ia_err ret = ia_aiq_dsd_run(m3aState.ia_aiq_handle, &mDSDInputParameters, &mDetectedSceneMode);
        if(ret == ia_err_none)
            LOG3A("@%s success, detected scene mode: %d", __FUNCTION__, mDetectedSceneMode);
    }
    return NO_ERROR;
}

status_t Aiq3A::run3aMain()
{
    LOG3A("@%s", __FUNCTION__);
    status_t ret = NO_ERROR;

    mBracketingRunning = false;

    ret |= runAfMain();
// if no DSD enable, should disable that
    ret |= runDSDMain();
    ret |= runAeMain();

    runAwbMain();

    if(mAeMode != CAM_AE_MODE_MANUAL)
        ret |= runGBCEMain();
    else
        mGBCEResults = NULL;

    // get AIC result and apply into ISP
    ret |= runAICMain();

    return ret;
}

status_t Aiq3A::runAICMain()
{
    LOG3A("@%s", __FUNCTION__);

    status_t ret = NO_ERROR;

    if (m3aState.ia_aiq_handle) {
        ia_aiq_pa_input_params pa_input_params;

        pa_input_params.frame_use = m3aState.frame_use;
        mIspInputParams.frame_use = m3aState.frame_use;

        pa_input_params.awb_results = NULL;
        pa_input_params.exposure_params = NULL;
        mIspInputParams.awb_results = NULL;
        pa_input_params.color_gains = NULL;
        mIspInputParams.exposure_results = NULL;

        if (mAeState.ae_results) {
            mIspInputParams.exposure_results = mAeState.ae_results->exposures[0].exposure;
            pa_input_params.exposure_params = mAeState.ae_results->exposures[0].exposure;
        }

        if (mAwbResults) {
            LOG3A("awb factor:%f", mAwbResults->accurate_b_per_g);
        }
        pa_input_params.awb_results = mAwbResults;
        mIspInputParams.awb_results = mAwbResults;

        if (mGBCEResults) {
            LOG3A("gbce :%d", mGBCEResults->ctc_gains_lut_size);
        }

        if (mGBCEResults) {
            mIspInputParams.gbce_results = *mGBCEResults;
            mIspInputParams.gbce_results_valid = true;
        } else {
            mIspInputParams.gbce_results_valid = false;
        }

        pa_input_params.sensor_frame_params = &m3aState.sensor_frame_params;
        mIspInputParams.sensor_frame_params = &m3aState.sensor_frame_params;
        LOG3A("@%s  2 sensor native width %d", __FUNCTION__, pa_input_params.sensor_frame_params->cropped_image_width);


        // Calculate ISP independent ISP parameters (e.g. LSC table, color correction matrix)
        ia_aiq_pa_results *pa_results;
        ret = ia_aiq_pa_run(m3aState.ia_aiq_handle, &pa_input_params, &pa_results);
        LOG3A("@%s  ia_aiq_pa_run :%d", __FUNCTION__, ret);
        mPaResults = pa_results;

        mIspInputParams.pa_results = pa_results;
        mIspInputParams.manual_brightness = 0;
        mIspInputParams.manual_contrast = 0;
        mIspInputParams.manual_hue = 0;
        mIspInputParams.manual_saturation = 0;
        mIspInputParams.isp_config_id = 0;
        mIspInputParams.noiseReduction = mNoiseReduction;
        mIspInputParams.edgeEnhancement = mEdgeEnhancement;
        mIspInputParams.lensShading = mLensShading;
        mIspInputParams.edgeStrength = mEdgeStrength;

        ret |= mISP->setIspParameters(&mIspInputParams);
    }

    return ret;
}

/*! \fn  getAEExpCfg
 * \brief Get sensor's configuration for AE
 * exp_time: Preview exposure time
 * aperture: Aperture
 * Get the AEC outputs (which we hope are used by the sensor)
 * @param exp_time - exposure time
 * @param aperture - aperture
 * @param aec_apex_Tv - Shutter speed
 * @param aec_apex_Sv - Sensitivity
 * @param aec_apex_Av - Aperture
 * @param digital_gain - digital_gain
 */
void Aiq3A::getAeExpCfg(int *exp_time,
                          short unsigned int *aperture_num,
                          short unsigned int *aperture_denum,
                     int *aec_apex_Tv, int *aec_apex_Sv, int *aec_apex_Av,
                     float *digital_gain,
                     float *total_gain)
{
    LOG3A("@%s", __FUNCTION__);

    mSensorCI->getExposureTime(exp_time);
    mSensorCI->getFNumber(aperture_num, aperture_denum);
    ia_aiq_ae_results *latest_ae_results = NULL;
    if (mBracketingRunning && mAEBracketingResult) {
        latest_ae_results = mAEBracketingResult;
    } else {
        latest_ae_results = mAeState.ae_results;
    }

    if (latest_ae_results != NULL) {
        *digital_gain = (latest_ae_results->exposures[0].exposure)->digital_gain;
        *aec_apex_Tv = -1.0 * (log10((double)(latest_ae_results->exposures[0].exposure)->exposure_time_us/1000000) / log10(2.0)) * 65536;
        *aec_apex_Av = log10(pow((latest_ae_results->exposures[0].exposure)->aperture_fn, 2))/log10(2.0) * 65536;
        *aec_apex_Sv = log10(pow(2.0, -7.0/4.0) * (latest_ae_results->exposures[0].exposure)->iso) / log10(2.0) * 65536;
        if (*digital_gain > float(0)) {
            *total_gain = latest_ae_results->exposures[0].exposure->analog_gain * *digital_gain;
        }
        else {
            *total_gain = latest_ae_results->exposures[0].exposure->analog_gain;
        }

        LOG3A("total_gain: %f  digital gain: %f", *total_gain, *digital_gain);
    }
}

status_t Aiq3A::getParameters(ispInputParameters * ispParams,
                                    ia_aiq_exposure_parameters * exposure)
{
    ia_aiq_ae_results *stored_results = peekAeStoredResults(0);
    if (stored_results && stored_results->exposures[0].exposure && exposure)
        memcpy(exposure, stored_results->exposures[0].exposure, sizeof(ia_aiq_exposure_parameters));
    if (ispParams)
        memcpy(ispParams, &mIspInputParams, sizeof(ispInputParameters));
    return NO_ERROR;
}

status_t Aiq3A::convertError(ia_err iaErr)
{
    switch (iaErr) {
    case ia_err_none:
        return NO_ERROR;
    case ia_err_general:
        return UNKNOWN_ERROR;
    case ia_err_nomemory:
        return NO_MEMORY;
    case ia_err_data:
        return BAD_VALUE;
    case ia_err_internal:
        return INVALID_OPERATION;
    case ia_err_argument:
        return BAD_VALUE;
    default:
        return UNKNOWN_ERROR;
    }
}

int32_t Aiq3A::mapUIISO2RealISO (int32_t iso)
{
    LOG2("@%s:", __FUNCTION__);
    int pIso;
    int rIso;
    const IPU2CameraCapInfo * capInfo;
    capInfo = getIPU2CameraCapInfo(mCameraId);
    if (!capInfo) {
        LOGE("%s: get capInfo failed, return 100\n", __FUNCTION__);
        return 100;
    }
    pIso = capInfo->getISOAnalogGain1();
    rIso = mBaseIso;
    //TODO: need to make sure that rIso from CPF is not 0.
    if (rIso == 0){
        LOGW("%s: rIso from CPF is 0, change value to 1.", __FUNCTION__);
        rIso = 1;
    }
    return iso*rIso/pIso;
}

int32_t Aiq3A::mapRealISO2UIISO (int32_t iso)
{
    LOG2("@%s:", __FUNCTION__);
    int pIso;
    int rIso;
    const IPU2CameraCapInfo * capInfo;
    capInfo = getIPU2CameraCapInfo(mCameraId);
    if (!capInfo) {
        LOGE("%s: get capInfo failed, return 100\n", __FUNCTION__);
        return 100;
    }
    pIso = capInfo->getISOAnalogGain1();
    rIso = mBaseIso;
    //TODO: need to make sure that rIso from CPF is not 0.
    if (rIso == 0){
        LOGW("%s: rIso from CPF is 0, change value to 1.", __FUNCTION__);
        rIso = 1;
    }
    return iso*pIso/rIso;
}

} // namespace camera2
} //  namespace android
