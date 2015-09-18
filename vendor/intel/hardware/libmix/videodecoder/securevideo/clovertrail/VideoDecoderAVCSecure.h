/*
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef VIDEO_DECODER_AVC_SECURE_H_
#define VIDEO_DECODER_AVC_SECURE_H_

#include "VideoDecoderBase.h"
#include "VideoDecoderAVC.h"
#include "VideoDecoderDefs.h"

class VideoDecoderAVCSecure : public VideoDecoderAVC {
public:
    VideoDecoderAVCSecure(const char *mimeType);
    virtual ~VideoDecoderAVCSecure();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);

    // data in the decoded buffer is all encrypted.
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

private:
    enum {
        MAX_SLICE_HEADER_SIZE  = 30,
        MAX_NALU_HEADER_BUFFER = 8192,
        MAX_NALU_NUMBER = 400,  // > 4096/12
    };

    // Information of Network Abstraction Layer Unit
    struct NaluInfo {
        int32_t naluOffset;                        // offset of NAL unit in the firewalled buffer
        int32_t naluLen;                           // length of NAL unit
        int32_t naluHeaderLen;                     // length of NAL unit header
    };

    struct NaluMetadata {
        NaluInfo *naluInfo;
        int32_t naluNumber;  // number of NAL units
    };

    struct NaluByteStream {
        int32_t naluOffset;
        int32_t naluLen;
        int32_t streamPos;
        uint8_t *byteStream;   // 4 bytes of naluCount, 4 bytes of naluOffset, 4 bytes of naulLen, 4 bytes of naluHeaderLen, followed by naluHeaderData
        int32_t naluCount;
    };

    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
    Decode_Status modularDecodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
    int32_t findNalUnitOffset(uint8_t *stream, int32_t offset, int32_t length);
    Decode_Status copyNaluHeader(uint8_t *stream, NaluByteStream *naluStream);
    Decode_Status parseAnnexBStream(uint8_t *stream, int32_t length, NaluByteStream *naluStream);

    virtual Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    virtual Decode_Status beginDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status continueDecodingFrame(vbp_data_h264 *data);

    Decode_Status processModularInputBuffer(VideoDecodeBuffer *buffer, vbp_data_h264 **data);
    Decode_Status parseModularSliceHeader(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    Decode_Status updateSliceParameter(vbp_data_h264 *data, void *sliceheaderbuf);
    virtual Decode_Status getCodecSpecificConfigs(VAProfile profile, VAConfigID*config);

private:
    NaluMetadata mMetadata;
    NaluByteStream mByteStream;
    uint8_t *mNaluHeaderBuffer;
    uint8_t *mInputBuffer;

    int32_t mIsEncryptData;
    int32_t mFrameSize;
    uint8_t* mFrameData;
    uint8_t* mClearData;
    uint8_t* mCachedHeader;
    int32_t mFrameIdx;
    int32_t mModularMode;

    int32_t mSliceNum;

    enum {
        MAX_SLICE_HEADER_NUM = 256,
    };

    // Information of Slices in the Modular DRM Mode
    struct SliceInfo {
        uint8_t sliceHeaderByte; // first byte of the slice header
        uint32_t sliceStartOffset; // offset of Slice unit in the firewalled buffer
        uint32_t sliceByteOffset; // extra offset from the blockAligned slice offset
        uint32_t sliceSize; // block aligned length of slice unit
        uint32_t sliceLength; // actual size of the slice
    };

    SliceInfo mSliceInfo[MAX_SLICE_HEADER_NUM];
};



#endif /* VIDEO_DECODER_AVC_SECURE_H_ */
