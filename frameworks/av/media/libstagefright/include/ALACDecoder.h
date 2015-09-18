/*
 * Copyright (C) 2010 The Android Open Source Project
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

namespace android {

// Box and chunck definitions per MPEG4 spec.

class Box {
    // sizeof Box = 8
    public:
    uint32_t size;
    uint32_t type;
};

class FullBox : public Box {
    // size of FullBox = 4 + sizeof(Box) = 12
    public:
    uint32_t version_flags; // 8bit msb as version, 24bit lsb as flags
};

class SampleEntry : public Box {
    // sizeof(SampleEntry) == 8 + sizeof(Box) = 16
    public:
    uint8_t box_reserved[6];
    uint16_t data_reference_index;
};

class AudioSampleEntry : public SampleEntry {
    // sizeof(AudioSampleEntry)= 20 + sizeof(SampleEntry) = 36
    public:
    uint32_t reserved[2];
    uint16_t channel_count;
    uint16_t sample_size;
    uint16_t predefined;
    uint16_t _reserved;
    uint32_t sample_rate;
};

class ALACSpecificConfig
{
    // sizeof(ALACSpecificConfg) = 24
public:
    uint32_t    frameLength;
    uint8_t        compatibleVersion;
    uint8_t        bitDepth;
    uint8_t        pb;
    uint8_t        mb;
    uint8_t        kb;
    uint8_t        numChannels;
    uint16_t    maxRun;
    uint32_t    maxFrameBytes;
    uint32_t    avgBitRate;
    uint32_t    sampleRate;
};

class ALACChannelLayoutInfo
{
    // sizeof(ALACChannelLayoutInfo) = 24
public:
    uint32_t    channelLayoutInfoSize;
    uint32_t    channelLayoutInfoID;
    uint32_t    versionFlags;
    uint32_t    channelLayoutTag;
    uint32_t    reserved1;
    uint32_t    reserved2;
};

// For compatibility purpose
class FormatAtom
{
    // sizeof(FormatAtom) = 12
public:
    uint32_t    size;   // == 12
    uint32_t    channel_layout_info_id;
    uint32_t    format_type;
};

class TerminatorAtom
{
    // sizeof(TerminatorAtom) = 8
public:
    uint32_t    channel_layout_info_size;   // == 8
    uint32_t    channel_layout_info_id;
};

}
