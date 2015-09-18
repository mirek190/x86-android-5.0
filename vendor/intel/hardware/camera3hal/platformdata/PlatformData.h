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

#ifndef _CAMERA3_HAL_PLATFORMDATA_H_
#define _CAMERA3_HAL_PLATFORMDATA_H_

#include <utils/Vector.h>
#include <hardware/camera3.h>
#include <libexpat/expat.h>

#include "Camera3GFXFormat.h"
#include "CameraConf.h"
#include "Metadata.h"

namespace android {
namespace camera2 {
#define DEFAULT_ENTRY_CAP 256
#define DEFAULT_DATA_CAP 2048

#define ENTRY_RESERVED 16
#define DATA_RESERVED 128


/**
 * Maximum number of CPF files cached by the HAL library.
 * On loading the HAL library we will detect all cameras in the system and try
 * to load the CPF files.
 * This define control the maximum number of cameras that we can keep the their
 * CPF loaded in memory.
 * This should  be always higher than the maximum number of cameras in the system
 *
 */
#define MAX_CPF_CACHED  16

/* These should be read from the platform configure file */
#define MAX_CAMERAS 2
#define BACK_CAMERA_ID 0
#define FRONT_CAMERA_ID 1
#define MAX_CAMERA_BUFFERS_NUM  32

#define RESOLUTION_14MP_WIDTH   4352
#define RESOLUTION_14MP_HEIGHT  3264
#define RESOLUTION_8MP_WIDTH    3264
#define RESOLUTION_8MP_HEIGHT   2448
#define RESOLUTION_5MP_WIDTH    2560
#define RESOLUTION_5MP_HEIGHT   1920
#define RESOLUTION_1_3MP_WIDTH    1280
#define RESOLUTION_1_3MP_HEIGHT   960
#define RESOLUTION_1080P_WIDTH  1920
#define RESOLUTION_1080P_HEIGHT 1080
#define RESOLUTION_720P_WIDTH   1280
#define RESOLUTION_720P_HEIGHT  720
#define RESOLUTION_480P_WIDTH   768
#define RESOLUTION_480P_HEIGHT  480
#define RESOLUTION_VGA_WIDTH    640
#define RESOLUTION_VGA_HEIGHT   480
#define RESOLUTION_POSTVIEW_WIDTH    320
#define RESOLUTION_POSTVIEW_HEIGHT   240

#define ALIGNED_128 128
#define ALIGNED_64  64

#define MAX_LSC_GRID_WIDTH  64
#define MAX_LSC_GRID_HEIGHT  64


typedef enum {
    SUPPORTED_HW_CSS_2400,
    SUPPORTED_HW_CSS_2500,
    SUPPORTED_HW_CSS_2600,
    SUPPORTED_HW_USB,
    SUPPORTED_HW_CIF,
    SUPPORTED_HW_UNKNOWN
} CameraHwType;

enum SensorType {
    SENSOR_TYPE_NONE = 0,
    SENSOR_TYPE_RAW,
    SENSOR_TYPE_SOC
};

enum SensorFlip {
    SENSOR_FLIP_NA     = -1,   // Support Not-Available
    SENSOR_FLIP_OFF    = 0x00, // Both flip ctrls set to 0
    SENSOR_FLIP_H      = 0x01, // V4L2_CID_HFLIP 1
    SENSOR_FLIP_V      = 0x02, // V4L2_CID_VFLIP 1
};

class CameraProfiles;

class CameraHWInfo {
public:
    CameraHWInfo();
    ~CameraHWInfo() {};
    const char* boardName(void) const { return mBoardName; }
    const char* productName(void) const { return mProductName; }
    const char* manufacturerName(void) const { return mManufacturerName; }
    bool supportDualVideo(void) const { return mSupportDualVideo; }
    int previewHALFormat(void) const { return mPreviewHALFormat; }
    bool supportExtendedMakernote(void) const { return mSupportExtendedMakernote; }


    String8 mProductName;
    String8 mManufacturerName;
    String8 mBoardName;
    int mPreviewHALFormat;  // specify the preview format for multi configured streams
    bool mSupportDualVideo;
    bool mSupportExtendedMakernote;

};

/**
 * \class CameraCapInfo
 *
 * Abstract interface implemented by PSL CameraCapInfo.
 * The PlatformData::getCameraCapInfo shall return a value of this type
 *
 */
class CameraCapInfo {
public:
    struct Size {
        int width;
        int height;
    };

    CameraCapInfo() {};
    virtual ~CameraCapInfo() {};
    virtual int sensorType(void) const = 0;
    virtual int getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat) const = 0;
};

/**
 * \class PSLConfParser
 *
 * Abstract interface implemented by PSL parser.
 * The ICameraHw::getPSLParser shall return a value of this type
 *
 */
class PSLConfParser {
public:
    PSLConfParser() {};
    virtual ~PSLConfParser() {};

    virtual CameraCapInfo* getCameraCapInfo(int cameraId) = 0;
    virtual camera_metadata_t *constructDefaultMetadata(int cameraId, int reqTemplate) = 0;
    virtual void ParserXML(const char* xmlPath)=0;
};

class PlatformData {
public:
    static void init();     // called when HAL is loaded
private:
    static CameraProfiles * mInstance;
    static CameraProfiles* getInstance(void);
    static CameraHWInfo * mCameraHWInfo;
    static CpfStore* sKnownCPFConfigurations[MAX_CPF_CACHED];
public:
    static AiqConf AiqConfig; // TO BE REMOVED!! use the getter

    static int numberOfCameras(void);
    static void getCameraInfo(int cameraId, struct camera_info * info);
    static const camera_metadata_t *getStaticMetadata(int cameraId);
    static camera_metadata_t *getDefaultMetadata(int cameraId, int reqTemplate);
    static CameraHwType getCameraHwType(int cameraId);
    static int getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat, int cameraId);
    static const AiqConf* getAiqConfiguration(int cameraId);

    static const CameraCapInfo * getCameraCapInfo(int cameraId);
    static const CameraHWInfo * getCameraHWInfo();
    static status_t createVendorPlatformProductName(String8& name);

    static const char* boardName(void);
    static const char* productName(void);
    static const char* manufacturerName(void);

    static bool supportDualVideo(void);
    static int previewHALFormat(void);
    static bool supportExtendedMakernote(void);
    /**
     * get the number of CPU cores
     * \return the number of CPU cores
     */
    static unsigned int getNumOfCPUCores();
    /**
    * Utility methods to retrieve particular fields from the static metadata
    * (a.k.a. Camera Characteristics), Please do NOT add anything else here
    * without a very good reason.
    */
    static int facing(int cameraId);
    static int orientation(int cameraId);
    static float getStepEv(int cameraId);
    static int getPartialMetadataCount(int cameraId);

private:
    static status_t readSpId(String8& spIdName, int& spIdValue);
};

} // namespace camera2
} // namespace android
#endif // _CAMERA3_HAL_PLATFORMDATA_H_
