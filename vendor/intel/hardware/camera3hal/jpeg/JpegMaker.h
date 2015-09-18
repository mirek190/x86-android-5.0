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

#ifndef _CAMERA3_HAL_JPEG_MAKER_H_
#define _CAMERA3_HAL_JPEG_MAKER_H_

#include "EXIFMaker.h"
#include "EXIFMetaData.h"
#include "ImgEncoder.h"

namespace android {
namespace camera2 {

/**
 * \class JpegMaker
 * Does the EXIF header creation and appending to the provided jpeg buffer
 *
 */
class JpegMaker {
public: /* Methods */
    JpegMaker(int cameraid);
    virtual ~JpegMaker();
    status_t init();
    status_t setupExifWithMetaData(ImgEncoder::EncodePackage & package,
                                   sp<ExifMetaData> metaData);
    status_t makeJpeg(ImgEncoder::EncodePackage & package, sp<CameraBuffer> dest);
    status_t makeJpegInPlace(ImgEncoder::EncodePackage & package); // makes the jpeg inside the main buf writing exif in-place
private:  /* Methods */
    status_t processExifSettings(const CameraMetadata  *settings,
                                 sp<ExifMetaData> metaData);
    status_t processJpegSettings(ImgEncoder::EncodePackage & package,
                                 sp<ExifMetaData> metaData);
    status_t processGpsSettings(const CameraMetadata &settings,
                                sp<ExifMetaData> metadata);
    status_t processAwbSettings(const CameraMetadata &settings,
                                sp<ExifMetaData> metaData);
    status_t processColoreffectSettings(const CameraMetadata &settings,
                                        sp<ExifMetaData> metaData);
    status_t processScalerCropSettings(const CameraMetadata &settings,
                                        sp<ExifMetaData> metaData);
    status_t processEvCompensationSettings(const CameraMetadata &settings,
                                           sp<ExifMetaData> metaData);
    int32_t  getJpegBufferSize(void);
    status_t processSceneSettings(const CameraMetadata &settings,
                                       sp<ExifMetaData> metaData);

private:  /* Members */
    EXIFMaker * mExifMaker;
    int mCameraId;
};
};  // namespace camera2
};  // namespace android

#endif  // _CAMERA3_HAL_JPEG_MAKER_H_
