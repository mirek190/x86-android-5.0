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
#ifndef CAMERA3_HAL_SENSOREMBEDDEDMETADATA_H_
#define CAMERA3_HAL_SENSOREMBEDDEDMETADATA_H_
#include <utils/threads.h>
#include <utils/Vector.h>
#include <ia_types.h>
#include <ia_aiq_types.h>
#include <ia_aiq.h>
#include <ia_emd_decoder.h>

#include "I3AControls.h"
#include "ICameraHwControls.h"
#include "ipu2/IPU2HwIsp.h"

namespace android {
namespace camera2 {

class SensorEmbeddedMetaData{
public:
    SensorEmbeddedMetaData(HWControlGroup &hwcg, I3AControls *aaa);
    virtual ~SensorEmbeddedMetaData();

    status_t init();
    void reset();
    status_t getDecodedExposureParams(ia_aiq_exposure_sensor_parameters* sensor_exp_p
               , ia_aiq_exposure_parameters* generic_exp_p, unsigned int exp_id = 0);
    bool checkSensorMetadataAvailable(unsigned int exp_id);
    status_t handleSensorEmbeddedMetaData(unsigned int &exp_id, uint8_t metadata_type);
    bool isSensorEmbeddedMetaDataSupported() { return mSensorMetaDataSupported; }

// prevent copy constructor and assignment operator
private:
    SensorEmbeddedMetaData(const SensorEmbeddedMetaData& other);
    SensorEmbeddedMetaData& operator=(const SensorEmbeddedMetaData& other);

//private types
private:
    /*!
     * \brief Common structure for the decoded parameters for Sensor Metadata
     *
     * The structure is a combination of all the sensor metadata needed for AIQ
     * and applications, it should be extended when sensor supports more sensor
     * metadata
     */
    struct decoded_sensor_metadata {
         unsigned int exp_id;
         ia_aiq_exposure_sensor_parameters* sensor_units_p;
         ia_aiq_exposure_parameters* generic_units_p;
    };

    enum {
         SENSOR_EXPOSURE_EXIST = 1,
         GENERAL_EXPOSURE_EXIST = 2,
    };
//private data
private:
    Mutex mLock;
    IHWIspControl *mISP;
    Vector<decoded_sensor_metadata> mSensorEmbeddedMetaDataStoredQueue;
    unsigned int mLatestExpId;
    //EMBEDDED METADATA
    sensor_embedded_metadata mSensorEmbeddedMetaData;
    ia_emd_decoder_t *mEmbeddedMetaDecoderHandler;
    ia_emd_mode_t mEmbeddedDataMode;
    ia_binary_data mEmbeddedDataBin;
    bool mSensorMetaDataSupported;
    int32_t mSensorMetaDataConfigFlag;
    I3AControls *m3AControls;
//private methods
private:
    status_t initSensorEmbeddedMetaDataQueue();
    void deinitSensorEmbeddedMetaDataQueue();
    status_t decodeSensorEmbeddedMetaData();
    status_t storeDecodedMetaData();

};

}; /* namespace camera2 */
}; /* namespace android */
#endif /* CAMERA3_HAL_SENSOREMBEDDEDMETADATA_H_ */

