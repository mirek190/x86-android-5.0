/*
 * Copyright 2012 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file cameranvm.h
 * \brief Definitions of NVM creator functions.
*/

#ifndef CAMERANVM_H_
#define CAMERANVM_H_

#include "ia_types.h"
#include "ia_nvm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief NVM parsing status codes.
 */
typedef enum
{
    nvm_error_none,            /*!< No error. */
    nvm_error_internal,        /*!< Parser internal failure (not enough memory). */
    nvm_error_no_data,         /*!< NULL pointer as input or not enough input data. */
    nvm_error_af,              /*!< Error parsing AF parameters. */
    nvm_error_lsc,             /*!< Error parsing AF parameters. (lsc dimensions, lsc data) */
    nvm_error_awb,             /*!< Error parsing AWB parameters (sensitivities, n_lights). */
    nvm_error_crc,             /*!< CRC check error. */
    nvm_error_not_implemented, /*!< Parser for given data type has not been implemented. */
    nvm_error_version,         /*!< invalid version. */
    nvm_status_intel_format    /*!< The NVM data is in Intel default format. */
} nvm_error;

typedef struct
{
    int lsc_color_temperature;             /*!< color temperature of shading table calibrate. as ct/100 */
    uint16_t *lsc_tables[IA_NVM_NUM_CHANNELS]; /*!< LSC table for Ch1, Ch2, Ch3 and Ch4. */
} nvm_lsc;

typedef struct
{
    uint8_t lsc_width;                         /*!< Width of LSC tables */
    uint8_t lsc_height;                        /*!< Height of LSC tables. */
    nvm_lsc *lsc[2];                           /*!< LSC tables 1. */
} nvm_data;

/*!
 * \brief Creates Intel specified NVM data from various NVM data.
 * Detection of NVM data is done based on camera ID string. Created NVM
 * data buffer must be deleted with function cameranvm_delete.
 * \param[in]     camera_name           String identifying NVM format from various cameras.
 * \param[in]     input_nvm_data        NVM data from camera module.
 * \param[in]     input_nvm_motor_data  NVM data from camera lens motor EEPROM.
 * \param[out]    output_nvm_data       NVM data converted into Intel specified format.
 * \return                              Error code from NVM creation.
 */
nvm_error
cameranvm_create(const char *camera_name,
                 const ia_binary_data *input_nvm_data,
                 const ia_binary_data *input_motor_nvm_data,
                 ia_binary_data **output_nvm_data);


/*!
 * \brief Creates Intel specified NVM data from NVM data in google format.
 * Detection of NVM data is done based on camera ID string. NVM data in
 * google format should be converted into Intel format, and Created NVM
 * data buffer must be deleted with function cameranvm_delete.
 * \param[in]     lsc  lsc tables in google format.
 * \param[out]    output_nvm_data       NVM data converted into Intel specified format.
 * \return                              Error code from NVM creation.
 */
nvm_error
cameranvm_convert(const nvm_data *input_nvm_data,
                 ia_binary_data **output_nvm_data);

/*!
 * \brief Deletes NVM data buffer created with function cameranvm_create.
 * This function only frees the allocated buffer and clears the parameters
 * in the given structure.
 * \param[in] nvm_data  Previously created NVM data buffer.
 */
void
cameranvm_delete(ia_binary_data *nvm_data);

#ifdef __cplusplus
}
#endif

#endif /* CAMERANVM_H_ */
