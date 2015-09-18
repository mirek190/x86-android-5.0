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

#define LOG_TAG "CIFConfParser"

#include "LogHelper.h"
#include "CIFConfParser.h"
#include <v4l2device.h>

namespace android {
namespace camera2 {

CIFCameraCapInfo::CIFCameraCapInfo(SensorType type)
{
    mSensorType = type;
}

int CIFCameraCapInfo::getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat) const
{
    int index = mGfxHalToV4L2PixelFmtTable.indexOfKey(gfxHalFormat);
    if (index == NAME_NOT_FOUND) {
        LOGE("@%s: V4L2 format not found, check xml", __FUNCTION__);
        return V4L2_PIX_FMT_NV21;
    }
    return mGfxHalToV4L2PixelFmtTable.valueAt(index);
}

CIFConfParser::CIFConfParser()
{
    mCurrentDataField = FIELD_INVALID;
    mSensorIndex  = -1;
    getPSLDataFromXmlFile();
}

CIFConfParser::~CIFConfParser()
{
    while (!mCaps.isEmpty()) {
        delete mCaps[0];
    }

    for (unsigned int i = 0; i < mDefaultRequests.size(); i++) {
       if (mDefaultRequests[i])
            free_camera_metadata(mDefaultRequests[i]);
    }

    mDefaultRequests.clear();
}

CameraCapInfo* CIFConfParser::getCameraCapInfo(int cameraId)
{
    if (cameraId > MAX_CAMERAS) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }

    return mCaps[cameraId];
}

status_t CIFConfParser::addCamera(int cameraId)
{
    LOGD("%s: for camera %d", __FUNCTION__, cameraId);
    camera_metadata_t * emptyReq = NULL;
    CLEAR(emptyReq);

    SensorType type = SENSOR_TYPE_RAW;

    CIFCameraCapInfo * info = new CIFCameraCapInfo(type);
    if (!info) {
        LOGE("%s: No memory for CIFCameraCapInfo!", __FUNCTION__);
        return NO_MEMORY;
    }
    mCaps.push_back(info);

    for (int i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        mDefaultRequests.push_back(emptyReq);

    return NO_ERROR;
}

void CIFConfParser::checkField(const char *name, const char **atts)
{
    if (!strcmp(name, "Profiles")) {
        mSensorIndex = atoi(atts[1]);

        if (mSensorIndex > MAX_CAMERAS) {
            LOGE("ERROR @%s: bad camera id %d!", __FUNCTION__, mSensorIndex);
            return;
        }
        addCamera(mSensorIndex);
    } else if (!strcmp(name, "Hal_tuning_CIF")) {
        mCurrentDataField = FIELD_HAL_TUNING_CIF;
    } else if (!strcmp(name, "Sensor_info_CIF")) {
        mCurrentDataField = FIELD_SENSOR_INFO_CIF;
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
void CIFConfParser::startElement(void *userData, const char *name, const char **atts)
{
    CIFConfParser *cifParser = (CIFConfParser *)userData;

    if (cifParser->mCurrentDataField == FIELD_INVALID) {
        cifParser->checkField(name, atts);
        return;
    }
    LOG2("@%s: name:%s, for sensor %d", __FUNCTION__, name, cifParser->mSensorIndex);
    switch (cifParser->mCurrentDataField) {
        case FIELD_HAL_TUNING_CIF:
            cifParser->handleHALTuning(name, atts);
            break;
        case FIELD_SENSOR_INFO_CIF:
            cifParser->handleSensorInfo(name, atts);
            break;
        default:
            LOGE("@%s, line:%d, go to default handling", __FUNCTION__, __LINE__);
            break;
    }

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
void CIFConfParser::handleHALTuning(const char *name, const char **atts)
{
    LOG2("@%s", __FUNCTION__);

    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }

    CIFCameraCapInfo *info = mCaps[mSensorIndex];
    if (strcmp(name, "flipping") == 0) {
        info->mSensorFlipping = SENSOR_FLIP_OFF;
        if (strcmp(atts[0], "value") == 0 && strcmp(atts[1], "SENSOR_FLIP_H") == 0)
            info->mSensorFlipping |= SENSOR_FLIP_H;
        if (strcmp(atts[2], "value_v") == 0 && strcmp(atts[3], "SENSOR_FLIP_V") == 0)
            info->mSensorFlipping |= SENSOR_FLIP_V;
    } else if (strcmp(name, "gfxHalToV4L2PixelFmtTable.impl_defined") == 0) {
        info->mGfxHalToV4L2PixelFmtTable.add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                             getV4L2PixelFormatAsValue(atts[1]));
    } else if (strcmp(name, "gfxHalToV4L2PixelFmtTable.raw_sensor") == 0) {
        info->mGfxHalToV4L2PixelFmtTable.add(HAL_PIXEL_FORMAT_RAW_SENSOR,
                                             getV4L2PixelFormatAsValue(atts[1]));
    } else if (strcmp(name, "gfxHalToV4L2PixelFmtTable.ycbcr_420_888") == 0) {
        info->mGfxHalToV4L2PixelFmtTable.add(HAL_PIXEL_FORMAT_YCbCr_420_888,
                                             getV4L2PixelFormatAsValue(atts[1]));
    }
}

/**
 * This function will handle all the parameters describing characteristic of
 * the sensor itself
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void CIFConfParser::handleSensorInfo(const char *name, const char **atts)
{
    LOG2("@%s", __FUNCTION__);
    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }

    CIFCameraCapInfo *info = mCaps[mSensorIndex];

    if (strcmp(name, "sensorType") == 0) {
        info->mSensorType = ((strcmp(atts[1], "SENSOR_TYPE_RAW") == 0) ? SENSOR_TYPE_RAW : SENSOR_TYPE_SOC);
    }  else if (strcmp(name, "exposure.sync") == 0) {
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
    } else if (strcmp(name, "statistics.initialSkip") == 0) {
        info->mStatisticsInitialSkip = atoi(atts[1]);
    } else if (strcmp(name, "frame.initialSkip") == 0) {
        info->mFrameInitialSkip = atoi(atts[1]);
    } else if (strcmp(name, "NVMFormat") == 0) {
        info->mNVMFormat = atts[1];
    }
}

/*
 * Helper function for converting string to int for the
 * V4L2 pixel formats requested for media controller set-up.
 * pixel format accepted by the media ctl entities
 */
int CIFConfParser::getV4L2PixelFormatAsValue(const char* format)
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

/**
 * the callback function of the libexpat for handling of one element end
 *
 * When it comes to the end of one element. This function will be called.
 *
 * \param userData: the pointer we set by the function XML_SetUserData.
 * \param name: the element's name.
 */
void CIFConfParser::endElement(void *userData, const char *name)
{
    CIFConfParser *cifParser = (CIFConfParser *)userData;
    CIFCameraCapInfo *info = cifParser->mCaps[cifParser->mSensorIndex];
    if (!strcmp(name, "Profiles")) {
        cifParser->mCurrentDataField = FIELD_INVALID;
    } else if (!strcmp(name, "Hal_tuning_CIF")
            || !strcmp(name, "Sensor_info_CIF")) {
        cifParser->mCurrentDataField = FIELD_INVALID;
   }
    return;
}

/**
 * Get camera configuration from xml file
 *
 * The camera setting is stored inside this IPU4CameraCapInfo class.
 *
 */
void CIFConfParser::getPSLDataFromXmlFile(void)
{
    int done;
    void *pBuf = NULL;
    FILE *fp = NULL;
    LOG1("@%s", __FUNCTION__);

    static const char *defaultXmlFile = "/etc/camera3_profiles.xml";

    fp = ::fopen(defaultXmlFile, "r");
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

camera_metadata_t* CIFConfParser::constructDefaultMetadata(int cameraId, int requestTemplate)
{
    LOG2("@%s: %d", __FUNCTION__, requestTemplate);
    if (requestTemplate >= CAMERA3_TEMPLATE_COUNT) {
        LOGE("ERROR @%s: bad template %d", __FUNCTION__, requestTemplate);
        return NULL;
    }

    int index = cameraId * CAMERA3_TEMPLATE_COUNT + requestTemplate;
    camera_metadata_t * req = mDefaultRequests[index];

    if (req)
        return req;

    camera_metadata_t * meta = NULL;
    meta = allocate_camera_metadata(DEFAULT_ENTRY_CAP, DEFAULT_DATA_CAP);
    if (meta == NULL) {
        LOGE("ERROR @%s: Allocate memory failed", __FUNCTION__);
        return NULL;
    }

    int64_t bogusValue = 0; // 8 bytes of bogus
    int64_t bogusValueArray[] = {0, 0, 0, 0, 0}; // 40 bytes of bogus

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
    default:
        intent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }

#define TAGINFO(tag, data) \
    add_camera_metadata_entry(meta, tag, &data, 1)
#define TAGINFO_ARRAY(tag, data, count) \
    add_camera_metadata_entry(meta, tag, data, count)

    TAGINFO(ANDROID_CONTROL_CAPTURE_INTENT, intent);

    uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    TAGINFO(ANDROID_CONTROL_MODE, controlMode);
    TAGINFO(ANDROID_CONTROL_EFFECT_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_SCENE_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, bogusValue);
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    TAGINFO(ANDROID_CONTROL_AE_MODE, aeMode);
    TAGINFO(ANDROID_CONTROL_AE_LOCK, bogusValue);
    TAGINFO_ARRAY(ANDROID_CONTROL_AF_REGIONS, bogusValueArray, 5);
    TAGINFO(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, bogusValue);

    TAGINFO_ARRAY(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, bogusValueArray, 2);

    TAGINFO(ANDROID_CONTROL_AE_ANTIBANDING_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_AWB_MODE, bogusValue);
    TAGINFO(ANDROID_CONTROL_AWB_LOCK, bogusValue);
    TAGINFO_ARRAY(ANDROID_CONTROL_AWB_REGIONS, bogusValueArray, 5);
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

    mDefaultRequests.editItemAt(index) = meta;
    return meta;
}



} // namespace camera2
} // namespace android
