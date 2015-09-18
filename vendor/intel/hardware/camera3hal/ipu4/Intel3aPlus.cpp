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

#define LOG_TAG "Camera_Intel3aPlus"

#include <utils/Errors.h>
#include <camera/CameraMetadata.h>

#include "Intel3aPlus.h"
#include "PlatformData.h"
#include "LogHelper.h"
#include "ia_mkn_encoder.h"
#include "ia_cmc_parser.h"
#include "ia_coordinate.h"

namespace android {
namespace camera2 {

#define MAX_STATISTICS_WIDTH 150
#define MAX_STATISTICS_HEIGHT 150

void AiqInputParams::init()
{
    CLEAR(aeInputParams.frame_use);
    CLEAR(aeInputParams.flash_mode);
    CLEAR(aeInputParams.operation_mode);
    CLEAR(aeInputParams.metering_mode);
    CLEAR(aeInputParams.priority_mode);
    CLEAR(aeInputParams.flicker_reduction_mode);
    CLEAR(sensorDescriptor);
    CLEAR(exposureWindow);
    CLEAR(exposureCoordinate);
    CLEAR(aeFeatures);
    CLEAR(aeManualLimits);
    CLEAR(focusRect);
    CLEAR(manualCctRange);
    CLEAR(manualWhiteCoordinate);

    aeInputParams.sensor_descriptor = &sensorDescriptor;
    aeInputParams.exposure_window = &exposureWindow;
    aeInputParams.exposure_coordinate = &exposureCoordinate;
    aeInputParams.aec_features = &aeFeatures;
    aeInputParams.manual_limits = &aeManualLimits;
}

void AiqResults::init()
{
    CLEAR(aeResults.num_exposures);
    CLEAR(aeResults.lux_level_estimate);
    CLEAR(aeResults.multiframe);
    CLEAR(aeResults.flicker_reduction_mode);
    CLEAR(exposureResults);
    CLEAR(weightGrid);
    CLEAR(flashes);
    CLEAR(genericExposure);
    CLEAR(sensorExposure);

    aeResults.exposures = &exposureResults;
    aeResults.weight_grid = &weightGrid;
    aeResults.flashes = flashes;
    aeResults.exposures->exposure = &genericExposure;
    aeResults.exposures->sensor_exposure = &sensorExposure;
}

Intel3aPlus::Intel3aPlus()
{
    LOG1("@%s", __FUNCTION__);
}

status_t Intel3aPlus::initAIQ()
{
    LOG1("@%s", __FUNCTION__);

    status_t status = NO_ERROR;
    ia_err iaErr = ia_err_none;
    ia_binary_data cpfData;

    status = getCpfData(&cpfData);

    // the camera should start even if the CPF data is missing
    // TODO: figure out where the responsibility of handling the error
    //       should be. For now the error is ignored.
    if (status != NO_ERROR) {
        LOGE("Error getting CPF data");
    }

    mMkn = ia_mkn_init(ia_mkn_cfg_compression, 32000, 100000);
    if (mMkn == NULL) {
        LOGE("Error in initing makernote");
        status = UNKNOWN_ERROR;
    }

    iaErr = ia_mkn_enable(mMkn, true);
    if (iaErr != ia_err_none) {
        status = convertError(iaErr);
        LOGE("Error in enabling makernote: %d", status);
    }

    ia_cmc_t *cmc = ia_cmc_parser_init((ia_binary_data*)&(cpfData));

    mIaAiqHandle = ia_aiq_init((ia_binary_data*)&(cpfData),
                                NULL,  // TODO: nvm when available,
                                NULL,
                                MAX_STATISTICS_WIDTH,
                                MAX_STATISTICS_HEIGHT,
                                NUM_EXPOSURES,
                                cmc,
                                mMkn);

    // TODO: initIaIspAdaptor

    ia_cmc_parser_deinit(cmc);

    if (!mIaAiqHandle) {
        LOGE("Error in IA AIQ init");
        status = UNKNOWN_ERROR;
    }

    // ToDo: ia_face_state stuff?

    return status;
}

void Intel3aPlus::deinitAIQ()
{
    LOG1("@%s", __FUNCTION__);

    ia_aiq_deinit(mIaAiqHandle);
    ia_mkn_uninit(mMkn);
}

status_t Intel3aPlus::getCpfData(ia_binary_data *cpfData)
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

/**
 * Converts ia_aiq errors into generic status_t
 */
status_t Intel3aPlus::convertError(ia_err iaErr)
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

/**
 * Converts AE related metadata into ia_aiq_ae_input_params
 * \param settings [IN] request settings in Google format
 * \param aeInputParams [OUT] AE input parameters in IA AIQ format
 */
status_t Intel3aPlus::fillAeInputParams(const CameraMetadata *settings,
                                        AiqInputParams &aiqInputParams,
                                        ia_aiq_exposure_sensor_descriptor *sensorDescriptor,
                                        bool aeOn)
{
    LOG2("@%s", __FUNCTION__);

    camera_metadata_ro_entry entry;

    // num_exposures
    aiqInputParams.aeInputParams.num_exposures = 1;

    // ******** frame_use
    entry = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
    if (entry.count == 1) {
        uint8_t captureIntent = entry.data.u8[0];
        switch (captureIntent) {
            case ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM:
                aiqInputParams.aeInputParams.frame_use = ia_aiq_frame_use_preview;
                break;
            case ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW:
                aiqInputParams.aeInputParams.frame_use = ia_aiq_frame_use_preview;
                break;
            case ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE:
                aiqInputParams.aeInputParams.frame_use = ia_aiq_frame_use_still;
                break;
            case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD:
                aiqInputParams.aeInputParams.frame_use = ia_aiq_frame_use_video;
                break;
            case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT:
                aiqInputParams.aeInputParams.frame_use = ia_aiq_frame_use_video;
                break;
            case ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
                aiqInputParams.aeInputParams.frame_use = ia_aiq_frame_use_continuous;
                break;
            default:
                LOGE("ERROR @%s: Unknow frame use %d", __FUNCTION__, captureIntent);
                return BAD_VALUE;
        }
    }

    // ******** aec_features
    aiqInputParams.aeInputParams.aec_features->backlight_compensation =
                                        ia_aiq_ae_feature_setting_disabled;
    aiqInputParams.aeInputParams.aec_features->backlight_compensation =
                                        ia_aiq_ae_feature_setting_disabled;
    aiqInputParams.aeInputParams.aec_features->face_utilization =
                                        ia_aiq_ae_feature_setting_disabled;
    aiqInputParams.aeInputParams.aec_features->fill_in_flash =
                                        ia_aiq_ae_feature_setting_disabled;
    aiqInputParams.aeInputParams.aec_features->motion_blur_control =
                                        ia_aiq_ae_feature_setting_disabled;
    aiqInputParams.aeInputParams.aec_features->red_eye_reduction_flash =
                                        ia_aiq_ae_feature_setting_disabled;

    // ******** flash_mode
    entry = settings->find(ANDROID_CONTROL_AE_MODE);
    if (entry.count == 1) {
        uint8_t aeMode = entry.data.u8[0];

        switch (aeMode) {
            case ANDROID_CONTROL_AE_MODE_OFF:
                aeOn = false;
                aiqInputParams.aeInputParams.flash_mode = ia_aiq_flash_mode_off;
                break;
            case ANDROID_CONTROL_AE_MODE_ON:
                aiqInputParams.aeInputParams.flash_mode = ia_aiq_flash_mode_auto;
                break;
            case ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH:
                aiqInputParams.aeInputParams.flash_mode = ia_aiq_flash_mode_auto;
                break;
            case ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH:
                aiqInputParams.aeInputParams.flash_mode = ia_aiq_flash_mode_on;
                break;
            case ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE:
                aiqInputParams.aeInputParams.aec_features->red_eye_reduction_flash =
                                             ia_aiq_ae_feature_setting_enabled;
                break;
            default:
                LOGE("ERROR @%s: Unknow AE mode %d", __FUNCTION__, aeMode);
                return BAD_VALUE;
        }

        // the flash modes are effective only when AE mode is ON or OFF
        if ((aeMode == ANDROID_CONTROL_AE_MODE_ON) ||
            (aeMode == ANDROID_CONTROL_AE_MODE_OFF)) {
            entry = settings->find(ANDROID_FLASH_MODE);
            if (entry.count == 1) {
                uint8_t flashMode = entry.data.u8[0];

                switch (flashMode) {
                    case ANDROID_FLASH_MODE_OFF:
                        aiqInputParams.aeInputParams.flash_mode = ia_aiq_flash_mode_off;
                        break;
                    case ANDROID_FLASH_MODE_SINGLE:
                        aiqInputParams.aeInputParams.flash_mode = ia_aiq_flash_mode_auto;
                        break;
                    case ANDROID_FLASH_MODE_TORCH:
                        // AE doesn't need to know if torch is on or off
                        // ToDo: set flash to torch either here or somewhere else
                        break;
                    default:
                        LOGE("ERROR @%s: Unknow flash mode %d", __FUNCTION__, flashMode);
                        return BAD_VALUE;
                }
            }
        }
    }

    entry = settings->find(ANDROID_CONTROL_MODE);
    if (entry.count == 1) {
        uint8_t controlMode = entry.data.u8[0];
        //TODO FIX VALUES
        switch (controlMode) {
            case ANDROID_CONTROL_MODE_OFF:
                aiqInputParams.aeInputParams.operation_mode = ia_aiq_ae_operation_mode_automatic;
                break;
            case ANDROID_CONTROL_MODE_AUTO:
                aiqInputParams.aeInputParams.operation_mode = ia_aiq_ae_operation_mode_automatic;
                break;
            case ANDROID_CONTROL_MODE_USE_SCENE_MODE:
                aiqInputParams.aeInputParams.operation_mode = ia_aiq_ae_operation_mode_automatic;
                break;
             default:
                LOGE("ERROR @%s: Unknow control mode %d", __FUNCTION__, controlMode);
             return BAD_VALUE;
        }
    }

    // ******** operation_mode
    entry = settings->find(ANDROID_CONTROL_SCENE_MODE);
    if (entry.count == 1) {
        uint8_t sceneMode = entry.data.u8[0];

        switch (sceneMode) {
            case ANDROID_CONTROL_SCENE_MODE_DISABLED:
                aiqInputParams.aeInputParams.operation_mode = ia_aiq_ae_operation_mode_automatic;
                break;
            case ANDROID_CONTROL_SCENE_MODE_ACTION:
            case ANDROID_CONTROL_SCENE_MODE_SPORTS:
                aiqInputParams.aeInputParams.operation_mode = ia_aiq_ae_operation_mode_action;
                break;
            case ANDROID_CONTROL_SCENE_MODE_FIREWORKS:
                aiqInputParams.aeInputParams.operation_mode = ia_aiq_ae_operation_mode_fireworks;
                break;
            // ToDo: many of the metadata tags are not mapped and also many of the
            //       ia_aiq_ae_operation_modes are not used
        }
    }


    // ******** metering_mode
    // TODO: implement the metering mode. For now the metering mode is fixed
    // to whole frame
    aiqInputParams.aeInputParams.metering_mode = ia_aiq_ae_metering_mode_evaluative;

    // ******** priority_mode
    // TODO: check if there is something that can be mapped to the priority mode
    // maybe NIGHT_PORTRAIT to highlight for example?
    aiqInputParams.aeInputParams.priority_mode = ia_aiq_ae_priority_mode_normal;

    // ******** flicker_reduction_mode
    entry = settings->find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
    if (entry.count == 1) {
        uint8_t flickerMode = entry.data.u8[0];

        switch (flickerMode) {
            case ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF:
                aiqInputParams.aeInputParams.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_off;
                break;
            case ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ:
                aiqInputParams.aeInputParams.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_50hz;
                break;
            case ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ:
                aiqInputParams.aeInputParams.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_60hz;
                break;
            case ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO:
                aiqInputParams.aeInputParams.flicker_reduction_mode = ia_aiq_ae_flicker_reduction_auto;
                break;
            default:
                LOGE("ERROR @%s: Unknow flicker mode %d", __FUNCTION__, flickerMode);
                return BAD_VALUE;
        }
    }

    // ******** sensor_descriptor
    if (sensorDescriptor == NULL) {
        LOGE("%s: sensorDescriptor is NULL!", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    aiqInputParams.aeInputParams.sensor_descriptor->pixel_clock_freq_mhz =
                                        sensorDescriptor->pixel_clock_freq_mhz;
    aiqInputParams.aeInputParams.sensor_descriptor->pixel_periods_per_line =
                                        sensorDescriptor->pixel_periods_per_line;
    aiqInputParams.aeInputParams.sensor_descriptor->line_periods_per_field =
                                        sensorDescriptor->line_periods_per_field;
    aiqInputParams.aeInputParams.sensor_descriptor->line_periods_vertical_blanking =
                                        sensorDescriptor->line_periods_vertical_blanking;
    aiqInputParams.aeInputParams.sensor_descriptor->fine_integration_time_min =
                                        sensorDescriptor->fine_integration_time_min;
    aiqInputParams.aeInputParams.sensor_descriptor->fine_integration_time_max_margin =
                                        sensorDescriptor->fine_integration_time_max_margin;
    aiqInputParams.aeInputParams.sensor_descriptor->coarse_integration_time_min =
                                        sensorDescriptor->coarse_integration_time_min;
    aiqInputParams.aeInputParams.sensor_descriptor->coarse_integration_time_max_margin =
                                        sensorDescriptor->coarse_integration_time_max_margin;

    // ******** exposure_window
    entry = settings->find(ANDROID_CONTROL_AE_REGIONS);
    if (entry.count == 5) {
        CameraWindow src;
        CameraWindow dst;
        src.x_left   = entry.data.i32[0];
        src.y_top    = entry.data.i32[1];
        src.x_right  = entry.data.i32[2];
        src.y_bottom = entry.data.i32[3];
        dst.weight   = entry.data.i32[4];  // weight is not converted yet

        convertFromAndroidToIaCoordinates(src, dst);

        aiqInputParams.aeInputParams.exposure_window->left = dst.x_left;
        aiqInputParams.aeInputParams.exposure_window->top = dst.y_top;
        aiqInputParams.aeInputParams.exposure_window->right = dst.x_right;
        aiqInputParams.aeInputParams.exposure_window->bottom = dst.y_bottom;
    }

    // ******** exposure_coordinate
    // TODO: set when needed
    aiqInputParams.aeInputParams.exposure_coordinate = NULL;

    // ******** manual_exposure_time_us

    // TODO: Since we don't get ANDROID_CONTROL_AE_MODE_OFF from the app
    // and currently the only AE that works is manual, the aeOn value needs to
    // be forced to false. Once the app gives ANDROID_CONTROL_AE_MODE_OFF
    // or automatic AE is tried out, the following line needs to be removed.
    aeOn = false;

    if (!aeOn) {
        // ******** ev_shift
       entry = settings->find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        if (entry.count == 1) {
            int32_t evCompensation = entry.data.i32[0];
            float step = 1 / 3.0f;  // TODO get from platformdata or cpf
            aiqInputParams.aeInputParams.ev_shift = evCompensation * step;
        }

        entry = settings->find(ANDROID_SENSOR_EXPOSURE_TIME);
        if (entry.count == 1) {
            uint64_t timeNs = (uint64_t) (entry.data.i64[0]/1000);
            aiqInputParams.aeInputParams.manual_exposure_time_us = timeNs;
        }

        // ******** manual_analog_gain
        aiqInputParams.aeInputParams.manual_analog_gain = -1;

        // ******** manual_iso
        entry = settings->find(ANDROID_SENSOR_SENSITIVITY);
        if (entry.count == 1) {
            int32_t iso = entry.data.i32[0];
            aiqInputParams.aeInputParams.manual_iso = iso;
        }
    } else {
        aiqInputParams.aeInputParams.ev_shift = 0;
        aiqInputParams.aeInputParams.manual_exposure_time_us = -1;
        aiqInputParams.aeInputParams.manual_analog_gain = -1;
        aiqInputParams.aeInputParams.manual_iso = -1;
    }

    // ******** manual_limits
    aiqInputParams.aeInputParams.manual_limits->manual_exposure_time_min = -1;
    aiqInputParams.aeInputParams.manual_limits->manual_exposure_time_max = -1;
    aiqInputParams.aeInputParams.manual_limits->manual_frame_time_us_min = -1;
    aiqInputParams.aeInputParams.manual_limits->manual_frame_time_us_max = -1;
    aiqInputParams.aeInputParams.manual_limits->manual_iso_min = -1;
    aiqInputParams.aeInputParams.manual_limits->manual_iso_max = -1;

    return NO_ERROR;
}

void Intel3aPlus::convertFromAndroidToIaCoordinates(const CameraWindow &srcWindow,
                                                    CameraWindow &toWindow)
{
    // TODO these magic values are from ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
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

status_t Intel3aPlus::setStatistics(ia_aiq_statistics_input_params *ispStatistics)
{
    LOG2("@%s", __FUNCTION__);

    status_t status = NO_ERROR;
    ia_err iaErr = ia_err_none;

    iaErr = ia_aiq_statistics_set(mIaAiqHandle,
                                  ispStatistics);
    status |= convertError(iaErr);

    if (status != NO_ERROR) {
        LOGE("Error setting statistics");
    }

    return status;
}

status_t Intel3aPlus::runAe(ia_aiq_ae_input_params *aeInputParams,
                                    ia_aiq_ae_results* aeResults)
{
    LOG2("@%s", __FUNCTION__);

    status_t status = NO_ERROR;
    ia_err iaErr = ia_err_none;
    ia_aiq_ae_results *new_ae_results;

    // ToDo: cases to be considered in 3ACU
    //          - invalidated (empty ae results)
    //          - AE locked
    //          - AF assist light case (set the statistics from before assist light)

    if (mIaAiqHandle) {
        if (aeInputParams != NULL) {
            LOG2("AEC manual_exposure_time_us: %d manual_analog_gain: %f manual_iso: %d",
                                            aeInputParams->manual_exposure_time_us,
                                            aeInputParams->manual_analog_gain,
                                            aeInputParams->manual_iso);
            LOG2("AEC frame_use: %d", aeInputParams->frame_use);
            if (aeInputParams->sensor_descriptor != NULL) {
                LOG2("AEC line_periods_per_field: %d",
                      aeInputParams->sensor_descriptor->line_periods_per_field);
            }
        }

        iaErr = ia_aiq_ae_run(mIaAiqHandle, aeInputParams, &new_ae_results);

        status |= convertError(iaErr);
        if (status != NO_ERROR) {
            LOGE("Error running AE");
        }
    } else {
        status = UNKNOWN_ERROR;
        LOGE("ia_aiq_handle does not exist.");
    }

    //storing results;
    if (new_ae_results != NULL)
        memcpy(aeResults, new_ae_results, sizeof(ia_aiq_ae_results));

    return status;
}
} //  namespace camera2
} //  namespace android
