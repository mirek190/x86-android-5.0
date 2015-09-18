/*
 * Copyright (C) 2013 Intel Corporation
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

#ifndef _CAMERA3_HAL_PROFILE_H_
#define _CAMERA3_HAL_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include <utils/Vector.h>
#include <hardware/camera3.h>

#include "PlatformData.h"
#include "ItemPool.h"

namespace android {
namespace camera2 {

class CameraCapInfo;
/**
 * \class CameraProfiles
 *
 * This class is used to parse the camera configuration file.
 * The configuration file is xml format.
 * This class will use the expat lib to do the xml parser.
 */
class CameraProfiles {
public:
    CameraProfiles(CameraHWInfo * cameraHWInfo);
    ~CameraProfiles();

    camera_metadata_t *constructDefaultMetadata(int cameraId, int reqTemplate);
    status_t addCamera(int cameraId);
    CameraHwType getCameraHwforId(int cameraId);
    const CameraCapInfo * getCameraCapInfo(int cameraId);
    /**********************************************************************
     * vendor_tag_query_ops override
     */
    static const char *get_camera_vendor_section_name(
        const vendor_tag_query_ops_t *v,
        uint32_t tag);
    static const char *get_camera_vendor_tag_name(
        const vendor_tag_query_ops_t *v,
        uint32_t tag);
    static int get_camera_vendor_tag_type(
        const vendor_tag_query_ops_t *v,
        uint32_t tag);

public: /* types */
    Vector<camera_metadata_t *> mStaticMeta;

private: /* private types */
    enum DataField {
        FIELD_INVALID = 0,
        FIELD_SUPPORTED_HARDWARE,
        FIELD_ANDROID_STATIC_METADATA,
        FIELD_SENSOR_VENDOR_METADATA,
        FIELD_COMMON
    } mCurrentDataField;

    struct CameraInfo {
        PSLConfParser* parser;
        String8 hwType;
        String8 sensorName;
    };

private: /* Constants*/
    static const int BUFFERSIZE = 4*1024;  // For xml file
    static const int METADATASIZE= 1024;
    static const int mMaxConfigNameLength = 64;

private: /* Methods */
    static void startElement(void *userData, const char *name, const char **atts);
    static void endElement(void *userData, const char *name);
    void getDataFromXmlFile(void);
    void checkField(const char *name, const char **atts);
    status_t initCurrent(int sensorId);
    status_t initSensorList();

    void handleSupportedHardware(const char *name, const char **atts);
    void handleAndroidStaticMetadata(const char *name, const char **atts);
    void handleVendorMetadata(const char *name, const char **atts);
    void handleCommon(const char *name, const char **atts);

    void dumpSupportedHWSection(int cameraId);
    void dumpAndroidmetadataSection(int cameraId);
    void dumpCommonSection();
    void dump(void);

    // Helpers
    int convertEnum(void *dest, const char *src, int type,
                    const metadata_value_t *table, int tableLen,
                    void **newDest);
    int parseEnum(const char * src,
                  const metadata_tag_t* tagInfo,
                  int metadataCacheSize,
                  int64_t* metadataCache);
    int parseEnumAndNumbers(const char * src,
                            const metadata_tag_t* tagInfo,
                            int metadataCacheSize,
                            int64_t* metadataCache);
    int parseStreamConfig(const char * src,
                          const metadata_tag_t* tagInfo,
                          int metadataCacheSize,
                          int64_t* metadataCache);
    int parseStreamConfigDuration(const char * src,
                                  const metadata_tag_t* tagInfo,
                                  int metadataCacheSize,
                                  int64_t* metadataCache);
    int parseData(const char * src,
                  const metadata_tag_t* tagInfo,
                  int metadataCacheSize,
                  int64_t* metadataCache);
    int parseSceneModeOverride(const char * src,
                               const metadata_tag_t* tagInfo,
                               int metadataCacheSize,
                               int64_t* metadataCache);
    int parseAvailableInputOutputFormatsMap(const char * src,
                                            const metadata_tag_t* tagInfo,
                                            int metadataCacheSize,
                                            int64_t* metadataCache);
    int parseSizes(const char * src,
                   const metadata_tag_t* tagInfo,
                   int metadataCacheSize,
                   int64_t* metadataCache);
    int parseRectangle(const char * src,
                       const metadata_tag_t* tagInfo,
                       int metadataCacheSize,
                       int64_t* metadataCache);
    int parseBlackLevelPattern(const char * src,
                               const metadata_tag_t* tagInfo,
                               int metadataCacheSize,
                               int64_t* metadataCache);
    int parseImageFormats(const char * src,
                          const metadata_tag_t* tagInfo,
                          int metadataCacheSize,
                          int64_t* metadataCache);
    int parseAvailableKeys(const char * src,
                           const metadata_tag_t* tagInfo,
                           int metadataCacheSize,
                           int64_t* metadataCache);

    const char *skipWhiteSpace(const char *src);

 private:  /* Members */
    int64_t * mMetadataCache;  // for metadata construct
    unsigned mSensorIndex;
    unsigned mItemsCount;
    String8  mCurrSensorName;
    CameraHWInfo * mCameraCommon;
    ItemPool<CameraInfo>  mCameraInfoPool;
    // To store the supported HW type for each camera id
    KeyedVector<unsigned int, CameraInfo*> mCameraIdToCameraInfo;
    Vector<int> mCharacteristicsKeys[MAX_CAMERAS];
    Vector<String8> mCamName;

};

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_PROFILE_H_
