/*
 **
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

#define LOG_TAG "SampleSpec"

#include "SampleSpec.h"
#include <cutils/log.h>
#include <stdint.h>
#include <errno.h>
#include <limits>
#include <math.h>

using namespace std;

namespace android_audio_legacy {

#define SAMPLE_SPEC_ITEM_IS_VALID(sampleSpecItem) \
                LOG_ALWAYS_FATAL_IF((sampleSpecItem) < 0 || (sampleSpecItem) >= NbSampleSpecItems)


SampleSpec::SampleSpec(uint32_t channel,
                       uint32_t format,
                       uint32_t rate)
{
    init(channel, format, rate);
}

SampleSpec::SampleSpec(uint32_t channel,
                       uint32_t format,
                       uint32_t rate,
                       const vector<ChannelsPolicy> &channelsPolicy)
{
    init(channel, format, rate);
    setChannelsPolicy(channelsPolicy);
}

void SampleSpec::init(uint32_t channel,
                      uint32_t format,
                      uint32_t rate)
{
    _channelMask = 0;
    setSampleSpecItem(ChannelCountSampleSpecItem, channel);
    setSampleSpecItem(FormatSampleSpecItem, format);
    setSampleSpecItem(RateSampleSpecItem, rate);
}

// Generic Accessor
void SampleSpec::setSampleSpecItem(SampleSpecItem sampleSpecItem, uint32_t value)
{
    SAMPLE_SPEC_ITEM_IS_VALID(sampleSpecItem);

    if (sampleSpecItem == ChannelCountSampleSpecItem) {

        LOG_ALWAYS_FATAL_IF(value >= MAX_CHANNELS);

        _channelsPolicy.clear();
        // Reset all the channels policy to copy by default
        for (uint32_t i = 0; i < value; i++) {

            _channelsPolicy.push_back(Copy);
        }
    }
    _sampleSpec[sampleSpecItem] = value;
}

void SampleSpec::setChannelsPolicy(const vector<ChannelsPolicy> &channelsPolicy)
{
    LOG_ALWAYS_FATAL_IF(channelsPolicy.size() > _sampleSpec[ChannelCountSampleSpecItem]);
    _channelsPolicy = channelsPolicy;
}

SampleSpec::ChannelsPolicy SampleSpec::getChannelsPolicy(uint32_t channelIndex) const
{
    LOG_ALWAYS_FATAL_IF(channelIndex >= _channelsPolicy.size());
    return _channelsPolicy[channelIndex];
}

uint32_t SampleSpec::getSampleSpecItem(SampleSpecItem sampleSpecItem) const
{
    SAMPLE_SPEC_ITEM_IS_VALID(sampleSpecItem);
    return _sampleSpec[sampleSpecItem];
}

size_t SampleSpec::getFrameSize() const
{
    return audio_bytes_per_sample(getFormat()) * getChannelCount();
}

size_t SampleSpec::convertBytesToFrames(size_t bytes) const
{
    LOG_ALWAYS_FATAL_IF(getFrameSize() == 0);
    return bytes / getFrameSize();
}

size_t SampleSpec::convertFramesToBytes(size_t frames) const
{
    LOG_ALWAYS_FATAL_IF(getFrameSize() == 0);
    LOG_ALWAYS_FATAL_IF(frames > numeric_limits<size_t>::max() / getFrameSize());
    return frames * getFrameSize();
}

size_t SampleSpec::convertFramesToUsec(uint32_t frames) const
{
    LOG_ALWAYS_FATAL_IF(getSampleRate() == 0);
    LOG_ALWAYS_FATAL_IF((frames / getSampleRate()) >
                        (numeric_limits<size_t>::max() / USEC_PER_SEC));
    return (USEC_PER_SEC * static_cast<uint64_t>(frames)) / getSampleRate();
}

size_t SampleSpec::convertUsecToframes(uint32_t intervalUsec) const
{
    double frames = static_cast<double>(intervalUsec) * getSampleRate() / USEC_PER_SEC;
    // As the framecount is having decimal value, adding one more frame to the
    // framecount
    return static_cast<size_t>(ceil(frames));
}

bool SampleSpec::isSampleSpecItemEqual(SampleSpecItem sampleSpecItem,
                                        const SampleSpec &ssSrc,
                                        const SampleSpec &ssDst)
{
    if (ssSrc.getSampleSpecItem(sampleSpecItem) != ssDst.getSampleSpecItem(sampleSpecItem)) {

        return false;
    }

    return ((sampleSpecItem != ChannelCountSampleSpecItem) ||
            ssSrc.getChannelsPolicy() == ssDst.getChannelsPolicy());
}

}; // namespace android

