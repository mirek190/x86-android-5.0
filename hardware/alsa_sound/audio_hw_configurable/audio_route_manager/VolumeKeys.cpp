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

#define LOG_TAG "RouteManager/VolumeKeys"

#include <utils/Log.h>

#include <fcntl.h>

#include "VolumeKeys.h"

namespace android_audio_legacy
{

const char* const CVolumeKeys::GPIO_KEYS_WAKEUP_ENABLE = "/sys/devices/platform/gpio-keys/enabled_wakeup";
const char* const CVolumeKeys::GPIO_KEYS_WAKEUP_DISABLE = "/sys/devices/platform/gpio-keys/disabled_wakeup";

const char* const CVolumeKeys::KEY_VOLUMEDOWN = "114";
const char* const CVolumeKeys::KEY_VOLUMEUP = "115";

bool CVolumeKeys::_bWakeupEnabled = false;

/*---------------------------------------------------------------------------*/
/* Enable wakeup                                                             */
/*---------------------------------------------------------------------------*/
int CVolumeKeys::wakeupEnable()
{
    if (_bWakeupEnabled) {

        // Nothing to do, bailing out
        return 0;
    }
    LOGD("Enable volume keys wakeup\n");

    int fd;
    int rc;

    fd = open(GPIO_KEYS_WAKEUP_ENABLE, O_RDWR);
    if (fd < 0) {
        LOGE("Cannot open sysfs gpio-keys interface (%d)\n", fd);
        goto return_error;
    }
    rc = write(fd, KEY_VOLUMEDOWN, sizeof(KEY_VOLUMEDOWN));
    rc += write(fd, KEY_VOLUMEUP, sizeof(KEY_VOLUMEUP));
    close(fd);
    if (rc != (sizeof(KEY_VOLUMEDOWN) + sizeof(KEY_VOLUMEUP))) {
        LOGE("sysfs gpio-keys write error\n");
        goto return_error;
    }

    _bWakeupEnabled = true;
    LOGD("Volume keys wakeup enable OK\n");
    return 0;

return_error:

    LOGE("Volume keys wakeup enable failed\n");
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Disable wakeup                                                            */
/*---------------------------------------------------------------------------*/
int CVolumeKeys::wakeupDisable()
{
    if (!_bWakeupEnabled) {

        // Nothing to do, bailing out
        return 0;
    }

    LOGD("Disable volume keys wakeup\n");

    int fd;
    int rc;

    fd = open(GPIO_KEYS_WAKEUP_DISABLE, O_RDWR);
    if (fd < 0) {
        LOGE("Cannot open sysfs gpio-keys interface (%d)\n", fd);
        goto return_error;
    }
    rc = write(fd, KEY_VOLUMEDOWN, sizeof(KEY_VOLUMEDOWN));
    rc += write(fd, KEY_VOLUMEUP, sizeof(KEY_VOLUMEUP));
    close(fd);
    if (rc != (sizeof(KEY_VOLUMEDOWN) + sizeof(KEY_VOLUMEUP))) {
        LOGE("sysfs gpio-keys write error\n");
        goto return_error;
    }

    _bWakeupEnabled = false;
    LOGD("Volume keys wakeup disable OK\n");
    return 0;

return_error:

    LOGE("Volume keys wakeup disable failed\n");
    return -1;
}

}

