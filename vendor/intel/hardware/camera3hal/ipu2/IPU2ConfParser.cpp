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

#define LOG_TAG "IPU2ConfParser"

#include "LogHelper.h"
#include "IPU2ConfParser.h"
#include <v4l2device.h>

namespace android {
namespace camera2 {

IPU2ConfParser::IPU2ConfParser()
{
    mCaps.clear();
}

void IPU2ConfParser::ParserXML(const char* profilePath)
{
    mCurrentDataField = FIELD_INVALID;
    mSensorIndex  = -1;
    getPSLDataFromXmlFile(profilePath);
    // Uncomment to display all the parsed values
    //dump();
}

IPU2ConfParser::~IPU2ConfParser()
{
    for (unsigned int i = 0; !mCaps.isEmpty(); i++) {
        delete mCaps.valueFor(i);
        mCaps.removeItem(i);
    }

    for (unsigned int i = 0; i < mDefaultRequests.size(); i++) {
        Vector<camera_metadata_t*> cm = mDefaultRequests.valueFor(i);
        for (unsigned int n = 0; n < cm.size(); n++)
            free_camera_metadata(cm[n]);
        cm.clear();
    }

    mDefaultRequests.clear();
}

CameraCapInfo* IPU2ConfParser::getCameraCapInfo(int cameraId)
{
    if (cameraId > MAX_CAMERAS) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }

    return mCaps.valueFor(cameraId);
}

camera_metadata_t* IPU2ConfParser::constructDefaultMetadata(int cameraId, int requestTemplate)
{
    LOG2("@%s: %d", __FUNCTION__, requestTemplate);
    if (requestTemplate >= CAMERA3_TEMPLATE_COUNT) {
        LOGE("ERROR @%s: bad template %d", __FUNCTION__, requestTemplate);
        return NULL;
    }

    //int index = cameraId * CAMERA3_TEMPLATE_COUNT + requestTemplate;
    if (mDefaultRequests.indexOfKey(cameraId) < 0) return NULL;

    camera_metadata_t * req = (mDefaultRequests.valueFor(cameraId))[requestTemplate];

    if (req) {
        //The metadata contents should be handled by service, so here do check
        int entryCount = get_camera_metadata_entry_count(req);
        int dataCount = get_camera_metadata_data_count(req);
        if (entryCount != 0 && dataCount != 0) {
            return req;
        } else {
            free_camera_metadata(req);
        }
    }
    camera_metadata_t * meta = NULL;
    meta = allocate_camera_metadata(DEFAULT_ENTRY_CAP, DEFAULT_DATA_CAP);
    if (meta == NULL) {
        LOGE("ERROR @%s: Allocate memory failed", __FUNCTION__);
        return NULL;
    }

    int64_t bogusValue = 0; // 8 bytes of bogus
    int64_t bogusValueArray[] = {0, 0, 0, 0, 0}; // 32 bytes of bogus

    uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    uint8_t intent = 0;
    switch (requestTemplate) {
    case CAMERA3_TEMPLATE_PREVIEW:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        break;
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
       break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        break;
    case CAMERA3_VENDOR_TEMPLATE_START:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    case CAMERA3_TEMPLATE_MANUAL:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_MANUAL;
        break;
    default:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }

#define TAGINFO(tag, data) \
    add_camera_metadata_entry(meta, tag, &data, 1)
#define TAGINFO_ARRAY(tag, data, count) \
    add_camera_metadata_entry(meta, tag, data, count)

    TAGINFO(ANDROID_CONTROL_CAPTURE_INTENT, intent);

    TAGINFO(ANDROID_CONTROL_EFFECT_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_SCENE_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_AE_LOCK, bogusValue);
    TAGINFO(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, bogusValue);

    TAGINFO_ARRAY(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, bogusValueArray, 2);

    TAGINFO(ANDROID_CONTROL_AE_ANTIBANDING_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_AWB_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_AWB_LOCK, bogusValue);
    TAGINFO(ANDROID_CONTROL_AWB_STATE, bogusValue);
    TAGINFO(ANDROID_CONTROL_AF_MODE, bogusValue);

    TAGINFO(ANDROID_FLASH_MODE, bogusValue);

    TAGINFO(ANDROID_LENS_FOCUS_DISTANCE, bogusValue);

    TAGINFO(ANDROID_REQUEST_TYPE, requestType);
    TAGINFO(ANDROID_REQUEST_METADATA_MODE, bogusValue);
    TAGINFO(ANDROID_REQUEST_FRAME_COUNT, bogusValue);

    TAGINFO_ARRAY(ANDROID_SCALER_CROP_REGION, bogusValueArray, 4);

    TAGINFO(ANDROID_STATISTICS_FACE_DETECT_MODE, bogusValue);

    TAGINFO(ANDROID_LENS_FOCAL_LENGTH, bogusValue);
    TAGINFO_ARRAY(ANDROID_CONTROL_AE_REGIONS, bogusValueArray, 5);
    TAGINFO(ANDROID_SENSOR_EXPOSURE_TIME, bogusValue);
    TAGINFO(ANDROID_SENSOR_SENSITIVITY, bogusValue);

#undef TAGINFO
#undef TAGINFO_ARRAY

    int entryCount = get_camera_metadata_entry_count(meta);
    int dataCount = get_camera_metadata_data_count(meta);
    LOG2("%s: Real metadata entry count %d, data count %d", __FUNCTION__,
        entryCount, dataCount);
    if ((entryCount > DEFAULT_ENTRY_CAP - ENTRY_RESERVED)
        || (dataCount > DEFAULT_DATA_CAP - DATA_RESERVED))
        ALOGW("%s: Need more memory, now entry %d (%d), data %d (%d)", __FUNCTION__,
        entryCount, DEFAULT_ENTRY_CAP, dataCount, DEFAULT_DATA_CAP);

    (mDefaultRequests.editValueFor(cameraId)).editItemAt(requestTemplate) = meta;
    return meta;
}

status_t IPU2ConfParser::addCamera(int cameraId)
{
    LOGE("%s: for camera %d", __FUNCTION__, cameraId);
    camera_metadata_t * emptyReq = NULL;
    CLEAR(emptyReq);

    SensorType type = (cameraId == BACK_CAMERA_ID) ? SENSOR_TYPE_RAW : SENSOR_TYPE_SOC;

    IPU2CameraCapInfo * info = new IPU2CameraCapInfo(type);
    if (!info) {
        LOGE("%s: No memory for IPU2CameraCapInfo!", __FUNCTION__);
        return NO_MEMORY;
    }

    if (mCaps.indexOfKey(cameraId) > 0) {
        delete mCaps.valueFor(cameraId);
        mCaps.removeItem(cameraId);
    }

    mCaps.add(cameraId, info);

    Vector<camera_metadata_t *> cm;

    if (mDefaultRequests.indexOfKey(cameraId) > 0) {
        cm = mDefaultRequests.valueFor(cameraId);
        for (unsigned int n = 0; n < cm.size(); n++) free_camera_metadata(cm[n]);
        cm.clear();
        mDefaultRequests.removeItem(cameraId);
    }

    for (int i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        cm.push_back(emptyReq);

    mDefaultRequests.add(cameraId, cm);

    return NO_ERROR;
}

// String format: "%d,%d,...,%d", or "%dx%d,...,%dx%d", or "(%d,...,%d),(%d,...,%d)
int convertXmlData(void * dest, int destMaxNum, const char * src, int type)
{
    int index = 0;
    char * endPtr = NULL;
    union {
        uint8_t * u8;
        int32_t * i32;
        int64_t * i64;
        float * f;
        double * d;
    } data;
    data.u8 = (uint8_t *)dest;

    do {
        switch (type) {
        case TYPE_BYTE:
            data.u8[index] = (char)strtol(src, &endPtr, 0);
            LOG2("    - %d -", data.u8[index]);
            break;
        case TYPE_INT32:
        case TYPE_RATIONAL:
            data.i32[index] = strtol(src, &endPtr, 0);
            LOG2("    - %d -", data.i32[index]);
            break;
        case TYPE_INT64:
            data.i64[index] = strtol(src, &endPtr, 0);
            LOG2("    - %lld -", data.i64[index]);
            break;
        case TYPE_FLOAT:
            data.f[index] = strtof(src, &endPtr);
            LOG2("    - %8.3f -", data.f[index]);
            break;
        case TYPE_DOUBLE:
            data.d[index] = strtof(src, &endPtr);
            LOG2("    - %8.3f -", data.d[index]);
            break;
        }
        index++;
        if (endPtr != NULL) {
            if (*endPtr == ',' || *endPtr == 'x')
                src = endPtr + 1;
            else if (*endPtr == ')')
                src = endPtr + 3;
            else if (*endPtr == 0)
                break;
        }
    } while (index < destMaxNum);

    return (type == TYPE_RATIONAL) ? ((index + 1) / 2) : index;
}

/**
 * This function will handle all the HAL parameters that are different
 * depending on the camera
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void IPU2ConfParser::handleHALTuning(const char *name, const char **atts)
{
    LOG2("@%s", __FUNCTION__);

    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }

    IPU2CameraCapInfo * info = mCaps.valueFor(mSensorIndex);
    if (strcmp(name, "flipping") == 0) {
        info->mSensorFlipping = SENSOR_FLIP_OFF;
        if (strcmp(atts[0], "value") == 0 && strcmp(atts[1], "SENSOR_FLIP_H") == 0)
            info->mSensorFlipping |= SENSOR_FLIP_H;
        if (strcmp(atts[2], "value_v") == 0 && strcmp(atts[3], "SENSOR_FLIP_V") == 0)
            info->mSensorFlipping |= SENSOR_FLIP_V;
    } else if (strcmp(name, "maxContinuousRawRingBuffer") == 0) {
        info->mMaxContinuousRawRingBuffer = atoi(atts[1]);
    } else if (strcmp(name, "shutterLagCompensationMs") == 0) {
        info->mShutterLagCompensationMs = atoi(atts[1]);
    } else if (strcmp(name, "subDevName") == 0) {
        info->mSubDeviceName = atts[1];
    } else if (strcmp(name, "fileInject") == 0) {
        info->mSupportFileInject = ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "enableIspPerframeSettings")== 0) {
        info->mEnableIspPerframeSettings =
            ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "streamConfigEnableRawLock")== 0) {
        info->mStreamConfigEnableRawLock =
            ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "pipelineDepth") == 0) {
        info->mPipelineDepthCount =
             convertXmlData(info->mPipelineDepths, PIPELINE_DEPTH_NUM, atts[1], TYPE_BYTE);
    } else if (strcmp(name, "gfxHalToV4L2PixelFmtTable.impl_defined") == 0) {
        info->mGfxHalToV4L2PixelFmtTable.add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                  getPixelFormatAsValue(atts[1]));
    } else if (strcmp(name, "gfxHalToV4L2PixelFmtTable.raw_sensor") == 0) {
        info->mGfxHalToV4L2PixelFmtTable.add(HAL_PIXEL_FORMAT_RAW_SENSOR,
                                  getPixelFormatAsValue(atts[1]));
    } else if (strcmp(name, "gfxHalToV4L2PixelFmtTable.ycbcr_420_888") == 0) {
        info->mGfxHalToV4L2PixelFmtTable.add(HAL_PIXEL_FORMAT_YCbCr_420_888,
                                  getPixelFormatAsValue(atts[1]));
    }
}

// String format: "%d,%d,...,%d", or "%dx%d,...,%dx%d", or "(%d,...,%d),(%d,...,%d)

/**
 * This function will handle all the parameters describing characteristic of
 * the sensor itself
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void IPU2ConfParser::handleSensorInfo(const char *name, const char **atts)
{
    LOG2("@%s", __FUNCTION__);
    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }
    IPU2CameraCapInfo * info = mCaps.valueFor(mSensorIndex);

    if (strcmp(name, "sensorType") == 0) {
        info->mSensorType = ((strcmp(atts[1], "SENSOR_TYPE_RAW") == 0) ? SENSOR_TYPE_RAW : SENSOR_TYPE_SOC);
    } else if (strcmp(name, "sensorLayout") == 0) {
        info->mSensorLayout = atoi(atts[1]);
    } else if (strcmp(name, "flipping") == 0) {
        info->mSensorFlipping = SENSOR_FLIP_OFF;
        if (strcmp(atts[0], "value") == 0 && strcmp(atts[1], "SENSOR_FLIP_H") == 0)
            info->mSensorFlipping |= SENSOR_FLIP_H;
        if (strcmp(atts[2], "value_v") == 0 && strcmp(atts[3], "SENSOR_FLIP_V") == 0)
            info->mSensorFlipping |= SENSOR_FLIP_V;
    } else if (strcmp(name, "exposure.sync") == 0) {
        info->mExposureSync = ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "gain.lag") == 0) {
        info->mGainLag = atoi(atts[1]);
    } else if (strcmp(name, "exposure.lag") == 0) {
        info->mExposureLag = atoi(atts[1]);
    } else if (strcmp(name, "gainExposure.compensation") == 0) {
        info->mGainExposureComp = ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "fov") == 0) {
        info->mFov[0] = atof(atts[1]);
        info->mFov[1] = atof(atts[3]);
    } else if (strcmp(name, "zoomDigital.max") == 0) {
        info->mZoomDigitalMax = atoi(atts[1]);
    } else if (strcmp(name, "ispVamemType") == 0) {
        info->mIspVamemType = atoi(atts[1]);
    } else if (strcmp(name, "supportedHighSpeedResolutionFps") == 0) {
         info->mSupportedHighSpeedResolutionFps = atts[1];
    } else if (strcmp(name, "maxHighSpeedDvsResolution") == 0) {
         info->mMaxHighSpeedDvsResolution = atts[1];
    } else if (strcmp(name, "statistics.initialSkip") == 0) {
        info->mStatisticsInitialSkip = atoi(atts[1]);
    } else if (strcmp(name, "frame.initialSkip") == 0) {
        info->mFrameInitialSkip = atoi(atts[1]);
    } else if (strcmp(name, "perFrameSettings") == 0) {
        info->mSupportPerFrameSettings =
           ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "NVMFormat") == 0) {
        info->mNVMFormat = atts[1];
    } else if (strcmp(name, "postviewSizes") == 0) {
        info->mPostviewSizeCount =
            convertXmlData(info->mPostviewSizes, POSTVIEW_SIZES_NUM, atts[1], TYPE_INT32);
    } else if (strcmp(name, "sdvVideoSizes") == 0) {
        info->mSdvVideoSizeCount =
            convertXmlData(info->mSdvVideoSizes, SDV_VIDEO_SIZES_NUM, atts[1], TYPE_INT32);
    } else if (strcmp(name, "minDepthOfReqQ") == 0) {
        info->mMinDepthOfReqQ = atoi(atts[1]);
    } else if (strcmp(name, "maxBurstLength") == 0) {
        info->mMaxBurstLength = atoi(atts[1]);
    } else if (strcmp(name, "isoAnalogGain1") == 0) {
        info->mISOAnalogGain1 = atoi(atts[1]);
    } else if (strcmp(name, "supportSensorEmbeddedMetadata") == 0) {
        info->mSupportSensorEmbeddedMetadata =
           ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "startExposureTime") == 0) {
        info->mStartExposureTime = atoi(atts[1]);
    } else if (strcmp(name, "supportSdv") == 0) {
        info->mSupportSdv =
           ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "supportDynamicSdv") == 0) {
        info->mSupportDynamicSdv =
           ((strcmp(atts[1], "true") == 0) ? true : false);
    }

}

/*
 * Helper function for converting string to int for the
 * V4L2 pixel formats requested for media controller set-up.
 * pixel format accepted by the media ctl entities
 */
int IPU2ConfParser::getPixelFormatAsValue(const char* format)
{
    /* for subdevs */
    if (!strcmp(format, "V4L2_MBUS_FMT_SBGGR10_1X10")) {
        return V4L2_MBUS_FMT_SBGGR10_1X10;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SGBRG10_1X10")) {
        return V4L2_MBUS_FMT_SGBRG10_1X10;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SGRBG10_1X10")) {
        return V4L2_MBUS_FMT_SGRBG10_1X10;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SRGGB10_1X10")) {
        return V4L2_MBUS_FMT_SRGGB10_1X10;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SBGGR8_1X8")) {
        return V4L2_MBUS_FMT_SBGGR8_1X8;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SGBRG8_1X8")) {
        return V4L2_MBUS_FMT_SGBRG8_1X8;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SGRBG8_1X8")) {
        return V4L2_MBUS_FMT_SGRBG8_1X8;
    } else if (!strcmp(format, "V4L2_MBUS_FMT_SRGGB8_1X8")) {
        return V4L2_MBUS_FMT_SRGGB8_1X8;
    /* for nodes */
    } else if (!strcmp(format, "V4L2_PIX_FMT_SBGGR10")) {
        return V4L2_PIX_FMT_SBGGR10;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SGBRG10")) {
        return V4L2_PIX_FMT_SGBRG10;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SGRBG10")) {
        return V4L2_PIX_FMT_SGRBG10;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SRGGB10")) {
        return V4L2_PIX_FMT_SRGGB10;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SBGGR8")) {
        return V4L2_PIX_FMT_SBGGR8;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SGBRG8")) {
        return V4L2_PIX_FMT_SGBRG8;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SGRBG8")) {
        return V4L2_PIX_FMT_SGRBG8;
    } else if (!strcmp(format, "V4L2_PIX_FMT_SRGGB8")) {
        return V4L2_PIX_FMT_SRGGB8;
    /* for stream formats */
    } else if (!strcmp(format, "V4L2_PIX_FMT_NV12")) {
        return V4L2_PIX_FMT_NV12;
    } else if (!strcmp(format, "V4L2_PIX_FMT_JPEG")) {
        return V4L2_PIX_FMT_JPEG;
    } else if (!strcmp(format, "V4L2_PIX_FMT_YUV420")) {
        return V4L2_PIX_FMT_YUV420;
    } else if (!strcmp(format, "V4L2_PIX_FMT_NV21")) {
        return V4L2_PIX_FMT_NV21;
    } else if (!strcmp(format, "V4L2_PIX_FMT_YUV422P")) {
        return V4L2_PIX_FMT_YUV422P;
    } else if (!strcmp(format, "V4L2_PIX_FMT_YVU420")) {
        return V4L2_PIX_FMT_YVU420;
    } else if (!strcmp(format, "V4L2_PIX_FMT_YUYV")) {
        return V4L2_PIX_FMT_YUYV;
    } else if (!strcmp(format, "V4L2_PIX_FMT_RGB565")) {
        return V4L2_PIX_FMT_RGB565;
    } else if (!strcmp(format, "V4L2_PIX_FMT_RGB24")) {
        return V4L2_PIX_FMT_RGB24;
    } else if (!strcmp(format, "V4L2_PIX_FMT_BGR32")) {
        return V4L2_PIX_FMT_BGR32;
    } else {
        LOGE("%s, Unknown Pixel Format (%s)", __FUNCTION__, format);
        return -1;
    }
}

void IPU2ConfParser::checkField(const char *name, const char **atts)
{
    if (!strcmp(name, "Profiles")) {
        mSensorIndex = atoi(atts[1]);

        if (mSensorIndex > MAX_CAMERAS) {
            LOGE("ERROR @%s: bad camera id %d!", __FUNCTION__, mSensorIndex);
            return;
        }
        addCamera(mSensorIndex);
    } else if (!strcmp(name, "Hal_tuning_IPU2")) {
        mCurrentDataField = FIELD_HAL_TUNING_IPU2;
    } else if (!strcmp(name, "Sensor_info_IPU2")) {
        mCurrentDataField = FIELD_SENSOR_INFO_IPU2;
    }
    LOG1("@%s: name:%s, field %d", __FUNCTION__, name, mCurrentDataField);
    return;
}


/**
 * the callback function of the libexpat for handling of one element start
 *
 * When it comes to the start of one element. This function will be called.
 *
 * \param userData: the pointer we set by the function XML_SetUserData.
 * \param name: the element's name.
 */
void IPU2ConfParser::startElement(void *userData, const char *name, const char **atts)
{
    IPU2ConfParser *ipu2Parser = (IPU2ConfParser *)userData;

    if (ipu2Parser->mCurrentDataField == FIELD_INVALID) {
        ipu2Parser->checkField(name, atts);
        return;
    }
    LOG2("@%s: name:%s, for sensor %d", __FUNCTION__, name, ipu2Parser->mSensorIndex);

    switch (ipu2Parser->mCurrentDataField) {
        case FIELD_HAL_TUNING_IPU2:
            ipu2Parser->handleHALTuning(name, atts);
            break;
        case FIELD_SENSOR_INFO_IPU2:
            ipu2Parser->handleSensorInfo(name, atts);
            break;
        default:
            LOGE("@%s, line:%d, go to default handling", __FUNCTION__, __LINE__);
            break;
    }
}

/**
 * the callback function of the libexpat for handling of one element end
 *
 * When it comes to the end of one element. This function will be called.
 *
 * \param userData: the pointer we set by the function XML_SetUserData.
 * \param name: the element's name.
 */
void IPU2ConfParser::endElement(void *userData, const char *name)
{
    IPU2ConfParser *ipu2Parser = (IPU2ConfParser *)userData;
    if (!strcmp(name, "Profiles")) {
        ipu2Parser->mCurrentDataField = FIELD_INVALID;
    } else if (!strcmp(name, "Hal_tuning_IPU2")
             || !strcmp(name, "Sensor_info_IPU2")) {
        ipu2Parser->mCurrentDataField = FIELD_INVALID;
    }
    return;
}

/**
 * Get camera configuration from xml file
 *
 * The camera setting is stored inside this IPU4CameraCapInfo class.
 *
 */
void IPU2ConfParser::getPSLDataFromXmlFile(const char* profilePath)
{
    int done;
    void *pBuf = NULL;
    FILE *fp = NULL;
    LOG1("@%s", __FUNCTION__);

    fp = ::fopen(profilePath, "r");
    if (NULL == fp) {
        LOGE("@%s, line:%d, fp is NULL", __FUNCTION__, __LINE__);
        return;
    }

    XML_Parser parser = ::XML_ParserCreate(NULL);
    if (NULL == parser) {
        LOGE("@%s, line:%d, parser is NULL", __FUNCTION__, __LINE__);
        goto exit;
    }
    ::XML_SetUserData(parser, this);
    ::XML_SetElementHandler(parser, startElement, endElement);

    pBuf = malloc(mBufSize);
    if (NULL == pBuf) {
        LOGE("@%s, line:%d, pBuf is NULL", __func__, __LINE__);
        goto exit;
    }

    do {
        int len = (int)::fread(pBuf, 1, mBufSize, fp);
        if (!len) {
            if (ferror(fp)) {
                clearerr(fp);
                goto exit;
            }
        }
        done = len < mBufSize;
        if (XML_Parse(parser, (const char *)pBuf, len, done) == XML_STATUS_ERROR) {
            LOGE("@%s, line:%d, XML_Parse error", __func__, __LINE__);
            goto exit;
        }
    } while (!done);

exit:
    if (parser)
        ::XML_ParserFree(parser);
    if (pBuf)
        free(pBuf);
    if (fp)
    ::fclose(fp);
}

void IPU2ConfParser::dumpHalTuningSection(int cameraId)
{
    LOGD("@%s", __FUNCTION__);

    IPU2CameraCapInfo * info = mCaps.valueFor(cameraId);

    LOGD("element name: flipping, element value = %d", info->mSensorFlipping);
    LOGD("element name: maxContinuousRawRingBuffer, element value = %d", info->mMaxContinuousRawRingBuffer);
    LOGD("element name: shutterLagCompensationMs, element value = %d", info->mShutterLagCompensationMs);
    LOGD("element name: enableIspPerframeSettings, element value = %d", info->mEnableIspPerframeSettings);
    LOGD("element name: halToV4L2Table.impl_defined, element value = %s",
         v4l2Fmt2Str(info->mGfxHalToV4L2PixelFmtTable.valueFor(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)));
    LOGD("element name: halToV4L2Table.raw_sensor, element value = %s",
         v4l2Fmt2Str(info->mGfxHalToV4L2PixelFmtTable.valueFor(HAL_PIXEL_FORMAT_RAW_SENSOR)));
    LOGD("element name: halToV4L2Table.ycbcr_420_888, element value = %s",
         v4l2Fmt2Str(info->mGfxHalToV4L2PixelFmtTable.valueFor(HAL_PIXEL_FORMAT_YCbCr_420_888)));

}

void IPU2ConfParser::dumpSensorInfoSection(int cameraId){
    LOGD("@%s", __FUNCTION__);

    IPU2CameraCapInfo * info = mCaps.valueFor(cameraId);

    LOGD("element name: sensorType, element value = %d", info->mSensorType);
    LOGD("element name: gain.lag, element value = %d", info->mGainLag);
    LOGD("element name: exposure.lag, element value = %d", info->mExposureLag);
    LOGD("element name: fov, element value = %f, %f", info->mFov[0], info->mFov[1]);
    LOGD("element name: zoomDigital.max, element value = %d", info->mZoomDigitalMax);
    LOGD("element name: ispVamemType, element value = %d", info->mIspVamemType);
    LOGD("element name: statistics.initialSkip, element value = %d", info->mStatisticsInitialSkip);
    LOGD("element name: frame.initialSkip, element value = %d", info->mFrameInitialSkip);
    LOGD("element name: perFrameSettings, element value = %d", info->mSupportPerFrameSettings);
    LOGD("element name: postviewSizes, element values");
    for (int i = 0; i < info->mPostviewSizeCount; i += 2 )
        LOGD("%d x %d", info->mPostviewSizes[i], info->mPostviewSizes[i + 1]);
    LOGD("element name: sdvVideoSizes, element values");
    for (int i = 0; i < info->mSdvVideoSizeCount; i += 2 )
        LOGD("%d x %d", info->mSdvVideoSizes[i], info->mSdvVideoSizes[i + 1]);

    LOGD("element name: supportedHighSpeedResolutionFps, element value = %s", info->mSupportedHighSpeedResolutionFps.string());
    LOGD("element name: minDepthOfReqQ, element value = %d", info->mMinDepthOfReqQ);
    LOGD("element name: maxBurstLength, element value = %d", info->mMaxBurstLength);
    LOGD("element name: isoAnalogGain1, element value = %d", info->mISOAnalogGain1);

}

// To be modified when new elements or sections are added
// Use LOGD for traces to be visible
void IPU2ConfParser::dump()
{
    LOGD("===========================@%s======================", __FUNCTION__);
    for (unsigned int i = 0; i < mCaps.size(); i++) {
        dumpHalTuningSection(i);
        dumpSensorInfoSection(i);
    }

    LOGD("===========================end======================");
}


IPU2CameraCapInfo::IPU2CameraCapInfo(SensorType type)
{
    mSensorType = type;

    mSensorFlipping = 0;
    mSensorLayout = 0;

    mMaxVFPPLimitedResolutions[0] = 1280;
    mMaxVFPPLimitedResolutions[1] = 720;

    if (type == SENSOR_TYPE_RAW) {
        mMaxContinuousRawRingBuffer = 6;
        mShutterLagCompensationMs = 40;
    } else {
        mMaxContinuousRawRingBuffer = 0;
        mShutterLagCompensationMs = 0;
    }
    mCVFUnsupportedSnapshotResolutions.clear();

    mMinDepthOfReqQ = 4;
    mMaxBurstLength = 4;

    mSubDeviceName = "/dev/v4l-subdev8";
    mSupportFileInject = false;
    mStreamConfigEnableRawLock = true; //If not defined in camera3.profiles.xml, default is true.

    mExposureSync = false;
    mGainExposureComp = false;
    mGainLag = -1;
    mExposureLag = -1;
    mZoomDigitalMax = -1;
    mIspVamemType = -1;
    mStatisticsInitialSkip = -1;
    mFrameInitialSkip = -1;
    mSupportPerFrameSettings = false;
    mPipelineDepthCount = -1;
    mPostviewSizeCount = -1;
    mSdvVideoSizeCount = -1;
    mEnableIspPerframeSettings = false;
    mISOAnalogGain1 = -1;
    mSupportSensorEmbeddedMetadata = false;
    mStartExposureTime = -1;
    mSupportSdv = false;
    mSupportDynamicSdv = false;

    CLEAR(mFov);
    CLEAR(mPipelineDepths);
    CLEAR(mPostviewSizes);
    CLEAR(mSdvVideoSizes);
}

bool IPU2CameraCapInfo::isPreviewSizeSupportedByVFPP(int w, int h) const
{
    return ((w <= mMaxVFPPLimitedResolutions[0]) && (h <= mMaxVFPPLimitedResolutions[1]));
}

bool IPU2CameraCapInfo::isOfflineCaptureSupported(void) const
{
    return (mMaxContinuousRawRingBuffer > 0);
}

bool IPU2CameraCapInfo::snapshotResolutionSupportedByCVF(int w,int h) const
{
    Vector<Size>::const_iterator it = mCVFUnsupportedSnapshotResolutions.begin();
    for (;it != mCVFUnsupportedSnapshotResolutions.end(); ++it) {
        if (it->width == w && it->height == h) {
            return false;
        }
    }
    return true;
}

const uint8_t * IPU2CameraCapInfo::getPilelineDepths(int * count) const
{
    *count = mPipelineDepthCount;
    return mPipelineDepths;
}

const int * IPU2CameraCapInfo::getPostViewSizes(int * count) const
{
    *count = mPostviewSizeCount;
    return mPostviewSizes;
}

const int * IPU2CameraCapInfo::getSdvVideoSizes(int * count) const
{
    *count = mSdvVideoSizeCount;
    return mSdvVideoSizes;
}

int IPU2CameraCapInfo::getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat) const
{
    int index = mGfxHalToV4L2PixelFmtTable.indexOfKey(gfxHalFormat);
    if (index == NAME_NOT_FOUND) {
        LOGE("@%s: V4L2 format not found, check xml", __FUNCTION__);
        return V4L2_PIX_FMT_NV12;
    }
    return mGfxHalToV4L2PixelFmtTable.valueAt(index);
}

} // namespace camera2
} // namespace android
