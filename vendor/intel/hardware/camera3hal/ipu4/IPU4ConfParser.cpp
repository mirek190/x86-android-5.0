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

#define LOG_TAG "IPU4ConfParser"

#include "LogHelper.h"
#include "IPU4ConfParser.h"
#include <v4l2device.h>

namespace android {
namespace camera2 {

IPU4ConfParser::IPU4ConfParser()
{
    mCaps.clear();
}

void IPU4ConfParser::ParserXML(const char* profilePath)
{
    mCurrentDataField = FIELD_INVALID;
    mSensorIndex  = -1;
    getPSLDataFromXmlFile(profilePath);
    // Uncomment to display all the parsed values
    dump();
}

IPU4ConfParser::~IPU4ConfParser()
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

CameraCapInfo* IPU4ConfParser::getCameraCapInfo(int cameraId)
{
    if (cameraId > MAX_CAMERAS) {
        LOGE("ERROR @%s: Invalid camera: %d", __FUNCTION__, cameraId);
        cameraId = 0;
    }

    return mCaps.valueFor(cameraId);
}

camera_metadata_t* IPU4ConfParser::constructDefaultMetadata(int cameraId, int requestTemplate)
{
    LOG2("@%s: %d", __FUNCTION__, requestTemplate);
    if (requestTemplate >= CAMERA3_TEMPLATE_COUNT) {
        LOGE("ERROR @%s: bad template %d", __FUNCTION__, requestTemplate);
        return NULL;
    }

    //int index = cameraId * CAMERA3_TEMPLATE_COUNT + requestTemplate;
    if (mDefaultRequests.indexOfKey(cameraId) < 0) return NULL;

    camera_metadata_t * req = (mDefaultRequests.valueFor(cameraId))[requestTemplate];

    if (req)
        return req;

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
    TAGINFO(ANDROID_SENSOR_FRAME_DURATION, bogusValue);

    float colorTransform[9] = {1.0, 0.0, 0.0,
                               0.0, 1.0, 0.0,
                               0.0, 0.0, 1.0};

    camera_metadata_rational_t transformMatrix[9];
    for (int i = 0; i < 9; i++) {
        transformMatrix[i].numerator = colorTransform[i];
        transformMatrix[i].denominator = 1.0;
    }
    TAGINFO_ARRAY(ANDROID_COLOR_CORRECTION_TRANSFORM, transformMatrix, 9);

    float colorGains[4] = {1.0, 1.0, 1.0, 1.0};
    TAGINFO_ARRAY(ANDROID_COLOR_CORRECTION_GAINS, colorGains, 4);

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

status_t IPU4ConfParser::addCamera(int cameraId)
{
    LOGE("%s: for camera %d", __FUNCTION__, cameraId);
    camera_metadata_t * emptyReq = NULL;
    CLEAR(emptyReq);

    SensorType type = (cameraId == BACK_CAMERA_ID) ? SENSOR_TYPE_RAW : SENSOR_TYPE_SOC;

    IPU4CameraCapInfo * info = new IPU4CameraCapInfo(type);
    if (!info) {
        LOGE("%s: No memory for IPU2CameraCapInfo!", __FUNCTION__);
        return NO_MEMORY;
    }

    if (mCaps.indexOfKey(cameraId) > 0) {
        delete mCaps.valueFor(cameraId);
        mCaps.removeItem(cameraId);
    }

    mCaps.add(info);

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

/**
 * This function will handle all the HAL parameters that are different
 * depending on the camera
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void IPU4ConfParser::handleHALTuning(const char *name, const char **atts)
{
    LOG2("@%s", __FUNCTION__);

    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }

    IPU4CameraCapInfo * info = mCaps.valueFor(mSensorIndex);
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
 * This function will handle all the parameters describing characteristic of
 * the sensor itself
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void IPU4ConfParser::handleSensorInfo(const char *name, const char **atts)
{
    LOG2("@%s", __FUNCTION__);
    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }
    IPU4CameraCapInfo * info = mCaps.valueFor(mSensorIndex);

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

/**
 * This function will handle all the camera pipe elements existing.
 * The goal is to enumerate all available camera media-ctl elements
 * from the camera profile file for later usage.
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void IPU4ConfParser::handleMediaCtlElements(const char *name, const char **atts)
{
    LOG1("@%s, type:%s", __FUNCTION__, name);

    IPU4CameraCapInfo * info = mCaps.valueFor(mSensorIndex);

    if (strcmp(name, "element") == 0) {
        MediaCtlElement currentElement;
        currentElement.videoNodeType = -1;
        while (*atts) {
            const XML_Char* attr_name = *atts++;
            const XML_Char* attr_value = *atts++;
            if (strcmp(attr_name, "name") == 0) {
                currentElement.name = attr_value;
            } else if (strcmp(attr_name, "type") == 0) {
                currentElement.type = attr_value;
            } else if (strcmp(attr_name, "videoNodeType") == 0) {
                currentElement.videoNodeType = getVideoNodeTypeAsValue(attr_value);
            } else {
                LOGW("Unhandled xml attribute in MediaCtl element (%s)", attr_name);
            }
        }
        if ((currentElement.type == "video_node") && (currentElement.videoNodeType == -1)) {
            LOGE("Video node type is not set for \"%s\"", currentElement.name.string());
            return;
        }
        info->mMediaCtlCamProps.mMediaCtlElements.push(currentElement);
    }
}
/**
 * This function will handle all the camera pipe config related elements.
 * The goal is to save all available camera media-ctl pipe configuration
 * from the camera profile file for later usage.
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void IPU4ConfParser::handleMediaCtlConfig(const char *name, const char **atts)
{
    LOG1("@%s, type:%s", __FUNCTION__, name);

    if (strcmp(name, "config") == 0) {
        if (strcmp(atts[0], "width") == 0) {
            mMediaCtlCamConfig.mCameraProps.outputWidth = atoi(atts[1]);
        }
        if (strcmp(atts[2], "height") == 0) {
            mMediaCtlCamConfig.mCameraProps.outputHeight = atoi(atts[3]);
        }
     } else if (strcmp(name, "link") == 0) {
        MediaCtlLinkParams currentLink;
        if (strcmp(atts[0], "srcName") == 0) {
            currentLink.srcName = atts[1];
        }
        if (strcmp(atts[2], "srcPad") == 0) {
            currentLink.srcPad = atoi(atts[3]);
        }
        if (strcmp(atts[4], "sinkName") == 0) {
            currentLink.sinkName = atts[5];
        }
        if (strcmp(atts[6], "sinkPad") == 0) {
            currentLink.sinkPad = atoi(atts[7]);
        }
        if (strcmp(atts[8], "enable") == 0) {
            currentLink.enable =
                ((strcmp(atts[9], "true") == 0) ? true : false);
        }
        /* push current link */
        mMediaCtlCamConfig.mLinkParams.push(currentLink); // push current Link
     } else if (strcmp(name, "format") == 0) {
        MediaCtlFormatParams currentFormat;
        if (strcmp(atts[0], "name") == 0) {
            currentFormat.entityName = atts[1];
        }
        if (strcmp(atts[2], "pad") == 0) {
            currentFormat.pad = atoi(atts[3]);
        }
        if (strcmp(atts[4], "width") == 0) {
            currentFormat.width = atoi(atts[5]);
        }
        if (strcmp(atts[6], "height") == 0) {
            currentFormat.height = atoi(atts[7]);
        }
        if (strcmp(atts[8], "format") == 0) {
            currentFormat.formatCode = getV4L2PixelFormatAsValue(atts[9]);
        }
        /* push current format */
        mMediaCtlCamConfig.mFormatParams.push(currentFormat);
     } else if (strcmp(name, "selection") == 0) {
        MediaCtlSelectionParams currentSelection;
        if (strcmp(atts[0], "name") == 0) {
            currentSelection.entityName = atts[1];
        }
        if (strcmp(atts[2], "pad") == 0) {
            currentSelection.pad = atoi(atts[3]);
        }
        if (strcmp(atts[4], "target") == 0) {
            currentSelection.target = getSelectionTargetAsValue(atts[5]);
        }
        if (strcmp(atts[6], "top") == 0) {
            currentSelection.top = atoi(atts[7]);
        }
        if (strcmp(atts[8], "left") == 0) {
            currentSelection.left = atoi(atts[9]);
        }
        if (strcmp(atts[10], "width") == 0) {
            currentSelection.width = atoi(atts[11]);
        }
        if (strcmp(atts[12], "height") == 0) {
            currentSelection.height = atoi(atts[13]);
        }
        /* push current selection */
        mMediaCtlCamConfig.mSelectionParams.push(currentSelection);
     } else if (strcmp(name, "control") == 0) {
        MediaCtlControlParams currentControl;
        if (strcmp(atts[0], "name") == 0) {
            currentControl.entityName = atts[1];
        }
        if (strcmp(atts[2], "ctrlId") == 0) {
            currentControl.controlId = getControlIdAsValue(atts[3]);
        }
        if (strcmp(atts[4], "value") == 0) {
            currentControl.value = atoi(atts[5]);
        }
        if (strcmp(atts[6], "ctrlName") == 0) {
            currentControl.controlName = atts[7];
        }
        /* push current control */
        mMediaCtlCamConfig.mControlParams.push(currentControl);
     } else if (strcmp(name, "videonode") == 0) {
        MediaCtlElement currentElement;
        if (strcmp(atts[0], "name") == 0) {
            currentElement.name = atts[1];
        }
        if (strcmp(atts[2], "videoNodeType") == 0) {
            currentElement.videoNodeType = getVideoNodeTypeAsValue(atts[3]);
        }
        /* push current video node */
        mMediaCtlCamConfig.mVideoNodes.push(currentElement);
     }
}

void IPU4ConfParser::checkField(const char *name, const char **atts)
{
    if (!strcmp(name, "Profiles")) {
        mSensorIndex = atoi(atts[1]);

        if (mSensorIndex > MAX_CAMERAS) {
            LOGE("ERROR @%s: bad camera id %d!", __FUNCTION__, mSensorIndex);
            return;
        }
        addCamera(mSensorIndex);
    } else if (!strcmp(name, "Hal_tuning_IPU4")) {
        mCurrentDataField = FIELD_HAL_TUNING_IPU4;
    } else if (!strcmp(name, "Sensor_info_IPU4")) {
        mCurrentDataField = FIELD_SENSOR_INFO_IPU4;
    } else if (!strcmp(name, "MediaCtl_elements_IPU4")) {
        mCurrentDataField = FIELD_MEDIACTL_ELEMENTS_IPU4;
    } else if (!strcmp(name, "MediaCtl_config_IPU4")) {
        mCurrentDataField = FIELD_MEDIACTL_CONFIG_IPU4;
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
void IPU4ConfParser::startElement(void *userData, const char *name, const char **atts)
{
    IPU4ConfParser *ipu4Parser = (IPU4ConfParser *)userData;

    if (ipu4Parser->mCurrentDataField == FIELD_INVALID) {
        ipu4Parser->checkField(name, atts);
        return;
    }
    LOG2("@%s: name:%s, for sensor %d", __FUNCTION__, name, ipu4Parser->mSensorIndex);

    switch (ipu4Parser->mCurrentDataField) {
        case FIELD_HAL_TUNING_IPU4:
            ipu4Parser->handleHALTuning(name, atts);
            break;
        case FIELD_SENSOR_INFO_IPU4:
            ipu4Parser->handleSensorInfo(name, atts);
            break;
        case FIELD_MEDIACTL_ELEMENTS_IPU4:
            ipu4Parser->handleMediaCtlElements(name, atts);
            break;
        case FIELD_MEDIACTL_CONFIG_IPU4:
            ipu4Parser->handleMediaCtlConfig(name, atts);
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
void IPU4ConfParser::endElement(void *userData, const char *name)
{
    IPU4ConfParser *ipu4Parser = (IPU4ConfParser *)userData;
    IPU4CameraCapInfo * info = ipu4Parser->mCaps.valueFor(ipu4Parser->mSensorIndex);
    if (!strcmp(name, "Profiles")) {
        ipu4Parser->mCurrentDataField = FIELD_INVALID;
    } else if (!strcmp(name, "MediaCtl_config_IPU4")) {
        if (ipu4Parser->mCurrentDataField == FIELD_MEDIACTL_CONFIG_IPU4) {
            info->mMediaCtlCamProps.mMediaCtlConfigs.push(ipu4Parser->mMediaCtlCamConfig);
            ipu4Parser->mMediaCtlCamConfig.mLinkParams.clear();
            ipu4Parser->mMediaCtlCamConfig.mFormatParams.clear();
            ipu4Parser->mMediaCtlCamConfig.mSelectionParams.clear();
            ipu4Parser->mMediaCtlCamConfig.mControlParams.clear();
            ipu4Parser->mMediaCtlCamConfig.mVideoNodes.clear();

            ipu4Parser->mCurrentDataField = FIELD_INVALID;
        }
    } else if (!strcmp(name, "Hal_tuning_IPU4")
             || !strcmp(name, "Sensor_info_IPU4")
             || !strcmp(name, "MediaCtl_elements_IPU4") ) {
        ipu4Parser->mCurrentDataField = FIELD_INVALID;
    }
    return;
}

/**
 * Get camera configuration from xml file
 *
 * The camera setting is stored inside this IPU4CameraCapInfo class.
 *
 */
void IPU4ConfParser::getPSLDataFromXmlFile(const char* profilePath)
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

/*
 * Helper function for converting string to int for the
 * V4L2 pixel formats requested for media controller set-up.
 * pixel format accepted by the media ctl entities
 */
int IPU4ConfParser::getV4L2PixelFormatAsValue(const char* format)
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

/*
 * Helper function for converting string to int for the
 * stream formats pixel setting.
 * Android specific pixel format, requested by the application
 */
int IPU4ConfParser::getStreamFormatAsValue(const char* format)
{
    /* Linear color formats */
    if (!strcmp(format, "HAL_PIXEL_FORMAT_RGBA_8888")) {
        return HAL_PIXEL_FORMAT_RGBA_8888;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_RGBX_8888")) {
        return HAL_PIXEL_FORMAT_RGBX_8888;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_RGB_888")) {
        return HAL_PIXEL_FORMAT_RGB_888;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_RGB_565")) {
        return HAL_PIXEL_FORMAT_RGB_565;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_BGRA_8888")) {
        return HAL_PIXEL_FORMAT_BGRA_8888;
    /* sRGB color pixel format */
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_sRGB_A_8888")) {
        return HAL_PIXEL_FORMAT_sRGB_A_8888;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_sRGB_X_8888")) {
        return HAL_PIXEL_FORMAT_sRGB_X_8888;
    /* YUV format */
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_YV12")) {
        return HAL_PIXEL_FORMAT_YV12;
    /* Y8 - Y16*/
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_Y8")) {
        return HAL_PIXEL_FORMAT_Y8;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_Y16")) {
        return HAL_PIXEL_FORMAT_Y16;
    /* OTHERS */
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_RAW_SENSOR")) {
        return HAL_PIXEL_FORMAT_RAW_SENSOR;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_BLOB")) {
        return HAL_PIXEL_FORMAT_BLOB;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED")) {
        return HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    /* YCbCr pixel format*/
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_YCbCr_420_888")) {
        return HAL_PIXEL_FORMAT_YCbCr_420_888;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_YCbCr_422_SP")) {
        return HAL_PIXEL_FORMAT_YCbCr_422_SP;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_YCrCb_420_SP")) {
        return HAL_PIXEL_FORMAT_YCrCb_420_SP;
    } else if (!strcmp(format, "HAL_PIXEL_FORMAT_YCbCr_422_I")) {
        return HAL_PIXEL_FORMAT_YCbCr_422_I;
    } else {
        LOGE("%s, Unknown Stream Format (%s)", __FUNCTION__, format);
        return -1;
    }
}

/*
 * Helper function for converting string to int for the
 * v4l2 command.
 * selection target defines what action we do on selection
 */
int IPU4ConfParser::getSelectionTargetAsValue(const char* target)
{
    if (!strcmp(target, "V4L2_SEL_TGT_CROP")) {
        return V4L2_SEL_TGT_CROP;
    } else if (!strcmp(target, "V4L2_SEL_TGT_CROP_DEFAULT")) {
        return V4L2_SEL_TGT_CROP_DEFAULT;
    } else if (!strcmp(target, "V4L2_SEL_TGT_CROP_BOUNDS")) {
        return V4L2_SEL_TGT_CROP_BOUNDS;
    } else if (!strcmp(target, "V4L2_SEL_TGT_COMPOSE")) {
        return V4L2_SEL_TGT_COMPOSE;
    } else if (!strcmp(target, "V4L2_SEL_TGT_COMPOSE_DEFAULT")) {
        return V4L2_SEL_TGT_COMPOSE_DEFAULT;
    } else if (!strcmp(target, "V4L2_SEL_TGT_COMPOSE_BOUNDS")) {
        return V4L2_SEL_TGT_COMPOSE_BOUNDS;
    } else if (!strcmp(target, "V4L2_SEL_TGT_COMPOSE_PADDED")) {
        return V4L2_SEL_TGT_COMPOSE_PADDED;
    } else {
        LOGE("%s, Unknown V4L2 Selection Target (%s)", __FUNCTION__, target);
        return -1;
    }
}

/*
 * Helper function for converting string to int for the
 * v4l2 command.
 * TODO: Make sure that we remove those when 3A is working
 * and provides good start values.
 */
int IPU4ConfParser::getControlIdAsValue(const char* format)
{
    if (!strcmp(format, "V4L2_CID_LINK_FREQ")) {
        return V4L2_CID_LINK_FREQ;
    } else if (!strcmp(format, "V4L2_CID_VBLANK")) {
        return V4L2_CID_VBLANK;
    } else if (!strcmp(format, "V4L2_CID_HBLANK")) {
        return V4L2_CID_HBLANK;
    } else if (!strcmp(format, "V4L2_CID_EXPOSURE")) {
        return V4L2_CID_EXPOSURE;
    } else if (!strcmp(format, "V4L2_CID_ANALOGUE_GAIN")) {
        return V4L2_CID_ANALOGUE_GAIN;
    } else if (!strcmp(format, "V4L2_CID_HFLIP")) {
        return V4L2_CID_HFLIP;
    } else if (!strcmp(format, "V4L2_CID_VFLIP")) {
        return V4L2_CID_VFLIP;
    } else if (!strcmp(format, "V4L2_CID_TEST_PATTERN")) {
        return V4L2_CID_TEST_PATTERN;
    } else {
        LOGE("%s, Unknown V4L2 ControlID (%s)", __FUNCTION__, format);
        return -1;
    }
}

/*
 * Helper function for converting string to int for the
 * v4l2 video node type.
 */
int IPU4ConfParser::getVideoNodeTypeAsValue(const char* nodeType)
{
    if (!strcmp(nodeType, "VIDEO_MIPI_COMPRESSED")) {
        return VIDEO_MIPI_COMPRESSED;
    } else if (!strcmp(nodeType, "VIDEO_MIPI_UNCOMPRESSED")) {
        return VIDEO_MIPI_UNCOMPRESSED;
    } else if (!strcmp(nodeType, "VIDEO_RAW_BAYER")) {
        return VIDEO_RAW_BAYER;
    } else if (!strcmp(nodeType, "VIDEO_RAW_BAYER_SCALED")) {
        return VIDEO_RAW_BAYER_SCALED;
    } else if (!strcmp(nodeType, "AA_STATS")) {
        return AA_STATS;
    } else if (!strcmp(nodeType, "AF_STATS")) {
        return AF_STATS;
    } else if (!strcmp(nodeType, "AE_HISTOGRAM")) {
        return AE_HISTOGRAM;
    } else {
        LOGE("%s, Unknown V4L2 video node type (%s)", __FUNCTION__, nodeType);
        return VIDEO_GENERIC;
    }
}

void IPU4ConfParser::dumpHalTuningSection(int cameraId)
{
    LOGD("@%s", __FUNCTION__);

    IPU4CameraCapInfo * info = mCaps.valueFor(cameraId);

    LOGD("element name: flipping, element value = %d", info->mSensorFlipping);
    LOGD("element name: halToV4L2Table.impl_defined, element value = %s",
         v4l2Fmt2Str(info->mGfxHalToV4L2PixelFmtTable.valueFor(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)));
    LOGD("element name: halToV4L2Table.raw_sensor, element value = %s",
         v4l2Fmt2Str(info->mGfxHalToV4L2PixelFmtTable.valueFor(HAL_PIXEL_FORMAT_RAW_SENSOR)));
    LOGD("element name: halToV4L2Table.ycbcr_420_888, element value = %s",
         v4l2Fmt2Str(info->mGfxHalToV4L2PixelFmtTable.valueFor(HAL_PIXEL_FORMAT_YCbCr_420_888)));
}

void IPU4ConfParser::dumpSensorInfoSection(int cameraId){
    LOGD("@%s", __FUNCTION__);

    IPU4CameraCapInfo * info = mCaps.valueFor(cameraId);

    LOGD("element name: sensorType, element value = %d", info->mSensorType);
    LOGD("element name: gain.lag, element value = %d", info->mGainLag);
    LOGD("element name: exposure.lag, element value = %d", info->mExposureLag);
    LOGD("element name: fov, element value = %f, %f", info->mFov[0], info->mFov[1]);
    LOGD("element name: statistics.initialSkip, element value = %d", info->mStatisticsInitialSkip);
}

void IPU4ConfParser::dumpMediaCtlElementsSection(int cameraId){
    LOGD("@%s", __FUNCTION__);

    unsigned int numidx;

    IPU4CameraCapInfo * info = mCaps.valueFor(cameraId);
    const MediaCtlElement *currentElement;
    for (numidx = 0; numidx < info->mMediaCtlCamProps.mMediaCtlElements.size(); numidx++) {
        currentElement = &info->mMediaCtlCamProps.mMediaCtlElements[numidx];
        LOGD("MediaCtl element name=%s ,type=%s, videoNodeType=%d"
             ,currentElement->name.string(),
             currentElement->type.string(),
             currentElement->videoNodeType);
    }


}

void IPU4ConfParser::dumpMediaCtlConfigSections(int cameraId){
    LOGD("@%s", __FUNCTION__);

    const MediaCtlConfig *currentConfig;
    unsigned int numidx, i;

    IPU4CameraCapInfo * info = mCaps.valueFor(cameraId);

    for (numidx = 0; numidx < info->mMediaCtlCamProps.mMediaCtlConfigs.size(); numidx++) {
        currentConfig = &info->mMediaCtlCamProps.mMediaCtlConfigs[numidx];
        LOGD("MediaCtl config w=%d ,height=%d, @ idx=%d"
             ,currentConfig->mCameraProps.outputWidth,
             currentConfig->mCameraProps.outputHeight, numidx);
        for (i = 0; i < currentConfig->mLinkParams.size() ; i++) {
            LOGD("Link Params srcName=%s  srcPad=%d ,sinkName=%s, sinkPad=%d enable=%d"
                 ,currentConfig->mLinkParams[i].srcName.string(),
                 currentConfig->mLinkParams[i].srcPad,
                 currentConfig->mLinkParams[i].sinkName.string(),
                 currentConfig->mLinkParams[i].sinkPad,
                 currentConfig->mLinkParams[i].enable);
        }
        for (i = 0; i < currentConfig->mFormatParams.size() ; i++) {
            LOGD("Format Params entityName=%s  pad=%d ,width=%d, height=%d formatCode=%x"
                 ,currentConfig->mFormatParams[i].entityName.string(),
                 currentConfig->mFormatParams[i].pad,
                 currentConfig->mFormatParams[i].width,
                 currentConfig->mFormatParams[i].height,
                 currentConfig->mFormatParams[i].formatCode);
        }
        for (i = 0; i < currentConfig->mSelectionParams.size() ; i++) {
            LOGD("Selection Params entityName=%s  pad=%d ,target=%d, top=%d left=%d width=%d, height=%d"
                 ,currentConfig->mSelectionParams[i].entityName.string(),
                 currentConfig->mSelectionParams[i].pad,
                 currentConfig->mSelectionParams[i].target,
                 currentConfig->mSelectionParams[i].top,
                 currentConfig->mSelectionParams[i].left,
                 currentConfig->mSelectionParams[i].width,
                 currentConfig->mSelectionParams[i].height);
        }
        for (i = 0; i < currentConfig->mControlParams.size() ; i++) {
            LOGD("Control Params entityName=%s  controlId=%x ,value=%d, controlName=%s"
                 ,currentConfig->mControlParams[i].entityName.string(),
                 currentConfig->mControlParams[i].controlId,
                 currentConfig->mControlParams[i].value,
                 currentConfig->mControlParams[i].controlName.string());
        }
    }
}

// To be modified when new elements or sections are added
// Use LOGD for traces to be visible
void IPU4ConfParser::dump()
{
    LOGD("===========================@%s======================", __FUNCTION__);
    for (unsigned int i = 0; i < mCaps.size(); i++) {
        dumpHalTuningSection(i);
        dumpSensorInfoSection(i);
        dumpMediaCtlElementsSection(i);
        dumpMediaCtlConfigSections(i);
    }

    LOGD("===========================end======================");
}


IPU4CameraCapInfo::IPU4CameraCapInfo(SensorType type)
{
    mSensorType = type;
}

const MediaCtlConfig *IPU4CameraCapInfo::getMediaCtlConfig(int width, int height) const
{
    LOG1("@%s", __FUNCTION__);

    const MediaCtlConfig *currentConfig;
    unsigned int numidx;

    for (numidx = 0; numidx < mMediaCtlCamProps.mMediaCtlConfigs.size(); numidx++) {
        currentConfig = &mMediaCtlCamProps.mMediaCtlConfigs[numidx];
        if(width == currentConfig->mCameraProps.outputWidth &&
            height == currentConfig->mCameraProps.outputHeight) {
            LOG1("MediaCtl config found for w=%d ,height=%d, @ idx=%d"
            ,width, height, numidx);
            return currentConfig;
         }
    }
    LOGE("%s: No MediaCtl config found for w=%d, h=%d"
        , __FUNCTION__, width, height);
    return NULL;
}

const MediaCtlProperties *IPU4CameraCapInfo::getMediaCtlProperties() const
{
    LOG1("@%s", __FUNCTION__);

    return &mMediaCtlCamProps;
}

const String8 IPU4CameraCapInfo::getMediaCtlEntityName(String8 type) const
{
    LOG1("@%s", __FUNCTION__);
    String8 name = String8("none");

    for (unsigned int numidx = 0; numidx < mMediaCtlCamProps.mMediaCtlElements.size(); numidx++) {
        if (type == mMediaCtlCamProps.mMediaCtlElements[numidx].type) {
            name = mMediaCtlCamProps.mMediaCtlElements[numidx].name;
            LOG2("@%s: found type %s, with name %s", __FUNCTION__, type.string(), name.string());
            break;
        }
    }
    return name;
}

int IPU4CameraCapInfo::getV4L2PixelFmtForGfxHalFmt(int gfxHalFormat) const
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
