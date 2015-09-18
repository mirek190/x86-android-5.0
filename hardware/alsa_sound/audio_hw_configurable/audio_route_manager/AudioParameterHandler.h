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


#include <utils/Errors.h>
#include <utils/String8.h>
#include <media/AudioParameter.h>

namespace android_audio_legacy
{

class CAudioParameterHandler
{
public:
    CAudioParameterHandler();
    android::status_t saveParameters(const android::String8& keyValuePairs); //Backup the parameters
    android::String8 getParameters() const; //Return the stored parameters from filesystem

private:
    android::status_t save(); //Save the mAudioParameter into filesystem
    android::status_t restore(); //Read the parameters from filesystem and add into mAudioParameters
    void add(const android::String8& keyValuePairs); //Add the parameters into mAudioParameters

    mutable android::AudioParameter mAudioParameter; //All of parameters will be saved into this variable

    static const char FILE_PATH[];
    static const int READ_BUF_SIZE;
};
}
