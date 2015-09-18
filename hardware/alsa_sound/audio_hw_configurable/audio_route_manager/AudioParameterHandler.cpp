/*
 ** Copyright 2013 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "RouteManager/ParameterHandler"

#include <utils/Log.h>
#include <utils/String8.h>
#include <utils/Errors.h>
#include <media/AudioParameter.h>

#include "AudioParameterHandler.h"

using namespace android;

namespace android_audio_legacy
{

const char CAudioParameterHandler::FILE_PATH[] = "/mnt/asec/media/audio_param.dat";
const int CAudioParameterHandler::READ_BUF_SIZE = 500;

CAudioParameterHandler::CAudioParameterHandler()
{
    restore();
}

status_t CAudioParameterHandler::saveParameters(const String8& keyValuePairs)
{
    add(keyValuePairs);

    return save();
}

void CAudioParameterHandler::add(const String8& keyValuePairs)
{
    AudioParameter newParameters(keyValuePairs);
    uint32_t uiParameter;

    for (uiParameter = 0; uiParameter < newParameters.size(); uiParameter++) {

        String8 key, value;

        // Retrieve new parameter
        newParameters.getAt(uiParameter, key, value);

        // Add / merge it with stored ones
        mAudioParameter.add(key, value);
    }
}

status_t CAudioParameterHandler::save()
{
    FILE *fp = fopen(FILE_PATH, "w+");
    if (!fp) {

        ALOGE("%s: error %s", __FUNCTION__, strerror(errno));
        return UNKNOWN_ERROR;
    }

    String8 param = mAudioParameter.toString();

    if (fwrite(param.string(), sizeof(char), param.length(), fp) != param.length()) {

        ALOGE("%s: write failed with error %s", __FUNCTION__, strerror(errno));
        fclose(fp);
        return UNKNOWN_ERROR;
    }

    fclose(fp);
    return NO_ERROR;
}

status_t CAudioParameterHandler::restore()
{
    FILE *fp = fopen(FILE_PATH, "r");
    if (!fp) {

        ALOGE("%s: error %s", __FUNCTION__, strerror(errno));
        return UNKNOWN_ERROR;
    }
    char str[READ_BUF_SIZE];
    int readSize = fread(str, sizeof(char), READ_BUF_SIZE - 1, fp);

    if (readSize < 0) {

        ALOGE("%s: read failed with error %s", __FUNCTION__, strerror(errno));
        fclose(fp);
        return UNKNOWN_ERROR;
    }
    str[readSize] = '\0';
    fclose(fp);

    add(String8(str));
    return NO_ERROR;
}

String8 CAudioParameterHandler::getParameters() const
{
    return mAudioParameter.toString();
}
}
