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

#pragma once

namespace android_audio_legacy
{

class CVolumeKeys
{
public :
    static int wakeupEnable();
    static int wakeupDisable();

private :
    static bool _bWakeupEnabled;

    static const char* const GPIO_KEYS_WAKEUP_ENABLE;
    static const char* const GPIO_KEYS_WAKEUP_DISABLE;

    static const char* const KEY_VOLUMEDOWN;
    static const char* const KEY_VOLUMEUP;

};

}

