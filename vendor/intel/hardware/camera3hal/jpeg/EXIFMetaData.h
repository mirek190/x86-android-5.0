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
#include <hardware/camera3.h>

#include "3ATypes.h"
#include "Camera3GFXFormat.h"
#include "camera/CameraMetadata.h"
#include "UtilityMacros.h"


#ifndef _EXIFMETADATA_H_
#define _EXIFMETADATA_H_

#include <ia_face.h>

namespace android {
namespace camera2 {

/**
 * \class ExifMetaData
 *
 */
class ExifMetaData : public RefBase{
public:
    ExifMetaData();
    virtual ~ExifMetaData();
    // jpeg info
    struct JpegSetting{
        int jpegQuality;
        int jpegThumbnailQuality;
        int thumbWidth;
        int thumbHeight;
        int orientation;
    };
    // GPS info
    struct GpsSetting{
        double latitude;
        double longitude;
        double altitude;
        char gpsProcessingMethod[64];
        long gpsTimeStamp;
    };
    // exif info
    struct Ia3ASetting{
        AeMode aeMode;
        MeteringMode meteringMode;
        AwbMode lightSource;
        SceneMode sceneMode;
        float brightness;
        int isoSpeed;
    };
    JpegSetting mJpegSetting;
    GpsSetting mGpsSetting;
    Ia3ASetting mIa3ASetting;
    SensorAeConfig *aeConfig;                  /*!< defined in I3AControls.h */
    atomisp_makernote_info *ispMkNote;
    /*!< kernel provided metadata, defined linux/atomisp.h */
    ia_binary_data *ia3AMkNote;                /*!< defined in ia_mkn_types.h */
    AwbMode awbMode;
    EffectMode effectMode;
    bool flashFired;                           /*!< whether flash was fired */
    bool saveMirrored;                         /*!< whether to do mirroring */
    int cameraOrientation;                     /*!< camera sensor orientation */
    int currentOrientation;                    /*!< Current orientation of the device */
    int zoomRatio;
    ia_face_state faceState;
};


};   // namespace camera2
};   // namespace android
#endif   // _EXIFMETADATA_H_

