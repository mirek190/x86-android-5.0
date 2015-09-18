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

#define LOG_TAG "AudioUtils"

#include "SampleSpec.h"
#include "AudioUtils.h"
#include <AudioCommsAssert.hpp>
#include <stdlib.h>
#include <stdint.h>
#include <utils/Log.h>
#include <system/audio.h>
#include <limits.h>
#include <limits>
#include <cerrno>
#include "SampleSpec.h"
#include "AudioUtils.h"
#include <hardware_legacy/AudioSystemLegacy.h>
#include <convert.hpp>

using namespace android;
using namespace std;
using audio_comms::utilities::convertTo;

namespace android_audio_legacy {

uint32_t AudioUtils::alignOn16(uint32_t u)
{
    LOG_ALWAYS_FATAL_IF((u / FRAME_ALIGNEMENT_ON_16) >
                        (numeric_limits<uint32_t>::max() / FRAME_ALIGNEMENT_ON_16));
    return (u + (FRAME_ALIGNEMENT_ON_16 - 1)) & ~(FRAME_ALIGNEMENT_ON_16 - 1);
}

size_t AudioUtils::convertSrcToDstInBytes(size_t bytes,
                                          const SampleSpec &ssSrc,
                                          const SampleSpec &ssDst)
{
    return ssDst.convertFramesToBytes(convertSrcToDstInFrames(ssSrc.convertBytesToFrames(bytes),
                                                              ssSrc,
                                                              ssDst));
}

size_t AudioUtils::convertSrcToDstInFrames(size_t frames,
                                           const SampleSpec &ssSrc,
                                           const SampleSpec &ssDst)
{
    LOG_ALWAYS_FATAL_IF(ssSrc.getSampleRate() == 0);
    AUDIOCOMMS_COMPILE_TIME_ASSERT(sizeof(uint64_t) >= (2 * sizeof(ssize_t)));
    int64_t dstFrames = ((uint64_t)frames * ssDst.getSampleRate() + ssSrc.getSampleRate() - 1) /
            ssSrc.getSampleRate();
    LOG_ALWAYS_FATAL_IF(dstFrames > numeric_limits<ssize_t>::max());
    return dstFrames;
}

audio_format_t AudioUtils::convertTinyToHalFormat(pcm_format format)
{
    audio_format_t convFormat;

    switch(format) {

    case PCM_FORMAT_S16_LE:
        convFormat = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case PCM_FORMAT_S32_LE:
        convFormat = AUDIO_FORMAT_PCM_8_24_BIT;
        break;
    default:
        ALOGE("%s: format not recognized", __FUNCTION__);
        convFormat = AUDIO_FORMAT_INVALID;
        break;
    }
    return convFormat;
}

pcm_format AudioUtils::convertHalToTinyFormat(audio_format_t format)
{
    pcm_format convFormat;

    switch(format) {

    case AUDIO_FORMAT_PCM_16_BIT:
        convFormat = PCM_FORMAT_S16_LE;
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        convFormat = PCM_FORMAT_S32_LE;
        break;
    default:
        ALOGE("%s: format not recognized", __FUNCTION__);
        convFormat = PCM_FORMAT_S16_LE;
        break;
    }
    return convFormat;
}

int AudioUtils::getCardIndexByName(const char *name)
{
    char id_filepath[PATH_MAX] = {0};
    char number_filepath[PATH_MAX] = {0};
    const int cardLen = strlen("card");

    ssize_t written;

    snprintf(id_filepath, sizeof(id_filepath), "/proc/asound/%s", name);

    written = readlink(id_filepath, number_filepath, sizeof(number_filepath));
    if (written < 0) {

        ALOGE("Sound card %s does not exist", name);
        return -errno;
    } else if (written >= (ssize_t)sizeof(id_filepath)) {

        // This will probably never happen
        return -ENAMETOOLONG;
    } else if (written <= cardLen) {

        // Waiting at least card length + index of the card
        return -EBADFD;
    }

    int indexCard = 0;
    if (!convertTo<int>(number_filepath + cardLen, indexCard)) {

        return -EINVAL;
    }
    return indexCard;
}

uint32_t AudioUtils::convertUsecToMsec(uint32_t timeUsec)
{
    // Round up to the nearest Msec
    return ((static_cast<uint64_t>(timeUsec) + 999) / 1000);
}

bool AudioUtils::isAudioInputDevice(uint32_t devices)
{
    return (popcount(devices) == 1) && ((devices & ~AudioSystem::DEVICE_IN_ALL) == 0);
}

}; // namespace android

