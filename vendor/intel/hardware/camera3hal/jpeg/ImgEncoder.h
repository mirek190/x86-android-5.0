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

#ifndef _CAMERA3_HAL_IMG_ENCODER_H_
#define _CAMERA3_HAL_IMG_ENCODER_H_

#include "CameraBuffer.h"
#include "Camera3Request.h"
#include "SWJpegEncoder.h"
#include "JpegHwEncoder.h"
#include "EXIFMaker.h"

namespace android {
namespace camera2 {

/**
 * \class ImgEncoder
 * This class does JPEG encoding of the main and the thumb buffers provided in
 * the EncodePackage. This class selects between hardware and software encoding
 * and provides the output in the jpeg and thumbout buffers in the EncodePackage.
 * JFIF output is produced by the JPEGMaker class
 *
 */
class ImgEncoder : public RefBase {
public:  /* types */
    struct JpegSettings {
         int jpegQuality;
         int jpegThumbnailQuality;
         int thumbWidth;
         int thumbHeight;
         int orientation;
     };

    struct EncodePackage {
        sp<CameraBuffer> main;        // for input
        sp<CameraBuffer> thumb;       // for input, can be NULL
        sp<CameraBuffer> jpegOut;     // for final JPEG output
        int              jpegSize;    // Jpeg output size
        sp<CameraBuffer> encodedData;    // encoder output for main image
        int              encodedDataSize;// main image encoded data size
        sp<CameraBuffer> thumbOut;    // for thumbnail output
        int              thumbSize;   // Thumb ouptut size
        const CameraMetadata    *settings;    // settings from request
        unsigned char    *jpegDQTAddr; // pointer to DQT marker inside jpeg, for in-place exif creation
        bool             padExif;     // Boolean to control if padding is preferred over copying during in-place exif creation
    };

public: /* Methods */
    ImgEncoder(int cameraid);
    virtual ~ImgEncoder();

    status_t init();
    void deInit(void);
    status_t encodeSync(EncodePackage & package, sp<ExifMetaData> metaData);

private:  /* Methods */
    status_t allocateBufferAndDownScale(EncodePackage & package);
    void thumbBufferDownScale(EncodePackage & package);
    void mainBufferDownScale(EncodePackage & package);
    status_t startHwEncoding(sp<CameraBuffer> srcBuf, int jpegQuality);
    int completeHwEncode(sp<CameraBuffer> destBuf,
                         unsigned int destOffset = 0);
    int doSwEncode(sp<CameraBuffer> srcBuf,
                   int quality,
                   sp<CameraBuffer> destBuf,
                   unsigned int destOffset = 0);
    status_t getJpegSettings(EncodePackage & package, sp<ExifMetaData> metaData);

private:  /* Members */
    JpegHwEncoder * mHwCompressor;
    SWJpegEncoder * mSwCompressor;

    sp<CameraBuffer> mThumbOutBuf;
    sp<CameraBuffer> mJpegDataBuf;
    sp<CameraBuffer> mMainScaled;
    sp<CameraBuffer> mThumbScaled;

    JpegSettings * mJpegSetting;
    int mCameraId;
};
};  // namespace camera2
};  // namespace android

#endif  // _CAMERA3_HAL_IMG_ENCODER_H_
