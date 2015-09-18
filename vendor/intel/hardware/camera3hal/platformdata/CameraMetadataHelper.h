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

#ifndef _CAMERA3_HAL_METADATAHELPER_H_
#define _CAMERA3_HAL_METADATAHELPER_H_


#include <camera/CameraMetadata.h>
namespace android {
namespace camera2 {

/* ********************************************************************
 * Camera metadata auxiliary API
 */

namespace MetadataHelper {

void dumpMetadata(const camera_metadata_t * meta);

bool getMetadataValue(const CameraMetadata &metadata, uint32_t tag, uint8_t & value, int count = -1);
bool getMetadataValue(const CameraMetadata &metadata, uint32_t tag, int32_t & value, int count = -1);
bool getMetadataValue(const CameraMetadata &metadata, uint32_t tag, int64_t & value, int count = -1);
bool getMetadataValue(const CameraMetadata &metadata, uint32_t tag, float & value, int count = -1);
bool getMetadataValue(const CameraMetadata &metadata, uint32_t tag, double & value, int count = -1);

const void * getMetadataValues(const CameraMetadata &metadata, uint32_t tag, int type, int * count = NULL);
const void * getMetadataValues(const camera_metadata_t * metadata, uint32_t tag, int type, int * count = NULL);

status_t updateMetadata(camera_metadata_t * metadata, uint32_t tag, const void* data, size_t data_count);

};

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_METADATAHELPER_H_
