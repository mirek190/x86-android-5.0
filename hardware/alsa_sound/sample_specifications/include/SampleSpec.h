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
#pragma once

#include <string.h>
#include <vector>

#include <tinyalsa/asoundlib.h>
#include <system/audio.h>

namespace android_audio_legacy {

//
// Do not change the order: convertion steps will be ordered to make
// the computing as light as possible, it will works successively
// on the channels, then the format and the rate
//
enum SampleSpecItem {
    ChannelCountSampleSpecItem = 0,
    FormatSampleSpecItem,
    RateSampleSpecItem,

    NbSampleSpecItems
};


class SampleSpec {

public:
    bool operator==(const SampleSpec &right) const {

        return !memcmp(_sampleSpec, right._sampleSpec, sizeof(_sampleSpec)) &&
                (_channelsPolicy == right._channelsPolicy);
    }

    bool operator!=(const SampleSpec &right) const {

        return !operator==(right);
    }

    /**
     * Channel policy definition.
     * The channel policy will be usefull in case of remap operation.
     * From this definition, the remapper must be able to infer conversion table.
     *
     * For example: on some stereo devices, a channel might be empty/invalid.
     * So, the other channel will be tagged as "average"
     *
     *      SOURCE              DESTINATION
     *  channel 1 (copy) ---> channel 1 (average) = (source channel 1 + source channel 2) / 2
     *  channel 2 (copy) ---> channel 2 (ignore)  = empty
     *
     */
    enum ChannelsPolicy {
        Copy = 0,      /**< This channel is valid. */
        Average,       /**< This channel contains all valid audio data. */
        Ignore,        /**< This channel does not contains any valid audio data. */

        NbChannelsPolicy
    };

    SampleSpec(uint32_t channel = DEFAULT_CHANNELS,
               uint32_t format = DEFAULT_FORMAT,
               uint32_t rate = DEFAULT_RATE);

    SampleSpec(uint32_t channel,
               uint32_t format,
               uint32_t rate,
               const std::vector<ChannelsPolicy> &channelsPolicy);

    // Specific Accessors
    void setChannelCount(uint32_t channelCount) {

        setSampleSpecItem(ChannelCountSampleSpecItem, channelCount);
    }
    uint32_t getChannelCount() const { return getSampleSpecItem(ChannelCountSampleSpecItem); }
    void setSampleRate(uint32_t sampleRate) {

        setSampleSpecItem(RateSampleSpecItem, sampleRate);
    }
    uint32_t getSampleRate() const { return getSampleSpecItem(RateSampleSpecItem); }
    void setFormat(uint32_t format) { setSampleSpecItem(FormatSampleSpecItem, format); }
    audio_format_t getFormat() const {

        return static_cast<audio_format_t>(getSampleSpecItem(FormatSampleSpecItem));
    }
    void setChannelMask(uint32_t channelMask) { _channelMask = channelMask; }
    uint32_t getChannelMask() const { return _channelMask; }

    void setChannelsPolicy(const std::vector<ChannelsPolicy> &channelsPolicy);
    const std::vector<ChannelsPolicy>& getChannelsPolicy() const { return _channelsPolicy; }
    ChannelsPolicy getChannelsPolicy(uint32_t channelIndex) const;

    // Generic Accessor
    void setSampleSpecItem(SampleSpecItem sampleSpecItem, uint32_t value);

    uint32_t getSampleSpecItem(SampleSpecItem sampleSpecItem) const;

    size_t getFrameSize() const;

    /**
     * Converts the bytes number to frames number.
     * It converts a bytes number into a frame number it represents in this sample spec instance.
     * It asserts if the frame size is null.

     * @param[in] number of bytes to be translated in frames.
     *
     * @return frames number in the sample spec given in param for this instance.
     */
    size_t convertBytesToFrames(size_t bytes) const;

    /**
     * Converts the frames number to bytes number.
     * It converts the frame number (independant of the channel number and format number) into
     * a byte number (sample spec dependant). In case of overflow, this function will assert.

     * @param[in] number of frames to be translated in bytes.
     *
     * @return bytes number in the sample spec given in param for this instance
     */
    size_t convertFramesToBytes(size_t frames) const;

    /**
     * Translates the frame number into time.
     * It converts the frame number into the time it represents in this sample spec instance.
     * It may assert if overflow is detected.
     *
     * @param[in] number of frames to be translated in time.
     *
     * @return time in microseconds.
     */
    size_t convertFramesToUsec(uint32_t frames) const;

    /**
     * Translates a time period into a frame number.
     * It converts a period of time into a frame number it represents in this sample spec instance.
     *
     * @param[in] time interval in micro second to be translated.
     *
     * @return number of frames corresponding to the period of time.
     */
    size_t convertUsecToframes(uint32_t intervalUsec) const;

    bool isMono() const { return _sampleSpec[ChannelCountSampleSpecItem] == 1; }

    bool isStereo() const { return _sampleSpec[ChannelCountSampleSpecItem] == 2; }

    /**
     * Checks upon equality of a sample spec item.
     *  For channels, it checks:
     *          -not only that channels count is equal
     *          -but also the channels policy of source and destination is the same.
     * @param[in] sampleSpecItem item to checks.
     * @param[in] ssSrc source sample specifications.
     * @param[in] ssDst destination sample specifications.
     *
     * @return true upon equality, false otherwise.
     */
    static bool isSampleSpecItemEqual(SampleSpecItem sampleSpecItem,
                                      const SampleSpec &ssSrc,
                                      const SampleSpec &ssDst);

private:
    /**
     * Initialise the sample specifications.
     * Parts of the private constructor. It sets the basic fields, reset the channel mask to 0,
     * and sets the channel policy to "Copy" for each of the channels used.
     *
     * @param[in] channel number of channels.
     * @param[in] format sample format, eg 16 or 24 bits(coded on 32 bits).
     * @param[in] rate sample rate.
     */
    void init(uint32_t channel, uint32_t format, uint32_t rate);

    uint32_t _sampleSpec[NbSampleSpecItems]; /**< Array of sample spec items:
                                                        -channel number
                                                        -format
                                                        -sample rate. */

    uint32_t _channelMask; /**< Bit field that defines the channels used. */

    std::vector<ChannelsPolicy> _channelsPolicy; /**< channels policy array. */

    static const uint32_t USEC_PER_SEC = 1000000; /**<  to convert sec to-from microseconds. */
    static const uint32_t DEFAULT_CHANNELS = 2; /**< default channel used is stereo. */
    static const uint32_t DEFAULT_FORMAT = AUDIO_FORMAT_PCM_16_BIT; /**< default format is 16bits.*/
    static const uint32_t DEFAULT_RATE = 48000; /**< default rate is 48 kHz. */
    static const uint32_t MAX_CHANNELS = 32; /**< supports until 32 channels. */
};

}; // namespace android

