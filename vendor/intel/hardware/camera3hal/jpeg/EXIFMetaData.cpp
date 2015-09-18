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

#define LOG_TAG "EXIFMetaData"
#include "LogHelper.h"
#include "EXIFMetaData.h"

namespace android {
namespace camera2 {

#define DEFAULT_ISO_SPEED 100

ExifMetaData::ExifMetaData():
    aeConfig(NULL)
    , ispMkNote(NULL)
    , ia3AMkNote(NULL)
    , flashFired(false)
    , saveMirrored(false)
    , cameraOrientation(0)
    , currentOrientation(0)
{
    LOG2("@%s", __FUNCTION__);
    mJpegSetting.jpegQuality = 90;
    mJpegSetting.jpegThumbnailQuality = 90;
    mJpegSetting.orientation = 0;
    mJpegSetting.thumbWidth = 320;
    mJpegSetting.thumbHeight = 240;
    mGpsSetting.latitude = 0.0;
    mGpsSetting.longitude = 0.0;
    mGpsSetting.altitude = 0.0;
    CLEAR(mGpsSetting.gpsProcessingMethod);
    mGpsSetting.gpsTimeStamp = 0;
    CLEAR(mIa3ASetting);
    mIa3ASetting.isoSpeed = DEFAULT_ISO_SPEED;
    faceState.num_faces = 0;
    faceState.faces = NULL;
}

ExifMetaData::~ExifMetaData()
{
    if (ia3AMkNote) {
        if (ia3AMkNote->data) {
            char *tmp = reinterpret_cast<char*> (ia3AMkNote->data);
            delete tmp;
            ia3AMkNote->data = NULL;
        }
        delete ia3AMkNote;
        ia3AMkNote = NULL;
    }
    if (ispMkNote) {
        delete ispMkNote;
        ispMkNote = NULL;
    }

    if (aeConfig) {
        delete aeConfig;
        aeConfig = NULL;
    }

    if (faceState.faces) {
        delete[] faceState.faces;
        faceState.faces = NULL;
    }
}

};  // namespace camera2
};  // namespace android
