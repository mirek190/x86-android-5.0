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

#define LOG_TAG "PlatformData"

#include "LogHelper.h"
#include "PlatformData.h"
#include "CameraProfiles.h"
#include "CameraMetadataHelper.h"

namespace android {
namespace camera2 {

static const int spIdLength = 4;

CameraProfiles * PlatformData::mInstance = NULL;
CameraHWInfo * PlatformData::mCameraHWInfo = NULL;
AiqConf PlatformData::AiqConfig;    // TO BE REMOVED
/**
 * index to this array is the camera id
 */
CpfStore* PlatformData::sKnownCPFConfigurations[MAX_CPF_CACHED];

__attribute__((constructor)) void initCameraHAL() {
    PlatformData::init();
}
/**
 * This method is only called once when the HAL library is loaded
 *
 * At this time we can load the XML config (camera3_prfiles.xml)and find the
 * CPF files for all the sensors. Once we load the CPF file
 * we fill the parts of the static metadata that is stored in the CMC
 *
 * Please note that the CpfStore objects are created once but not released
 * This is because this array is only freed if the process is destroyed.
 */
void PlatformData::init()
{
    LOGD("Camera HAL static init");
    if (mCameraHWInfo)
        delete mCameraHWInfo;
    if (mInstance)
        delete mInstance;

    CLEAR(sKnownCPFConfigurations);

    mCameraHWInfo =  new CameraHWInfo();
    mInstance = new CameraProfiles(mCameraHWInfo);
    int numberOfCameras = PlatformData::numberOfCameras();

    if ((mCameraHWInfo == NULL || mInstance == NULL) ||
        (numberOfCameras > MAX_CPF_CACHED)) {
        LOGE("Camera HAL Basic Platform initialization failed !!");
        assert(0);
    }

    /**
     * This number currently comes from the number if sections in the XML
     * in the future this is not reliable if we want to have multiple cameras
     * supported in the same XML.
     * TODO: add a common field in the XML that lists the camera's OR
     * query from driver at runtime
     */
    CpfStore *cpf;
    camera_metadata_t * metadata;
    for (int i = 0; i < numberOfCameras; i++) {
        cpf = new CpfStore(i);
        cpf->AiqConfig.fillStaticMetadataFromCMC(mInstance->mStaticMeta[i]);
        sKnownCPFConfigurations[i] = cpf;
    }
    LOGD("Camera HAL static init - Done!");
}

/**
 * static acces method to implement the singleton
 * mInstance should have been instantiated when the library loaded.
 * Having NULL is a serious error
 */
CameraProfiles *PlatformData::getInstance(void)
{
    if (mInstance == NULL) {
        LOGE("@%s Failed to create CameraProfiles instance", __FUNCTION__);
        return NULL;
    }
    return mInstance;
}

int PlatformData::numberOfCameras(void)
{
    CameraProfiles * i = getInstance();
    return (int)i->mStaticMeta.size();
}

void PlatformData::getCameraInfo(int cameraId, struct camera_info * info)
{
    info->facing = facing(cameraId);
    info->orientation = orientation(cameraId);
    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    if (info->device_version == CAMERA_DEVICE_API_VERSION_3_2)
        LOGD("@%s, current HAL version:CAMERA_DEVICE_API_VERSION_3_2", __FUNCTION__);
    else
        LOGD("@%s, current HAL version:CAMERA_DEVICE_API_VERSION_3_0", __FUNCTION__);
    info->static_camera_characteristics = getStaticMetadata(cameraId);
}

const AiqConf* PlatformData::getAiqConfiguration(int cameraId)
{
    return &(sKnownCPFConfigurations[cameraId]->AiqConfig);
}

/**
 * Function converts the "lens.facing" android static meta data value to
 * value needed by camera service
 * Camera service uses different values from the android metadata
 * Refer system/core/include/system/camera.h
 */

int PlatformData::facing(int cameraId)
{
    uint8_t facing;
    CameraMetadata staticMeta;
    staticMeta = getStaticMetadata(cameraId);
    MetadataHelper::getMetadataValue(staticMeta, ANDROID_LENS_FACING, facing);
    facing = (facing == FRONT_CAMERA_ID) ? CAMERA_FACING_BACK : CAMERA_FACING_FRONT;

    return facing;
}

int PlatformData::orientation(int cameraId)
{
    int orientation;
    CameraMetadata staticMeta;
    staticMeta = getStaticMetadata(cameraId);
    MetadataHelper::getMetadataValue(staticMeta, ANDROID_SENSOR_ORIENTATION, orientation);

    return orientation;
}

/**
 * Retrieves the partial result count from the static metadata
 * This number is the pieces that we return the the result for a single
 * capture request. This number is specific to PSL implementations
 * It has to be at least 1.
 * \param cameraId[IN]: Camera Id  that we are querying the value for
 * \return value
 */
int PlatformData::getPartialMetadataCount(int cameraId)
{
    int partialMetadataCount = 0;
    CameraMetadata staticMeta;
    staticMeta = getStaticMetadata(cameraId);
    MetadataHelper::getMetadataValue(staticMeta,
            ANDROID_REQUEST_PARTIAL_RESULT_COUNT, partialMetadataCount);
    if (partialMetadataCount <= 0) {
        LOGW("Invalid value (%d) for ANDROID_REQUEST_PARTIAL_RESULT_COUNT"
                "FIX your config", partialMetadataCount);
        partialMetadataCount = 1;
    }
    return partialMetadataCount;
}

const camera_metadata_t * PlatformData::getStaticMetadata(int cameraId)
{
    if (cameraId >= numberOfCameras()) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }

    CameraProfiles * i = getInstance();
    camera_metadata_t * metadata = i->mStaticMeta[cameraId];
    return metadata;
}

camera_metadata_t *PlatformData::getDefaultMetadata(int cameraId, int requestType)
{
    if (cameraId >= numberOfCameras()) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }
    CameraProfiles * i = getInstance();
    return i->constructDefaultMetadata(cameraId, requestType);
}

const CameraCapInfo * PlatformData::getCameraCapInfo(int cameraId)
{
    // Use MAX_CAMERAS instead of numberOfCameras() as it will cause a recursive loop
    if (cameraId > MAX_CAMERAS) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }
    CameraProfiles * i = getInstance();
    return i->getCameraCapInfo(cameraId);
}

status_t PlatformData::createVendorPlatformProductName(String8& name)
{
    int vendorIdValue;
    int platformFamilyIdValue;
    int productLineIdValue;

    name = "";

    String8 vendorIdName = String8("vendor_id");
    String8 platformFamilyIdName = String8("platform_family_id");
    String8 productLineIdName = String8("product_line_id");

    if (readSpId(vendorIdName, vendorIdValue) < 0) {
        LOGE("%s could not be read from sysfs", vendorIdName.string());
        return UNKNOWN_ERROR;
    }
    if (readSpId(platformFamilyIdName, platformFamilyIdValue) < 0) {
        LOGE("%s could not be read from sysfs", platformFamilyIdName.string());
        return UNKNOWN_ERROR;
    }
    if (readSpId(productLineIdName, productLineIdValue) < 0){
        LOGE("%s could not be read from sysfs", productLineIdName.string());
        return UNKNOWN_ERROR;
    }

    char vendorIdValueStr[spIdLength];
    char platformFamilyIdValueStr[spIdLength];
    char productLineIdValueStr[spIdLength];

    snprintf(vendorIdValueStr, spIdLength, "%#x", vendorIdValue);
    snprintf(platformFamilyIdValueStr, spIdLength, "%#x", platformFamilyIdValue);
    snprintf(productLineIdValueStr, spIdLength, "%#x", productLineIdValue);

    name = vendorIdValueStr;
    name += String8("-");
    name += platformFamilyIdValueStr;
    name += String8("-");
    name += productLineIdValueStr;

    return OK;
}

CameraHwType PlatformData::getCameraHwType(int cameraId)
{
    CameraProfiles * i = getInstance();
    return i->getCameraHwforId(cameraId);
}

int PlatformData::getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat, int cameraId)
{
    CameraProfiles * i = getInstance();
    const CameraCapInfo *info = i->getCameraCapInfo(cameraId);
    if (info == NULL) {
        LOGE("ERROR getting  CameraCapInfo");
        return -1;
    }
    return info->getV4L2PixelFmtForGfxHalFmt(gfxHalFormat);
}

const char* PlatformData::boardName(void)
{
    return mCameraHWInfo->boardName();
}

const char* PlatformData::productName(void)
{
    return mCameraHWInfo->productName();
}

const char* PlatformData::manufacturerName(void)
{
    return mCameraHWInfo->manufacturerName();
}

bool PlatformData::supportDualVideo(void)
{
    return mCameraHWInfo->supportDualVideo();
}

int PlatformData::previewHALFormat(void)
{
    return mCameraHWInfo->previewHALFormat();
}

bool PlatformData::supportExtendedMakernote(void)
{
    return mCameraHWInfo->supportExtendedMakernote();
}


unsigned int PlatformData::getNumOfCPUCores()
{
    LOG1("@%s, line:%d", __FUNCTION__, __LINE__);
    unsigned int cpuCores = 1;

    char buf[20];
    FILE *cpuOnline = fopen("/sys/devices/system/cpu/online", "r");
    if (cpuOnline) {
        CLEAR(buf);
        fread(buf, 1, sizeof(buf), cpuOnline);
        buf[sizeof(buf) - 1] = '\0';
        cpuCores = 1 + atoi(strstr(buf, "-") + 1);
        fclose(cpuOnline);
    }
    LOG1("@%s, line:%d, cpu core number:%d", __FUNCTION__, __LINE__, cpuCores);
    return cpuCores;
}

status_t PlatformData::readSpId(String8& spIdName, int& spIdValue)
{
    FILE *file;
    status_t ret = OK;
    String8 sysfsSpIdPath = String8("/sys/spid/");
    String8 fullPath;

    fullPath = sysfsSpIdPath;
    fullPath.append(spIdName);

    file = fopen(fullPath, "rb");
    if (!file) {
        LOGE("ERROR in opening file %s", fullPath.string());
        return NAME_NOT_FOUND;
    }
    ret = fscanf(file, "%x", &spIdValue);
    if (ret < 0) {
        LOGE("ERROR in reading %s", fullPath.string());
        spIdValue = 0;
        fclose(file);
        return UNKNOWN_ERROR;
    }
    fclose(file);
    return ret;
}

float PlatformData::getStepEv(int cameraId)
{
    // Get the ev step
    CameraMetadata staticMeta;
    float stepEV = 1 / 3.0f;
    int count = 0;
    staticMeta = getStaticMetadata(cameraId);
    camera_metadata_rational_t* aeCompStep =
        (camera_metadata_rational_t*)MetadataHelper::getMetadataValues(staticMeta,
                                     ANDROID_CONTROL_AE_COMPENSATION_STEP,
                                     TYPE_RATIONAL,
                                     &count);
    if (count == 1 && aeCompStep != NULL) {
        stepEV = (float)aeCompStep->numerator / aeCompStep->denominator;
    }
    return stepEV;

}
CameraHWInfo::CameraHWInfo()
{
    mBoardName = "saltbay";
    mProductName = "ExampleModel";
    mManufacturerName = "ExampleMaker";
    mSupportDualVideo = false;
    mSupportExtendedMakernote = false;

    // -1 means mPreviewHALFormat is not set
    mPreviewHALFormat = -1;
}


} // namespace camera2
} // namespace android
