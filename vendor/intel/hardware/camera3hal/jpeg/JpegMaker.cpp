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

#define LOG_TAG "JpegMaker"

#include "JpegMaker.h"
#include <hardware/camera3.h>
#include "LogHelper.h"
#include "PlatformData.h"
#include "CameraMetadataHelper.h"

namespace android {
namespace camera2 {

static const unsigned char JPEG_MARKER_SOI[2] = {0xFF, 0xD8};  // JPEG StartOfImage marker

JpegMaker::JpegMaker(int cameraid) :
    mExifMaker(NULL)
    ,mCameraId(cameraid)
{
    LOG2("@%s", __FUNCTION__);
}

JpegMaker::~JpegMaker()
{
    LOG2("@%s", __FUNCTION__);

    if (mExifMaker != NULL) {
        delete mExifMaker;
        mExifMaker = NULL;
    }
}

status_t JpegMaker::init(void)
{
    LOG2("@%s", __FUNCTION__);

    if (mExifMaker == NULL) {
        mExifMaker = new EXIFMaker();
        if (mExifMaker == NULL) {
            LOGE("ERROR: Can't create EXIF maker!");
            return NO_INIT;
        }
    }

    return NO_ERROR;
}

status_t JpegMaker::setupExifWithMetaData(ImgEncoder::EncodePackage & package,
                                          sp<ExifMetaData> metaData)
{
    LOG2("@%s", __FUNCTION__);

    status_t status = NO_ERROR;

    status = processJpegSettings(package, metaData);

    status = processExifSettings(package.settings, metaData);
    if (status != NO_ERROR) {
        LOGE("@%s: Process settngs for Exif! %d", __FUNCTION__, status);
        return status;
    }

    mExifMaker->initialize(package.main->width(), package.main->height());
    mExifMaker->pictureTaken(metaData);
    if (metaData->ispMkNote)
        mExifMaker->setDriverData(*metaData->ispMkNote);
    if (metaData->ia3AMkNote)
        mExifMaker->setMakerNote(*metaData->ia3AMkNote);
    if (metaData->aeConfig)
        mExifMaker->setSensorAeConfig(*metaData->aeConfig);
    if (metaData->flashFired)
        mExifMaker->enableFlash();
    mExifMaker->initializeLocation(metaData);

    return status;
}

/**
 * makeJpegInPlace
 *
 * Create and prefix the exif header to the encoded jpeg data. Use the existing
 * JPEG buffer which already has the data starting with an offset (space for exif).
 *
 * Note that the given package.jpegSize must be the size between DQT and EOI, including
 * the marker sizes.
 *
 * \param package [IN] EncodePackage from the caller with encoded main and thumb
 *  buffers, Jpeg settings, encoded sizes, jpeg DQT offset and whether to pad
 */
status_t JpegMaker::makeJpegInPlace(ImgEncoder::EncodePackage &package)
{
    LOG1("@%s", __FUNCTION__);

    if (package.thumbOut.get())
        mExifMaker->setThumbnail((unsigned char *)package.thumbOut->data(), package.thumbSize);
    else
        LOGW("Exif created without thumbnail stream!");

    unsigned char *bufferStartAddr = (unsigned char*)(package.main->data());
    unsigned int exifSize = mExifMaker->makeExifInPlace(bufferStartAddr,
                                                        package.jpegDQTAddr,
                                                        package.jpegSize,
                                                        package.padExif);
    if (exifSize == 0) {
        ALOGE("Couldn't write exif!");
        return UNKNOWN_ERROR;
    }

    // save jpeg size at the end of file
    int32_t bufferSize = package.main->size();
    int32_t jpegSize = exifSize + package.jpegSize;
    uint8_t *destPtr = (uint8_t*) package.main->data();

    if (bufferSize) {
        LOG1("actual jpeg size=%d, jpegbuffer size=%d destptr %p", jpegSize, bufferSize, destPtr);
        void *copyTo = destPtr + bufferSize - sizeof(struct camera3_jpeg_blob);
        struct camera3_jpeg_blob blob;
        blob.jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
        blob.jpeg_size = jpegSize;
        memcpy(copyTo, &blob, sizeof(struct camera3_jpeg_blob));
    } else {
        ALOGE("ERROR: buffer size is 0 !");
    }

    return OK;
}

/**
 * makeJpeg
 *
 * Create and prefix the exif header to the encoded jpeg data.
 * Skip the SOI marker got from the JPEG encoding. Append the camera3_jpeg_blob
 * at the end of the buffer.
 *
 * \param package [IN] EncodePackage from the caller with encoded main and thumb
 *  buffers , Jpeg settings, and encoded sizes
 * \param dest [IN] The final output buffer to client
 *
 */
status_t JpegMaker::makeJpeg(ImgEncoder::EncodePackage & package, sp<CameraBuffer> dest)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(dest);
    unsigned int exifSize = 0;
    unsigned char* currentPtr = (unsigned char*)(package.jpegOut->data());

    // Copy the SOI marker
    unsigned int jSOISize = sizeof(JPEG_MARKER_SOI);
    memcpy(currentPtr, JPEG_MARKER_SOI, jSOISize);
    currentPtr += jSOISize;

    if (package.thumbOut.get()) {
        mExifMaker->setThumbnail((unsigned char *)package.thumbOut->data(),
                                 package.thumbSize);
        exifSize = mExifMaker->makeExif(&currentPtr);
    } else {
        // No thumb is not critical, we can continue with main picture image
        exifSize = mExifMaker->makeExif(&currentPtr);
        LOGW("Exif created without thumbnail stream!");
    }
    currentPtr += exifSize;

    // Since the jpeg got from libmix JPEG encoder start with SOI marker
    // and EXIF also have the SOI marker so need to remove SOI marker fom
    // the start of the jpeg data
    memcpy(currentPtr, reinterpret_cast<char *>(package.encodedData->data())+jSOISize,
           package.encodedDataSize-jSOISize);

    // save jpeg size at the end of file
    int finalSize = exifSize + package.encodedDataSize;
    int32_t jpegBufferSize = package.jpegOut->size();
    if (jpegBufferSize) {
        LOG1("actual jpeg size=%d, jpegbuffer size=%d", finalSize, jpegBufferSize);
        currentPtr = (unsigned char*)(package.jpegOut->data()) + jpegBufferSize
                - sizeof(struct camera3_jpeg_blob);
        struct camera3_jpeg_blob *blob = new struct camera3_jpeg_blob;
        blob->jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
        blob->jpeg_size = finalSize;
        memcpy(currentPtr, (void*)blob, sizeof(struct camera3_jpeg_blob));
        delete blob;
    } else {
        LOGE("ERROR: JPEG_MAX_SIZE is 0 !");
    }

    return NO_ERROR;
}

status_t JpegMaker::processExifSettings(const CameraMetadata  *settings,
                                        sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;

    status = processAwbSettings(*settings, metaData);
    status |= processGpsSettings(*settings, metaData);
    status |= processColoreffectSettings(*settings, metaData);
    status |= processScalerCropSettings(*settings, metaData);
    status |= processEvCompensationSettings(*settings, metaData);
    status |= processSceneSettings(*settings, metaData);

    return status;
}

/**
 * processJpegSettings
 *
 * Store JPEG settings to the exif metadata
 *
 * \param settings [IN] settings metadata of the request
 *
 */
status_t JpegMaker::processJpegSettings(ImgEncoder::EncodePackage & package,
                                    sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;

    if (metaData == NULL) {
        LOGE("MetaData struct not intialized");
        return UNKNOWN_ERROR;
    }

    const CameraMetadata *settings = package.settings;

    // get jpeg quality
    unsigned int tag = ANDROID_JPEG_QUALITY;
    camera_metadata_ro_entry entry = settings->find(tag);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];
        metaData->mJpegSetting.jpegQuality = value;
    }

    // get jpeg thumbnail quality
    tag = ANDROID_JPEG_THUMBNAIL_QUALITY;
    entry = settings->find(tag);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];
        metaData->mJpegSetting.jpegThumbnailQuality = value;
    }

    // get jpeg thumbnail size
    tag = ANDROID_JPEG_THUMBNAIL_SIZE;
    entry = settings->find(tag);
    if (entry.count == 2) {
        metaData->mJpegSetting.thumbWidth = entry.data.i32[0];
        metaData->mJpegSetting.thumbHeight = entry.data.i32[1];
    }

    // get jpeg orientation
    tag = ANDROID_JPEG_ORIENTATION;
    entry = settings->find(tag);
    if (entry.count == 1) {
       metaData->mJpegSetting.orientation = entry.data.i32[0];
    }

    LOG1("jpegQuality=%d,thumbQuality=%d,thumbW=%d,thumbH=%d,orientation=%d",
         metaData->mJpegSetting.jpegQuality,
         metaData->mJpegSetting.jpegThumbnailQuality,
         metaData->mJpegSetting.thumbWidth,
         metaData->mJpegSetting.thumbHeight,
         metaData->mJpegSetting.orientation);

    return status;
}

/*
 * This function will get GPS metadata from request setting
 */
status_t JpegMaker::processGpsSettings(const CameraMetadata &settings,
                                       sp<ExifMetaData> metadata)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    // get GPS
    unsigned int tag = ANDROID_JPEG_GPS_COORDINATES;
    camera_metadata_ro_entry entry = settings.find(tag);
    if (entry.count == 3) {
        metadata->mGpsSetting.latitude = entry.data.d[0];
        metadata->mGpsSetting.longitude = entry.data.d[1];
        metadata->mGpsSetting.altitude = entry.data.d[2];
    }
    LOG2("GPS COORDINATES(%f, %f, %f)", metadata->mGpsSetting.latitude,
         metadata->mGpsSetting.longitude, metadata->mGpsSetting.altitude);

    tag = ANDROID_JPEG_GPS_PROCESSING_METHOD;
    entry = settings.find(tag);
    if (entry.count > 0) {
        strncpy(metadata->mGpsSetting.gpsProcessingMethod,
                (char*)entry.data.u8, entry.count);
    }

    tag = ANDROID_JPEG_GPS_TIMESTAMP;
        entry = settings.find(tag);
    if (entry.count == 1) {
        metadata->mGpsSetting.gpsTimeStamp = entry.data.i64[0];
    }
    return status;
}

status_t JpegMaker::processAwbSettings(const CameraMetadata &settings,
                                       sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;

    if (metaData == NULL) {
        LOGE("MetaData struct not intialized");
        return UNKNOWN_ERROR;
    }

    // get awb mode
    unsigned int tag = ANDROID_CONTROL_AWB_MODE;
    camera_metadata_ro_entry entry = settings.find(tag);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];
        AwbMode newValue = \
            (value == ANDROID_CONTROL_AWB_MODE_INCANDESCENT) ?
                    CAM_AWB_MODE_WARM_INCANDESCENT : \
            (value == ANDROID_CONTROL_AWB_MODE_FLUORESCENT) ?
                    CAM_AWB_MODE_FLUORESCENT : \
            (value == ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT) ?
                    CAM_AWB_MODE_WARM_FLUORESCENT : \
            (value == ANDROID_CONTROL_AWB_MODE_DAYLIGHT) ?
                    CAM_AWB_MODE_DAYLIGHT : \
            (value == ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT) ?
                    CAM_AWB_MODE_CLOUDY : \
            (value == ANDROID_CONTROL_AWB_MODE_TWILIGHT) ?
                    CAM_AWB_MODE_SUNSET : \
            (value == ANDROID_CONTROL_AWB_MODE_SHADE) ?
                    CAM_AWB_MODE_SHADOW : \
            CAM_AWB_MODE_AUTO;

        metaData->awbMode = newValue;
    }
    LOG2("awb mode=%d", metaData->awbMode);
    return status;
}

status_t JpegMaker::processSceneSettings(const CameraMetadata &settings,
                                       sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_SCENE_MODE);
    if (entry.count != 1) {
        return OK;
    }

    uint8_t value = entry.data.u8[0];
    SceneMode newValue = CAM_AE_SCENE_MODE_AUTO;
    switch (value)
    {
    case ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY:
        newValue = CAM_AE_SCENE_MODE_AUTO;
        break;
    case ANDROID_CONTROL_SCENE_MODE_ACTION:
        newValue = CAM_AE_SCENE_MODE_SPORTS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_PORTRAIT:
        newValue = CAM_AE_SCENE_MODE_PORTRAIT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_LANDSCAPE:
        newValue = CAM_AE_SCENE_MODE_LANDSCAPE;
        break;
    case ANDROID_CONTROL_SCENE_MODE_NIGHT:
        newValue = CAM_AE_SCENE_MODE_NIGHT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT:
        newValue = CAM_AE_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_THEATRE:
        newValue = CAM_AE_SCENE_MODE_AUTO;
        break;
    case ANDROID_CONTROL_SCENE_MODE_BEACH:
        newValue = CAM_AE_SCENE_MODE_BEACH_SNOW;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SNOW:
        newValue = CAM_AE_SCENE_MODE_BEACH_SNOW;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SUNSET:
        newValue = CAM_AE_SCENE_MODE_SUNSET;
        break;
    case ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO:
        newValue = CAM_AE_SCENE_MODE_AUTO;
        break;
    case ANDROID_CONTROL_SCENE_MODE_FIREWORKS:
        newValue = CAM_AE_SCENE_MODE_FIREWORKS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SPORTS:
        newValue = CAM_AE_SCENE_MODE_SPORTS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_PARTY:
        newValue = CAM_AE_SCENE_MODE_PARTY;
        break;
    case ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT:
        newValue = CAM_AE_SCENE_MODE_CANDLELIGHT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_BARCODE:
        newValue = CAM_AE_SCENE_MODE_TEXT;
        break;
    default:
        newValue = CAM_AE_SCENE_MODE_AUTO;
        break;
    }
    metaData->mIa3ASetting.sceneMode = newValue;
    LOG2("scene mode=%d", metaData->mIa3ASetting.sceneMode);
    return status;
}


status_t JpegMaker::processColoreffectSettings(const CameraMetadata &settings,
                                               sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;

    if (metaData == NULL) {
        LOGE("MetaData struct not intialized");
        return UNKNOWN_ERROR;
    }

    // get effect mode
    unsigned int tag = ANDROID_CONTROL_EFFECT_MODE;
    camera_metadata_ro_entry entry = settings.find(tag);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];
        EffectMode newValue = \
            (value == ANDROID_CONTROL_EFFECT_MODE_MONO) ?
                    CAM_EFFECT_MONO : \
            (value == ANDROID_CONTROL_EFFECT_MODE_SEPIA) ?
                    CAM_EFFECT_SEPIA : \
            (value == ANDROID_CONTROL_EFFECT_MODE_NEGATIVE) ?
                    CAM_EFFECT_NEGATIVE : \
            CAM_EFFECT_NONE;
      metaData->effectMode = newValue;
    }

    if (metaData.get() != NULL)
        LOG2("effect mode=%d", metaData->effectMode);
    return status;
}

status_t JpegMaker::processScalerCropSettings(const CameraMetadata &settings,
                                               sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    CameraMetadata staticMeta;
    const int32_t sensorActiveArrayCount = 4;
    const uint32_t scalerCropCount = 4;
    int count = 0;

    if (metaData == NULL) {
        LOGE("MetaData struct not intialized");
        return BAD_VALUE;
    }


    staticMeta = PlatformData::getStaticMetadata(mCameraId);
    const int32_t *rangePtr = (int32_t *)MetadataHelper::getMetadataValues(
        staticMeta, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, TYPE_INT32, &count);

    camera_metadata_ro_entry entry = settings.find(ANDROID_SCALER_CROP_REGION);
    if (entry.count == scalerCropCount && count == sensorActiveArrayCount && rangePtr) {
        if (entry.data.i32[2] != 0 && entry.data.i32[3] != 0
            && rangePtr[2] != 0 && rangePtr[3] != 0) {
            metaData->zoomRatio = (rangePtr[2] * 100)/ entry.data.i32[2];

            LOG2("scaler width %d height %d, sensor active array width %d height : %d",
                entry.data.i32[2], entry.data.i32[3], rangePtr[2], rangePtr[3]);
        }
    }

    return status;
}

status_t JpegMaker::processEvCompensationSettings(const CameraMetadata &settings,
                                               sp<ExifMetaData> metaData)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    float stepEV = 1 / 3.0f;
    int32_t evCompensation = 0;

    if (metaData == NULL) {
        LOGE("MetaData struct not intialized");
        return BAD_VALUE;
    }

    camera_metadata_ro_entry entry =
            settings.find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
    if (entry.count != 1)
        return status;

    evCompensation = entry.data.i32[0];
    stepEV = PlatformData::getStepEv(mCameraId);
    // Fill the evBias
    if (metaData->aeConfig != NULL)
        metaData->aeConfig->evBias = evCompensation * stepEV;

    return status;
}


};  // namespace camera2
};  // namespace android
