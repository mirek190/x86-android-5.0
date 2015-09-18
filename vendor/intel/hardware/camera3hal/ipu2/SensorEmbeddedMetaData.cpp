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
#define LOG_TAG "Camera_SensorEmbeddedMetaData"

#include <utils/Errors.h>
#include "LogHelper.h"
#include "ia_cmc_parser.h"
#include "SensorEmbeddedMetaData.h"

#define MAX_SENSOR_METADATA_QUEUE_SIZE 64
#define MAX_EXP_ID 250

namespace android {
namespace camera2 {

SensorEmbeddedMetaData::SensorEmbeddedMetaData(HWControlGroup &hwcg,
                                               I3AControls *aaa)
            :mISP(hwcg.mIspCI)
            ,mLatestExpId(-1)
            ,mEmbeddedMetaDecoderHandler(NULL)
            ,mSensorMetaDataSupported(false)
            ,mSensorMetaDataConfigFlag(0)
            ,m3AControls(aaa)
{
    LOG2("@%s", __FUNCTION__);
    CLEAR(mEmbeddedDataBin);
    CLEAR(mSensorEmbeddedMetaData);
    CLEAR(mEmbeddedDataMode);
}

SensorEmbeddedMetaData::~SensorEmbeddedMetaData()
{
    LOG2("@%s", __FUNCTION__);
    deinitSensorEmbeddedMetaDataQueue();
    free(mSensorEmbeddedMetaData.data);
    free(mSensorEmbeddedMetaData.effective_width);
    ia_emd_decoder_deinit(mEmbeddedMetaDecoderHandler);
}

status_t SensorEmbeddedMetaData::init()
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    ia_binary_data cpfData;
    CLEAR(cpfData);
    if (PlatformData::AiqConfig) {
        cpfData.data = PlatformData::AiqConfig.ptr();
        cpfData.size = PlatformData::AiqConfig.size();
    }

    mEmbeddedMetaDecoderHandler = ia_emd_decoder_init(&cpfData);
    if (mEmbeddedMetaDecoderHandler == NULL)
        return UNKNOWN_ERROR;
    //get embedded metadata buffer size
    struct atomisp_parm isp_params;
    mISP->getIspParameters(&isp_params);
    int height = isp_params.metadata_config.metadata_height;
    int width = isp_params.metadata_config.metadata_stride;
    int size = height * width;

    if (size > 0) {
        mSensorEmbeddedMetaData.data = (void*) malloc (size);
        if (mSensorEmbeddedMetaData.data == NULL) {
            status = NO_MEMORY;
            goto errorFree;
        }

        mSensorEmbeddedMetaData.effective_width = (int32_t*) malloc (height * sizeof(int32_t));
        if (mSensorEmbeddedMetaData.effective_width == NULL) {
            status = NO_MEMORY;
            goto errorFree;
        }

        mEmbeddedDataBin.data = mSensorEmbeddedMetaData.data;
        mEmbeddedDataBin.size = size;
        status = initSensorEmbeddedMetaDataQueue();
        if (status == NO_ERROR)
            mSensorMetaDataSupported = true;

    } else {
        // sensor doesn't support sensor embedded metadata
        return UNKNOWN_ERROR;
    }

    return status;

errorFree:
    if (mSensorEmbeddedMetaData.data) {
        free(mSensorEmbeddedMetaData.data);
        mSensorEmbeddedMetaData.data = NULL;
    }
    if (mSensorEmbeddedMetaData.effective_width) {
        free(mSensorEmbeddedMetaData.effective_width);
        mSensorEmbeddedMetaData.effective_width = NULL;
    }
    return status;
}

void SensorEmbeddedMetaData::reset()
{
    mLatestExpId = 0;
}

status_t SensorEmbeddedMetaData::initSensorEmbeddedMetaDataQueue()
{
    LOG2("@%s", __FUNCTION__);

    status_t status = NO_ERROR;
    mSensorEmbeddedMetaDataStoredQueue.clear();
    mSensorEmbeddedMetaDataStoredQueue.setCapacity(MAX_SENSOR_METADATA_QUEUE_SIZE);
    decoded_sensor_metadata metadata;
    for(unsigned int i = 0; i < mSensorEmbeddedMetaDataStoredQueue.capacity(); i++) {
        CLEAR(metadata);
        metadata.sensor_units_p = (ia_aiq_exposure_sensor_parameters*) malloc (sizeof(ia_aiq_exposure_sensor_parameters));
        if (metadata.sensor_units_p == NULL) {
            status = NO_MEMORY;
            goto errorFree;
        }
        CLEAR(*metadata.sensor_units_p);
        metadata.generic_units_p = (ia_aiq_exposure_parameters*) malloc (sizeof(ia_aiq_exposure_parameters));
        if (metadata.generic_units_p == NULL) {
            status = NO_MEMORY;
            goto errorFree;
        }
        CLEAR(*metadata.generic_units_p);
        metadata.exp_id = 0;
        mSensorEmbeddedMetaDataStoredQueue.push_front(metadata);
    }
    return status;

errorFree:
    for(size_t i = 0; i < mSensorEmbeddedMetaDataStoredQueue.size(); i++) {
        if (mSensorEmbeddedMetaDataStoredQueue.itemAt(i).sensor_units_p) {
            free(metadata.sensor_units_p);
            metadata.sensor_units_p = NULL;
        }
        if (mSensorEmbeddedMetaDataStoredQueue.itemAt(i).generic_units_p) {
            free(metadata.generic_units_p);
            metadata.generic_units_p = NULL;
        }
    }
    return status;
}

void SensorEmbeddedMetaData::deinitSensorEmbeddedMetaDataQueue()
{
    LOG2("@%s", __FUNCTION__);
    for (Vector<decoded_sensor_metadata>::iterator it = mSensorEmbeddedMetaDataStoredQueue.begin(); it != mSensorEmbeddedMetaDataStoredQueue.end(); ++it) {
        if (it->sensor_units_p) {
            free(it->sensor_units_p);
            it->sensor_units_p = NULL;
        }

        if (it->generic_units_p) {
            free(it->generic_units_p);
            it->generic_units_p = NULL;
        }
    }
    mSensorEmbeddedMetaDataStoredQueue.clear();
}

/**
  * New sensor metadata available
  * the sensor metadata buffer is from ISP, after parsing by sensor metadata decoder,
  * the decoded results should be stored in Vector.
  */
status_t SensorEmbeddedMetaData::handleSensorEmbeddedMetaData(unsigned int &exp_id,
        uint8_t metadata_type)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    if (!mSensorMetaDataSupported)
        return UNKNOWN_ERROR;

    if (metadata_type != ATOMISP_MAIN_METADATA)
        return NO_ERROR;

    //deque the embedded metadata
    if (mISP && mSensorEmbeddedMetaData.data && mSensorEmbeddedMetaData.effective_width) {
        int ret = mISP->getSensorEmbeddedMetaData(&mSensorEmbeddedMetaData, metadata_type);
        if (ret == 0) {
            status = NO_ERROR;
            mEmbeddedDataMode.exp_id = mSensorEmbeddedMetaData.exp_id;
            mEmbeddedDataMode.stride = mSensorEmbeddedMetaData.stride;
            mEmbeddedDataMode.height = mSensorEmbeddedMetaData.height;
            mEmbeddedDataMode.effective_width = (int32_t*)mSensorEmbeddedMetaData.effective_width;
            exp_id = mEmbeddedDataMode.exp_id;
        }
    }

    if (status == NO_ERROR)
        status = decodeSensorEmbeddedMetaData();
    if (status == NO_ERROR)
        status = storeDecodedMetaData();

    return status;
}

/**
 * pop the sensor metadata from the stored queue according to the exp_id
 * if the value of "exp_id" is equal to -1, means don't need to sync sensor metadata with exp_id,
 , just use the oldest one
 */
status_t SensorEmbeddedMetaData::getDecodedExposureParams(ia_aiq_exposure_sensor_parameters* sensor_exp_p
                       , ia_aiq_exposure_parameters* generic_exp_p, unsigned int exp_id)
{
    Mutex::Autolock lock(mLock);
    LOG2("@%s exp_id:%u", __FUNCTION__, exp_id);
    status_t status = UNKNOWN_ERROR;

    if (!mSensorMetaDataSupported || sensor_exp_p == NULL || generic_exp_p == NULL)
        return status;

    Vector<decoded_sensor_metadata>::iterator it = mSensorEmbeddedMetaDataStoredQueue.begin();
    for (;it != mSensorEmbeddedMetaDataStoredQueue.end(); ++it) {
        // Get the exposure params matched by exposure id or latest exposure params without
        // exposure id (in the case exp_id is equal to zero)
        if (it->exp_id == exp_id || (exp_id == 0 && it->exp_id != 0)) {
            if ((mSensorMetaDataConfigFlag & SENSOR_EXPOSURE_EXIST) && it->sensor_units_p != NULL) {
                memcpy(sensor_exp_p, it->sensor_units_p, sizeof(ia_aiq_exposure_sensor_parameters));
                LOG2("get metadata: (exp_id: %d), sensor_exposure_params fine_integration: %d, coarse_integration:%d, ag:%d, dg:%d",
                     exp_id,
                     sensor_exp_p->fine_integration_time,
                     sensor_exp_p->coarse_integration_time,
                     sensor_exp_p->analog_gain_code_global,
                     sensor_exp_p->digital_gain_global);
                if (!(mSensorMetaDataConfigFlag & GENERAL_EXPOSURE_EXIST) && it->generic_units_p != NULL) {
                    // convert sensor exposure to generic exposure
                    m3AControls->convertSensorExposure2GenericExposure(sensor_exp_p->coarse_integration_time,
                                                                       sensor_exp_p->fine_integration_time,
                                                                       sensor_exp_p->analog_gain_code_global,
                                                                       sensor_exp_p->digital_gain_global,
                                                                       generic_exp_p);
                }
            }

            if ((mSensorMetaDataConfigFlag & GENERAL_EXPOSURE_EXIST) && it->generic_units_p != NULL) {
                memcpy(generic_exp_p, it->generic_units_p, sizeof(ia_aiq_exposure_parameters));
            }
            return NO_ERROR;
        }
    }
    LOGW("No available sensor metadata for exp_id %d", exp_id);
    return status;
}

bool SensorEmbeddedMetaData::checkSensorMetadataAvailable(unsigned int exp_id)
{
    LOG2("@%s", __FUNCTION__);
    if (!mSensorMetaDataSupported)
        return false;

    bool available = false;
    if (mLatestExpId >= MAX_SENSOR_METADATA_QUEUE_SIZE) {
        /*
         *                             | latest
         * [ _ _ _ * * * * * * * * * * * _ _ _ ]
         *         |<-    stored     ->|
         */
        available = (exp_id <= mLatestExpId);
    } else {
        /*
         *            | latest
         * [*.*.*.* * * _ _ _ _ _ _ _ _ _ * * ]
         *              |<-not stored ->|
         */
        available = (exp_id <= mLatestExpId || exp_id > mLatestExpId + MAX_EXP_ID - MAX_SENSOR_METADATA_QUEUE_SIZE);
    }
#if 0
    if (!available) {
        // Try to find it
        unsigned int newExpId = 0;
        status_t status = handleSensorEmbeddedMetaData(newExpId, ATOMISP_MAIN_METADATA);
        if (status == NO_ERROR) {
            LOGE("get embedded %d in check function", newExpId);
        }
    }
#endif
    return available;
}

/**
  * decode sensor metadata buffer by iq_tool
  */
status_t SensorEmbeddedMetaData::decodeSensorEmbeddedMetaData()
{
    LOG2("@%s", __FUNCTION__);
    ia_err err = ia_err_none;
    status_t ret = NO_ERROR;

    err = ia_emd_decoder_run(&mEmbeddedDataBin, &mEmbeddedDataMode, mEmbeddedMetaDecoderHandler);
    if (err !=  ia_err_none) {
        LOGW("decoder error ret:%d", err);
        ret = UNKNOWN_ERROR;
    }
    // run only once
    if (mSensorMetaDataConfigFlag == 0) {
        if ((mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p) {
            mSensorMetaDataConfigFlag |= SENSOR_EXPOSURE_EXIST;
        }
        if ((mEmbeddedMetaDecoderHandler->decoded_data).generic_units_p) {
            mSensorMetaDataConfigFlag |= GENERAL_EXPOSURE_EXIST;
        }
    }
    if ((mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p)
        LOG2("decoded metadata: sensor_exposure_params fine_integration: %d, coarse_integration:%d, ag:%d, dg:%d",
             (mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p->fine_integration_time,
             (mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p->coarse_integration_time,
             (mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p->analog_gain_code_global,
             (mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p->digital_gain_global);

    return ret;
}

/**
  * store the sensor metadata into FIFO queque.
  */
status_t SensorEmbeddedMetaData::storeDecodedMetaData()
{
    Mutex::Autolock lock(mLock);
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    decoded_sensor_metadata new_stored_element;

    // Stored Q is full, pop the oldest buffer to fill the latest data.
    new_stored_element = mSensorEmbeddedMetaDataStoredQueue.top();
    mSensorEmbeddedMetaDataStoredQueue.pop();

    // Update the content with the latest data,
    // then push back to mSensorEmbeddedMetaDataStoredQueue.
    if (mSensorMetaDataConfigFlag & SENSOR_EXPOSURE_EXIST) {
        ia_aiq_exposure_sensor_parameters *sensor_exposure_params =
                                            new_stored_element.sensor_units_p;
        memcpy(sensor_exposure_params,
               (mEmbeddedMetaDecoderHandler->decoded_data).sensor_units_p,
               sizeof(ia_aiq_exposure_sensor_parameters));
    }

    if (mSensorMetaDataConfigFlag & GENERAL_EXPOSURE_EXIST) {
        ia_aiq_exposure_parameters *generic_exposure_params =
                                            new_stored_element.generic_units_p;
        memcpy(generic_exposure_params,
               (mEmbeddedMetaDecoderHandler->decoded_data).generic_units_p,
               sizeof(ia_aiq_exposure_parameters));
    }

    new_stored_element.exp_id = mSensorEmbeddedMetaData.exp_id;
    LOG2("stored metadata exposure id: %u", new_stored_element.exp_id);
    mSensorEmbeddedMetaDataStoredQueue.push_front(new_stored_element);
    mLatestExpId = mSensorEmbeddedMetaData.exp_id;

    return status;
}


} /* namespace camera2 */
} /* namespace android */

