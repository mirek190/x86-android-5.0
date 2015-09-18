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

#ifndef _CAMERA3_IPU4CONFPARSER_H_
#define _CAMERA3_IPU4CONFPARSER_H

#include <utils/Vector.h>
#include "PlatformData.h"
#include "IPU4CameraCapInfo.h"
#include "MediaCtlPipeConfig.h"

namespace android {
namespace camera2 {

class IPU4ConfParser : public PSLConfParser {
public:
    IPU4ConfParser();
    void ParserXML(const char* profilePath);
    virtual ~IPU4ConfParser();

    virtual CameraCapInfo* getCameraCapInfo(int cameraId);
    virtual camera_metadata_t *constructDefaultMetadata(int cameraId, int reqTemplate);

private:
    static const int mBufSize = 1*1024;  // For xml file
    enum DataField {
        FIELD_INVALID = 0,
        FIELD_HAL_TUNING_IPU4,
        FIELD_SENSOR_INFO_IPU4,
        FIELD_MEDIACTL_ELEMENTS_IPU4,
        FIELD_MEDIACTL_CONFIG_IPU4
    } mCurrentDataField;
    unsigned mSensorIndex;
    MediaCtlConfig mMediaCtlCamConfig;     // one-selected camera pipe config.

    static void startElement(void *userData, const char *name, const char **atts);
    static void endElement(void *userData, const char *name);
    void checkField(const char *name, const char **atts);
    void getDataFromXmlFile(void);
    status_t addCamera(int cameraId);
    void handleHALTuning(const char *name, const char **atts);
    void handleSensorInfo(const char *name, const char **atts);
    void handleMediaCtlElements(const char *name, const char **atts);
    void handleMediaCtlConfig(const char *name, const char **atts);
    void getPSLDataFromXmlFile(const char* profilePath);

    int getV4L2PixelFormatAsValue(const char* format);
    int getStreamFormatAsValue(const char* format);
    int getSelectionTargetAsValue(const char* target);
    int getControlIdAsValue(const char* format);
    int getVideoNodeTypeAsValue(const char* nodeType);

    void dumpHalTuningSection(int cameraId);
    void dumpSensorInfoSection(int cameraId);
    void dumpMediaCtlElementsSection(int cameraId);
    void dumpMediaCtlConfigSections(int cameraId);
    void dump(void);

private:
    KeyedVector<int, IPU4CameraCapInfo *> mCaps;
    KeyedVector<int, Vector<camera_metadata_t *> > mDefaultRequests;
};

} // namespace camera2
} // namespace android
#endif
