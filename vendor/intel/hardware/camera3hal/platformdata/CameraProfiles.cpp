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

#define LOG_TAG "Camera_Profiles"

#include "LogHelper.h"
#include <string.h>
#include <libexpat/expat.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <linux/media.h>
#include "PlatformData.h"
#include "CameraProfiles.h"
#include <system/camera_metadata.h>
#include <cutils/properties.h>
#include "Metadata.h"
#include "CameraMetadataHelper.h"
#ifdef CAMERA_IPU2_SUPPORT
#include "IPU2ConfParser.h"
#endif
#ifdef CAMERA_IPU4_SUPPORT
#include "IPU4ConfParser.h"
#endif
#ifdef CAMERA_CIF_SUPPORT
#include "CIFConfParser.h"
#endif

namespace android {
namespace camera2 {

#define STATIC_ENTRY_CAP 256
#define STATIC_DATA_CAP 6688 // TODO: we may need to increase it again if more metadata is added

static const int cameraHWLength = 25;

CameraProfiles::CameraProfiles(CameraHWInfo * cameraHWInfo)
{
    LOG2("@%s", __FUNCTION__);

    mCurrentDataField = FIELD_INVALID;
    mSensorIndex  = -1;
    mItemsCount = -1;
    mMetadataCache = NULL;

    //Create CameraCommon class
    mCameraCommon = cameraHWInfo;

    mCameraInfoPool.init(MAX_CAMERAS);
    for (int i = 0;i < MAX_CAMERAS; i++)
         mCharacteristicsKeys[i].clear();
#ifdef CAMERA_IPU2_SUPPORT
    PSLConfParser* ipu2ParserInstance = NULL;
#endif
#ifdef CAMERA_IPU4_SUPPORT
    PSLConfParser* ipu4ParserInstance = NULL;
#endif
#ifdef CAMERA_CIF_SUPPORT
    PSLConfParser* aCIFParserInstance = NULL;
#endif

    // Parse common sections
    getDataFromXmlFile();
    // Uncomment to display all the parsed values
    //dump();

    // Now parse PSL sections
    for (unsigned int i = 0 ; i < mCameraIdToCameraInfo.size(); i++) {
        // get psl parser for id
        CameraInfo *info = mCameraIdToCameraInfo.valueFor(i);
        CameraHwType hwType = getCameraHwforId(i);
        String8 path = String8::format("/etc/camera3_profiles%s.xml",
                                       info->sensorName.length() > 0 ? String8::format("_%s", info->sensorName.string()).string()
                                       : info->sensorName.string());


        switch (hwType) {
        case SUPPORTED_HW_CSS_2400:
#ifdef CAMERA_IPU2_SUPPORT
            if (ipu2ParserInstance == NULL) {
                ipu2ParserInstance = new IPU2ConfParser();
            }
            ipu2ParserInstance->ParserXML(path.string());
            info->parser = ipu2ParserInstance;
#else
            ALOGE("IPU2 not supported");
#endif
            break;
        case SUPPORTED_HW_CSS_2600:
#ifdef CAMERA_IPU4_SUPPORT
            if (ipu4ParserInstance == NULL) {
                ipu4ParserInstance = new IPU4ConfParser();
            }
            ipu4ParserInstance->ParserXML(path.string());
            info->parser = ipu4ParserInstance;
#else
            ALOGE("IPU4 not supported");
#endif
            break;
        case SUPPORTED_HW_CIF:
#ifdef CAMERA_CIF_SUPPORT
            if (aCIFParserInstance == NULL) {
                aCIFParserInstance = new CIFConfParser();
            }
            info->parser = aCIFParserInstance;
#else
            ALOGE("CIF not supported");
#endif
            break;
        default:
            LOGE("No PSL parser for this HW type");
            break;
        }
    }
}

CameraProfiles::~CameraProfiles()
{
    for (unsigned int i = 0; i < mStaticMeta.size(); i++) {
        if (mStaticMeta[i])
            free_camera_metadata(mStaticMeta[i]);
    }

    mStaticMeta.clear();

    for (unsigned int i = 0 ; i < mCameraIdToCameraInfo.size(); i++) {
        CameraInfo *info = mCameraIdToCameraInfo.valueFor(i);
        mCameraInfoPool.releaseItem(info);
    }

    mCameraIdToCameraInfo.clear();
}

const CameraCapInfo * CameraProfiles::getCameraCapInfo(int cameraId)
{
    // get the psl parser instance for cameraid
    PSLConfParser* parserInstance;
    CameraInfo *info = mCameraIdToCameraInfo.valueFor(cameraId);
    parserInstance = info->parser;

    if(parserInstance == NULL) {
        LOGE("@%s Failed to get PSL parser instance", __FUNCTION__);
        return NULL;
    }

    return parserInstance->getCameraCapInfo(cameraId);
}

camera_metadata_t* CameraProfiles::constructDefaultMetadata(int cameraId, int requestTemplate)
{
    // get the psl parser instance for cameraid
    PSLConfParser* parserInstance;
    CameraInfo *info = mCameraIdToCameraInfo.valueFor(cameraId);
    parserInstance = info->parser;

    if(parserInstance == NULL) {
        LOGE("@%s Failed to get PSL parser instance", __FUNCTION__);
        return NULL;
    }

    return parserInstance->constructDefaultMetadata(cameraId, requestTemplate);

}

status_t CameraProfiles::addCamera(int cameraId)
{
    LOG2("%s: for camera %d", __FUNCTION__, cameraId);

    camera_metadata_t * meta = allocate_camera_metadata(STATIC_ENTRY_CAP, STATIC_DATA_CAP);
    if (!meta) {
        LOGE("%s: No memory for camera metadata!", __FUNCTION__);
        return NO_MEMORY;
    }
    mStaticMeta.push_back(meta);

    return NO_ERROR;
}

const char * CameraProfiles::get_camera_vendor_section_name(
        const vendor_tag_query_ops_t *v,
        uint32_t tag)
{
    UNUSED(v);
    UNUSED(tag);
    return "intel.ext";
}

const char * CameraProfiles::get_camera_vendor_tag_name(
        const vendor_tag_query_ops_t *v,
        uint32_t tag)
{
    UNUSED(v);
    UNUSED(tag);
    return NULL;
}

int CameraProfiles::get_camera_vendor_tag_type(
        const vendor_tag_query_ops_t *v,
        uint32_t tag)
{
    UNUSED(v);
    UNUSED(tag);
    return  TYPE_BYTE;
}


/**
 * convertEnum
 * Converts from the string provided as src to an enum value.
 * It uses a table to convert from the string to an enum value (usually BYTE)
 * \param [IN] dest: data buffer where to store the result
 * \param [IN] src: pointer to the string to parse
 * \param [IN] table: table to convert from string to value
 * \param [IN] tableNum: size of the enum table
 * \param [IN] type: data type to be parsed (byte, int32, int64 etc..)
 * \param [OUT] newDest: pointer to the new write location
 */
int CameraProfiles::convertEnum(void *dest, const char *src, int type,
                                const metadata_value_t *table, int tableLen,
                                void **newDest)
{
    int ret = 0;
    union {
        uint8_t * u8;
        int32_t * i32;
        int64_t * i64;
    } data;

    *newDest = dest;
    data.u8 = (uint8_t *)dest;

    for (int i = 0; i < tableLen; i++ ) {
        if (!strncasecmp(src, table[i].name, strlen(table[i].name))
            && (strlen(src) == strlen(table[i].name))) {
            if (type == TYPE_BYTE) {
                data.u8[0] = table[i].value;
                LOG1("byte    - %s: %d -", table[i].name, data.u8[0]);
                *newDest = (void*) &data.u8[1];
            } else if (type == TYPE_INT32) {
                data.i32[0] = table[i].value;
                LOG1("int    - %s: %d -", table[i].name, data.i32[0]);
                *newDest = (void*) &data.i32[1];
            } else if (type == TYPE_INT64) {
                data.i64[0] = table[i].value;
                LOG1("int64    - %s: %lld -", table[i].name, data.i64[0]);
                *newDest = (void*) &data.i64[1];
            }
            ret = 1;
            break;
        }
    }

    return ret;
}


/**
 * parseEnum
 * parses an enumeration type or a list of enumeration types it stores the data
 * in the member buffer mMetadataCache that is of size mMetadataCacheSize.
 *
 * \param src: string to be parsed
 * \param tagInfo: structure with the description of the static metadata
 * \param metadataCacheSize:the upper limited size of the dest buffer
 * \param metadataCache: the dest buffer to store the medatada after persed
 * \return number of elements parsed
 */
int CameraProfiles::parseEnum(const char * src,
                              const metadata_tag_t* tagInfo,
                              int metadataCacheSize,
                              int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int count = 0;
    int maxCount = metadataCacheSize / camera_metadata_type_size[tagInfo->type];
    char * endPtr = NULL;

    /**
     * pointer to the metadata cache buffer
     */
    void *storeBuf = metadataCache;
    void *next;

    do {
        endPtr = strchr(src, ',');
        if (endPtr)
            *endPtr = 0;

        count += convertEnum(storeBuf, src,tagInfo->type,
                             tagInfo->enumTable, tagInfo->tableLength, &next);
        if (endPtr) {
            src = endPtr + 1;
            storeBuf = next;
        }
    } while (count < maxCount && endPtr);

    return count;
}

/**
 * parseEnumAndNumbers
 * parses an enumeration type or a list of enumeration types or tries to convert string to a number
 * it stores the data in the member buffer mMetadataCache that is of size mMetadataCacheSize.
 *
 * \param src: string to be parsed
 * \param tagInfo: structure with the description of the static metadata
  * \param metadataCacheSize:the upper limited size of the dest buffer
 * \param metadataCache: the dest buffer to store the medatada after persed
 * \return number of elements parsed
 */
int CameraProfiles::parseEnumAndNumbers(const char * src,
                                        const metadata_tag_t* tagInfo,
                                        int metadataCacheSize,
                                        int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int count = 0;
    int maxCount = metadataCacheSize / camera_metadata_type_size[tagInfo->type];
    char * endPtr = NULL;

    /**
     * pointer to the metadata cache buffer
     */
    void *storeBuf = metadataCache;
    void *next;

    do {
        endPtr = strchr(src, ',');
        if (endPtr)
            *endPtr = 0;

        count += convertEnum(storeBuf, src,tagInfo->type,
                             tagInfo->enumTable, tagInfo->tableLength, &next);
        /* Try to convert value to number */
        if (count == 0) {
            long int *number = reinterpret_cast<long int*>(storeBuf);
            *number = strtol(src, &endPtr, 10);
            if (*number == LONG_MAX || *number == LONG_MIN)
                LOGW("You might have invalid value in the camera profiles: %s", src);
            count++;
        }

        if (endPtr) {
            src = endPtr + 1;
            storeBuf = next;
        }
    } while (count < maxCount && endPtr);

    return count;
}

/**
 * parseData
 * parses a generic array type. It stores the data in the member buffer
 * mMetadataCache that is of size mMetadataCacheSize.
 *
 * \param src: string to be parsed
 * \param tagInfo: structure with the description of the static metadata
 * \param metadataCacheSize:the upper limited size of the dest buffer
 * \param metadataCache: the dest buffer to store the medatada after persed
 * \return number of elements parsed
 */
int CameraProfiles::parseData(const char * src,
                              const metadata_tag_t* tagInfo,
                              int metadataCacheSize,
                              int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int index = 0;
    int maxIndex = metadataCacheSize/sizeof(double); // worst case

    char * endPtr = NULL;
    union {
        uint8_t * u8;
        int32_t * i32;
        int64_t * i64;
        float * f;
        double * d;
    } data;
    data.u8 = (uint8_t *)metadataCache;

    do {
        switch (tagInfo->type) {
        case TYPE_BYTE:
            data.u8[index] = (char)strtol(src, &endPtr, 10);
            LOG2("    - %d -", data.u8[index]);
            break;
        case TYPE_INT32:
        case TYPE_RATIONAL:
            data.i32[index] = strtol(src, &endPtr, 10);
            LOG2("    - %d -", data.i32[index]);
            break;
        case TYPE_INT64:
            data.i64[index] = strtol(src, &endPtr, 10);
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
    } while (index < maxIndex);

    if (tagInfo->type == TYPE_RATIONAL) {
        if (index % 2) {
            LOGW("Invalid number of entries to define rational (%d)."
                            " It should be even", index);
            // lets make it even
            index -= 1;
        }
        index = index / 2;
        // we divide by 2 because one rational is made of 2 ints
    }

    return index;
}


const char *CameraProfiles::skipWhiteSpace(const char *src)
{
    /* Skip whitespace. (space, tab, newline, vertical tab, feed, carriage return) */
    while( *src == '\n' || *src == '\t' || *src == ' ' || *src == '\v' || *src == '\r' || *src == '\f'  ) {
        src++;
    }
    return src;
}

/**
 * Parses the string with the supported stream configurations
 * a stream configuration is made of 3 elements
 * - Format
 * - Resolution
 * - Direction (input or output)
 * we parse the string in 3 steps
 * example of valid stream configuration is:
 * RAW16,4208x3120,INPUT
 * \param src: string to be parsed
 * \param tagInfo: descriptor of the static metadata. this is the entry from the
 * table defined in the autogenerated code
 * \param metadataCacheSize:the upper limited size of the dest buffer
 * \param metadataCache: the dest buffer to store the medatada after persed
 *
 * \return number of int32 entries to be stored (i.e. 4 per configuration found)
 */
int CameraProfiles::parseStreamConfig(const char * src,
                                      const metadata_tag_t* tagInfo,
                                      int metadataCacheSize,
                                      int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);

    int fmtTableLen = ELEMENT(android_scaler_availableFormats_values);
    int dirTableLen = ELEMENT(android_scaler_availableStreamConfigurations_values);
    int count = 0;  // entry count
    int maxCount = metadataCacheSize/sizeof(int32_t);
    int ret;
    char * endPtr = NULL;
    int parseStep = 1;
    int32_t *i32;

    const metadata_value_t * activeTable;
    int activeTableLen = 0;

    void *storeBuf = metadataCache;
    void *next;

    do {
        endPtr = strchr(src, ',');
        if (endPtr)
            *endPtr = 0;

        if (parseStep == 1) {
            activeTable = android_scaler_availableFormats_values;
            activeTableLen = fmtTableLen;
        } else if (parseStep == 3) {
            activeTable = android_scaler_availableStreamConfigurations_values;
            activeTableLen = dirTableLen;
        }

        if (parseStep == 1 || parseStep == 3) {
            ret = convertEnum(storeBuf, src, tagInfo->type, activeTable,
                              activeTableLen, &next);
            if (ret == 1) {
                count++;
                storeBuf = next;
            } else {
                LOGE("Malformed enum in stream configuration %s", src);
                goto parseError;
            }

        } else {  // Step 2: Parse the resolution
            i32 = reinterpret_cast<int32_t*>(storeBuf);
            i32[0] = strtol(src, &endPtr, 10);
            if (endPtr == NULL || *endPtr != 'x') {
                LOGE("Malformed resolution in stream configuration");
                goto parseError;
            }
            src = endPtr + 1;
            i32[1] = strtol(src, &endPtr, 10);
            storeBuf = reinterpret_cast<void*>(&i32[2]);
            count += 2;
            LOG1("  - %dx%d -", i32[0], i32[1]);
        }

        if (endPtr) {
            src = endPtr + 1;
            src = skipWhiteSpace(src);
            parseStep++;
            /* parsing steps go from 1 to 3 */
            if (parseStep == 4) {
                parseStep = 1;
                LOG1("Stream Configuration found");
            }
        } else {
            break;
        }
    } while (count < maxCount);

    if (endPtr != NULL) {
        LOGW("Stream configuration stream too long for parser");
    }
    /**
     * Total number of entries per stream configuration is 4
     * - one for the format
     * - two for the resolution
     * - one for the direction
     * The total entry count should be multiple of 4
     */
    if (count % 4) {
        LOGE("Malformed string for stream configuration."
             " ignoring last %d entries", count % 4);
        count -= count % 4;
    }
    return count;

parseError:
    LOGE("Error parsing stream configuration ");
    return 0;
}
/**
 * parseAvailableKeys
 * This method is used to parse the following two static metadata tags:
 * android.request.availableRequestKeys
 * android.request.availableResultKeys
 *
 * It uses the auto-generated table metadataNames to look for all the non
 * static tags.
 */
int CameraProfiles::parseAvailableKeys(const char * src,
                                       const metadata_tag_t* tagInfo,
                                       int metadataCacheSize,
                                       int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int count = 0;  // entry count
    int maxCount = metadataCacheSize/camera_metadata_type_size[tagInfo->type];
    size_t tableSize = ELEMENT(metadataNames);
    size_t stringSize = strlen(src);
    size_t blanks, tokenSize;
    char *token;
    char *cleanToken;
    int32_t* storeBuf = (int32_t*)metadataCache;

    token = strtok( (char*)src, ", ");

    while (token != NULL) {
        /* ignore any spaces in front of the string */
        blanks = strspn(token," ");
        cleanToken = token + blanks;
        /**
         * Parse the token without blanks.
         * TODO: Add support for simple wildcard to allow things like
         * android.request.*
         */
        tokenSize = strlen(cleanToken);
        for (unsigned int i = 0; i< tableSize; i++) {
            if (strncmp(cleanToken, metadataNames[i].name, tokenSize) == 0) {
                storeBuf[count] = metadataNames[i].value;
                count++;
            }
        }
        if (count >= maxCount) {
            LOGW("Too many keys found (%d)- ignoring the rest", count);
            /* if this happens then we should increase the size of the
             * mMetadataCache
             */
            break;
        }
        token = strtok(NULL,",");
    }
    return count;
}

int CameraProfiles::parseSceneModeOverride(const char * src,
                                           const metadata_tag_t* tagInfo,
                                           int metadataCacheSize,
                                           int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    UNUSED(src);
    UNUSED(tagInfo);
    UNUSED(metadataCacheSize);
    UNUSED(metadataCache);
    return 0;
}

/**
 * Parses the string with the avaialble input-output formats map
 * a format map is made of 3 elements
 * - Input Format
 * - Number output formats it can be converted in to
 * - List of the output formats.
 * we parse the string in 3 steps
 * example of valid input-output formats map is:
 * RAW_OPAQUE,3,BLOB,IMPLEMENTATION_DEFINED,YCbCr_420_888
 *
 * \param src: string to be parsed
 * \param tagInfo: descriptor of the static metadata. this is the entry from the
 * table defined in the autogenerated code
 * \param metadataCacheSize:the upper limit size of the dest buffer
 * \param metadataCache: the dest buffer to store the medatada after persed
 *
 * \return number of int32 entries to be stored
 */
int CameraProfiles::parseAvailableInputOutputFormatsMap(const char * src,
                                                        const metadata_tag_t* tagInfo,
                                                        int metadataCacheSize,
                                                        int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);

    int count = 0;  // entry count
    int maxCount = metadataCacheSize/camera_metadata_type_size[tagInfo->type];
    int ret;
    char * endPtr = NULL;
    int parseStep = 1;
    int32_t *i32;
    int numOutputFormats = 0;

    const metadata_value_t * activeTable;
    int activeTableLen = 0;

    void *storeBuf = metadataCache;
    void *next;

    do {
        endPtr = strchr(src, ',');
        if (endPtr)
            *endPtr = 0;

        if (parseStep == 1) {  // Step 1 parse the input format
            if (strlen(src) == 0) break;
            // detect empty string. It means we are done, so get out of the loop
            activeTable = android_scaler_availableFormats_values;
            activeTableLen = ELEMENT(android_scaler_availableFormats_values);
            ret = convertEnum(storeBuf, src, tagInfo->type, activeTable,
                              activeTableLen, &next);
            if (ret == 1) {
                count++;
                storeBuf = next;
            } else {
                LOGE("Malformed enum in format map %s", src);
                break;
            }

        } else if (parseStep == 2) {  // Step 2: Parse the num of output formats
            i32 = reinterpret_cast<int32_t*>(storeBuf);
            i32[0] = strtol(src, &endPtr, 10);
            numOutputFormats = i32[0];
            count += 1;
            storeBuf = reinterpret_cast<void*>(&i32[1]);
            LOGD("Num of output formats = %d", i32[0]);

        } else {  // Step3 parse the output formats
            activeTable = android_scaler_availableFormats_values;
            activeTableLen =  ELEMENT(android_scaler_availableFormats_values);
            for (int i = 0; i < numOutputFormats; i++) {
                ret = convertEnum(storeBuf, src, tagInfo->type, activeTable,
                                  activeTableLen, &next);
                if (ret == 1) {
                    if (endPtr == NULL) return 0;
                    src = endPtr + 1;
                    storeBuf = next;
                    count += 1;
                } else {
                    LOGE("Malformed enum in format map %s", src);
                    break;
                }
                if (i < numOutputFormats - 1) {
                    endPtr = strchr(src, ',');
                    if (endPtr)
                        *endPtr = 0;
                }
            }
        }

        if (endPtr) {
            src = endPtr + 1;
            src = skipWhiteSpace(src);
            parseStep++;
            /* parsing steps go from 1 to 3 */
            if (parseStep == 4) {
                parseStep = 1;
            }
        }
    } while (count < maxCount && endPtr);

    if (endPtr != NULL) {
        LOGW("Formats Map string too long for parser");
    }

    return count;
}


int CameraProfiles::parseSizes(const char * src,
                               const metadata_tag_t* tagInfo,
                               int metadataCacheSize,
                               int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int entriesFound = 0;

    entriesFound = parseData(src, tagInfo, metadataCacheSize, metadataCache);

    if (entriesFound % 2) {
        LOGE("Odd number of entries (%d), resolutions should have an even "
              "number of entries", entriesFound);
        entriesFound -= 1; //make it even Ignore the last one
    }
    return entriesFound;
}

int CameraProfiles::parseImageFormats(const char * src,
                                      const metadata_tag_t* tagInfo,
                                      int metadataCacheSize,
                                      int64_t* metadataCache)
{
    /**
     * DEPRECATED in V 3.2: TODO: add warning and extra checks
     */
    HAL_TRACE_CALL(1);
    int entriesFound = 0;

    entriesFound = parseEnum(src, tagInfo, metadataCacheSize, metadataCache);

    return entriesFound;
}

int CameraProfiles::parseRectangle(const char * src,
                                   const metadata_tag_t* tagInfo,
                                   int metadataCacheSize,
                                   int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int entriesFound = 0;
    entriesFound = parseData(src, tagInfo, metadataCacheSize, metadataCache);

    if (entriesFound % 4) {
        LOGE("incorrect number of entries (%d), rectangles have 4 values",
                    entriesFound);
        entriesFound -= entriesFound % 4; //round to multiple of 4
    }

    return entriesFound;
}
int CameraProfiles::parseBlackLevelPattern(const char * src, const metadata_tag_t* tagInfo,
                                                 int metadataCacheSize, int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int entriesFound = 0;
    entriesFound = parseData(src, tagInfo, metadataCacheSize, metadataCache);
    if (entriesFound % 4) {
        LOGE("incorrect number of entries (%d), black level pattern have 4 values",
                    entriesFound);
        entriesFound -= entriesFound % 4; //round to multiple of 4
    }
    return entriesFound;
}

int CameraProfiles::parseStreamConfigDuration(const char * src,
                                              const metadata_tag_t* tagInfo,
                                              int metadataCacheSize,
                                              int64_t* metadataCache)
{
    HAL_TRACE_CALL(1);
    int fmtTableLen = ELEMENT(android_scaler_availableFormats_values);
    int count = 0;  // entry count
    int maxCount = metadataCacheSize/camera_metadata_type_size[tagInfo->type];
    int ret;
    char * endPtr = NULL;
    int parseStep = 1;
    int64_t *i64;

    const metadata_value_t * activeTable;
    int activeTableLen = 0;

    void *storeBuf = metadataCache;
    void *next;

    do {
        endPtr = strchr(src, ',');
        if (endPtr)
            *endPtr = 0;

        if (parseStep == 1) {  // Step 1 parse the format
            if (strlen(src) == 0) break;
            // detect empty string. It means we are done, so get out of the loop
            activeTable = android_scaler_availableFormats_values;
            activeTableLen = fmtTableLen;
            ret = convertEnum(storeBuf, src, tagInfo->type, activeTable,
                              activeTableLen, &next);
            if (ret == 1) {
                count++;
                storeBuf = next;
            } else {
                LOGE("Malformed enum in stream configuration duration %s", src);
                break;
            }

        } else if (parseStep == 2){  // Step 2: Parse the resolution
            i64 = reinterpret_cast<int64_t*>(storeBuf);
            i64[0] = strtol(src, &endPtr, 10);
            if (endPtr == NULL || *endPtr != 'x') {
                LOGE("Malformed resolution in stream configuration");
                break;
            }
            src = endPtr + 1;
            i64[1] = strtol(src, &endPtr, 10);
            storeBuf = reinterpret_cast<void*>(&i64[2]);
            count += 2;
            LOG1("  - %lldx%lld -", i64[0], i64[1]);

        } else {  // Step3 parse the duration

            i64 = reinterpret_cast<int64_t*>(storeBuf);
            if (endPtr)
                i64[0] = strtol(src, &endPtr, 10);
            else
                i64[0] = strtol(src, NULL, 10); // Do not update endPtr
            storeBuf = reinterpret_cast<void*>(&i64[1]);
            count += 1;
            LOG1("  - %lld ns -", i64[0]);
        }

        if (endPtr) {
            src = endPtr + 1;
            src = skipWhiteSpace(src);
            parseStep++;
            /* parsing steps go from 1 to 3 */
            if (parseStep == 4) {
                parseStep = 1;
                LOG1("Stream Configuration found");
            }
        }
    } while (count < maxCount && endPtr);

    if (endPtr != NULL) {
        LOGW("Stream configuration duration string too long for parser");
    }
    /**
     * Total number of entries per stream configuration is 4
     * - one for the format
     * - two for the resolution
     * - one for the direction
     * The total entry count should be multiple of 4
     */
    if (count % 4) {
        LOGE("Malformed string for stream configuration."
                " ignoring last %d entries", count % 4);
        count -= count % 4;
    }
    return count;
}

/**
 * This function will check which field that the parser parses to.
 *
 * The field is set to 5 types.
 * FIELD_INVALID FIELD_SENSOR_COMMON FIELD_SENSOR_ANDROID_METADATA FIELD_SENSOR_VENDOR_METADATA and FIELD_COMMON
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void CameraProfiles::checkField(const char *name, const char **atts)
{
    if (!strcmp(name, "Profiles")) {
        mSensorIndex = atoi(atts[1]);

        if (mSensorIndex > MAX_CAMERAS) {
            LOGE("ERROR @%s: bad camera id %d!", __FUNCTION__, mSensorIndex);
            return;
        }

        if (mCameraIdToCameraInfo.indexOfKey(mSensorIndex) >= 0)
            mCameraInfoPool.releaseItem(mCameraIdToCameraInfo.valueFor(mSensorIndex));

        mCameraIdToCameraInfo.removeItem(mSensorIndex);
        mCharacteristicsKeys[mSensorIndex].clear();

        while (mSensorIndex >= mStaticMeta.size())
            addCamera(mSensorIndex);
    } else if (!strcmp(name, "Supported_hardware")) {
        mCurrentDataField = FIELD_SUPPORTED_HARDWARE;
        mItemsCount = -1;
    } else if (!strcmp(name, "Android_metadata")) {
        mCurrentDataField = FIELD_ANDROID_STATIC_METADATA;
        mItemsCount = -1;
    } else if (strcmp(name, "Vendor_metadata") == 0) {
        mCurrentDataField = FIELD_SENSOR_VENDOR_METADATA;
        mItemsCount = -1;
    } else if (strcmp(name, "Common") == 0) {
        mCurrentDataField = FIELD_COMMON;
        mItemsCount = -1;
    }
    LOG1("@%s: name:%s, field %d", __FUNCTION__, name, mCurrentDataField);
    return;
}

void CameraProfiles::handleSupportedHardware(const char *name, const char **atts)
{
    LOG1("@%s, type:%s", __FUNCTION__, name);
    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __func__, name, atts[0]);
        return;
    }

    if (strcmp(name, "hwType") == 0) {
        CameraInfo *info = NULL;
        mCameraInfoPool.acquireItem(&info);
        info->parser = NULL;
        info->hwType = atts[1];
        info->sensorName = mCurrSensorName;
        mCameraIdToCameraInfo.add(mSensorIndex, info);
    } else {
        LOGE("Unhandled xml attribute in Supported_hardware");
    }
}

/**
 * This function will handle all the common related elements.
 *
 * It will be called in the function startElement
 *
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void CameraProfiles::handleCommon(const char *name, const char **atts)
{
    LOG2("@%s, name:%s, atts[0]:%s", __FUNCTION__, name, atts[0]);

    if (strcmp(atts[0], "value") != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __FUNCTION__, name, atts[0]);
        return;
    }

    if (strcmp(name, "boardName") == 0) {
        mCameraCommon->mBoardName = atts[1];
    } else if (strcmp(name, "productName") == 0) {
        mCameraCommon->mProductName = atts[1];
    } else if (strcmp(name, "manufacturerName") == 0) {
        mCameraCommon->mManufacturerName = atts[1];
    } else if (strcmp(name, "supportDualVideo") == 0) {
        mCameraCommon-> mSupportDualVideo = ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "supportExtendedMakernote") == 0) {
        mCameraCommon-> mSupportExtendedMakernote = ((strcmp(atts[1], "true") == 0) ? true : false);
    } else if (strcmp(name, "previewHALFormat") == 0) {
        if (!strcmp(atts[1], "HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED"))
            mCameraCommon->mPreviewHALFormat = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
        else if (!strcmp(atts[1], "HAL_PIXEL_FORMAT_YCbCr_420_888"))
            mCameraCommon->mPreviewHALFormat = HAL_PIXEL_FORMAT_YCbCr_420_888;
        else if (!strcmp(atts[1], "HAL_PIXEL_FORMAT_YCbCr_422_SP"))
            mCameraCommon->mPreviewHALFormat = HAL_PIXEL_FORMAT_YCbCr_422_SP;
        else if (!strcmp(atts[1], "HAL_PIXEL_FORMAT_YCrCb_420_SP"))
            mCameraCommon->mPreviewHALFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        else if (!strcmp(atts[1], "HAL_PIXEL_FORMAT_YCbCr_422_I"))
            mCameraCommon->mPreviewHALFormat = HAL_PIXEL_FORMAT_YCbCr_422_I;
    }
}

/**
 * This function will handle all the android static metadata related elements of sensor.
 *
 * It will be called in the function startElement
 * This method parses the input from the XML file, that can be manipulated.
 * So extra care is applied in the validation of strings
 *
 * \param profiles: the pointer of the CameraProfiles.
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void CameraProfiles::handleAndroidStaticMetadata(const char *name, const char **atts)
{
    /**
     * string validation
     */
    const unsigned int MAX_METADATA_NAME_LENTGTH = 128;
    const unsigned int MAX_METADATA_ATTRIBUTE_NAME_LENTGTH = 128;
    const unsigned int MAX_METADATA_ATTRIBUTE_VALUE_LENTGTH = 2048;

    size_t nameSize = strnlen(name, MAX_METADATA_NAME_LENTGTH);
    size_t attrNameSize = strnlen(atts[0], MAX_METADATA_ATTRIBUTE_NAME_LENTGTH);
    size_t attrValueSize = strnlen(atts[1], MAX_METADATA_ATTRIBUTE_VALUE_LENTGTH);
    if ((attrValueSize == MAX_METADATA_ATTRIBUTE_VALUE_LENTGTH) ||
        (attrNameSize == MAX_METADATA_ATTRIBUTE_NAME_LENTGTH) ||
        (nameSize == MAX_METADATA_NAME_LENTGTH)) {
        LOGW("Warning XML strings too long ignoring this tag %s", name);
        return;
    }

    if (strncmp(atts[0], "value",attrNameSize) != 0) {
        LOGE("@%s, name:%s, atts[0]:%s, xml format wrong", __FUNCTION__, name, atts[0]);
        return;
    }

    // Find tag
    const metadata_tag_t *tagInfo = NULL;
    unsigned int index = 0;
    for (index = 0; index < STATIC_TAGS_TABLE_SIZE; index++) {
        if (!strncmp(name, android_static_tags_table[index].name, nameSize)) {
            tagInfo = &android_static_tags_table[index];
            break;
        }
    }
    if (index >= STATIC_TAGS_TABLE_SIZE) {
        LOGW("Parser does not support tag %s! - ignoring", name);
        return;
    }

    int count = 0;

    LOG1("@%s: Parsing static tag %s: value %s", __FUNCTION__,
            tagInfo->name,  atts[1]);

    /**
     * Complex parsing types done manually (exceptions)
     * scene overrides uses different tables for each entry one from ae/awb/af
     * mode
     */
    if (tagInfo->value == ANDROID_CONTROL_SCENE_MODE_OVERRIDES) {
        count = parseSceneModeOverride(atts[1], tagInfo, METADATASIZE, mMetadataCache);
    } else if (tagInfo->value == ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP) {
        count = parseAvailableInputOutputFormatsMap(atts[1], tagInfo, METADATASIZE, mMetadataCache);
    } else if ((tagInfo->value == ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS) ||
               (tagInfo->value == ANDROID_REQUEST_AVAILABLE_RESULT_KEYS)) {
        count = parseAvailableKeys(atts[1], tagInfo, METADATASIZE, mMetadataCache);
    } else if (tagInfo->value == ANDROID_SYNC_MAX_LATENCY) {
        count = parseEnumAndNumbers(atts[1], tagInfo, METADATASIZE, mMetadataCache);
    } else { /* Parsing of generic types */
        switch (tagInfo->arrayTypedef) {
        case BOOLEAN:
        case ENUM_LIST:
            count = parseEnum(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case STREAM_CONFIGURATION:
            count = parseStreamConfig(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case STREAM_CONFIGURATION_DURATION:
            count = parseStreamConfigDuration(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case RANGE_INT:
        case RANGE_LONG:
            count = parseData(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case SIZE_F:
        case SIZE:
            count = parseSizes(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case RECTANGLE:
            count = parseRectangle(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case IMAGE_FORMAT:
            count = parseImageFormats(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case BLACK_LEVEL_PATTERN:
            count = parseBlackLevelPattern(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            break;
        case TYPEDEF_NONE: /* Single values*/
            if (tagInfo->enumTable) {
                count = parseEnum(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            } else {
                count = parseData(atts[1], tagInfo, METADATASIZE, mMetadataCache);
            }
            break;
        default:
            LOGW("Unsupported typedef %s", tagInfo->name);
            break;
        }
    }
    if (count == 0) {
        LOGW("Error parsing static tag %s. ignoring", tagInfo->name);
        return;
    }

    LOG1("@%s: writing static tag %s: count %d", __FUNCTION__,
                                                 tagInfo->name,
                                                 count);

    camera_metadata_t * currentMeta = mStaticMeta.editItemAt(mSensorIndex);
    if (MetadataHelper::updateMetadata(currentMeta, tagInfo->value, mMetadataCache, count))
        LOGE("@%s: call MetadataHelper::updateMetadata fail for tag:%s", __FUNCTION__, get_camera_metadata_tag_name(tagInfo->value));
    // save the key to mCharacteristicsKeys used to update the
    // ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS
    mCharacteristicsKeys[mSensorIndex].push(tagInfo->value);
}

/**
 * This function will handle all the vendor metadata related elements of sensor.
 *
 * It will be called in the function startElement
 *
 * \param profiles: the pointer of the CameraProfiles.
 * \param name: the element's name.
 * \param atts: the element's attribute.
 */
void CameraProfiles::handleVendorMetadata(const char *name, const char **atts)
{
    // TODO:
    UNUSED(name);
    UNUSED(atts);
}

/**
 * the callback function of the libexpat for handling of one element start
 *
 * When it comes to the start of one element. This function will be called.
 *
 * \param userData: the pointer we set by the function XML_SetUserData.
 * \param name: the element's name.
 */
void CameraProfiles::startElement(void *userData, const char *name, const char **atts)
{
    CameraProfiles *profiles = (CameraProfiles *)userData;

    if (profiles->mCurrentDataField == FIELD_INVALID) {
        profiles->checkField(name, atts);
        return;
    }
    LOG2("@%s: name:%s, for sensor %d", __FUNCTION__, name, profiles->mSensorIndex);

    profiles->mItemsCount++;
    switch (profiles->mCurrentDataField) {
        case FIELD_SUPPORTED_HARDWARE:
            profiles->handleSupportedHardware(name, atts);
            break;
        case FIELD_ANDROID_STATIC_METADATA:
            profiles->handleAndroidStaticMetadata(name, atts);
            break;
        case FIELD_SENSOR_VENDOR_METADATA:
            profiles->handleVendorMetadata(name, atts);
            break;
        case FIELD_COMMON:
            profiles->handleCommon(name, atts);
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
void CameraProfiles::endElement(void *userData, const char *name)
{
    CameraProfiles *profiles = (CameraProfiles *)userData;
    if (!strcmp(name, "Profiles")) {
        profiles->mCurrentDataField = FIELD_INVALID;
    } else if (!strcmp(name, "Supported_hardware")
             || !strcmp(name, "Android_metadata")
             || !strcmp(name, "Vendor_metadata")
             || !strcmp(name, "Common")) {
        profiles->mCurrentDataField = FIELD_INVALID;
        profiles->mItemsCount = -1;
    }
    return;
}

/**
 * Get camera configuration from xml file
 *
 * The function will read the xml configuration file firstly.
 * Then it will parse out the camera settings.
 * The camera setting is stored inside this CameraProfiles class.
 *
 */
void CameraProfiles::getDataFromXmlFile(void)
{
    int done;
    void *pBuf = NULL;
    FILE *fp = NULL;
    XML_Parser parser = NULL;
    LOG1("@%s", __FUNCTION__);

    static char configXmlFile[mMaxConfigNameLength] = {0};
    camera_metadata_t * currentMeta = NULL;
    status_t res;
    int tag = ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS;

    char gHalConfigId[PROPERTY_VALUE_MAX]={0};
    if (property_get("persist.camera.hal.config.id", gHalConfigId, NULL))
        LOGD("Support hardware level is %s", gHalConfigId);

    if (strlen(gHalConfigId) != 0) {
        snprintf(configXmlFile, sizeof(configXmlFile), "/etc/camera3_profiles.%s.xml", gHalConfigId);
        if (0 != access(configXmlFile, R_OK)) {
            LOGD("can't find the %s,open default config file",configXmlFile);
        }
    }

    initSensorList();
    mCamName.push_front(String8::format("%s", ""));
    for (unsigned n = 0; n < mCamName.size(); n++) {
        mCurrSensorName = mCamName[n];
        String8 path = String8::format("/etc/camera3_profiles%s.xml",
                                       mCurrSensorName.length() > 0 ? String8::format("_%s", mCurrSensorName.string()).string()
                                       : mCurrSensorName.string());

        fp = ::fopen(path.string(), "r");
        if (NULL == fp) {
            LOGE("@%s, open xml: %s", __FUNCTION__, path.string());
            //return;
            continue;
        }

        parser = ::XML_ParserCreate(NULL);
        if (NULL == parser) {
            LOGE("@%s, line:%d, parser is NULL", __FUNCTION__, __LINE__);
            goto exit;
        }
        ::XML_SetUserData(parser, this);
        ::XML_SetElementHandler(parser, startElement, endElement);

        pBuf = malloc(BUFFERSIZE);
        if (NULL == pBuf) {
            LOGE("@%s, line:%d, pBuf is NULL", __func__, __LINE__);
            goto exit;
        }
        mMetadataCache = new int64_t[METADATASIZE];
        if (NULL == mMetadataCache) {
            LOGE("@%s, line:%d, no memory", __func__, __LINE__);
            goto exit;
        }

        do {
            int len = (int)::fread(pBuf, 1, BUFFERSIZE, fp);
            if (!len) {
                if (ferror(fp)) {
                    clearerr(fp);
                    goto exit;
                }
            }
            done = len < BUFFERSIZE;
            if (XML_Parse(parser, (const char *)pBuf, len, done) == XML_STATUS_ERROR) {
                LOGE("@%s, line:%d, XML_Parse error", __func__, __LINE__);
                goto exit;
            }
        } while (!done);
    }

    for (int i = 0; i < MAX_CAMERAS; i++) {
         currentMeta = mStaticMeta.editItemAt(i);
         if (currentMeta == NULL) {
             LOGE("can't get the static metadata");
             goto exit;
         }
         // update ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS
         int *keys = mCharacteristicsKeys[i].editArray();
         res = MetadataHelper::updateMetadata(currentMeta, tag, keys, mCharacteristicsKeys[i].size());
         if (res != OK)
             LOGE("@%s: call add/update_camera_metadata_entry fail for request.availableCharacteristicsKeys", __FUNCTION__);
    }

exit:
    if (parser)
        ::XML_ParserFree(parser);
    if (pBuf)
        free(pBuf);
    if (mMetadataCache) {
        delete [] mMetadataCache;
        mMetadataCache = NULL;
    }
    if (fp)
    ::fclose(fp);
}

CameraHwType CameraProfiles::getCameraHwforId(int cameraId)
{
    LOG2("@%s", __FUNCTION__);

    int index = mCameraIdToCameraInfo.indexOfKey(cameraId);
    if (index == NAME_NOT_FOUND) {
        LOGE("%s camera id not found, BUG, this should not happen!!",
             __FUNCTION__);
    }
    CameraInfo *info = mCameraIdToCameraInfo.valueFor(cameraId);
    String8 hwType = info->hwType;

    if (strncmp(hwType, "SUPPORTED_HW_CSS_2400", cameraHWLength) == 0) {
        return SUPPORTED_HW_CSS_2400;
    } else if (strncmp(hwType, "SUPPORTED_HW_CSS_2500", cameraHWLength) == 0) {
        return SUPPORTED_HW_CSS_2500;
    } else if (strncmp(hwType, "SUPPORTED_HW_CSS_2600", cameraHWLength) == 0) {
        return SUPPORTED_HW_CSS_2600;
    } else if (strncmp(hwType, "SUPPORTED_HW_USB", cameraHWLength) == 0) {
        return SUPPORTED_HW_USB;
    } else if (strncmp(hwType, "SUPPORTED_HW_CIF", cameraHWLength) == 0) {
        return SUPPORTED_HW_CIF;
    } else
        LOGE("ERROR @%s: Camera HW type wrong in xml", __FUNCTION__);
        return SUPPORTED_HW_UNKNOWN;
}

status_t CameraProfiles::initSensorList()
{
    status_t ret = 0;

	char medProPath[128] = {0};
    const char *mcPathName = "/dev/media0";
    const char *mProfilePathName = "/etc/camera3_profiles_%s.xml";
    int fd = open(mcPathName, O_RDONLY);
    if (fd == -1) {
        LOGE("ERROR in opening media controller: %s!", strerror(errno));
        return ENXIO;
    }

    struct media_entity_desc entity;
    memset(&entity, 0, sizeof(entity));
	mCamName.clear();

    do {
        entity.id |= MEDIA_ENT_ID_FLAG_NEXT;
        if (ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entity) < 0) {
            if (errno == EINVAL) {
                if (mCamName.size() == 0) {
                    LOGE("ERROR no sensor driver registered in media controller!");
                    ret = NO_INIT;
                }
            } else {
                LOGE("ERROR in browsing media controller entities: %s!", strerror(errno));
                ret = FAILED_TRANSACTION;
            }
            break;
        } else {
            if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR) {
                String8 sensorName = String8::format("%s", entity.name);
                for (int i = 0; (i = sensorName.find(" ")) > 0; sensorName.setTo(sensorName, i));
                mCamName.push(sensorName);
                sprintf(medProPath, "%s|/etc/media_profiles_%s.xml", medProPath, sensorName.string());
				LOGD("@%s,SensorName:%s",__func__,sensorName.string());
            }
        }
    } while (!ret);

	if(medProPath[0]) {
		property_set("media.settings.xml", &medProPath[1]);
		LOGD("@%s,media.settings.xml property:%s\n",__func__, &medProPath[1]);
	}

    return ret;
}

void CameraProfiles::dumpSupportedHWSection(int cameraId){
    LOGD("@%s", __FUNCTION__);
    int index = mCameraIdToCameraInfo.indexOfKey(cameraId);
    if (index == NAME_NOT_FOUND) {
        LOGE("%s camera id not found, BUG, this should not happen!!",
             __FUNCTION__);
    }

    CameraInfo *info = mCameraIdToCameraInfo.valueFor(cameraId);
    LOGD("element name hwType element value = %s", info->hwType.string());
}

void CameraProfiles::dumpAndroidmetadataSection(int cameraId)
{
    LOGD("@%s", __FUNCTION__);
    MetadataHelper::dumpMetadata(mStaticMeta[cameraId]);

}

void CameraProfiles::dumpCommonSection()
{
    LOGD("@%s", __FUNCTION__);
    LOGD("element name: boardName, element value = %s", mCameraCommon->mBoardName.string());
    LOGD("element name: productName, element value = %s", mCameraCommon->mProductName.string());
    LOGD("element name: manufacturerName, element value = %s", mCameraCommon->mManufacturerName.string());
    LOGD("element name: mSupportDualVideo, element value = %d", mCameraCommon-> mSupportDualVideo);
    LOGD("element name: supportExtendedMakernote, element value = %d", mCameraCommon->mSupportExtendedMakernote);
}

// To be modified when new elements or sections are added
// Use LOGD for traces to be visible
void CameraProfiles::dump()
{
    LOGD("===========================@%s======================", __FUNCTION__);
    for (unsigned int i = 0; i < mCameraIdToCameraInfo.size(); i++) {
        dumpSupportedHWSection(i);
        dumpAndroidmetadataSection(i);
    }
    dumpCommonSection();
    LOGD("===========================end======================");
}

}; // namespace camera2
}; // namespace android
