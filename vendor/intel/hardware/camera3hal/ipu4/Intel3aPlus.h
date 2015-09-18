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

#ifndef CAMERA2600_INTEL3APLUS_H_
#define CAMERA2600_INTEL3APLUS_H_

#include <ia_aiq.h>
#include "CameraAreas.h"

namespace android {
namespace camera2 {

static const unsigned int NUM_EXPOSURES = 1;   /*!> Number of frames AIQ algorithm
                                                    provides output for */
static const unsigned int NUM_FLASH_LEDS = 1;   /*!> Number of leds AEC algorithm
                                                    provides output for */

/**
 * \struct AiqInputParams
 * The private structs are part of AE, AF and AWB input parameters.
 * They need to separately be introduced to store the contents for
 * the corresponding pointers.
 */
struct AiqInputParams {
    void init();

    ia_aiq_ae_input_params  aeInputParams;
    ia_aiq_af_input_params  afParams;
    ia_aiq_awb_input_params awbParams;

private:
    /*!< ia_aiq_ae_input_params pointer contents */
    ia_aiq_exposure_sensor_descriptor sensorDescriptor;
    ia_rectangle exposureWindow;
    ia_coordinate exposureCoordinate;
    ia_aiq_ae_features aeFeatures;
    ia_aiq_ae_manual_limits aeManualLimits;

    /*!< ia_aiq_af_input_params pointer contents */
    ia_aiq_rect focusRect;

    /*!< ia_aiq_awb_input_params pointer contents */
    ia_aiq_awb_manual_cct_range manualCctRange;
    ia_coordinate manualWhiteCoordinate;
};

/**
 * \struct AiqResults
 * The private structs are part of AE and AF results.
 * They need to separately be introduced to store the contents for
 * the corresponding pointers.
 */
struct AiqResults {
    void init();
    ia_aiq_ae_results aeResults;
    ia_aiq_af_results afResults;

private:
    /*!< ia_aiq_ae_results pointer contents */
    ia_aiq_ae_exposure_result exposureResults;
    ia_aiq_hist_weight_grid   weightGrid;
    ia_aiq_flash_parameters   flashes[NUM_FLASH_LEDS];

    /*!< ia_aiq_ae_exposure_result pointer contents */
    ia_aiq_exposure_parameters        genericExposure;
    ia_aiq_exposure_sensor_parameters sensorExposure;
};
/**
 * \class Intel3aPlus
 *
 * Intel3aPlus is an interface to Intel 3A Library (known as libia_aiq).
 * The purpose of this class is to
 * - convert Google specific metadata to ia_aiq input parameters
 * - convert ia_aiq output parameters to Google specific metadata
 * - call libia_aiq functions such as run 3A functions
 *
 */
class Intel3aPlus {
public:
    Intel3aPlus();
    status_t initAIQ();
    void deinitAIQ();
    status_t getCpfData(ia_binary_data *cpfData);
    status_t fillAeInputParams(const CameraMetadata *settings,
                               AiqInputParams &aiqInputParams,
                               ia_aiq_exposure_sensor_descriptor *sensorDescriptor,
                               bool aeOn);

    status_t setStatistics(ia_aiq_statistics_input_params *ispStatistics);

    status_t runAe(ia_aiq_ae_input_params *aeInputParams,
                   ia_aiq_ae_results *aeResults);

private:
    status_t convertError(ia_err iaErr);
    void convertFromAndroidToIaCoordinates(const CameraWindow &srcWindow,
                                           CameraWindow &toWindow);

// private members
private:
    ia_aiq *mIaAiqHandle;
    ia_mkn  *mMkn;
}; //  class Intel3aPlus


}; //  namespace camera2
}; //  namespace android
#endif  //  CAMERA2600_INTEL3APLUS_H_
