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

#ifndef _CAMERA3_CIFCONFPARSER_H_
#define _CAMERA3_CIFCONFPARSER_H

#include "PlatformData.h"
#include "CIFCameraCapInfo.h"

namespace android {
namespace camera2 {

class CIFConfParser : public PSLConfParser {
public:
    CIFConfParser();
    virtual ~CIFConfParser();

    virtual CameraCapInfo* getCameraCapInfo(int cameraId);
    virtual camera_metadata_t* constructDefaultMetadata(int cameraId, int reqTemplate);

private:
    enum DataField {
        FIELD_INVALID = 0,
        FIELD_HAL_TUNING_CIF,
        FIELD_SENSOR_INFO_CIF,
    } mCurrentDataField;

    static void startElement(void *userData, const char *name, const char **atts);
    static void endElement(void *userData, const char *name);
    void checkField(const char *name, const char **atts);
    status_t addCamera(int cameraId);
    void handleHALTuning(const char *name, const char **atts);
    void handleSensorInfo(const char *name, const char **atts);
    void getPSLDataFromXmlFile(void);

    int getV4L2PixelFormatAsValue(const char* format);

    static const int mBufSize = 1 * 1024;  // For xml file
    unsigned mSensorIndex;

private:
    Vector<CIFCameraCapInfo *> mCaps;
    Vector<camera_metadata_t *> mDefaultRequests;
};

} // namespace camera2
} // namespace android
#endif
