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

#include "VideoDecoderAVCSecure.h"
#include "VideoDecoderTrace.h"
#include <string.h>

#include <va/va.h>
#include "VideoDecoderBase.h"
#include "VideoDecoderAVC.h"
#include "vbp_loader.h"
#include "VideoFrameInfo.h"

#define STARTCODE_00                0x00
#define STARTCODE_01                0x01
#define STARTCODE_PREFIX_LEN        3
#define NALU_TYPE_MASK              0x1F

#define MAX_SLICEHEADER_BUFFER_SIZE 4096
#define MAX_NALU_HEADER_BUFFER 8192

// mask for little endian, to mast the second and fourth bytes in the byte stream
#define STARTCODE_MASK0             0xFF000000 //0x00FF0000
#define STARTCODE_MASK1             0x0000FF00  //0x000000FF


typedef enum {
    NAL_UNIT_TYPE_unspecified0 = 0,
    NAL_UNIT_TYPE_SLICE,
    NAL_UNIT_TYPE_DPA,
    NAL_UNIT_TYPE_DPB,
    NAL_UNIT_TYPE_DPC,
    NAL_UNIT_TYPE_IDR,
    NAL_UNIT_TYPE_SEI,
    NAL_UNIT_TYPE_SPS,
    NAL_UNIT_TYPE_PPS,
    NAL_UNIT_TYPE_Acc_unit_delimiter,
    NAL_UNIT_TYPE_EOSeq,
    NAL_UNIT_TYPE_EOstream,
    NAL_UNIT_TYPE_filler_data,
    NAL_UNIT_TYPE_SPS_extension,
    NAL_UNIT_TYPE_Reserved14,
    NAL_UNIT_TYPE_Reserved15,
    NAL_UNIT_TYPE_Reserved16,
    NAL_UNIT_TYPE_Reserved17,
    NAL_UNIT_TYPE_Reserved18,
    NAL_UNIT_TYPE_ACP,
    NAL_UNIT_TYPE_Reserved20,
    NAL_UNIT_TYPE_Reserved21,
    NAL_UNIT_TYPE_Reserved22,
    NAL_UNIT_TYPE_Reserved23,
    NAL_UNIT_TYPE_unspecified24,
} NAL_UNIT_TYPE;

#ifndef min
#define min(X, Y)  ((X) <(Y) ? (X) : (Y))
#endif


static const uint8_t startcodePrefix[STARTCODE_PREFIX_LEN] = {0x00, 0x00, 0x01};


VideoDecoderAVCSecure::VideoDecoderAVCSecure(const char *mimeType)
    : VideoDecoderAVC(mimeType),
      mNaluHeaderBuffer(NULL),
      mInputBuffer(NULL) {
    mFrameSize = 0;
    mFrameData = NULL;
    mIsEncryptData = 0;
    mClearData = NULL;
    mCachedHeader = NULL;
    setParserType(VBP_H264SECURE);
    mFrameIdx = 0;
    mModularMode = 0;
    mSliceNum = 0;

    memset(&mMetadata, 0, sizeof(NaluMetadata));
    memset(&mByteStream, 0, sizeof(NaluByteStream));
}

VideoDecoderAVCSecure::~VideoDecoderAVCSecure() {
}

Decode_Status VideoDecoderAVCSecure::start(VideoConfigBuffer *buffer) {
    Decode_Status status = VideoDecoderAVC::start(buffer);
    if (status != DECODE_SUCCESS) {
        return status;
    }

    mClearData = new uint8_t [MAX_NALU_HEADER_BUFFER];
    if (mClearData == NULL) {
        ETRACE("Failed to allocate memory for mClearData");
        return DECODE_MEMORY_FAIL;
    }

    mCachedHeader= new uint8_t [MAX_SLICEHEADER_BUFFER_SIZE];
    if (mCachedHeader == NULL) {
        ETRACE("Failed to allocate memory for mCachedHeader");
        return DECODE_MEMORY_FAIL;
    }

    mMetadata.naluInfo = new NaluInfo [MAX_NALU_NUMBER];
    mByteStream.byteStream = new uint8_t [MAX_NALU_HEADER_BUFFER];
    mNaluHeaderBuffer = new uint8_t [MAX_NALU_HEADER_BUFFER];

    if (mMetadata.naluInfo == NULL ||
        mByteStream.byteStream == NULL ||
        mNaluHeaderBuffer == NULL) {
        ETRACE("Failed to allocate memory.");
        // TODO: release all allocated memory
        return DECODE_MEMORY_FAIL;
    }
    return status;
}

void VideoDecoderAVCSecure::stop(void) {
    VideoDecoderAVC::stop();

    if (mClearData) {
        delete [] mClearData;
        mClearData = NULL;
    }

    if (mCachedHeader) {
        delete [] mCachedHeader;
        mCachedHeader = NULL;
    }

    if (mMetadata.naluInfo) {
        delete [] mMetadata.naluInfo;
        mMetadata.naluInfo = NULL;
    }

    if (mByteStream.byteStream) {
        delete [] mByteStream.byteStream;
        mByteStream.byteStream = NULL;
    }

    if (mNaluHeaderBuffer) {
        delete [] mNaluHeaderBuffer;
        mNaluHeaderBuffer = NULL;
    }
}

Decode_Status VideoDecoderAVCSecure::decode(VideoDecodeBuffer *buffer) {
    VTRACE("VideoDecoderAVCSecure::decode");

    Decode_Status status;

    vbp_data_h264 *data = NULL;
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    if (buffer->flag & IS_SUBSAMPLE_ENCRYPTION) {
        mModularMode = 1;
    }

    if (mModularMode) {
        status = processModularInputBuffer(buffer,&data);
        if(!data) {
            return status;
        }

        if (!mVAStarted) {
            if (data->has_sps && data->has_pps) {
                status = startVA(data);
                CHECK_STATUS("startVA");
            } else {
                WTRACE("Can't start VA as either SPS or PPS is still not available.");
                return DECODE_SUCCESS;
            }
        }

        status = decodeFrame(buffer, data);

        return status;
    }

    // legacy classic mode
    int32_t sizeAccumulated = 0;
    int32_t sizeLeft = 0;
    uint8_t *pByteStream = NULL;
    NaluInfo *pNaluInfo = mMetadata.naluInfo;

    if (buffer->flag & IS_SECURE_DATA) {
        pByteStream = buffer->data;
        sizeLeft = buffer->size;
        mInputBuffer = NULL;
    } else {
        status = parseAnnexBStream(buffer->data, buffer->size, &mByteStream);
        CHECK_STATUS("parseAnnexBStream");
        pByteStream = mByteStream.byteStream;
        sizeLeft = mByteStream.streamPos;
        mInputBuffer = buffer->data;
    }
    if (sizeLeft < 4) {
        ETRACE("Not enough data to read number of NALU.");
        return DECODE_INVALID_DATA;
    }

    // read number of NALU
    memcpy(&(mMetadata.naluNumber), pByteStream, sizeof(int32_t));
    pByteStream += 4;
    sizeLeft -= 4;

    if (mMetadata.naluNumber == 0) {
        WTRACE("Number of NALU is ZERO!");
        return DECODE_SUCCESS;
    }

    for (int32_t i = 0; i < mMetadata.naluNumber; i++) {
        if (sizeLeft < 12) {
            ETRACE("Not enough data to parse NALU offset, size, header length for NALU %d, left = %d", i, sizeLeft);
            return DECODE_INVALID_DATA;
        }
        sizeLeft -= 12;
        // read NALU offset
        memcpy(&(pNaluInfo->naluOffset), pByteStream, sizeof(int32_t));
        pByteStream += 4;

        // read NALU size
        memcpy(&(pNaluInfo->naluLen), pByteStream, sizeof(int32_t));
        pByteStream += 4;

        // read NALU header length
        memcpy(&(pNaluInfo->naluHeaderLen), pByteStream, sizeof(int32_t));
        pByteStream += 4;

        if (sizeLeft < pNaluInfo->naluHeaderLen) {
            ETRACE("Not enough data to copy NALU header for %d, left = %d, header len = %d", i, sizeLeft, pNaluInfo->naluHeaderLen);
            return DECODE_INVALID_DATA;
        }

        sizeLeft -=  pNaluInfo->naluHeaderLen;

        if (pNaluInfo->naluHeaderLen) {
            // copy start code prefix to buffer
            memcpy(mNaluHeaderBuffer + sizeAccumulated,
                startcodePrefix,
                STARTCODE_PREFIX_LEN);
            sizeAccumulated += STARTCODE_PREFIX_LEN;

            // copy NALU header
            memcpy(mNaluHeaderBuffer + sizeAccumulated, pByteStream, pNaluInfo->naluHeaderLen);
            pByteStream += pNaluInfo->naluHeaderLen;

            sizeAccumulated += pNaluInfo->naluHeaderLen;
        } else {
            WTRACE("header len is zero for NALU %d", i);
        }

        // for next NALU
        pNaluInfo++;
    }

    buffer->data = mNaluHeaderBuffer;
    buffer->size = sizeAccumulated;

    return VideoDecoderAVC::decode(buffer);
}


Decode_Status VideoDecoderAVCSecure::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    if (mModularMode) {
        return modularDecodeSlice(data, picIndex, sliceIndex);
    }

    // legacy classic mode
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID bufferIDs[4];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);

    if (sliceParam->first_mb_in_slice == 0 || mDecodingFrame == false) {
        // either condition indicates start of a new frame
        if (sliceParam->first_mb_in_slice != 0) {
            WTRACE("The first slice is lost.");
            // TODO: handle the first slice lost
        }
        if (mDecodingFrame) {
            // interlace content, complete decoding the first field
            vaStatus = vaEndPicture(mVADisplay, mVAContext);
            CHECK_VA_STATUS("vaEndPicture");

            // for interlace content, top field may be valid only after the second field is parsed
            mAcquiredBuffer->pictureOrder= picParam->CurrPic.TopFieldOrderCnt;
        }

        // Check there is no reference frame loss before decoding a frame

        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        status = updateReferenceFrames(picData);
        CHECK_STATUS("updateReferenceFrames");

        vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
        CHECK_VA_STATUS("vaBeginPicture");

        // start decoding a frame
        mDecodingFrame = true;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264),
            1,
            picParam,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264),
            1,
            data->IQ_matrix_buf,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateIQMatrixBuffer");
        bufferIDCount++;
    }

    status = setReference(sliceParam);
    CHECK_STATUS("setReference");

    // find which naluinfo is correlated to current slice
    int naluIndex = 0;
    uint32_t accumulatedHeaderLen = 0;
    uint32_t headerLen = 0;
    for (; naluIndex < mMetadata.naluNumber; naluIndex++)  {
        headerLen = mMetadata.naluInfo[naluIndex].naluHeaderLen;
        if (headerLen == 0) {
            WTRACE("lenght of current NAL unit is 0.");
            continue;
        }
        accumulatedHeaderLen += STARTCODE_PREFIX_LEN;
        if (accumulatedHeaderLen + headerLen > sliceData->slice_offset) {
            break;
        }
        accumulatedHeaderLen += headerLen;
    }

    if (sliceData->slice_offset != accumulatedHeaderLen) {
        WTRACE("unexpected slice offset %d, accumulatedHeaderLen = %d", sliceData->slice_offset, accumulatedHeaderLen);
    }

    sliceParam->slice_data_size = mMetadata.naluInfo[naluIndex].naluLen;
    sliceData->slice_size = sliceParam->slice_data_size;

    // no need to update:
    // sliceParam->slice_data_offset - 0 always
    // sliceParam->slice_data_bit_offset - relative to  sliceData->slice_offset

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
    bufferIDCount++;

    // sliceData->slice_offset - accumulatedHeaderLen is the absolute offset to start codes of current NAL unit
    // offset points to first byte of NAL unit
    uint32_t sliceOffset = mMetadata.naluInfo[naluIndex].naluOffset;
    if (mInputBuffer != NULL) {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VASliceDataBufferType,
            sliceData->slice_size, //size
            1,        //num_elements
            mInputBuffer  + sliceOffset,
            &bufferIDs[bufferIDCount]);
    } else {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAProtectedSliceDataBufferType,
            sliceData->slice_size, //size
            1,        //num_elements
            (uint8_t*)sliceOffset, // IMR offset
            &bufferIDs[bufferIDCount]);
    }
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");
    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::modularDecodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 3 buffers to render a slice: picture parameter, IQMatrix, slice parameter
    VABufferID bufferIDs[3];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);
    uint32_t slice_data_size = 0;
    uint8_t* slice_data_addr = NULL;

    if (sliceParam->first_mb_in_slice == 0 || mDecodingFrame == false) {
        // either condition indicates start of a new frame
        if (sliceParam->first_mb_in_slice != 0) {
            WTRACE("The first slice is lost.");
        }
        VTRACE("Current frameidx = %d", mFrameIdx++);
        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        status = updateReferenceFrames(picData);
        CHECK_STATUS("updateReferenceFrames");

        mDecodingFrame = true;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264),
            1,
            picParam,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264),
            1,
            data->IQ_matrix_buf,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateIQMatrixBuffer");
        bufferIDCount++;
    }

    status = setReference(sliceParam);
    CHECK_STATUS("setReference");

    if (mModularMode) {
        if (mIsEncryptData) {
            sliceParam->slice_data_size = mSliceInfo[sliceIndex].sliceSize;
            slice_data_size = mSliceInfo[sliceIndex].sliceSize;
            //slice_data_addr = mFrameData + mSliceInfo[sliceIndex].sliceStartOffset;
        } else {
            slice_data_size = sliceData->slice_size;
            slice_data_addr = sliceData->buffer_addr + sliceData->slice_offset;
        }
    }

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");

    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    VABufferID slicebufferID;

    if (mIsEncryptData) {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAProtectedSliceDataBufferType,
            mSliceInfo[sliceIndex].sliceSize, //size
            1,        //num_elements
            (uint8_t*)mSliceInfo[sliceIndex].sliceStartOffset, // IMR offset, need (uint8_t*)?
            &slicebufferID);

    } else {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VASliceDataBufferType,
            slice_data_size, //size
            1,        //num_elements
            slice_data_addr,
            &slicebufferID);
    }
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        &slicebufferID,
        1);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;

}

// Parse byte string pattern "0x000001" (3 bytes)  in the current buffer.
// Returns offset of position following  the pattern in the buffer if pattern is found or -1 if not found.
int32_t VideoDecoderAVCSecure::findNalUnitOffset(uint8_t *stream, int32_t offset, int32_t length) {
    uint8_t *ptr;
    uint32_t left = 0, data = 0, phase = 0;
    uint8_t mask1 = 0, mask2 = 0;

    /* Meaning of phase:
        0: initial status, "0x000001" bytes are not found so far;
        1: one "0x00" byte is found;
        2: two or more consecutive "0x00" bytes" are found;
        3: "0x000001" patten is found ;
        4: if there is one more byte after "0x000001";
       */

    left = length;
    ptr = (uint8_t *) (stream + offset);
    phase = 0;

    // parse until there is more data and start code not found
    while ((left > 0) && (phase < 3)) {
        // Check if the address is 32-bit aligned & phase=0, if thats the case we can check 4 bytes instead of one byte at a time.
        if (((((uint32_t)ptr) & 0x3) == 0) && (phase == 0)) {
            while (left > 3) {
                data = *((uint32_t *)ptr);
                mask1 = (STARTCODE_00 != (data & STARTCODE_MASK0));
                mask2 = (STARTCODE_00 != (data & STARTCODE_MASK1));
                // If second byte and fourth byte are not zero's then we cannot have a start code here,
                //  as we need two consecutive zero bytes for a start code pattern.
                if (mask1 && mask2) {
                    // skip 4 bytes and start over
                    ptr += 4;
                    left -=4;
                    continue;
                } else {
                    break;
                }
            }
        }

        // At this point either data is not on a 32-bit boundary or phase > 0 so we look at one byte at a time
        if (left > 0) {
            if (*ptr == STARTCODE_00) {
                phase++;
                if (phase > 2) {
                    // more than 2 consecutive '0x00' bytes is found
                    phase = 2;
                }
            } else if ((*ptr == STARTCODE_01) && (phase == 2)) {
                // start code is found
                phase = 3;
            } else {
                // reset lookup
                phase = 0;
            }
            ptr++;
            left--;
        }
    }

    if ((left > 0) && (phase == 3)) {
        phase = 4;
        // return offset of position following the pattern in the buffer which matches "0x000001" byte string
        return (int32_t)(ptr - stream);
    }
    return -1;
}


Decode_Status VideoDecoderAVCSecure::copyNaluHeader(uint8_t *stream, NaluByteStream *naluStream) {
    uint8_t naluType;
    int32_t naluHeaderLen;

    naluType = *(uint8_t *)(stream + naluStream->naluOffset);
    naluType &= NALU_TYPE_MASK;
    // first update nalu header length based on nalu type
    if (naluType >= NAL_UNIT_TYPE_SLICE && naluType <= NAL_UNIT_TYPE_IDR) {
        // coded slice, return only up to MAX_SLICE_HEADER_SIZE bytes
        naluHeaderLen = min(naluStream->naluLen, MAX_SLICE_HEADER_SIZE);
    } else if (naluType >= NAL_UNIT_TYPE_SEI && naluType <= NAL_UNIT_TYPE_PPS) {
        //sps, pps, sei, etc, return the entire NAL unit in clear
        naluHeaderLen = naluStream->naluLen;
    } else {
        return DECODE_FRAME_DROPPED;
    }

    memcpy(naluStream->byteStream + naluStream->streamPos, &(naluStream->naluOffset), sizeof(int32_t));
    naluStream->streamPos += 4;

    memcpy(naluStream->byteStream + naluStream->streamPos, &(naluStream->naluLen), sizeof(int32_t));
    naluStream->streamPos += 4;

    memcpy(naluStream->byteStream + naluStream->streamPos, &naluHeaderLen, sizeof(int32_t));
    naluStream->streamPos += 4;

    if (naluHeaderLen) {
        memcpy(naluStream->byteStream + naluStream->streamPos, (uint8_t*)(stream + naluStream->naluOffset), naluHeaderLen);
        naluStream->streamPos += naluHeaderLen;
    }
    return DECODE_SUCCESS;
}


// parse start-code prefixed stream, also knowns as Annex B byte stream, commonly used in AVI, ES, MPEG2 TS container
Decode_Status VideoDecoderAVCSecure::parseAnnexBStream(uint8_t *stream, int32_t length, NaluByteStream *naluStream) {
    int32_t naluOffset, offset, left;
    NaluInfo *info;
    uint32_t ret = DECODE_SUCCESS;

    naluOffset = 0;
    offset = 0;
    left = length;

    // leave 4 bytes to copy nalu count
    naluStream->streamPos = 4;
    naluStream->naluCount = 0;
    memset(naluStream->byteStream, 0, MAX_NALU_HEADER_BUFFER);

    for (; ;) {
        naluOffset = findNalUnitOffset(stream, offset, left);
        if (naluOffset == -1) {
            break;
        }

        if (naluStream->naluCount == 0) {
            naluStream->naluOffset = naluOffset;
        } else {
            naluStream->naluLen = naluOffset - naluStream->naluOffset - STARTCODE_PREFIX_LEN;
            ret = copyNaluHeader(stream, naluStream);
            if (ret != DECODE_SUCCESS && ret != DECODE_FRAME_DROPPED) {
                WTRACE("copyNaluHeader returned %d", ret);
                return ret;
            }
            // starting position for next NALU
            naluStream->naluOffset = naluOffset;
        }

        if (ret == DECODE_SUCCESS) {
            naluStream->naluCount++;
        }

        // update next lookup position and length
        offset = naluOffset + 1; // skip one byte of NAL unit type
        left = length - offset;
    }

    if (naluStream->naluCount > 0) {
        naluStream->naluLen = length - naluStream->naluOffset;
        memcpy(naluStream->byteStream, &(naluStream->naluCount), sizeof(int32_t));
        // ignore return value, either DECODE_SUCCESS or DECODE_FRAME_DROPPED
        copyNaluHeader(stream, naluStream);
        return DECODE_SUCCESS;
    }

    WTRACE("number of valid NALU is 0!");
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data) {
    VTRACE("VideoDecoderAVCSecure::decodeFrame");
    Decode_Status status;
    VTRACE("data->has_sps = %d, data->has_pps = %d", data->has_sps, data->has_pps);

#if 1
    // Don't remove the following codes, it can be enabled for debugging DPB.
    for (unsigned int i = 0; i < data->num_pictures; i++) {
        VAPictureH264 &pic = data->pic_data[i].pic_parms->CurrPic;
        WTRACE("### %d: decoding frame %.2f, poc top = %d, poc bottom = %d, flags = %d,  reference = %d",
                i,
                buffer->timeStamp/1E6,
                pic.TopFieldOrderCnt,
                pic.BottomFieldOrderCnt,
                pic.flags,
                (pic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
                (pic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE));
    }
#endif
    if (data->new_sps || data->new_pps) {
        status = handleNewSequence(data);
        CHECK_STATUS("handleNewSequence");
    }

    if (mModularMode && (!mIsEncryptData)) {
        if (data->pic_data[0].num_slices == 0) {
            ITRACE("No slice available for decoding.");
            status = mSizeChanged ? DECODE_FORMAT_CHANGE : DECODE_SUCCESS;
            mSizeChanged = false;
            return status;
        }
    }

    uint64_t lastPTS = mCurrentPTS;
    mCurrentPTS = buffer->timeStamp;

    // start decoding a new frame
    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");

    if (mModularMode) { // && mIsEncryptData ??
        parseModularSliceHeader(buffer,data);
    }

    if (status != DECODE_SUCCESS) {
        endDecodingFrame(true);
        return status;
    }

    status = beginDecodingFrame(data);
    CHECK_STATUS("beginDecodingFrame");

   // finish decoding the last frame
    status = endDecodingFrame(false);
    CHECK_STATUS("endDecodingFrame");

    if (isNewFrame(data, lastPTS == mCurrentPTS) == 0) {
        ETRACE("Can't handle interlaced frames yet");
        return DECODE_FAIL;
    }

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::beginDecodingFrame(vbp_data_h264 *data) {
    VTRACE("VideoDecoderAVCSecure::beginDecodingFrame");
    Decode_Status status;
    VAPictureH264 *picture = &(data->pic_data[0].pic_parms->CurrPic);
    if ((picture->flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picture->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        mAcquiredBuffer->referenceFrame = true;
    } else {
        mAcquiredBuffer->referenceFrame = false;
    }

    if (picture->flags & VA_PICTURE_H264_TOP_FIELD) {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_BOTTOM_FIELD | VA_TOP_FIELD;
    } else {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    }

    mAcquiredBuffer->renderBuffer.flag = 0;
    mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;
    mAcquiredBuffer->pictureOrder = getPOC(picture);

    if (mSizeChanged) {
        mAcquiredBuffer->renderBuffer.flag |= IS_RESOLUTION_CHANGE;
        mSizeChanged = false;
    }

    status  = continueDecodingFrame(data);
    return status;
}

Decode_Status VideoDecoderAVCSecure::continueDecodingFrame(vbp_data_h264 *data) {
    VTRACE("VideoDecoderAVCSecure::continueDecodingFrame");
    Decode_Status status;
    vbp_picture_data_h264 *picData = data->pic_data;

    if (mAcquiredBuffer == NULL || mAcquiredBuffer->renderBuffer.surface == VA_INVALID_SURFACE) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        return DECODE_FAIL;
    }
    VTRACE("data->num_pictures = %d", data->num_pictures);
    for (uint32_t picIndex = 0; picIndex < data->num_pictures; picIndex++, picData++) {
        if (picData == NULL || picData->pic_parms == NULL || picData->slc_data == NULL || picData->num_slices == 0) {
            return DECODE_PARSER_FAIL;
        }

        if (picIndex > 0 &&
            (picData->pic_parms->CurrPic.flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD)) == 0) {
            ETRACE("Packed frame is not supported yet!");
            return DECODE_FAIL;
        }
        VTRACE("picData->num_slices = %d", picData->num_slices);
        for (uint32_t sliceIndex = 0; sliceIndex < picData->num_slices; sliceIndex++) {
            status = decodeSlice(data, picIndex, sliceIndex);
            if (status != DECODE_SUCCESS) {
                endDecodingFrame(true);
                // remove current frame from DPB as it can't be decoded.
                removeReferenceFromDPB(picData->pic_parms);
                return status;
            }
        }
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::processModularInputBuffer(VideoDecodeBuffer *buffer, vbp_data_h264 **data)
{
    VTRACE("processModularInputBuffer +++");

    Decode_Status status;
    int32_t clear_data_size = 0;
    uint8_t *clear_data = NULL;

    int32_t nalu_num = 0;
    uint8_t nalu_type = 0;
    int32_t nalu_offset = 0;
    uint32_t nalu_size = 0;
    uint8_t naluType = 0;
    uint8_t *nalu_data = NULL;
    uint32_t sliceidx = 0;

    frame_info_t *pFrameInfo = NULL;
    mSliceNum = 0;
    memset(&mSliceInfo, 0, sizeof(mSliceInfo));
    mIsEncryptData = 0;

    if (buffer->flag & IS_SECURE_DATA) {
        VTRACE("Decoding protected video ...");

        pFrameInfo = (frame_info_t *) buffer->data;
        if (pFrameInfo == NULL) {
            ETRACE("Invalid parameter: pFrameInfo is NULL!");
            return DECODE_MEMORY_FAIL;
        }

        nalu_num  = pFrameInfo->num_nalus;

        if (nalu_num <= 0 || nalu_num >= MAX_NUM_NALUS) {
            ETRACE("Invalid parameter: nalu_num = %d", nalu_num);
            return DECODE_MEMORY_FAIL;
        }

        for (int32_t i = 0; i < nalu_num; i++) {

            nalu_size = pFrameInfo->nalus[i].length;
            nalu_type = pFrameInfo->nalus[i].type;
            nalu_offset = pFrameInfo->nalus[i].imr_offset;
            nalu_data = pFrameInfo->nalus[i].data;
            naluType  = nalu_type & NALU_TYPE_MASK;

            if (naluType >= NAL_UNIT_TYPE_SLICE && naluType <= NAL_UNIT_TYPE_IDR) {
                mIsEncryptData = 1;
                WTRACE("slice idx = %d", sliceidx);
                mSliceInfo[sliceidx].sliceHeaderByte = nalu_type;
                mSliceInfo[sliceidx].sliceStartOffset = nalu_offset;
                mSliceInfo[sliceidx].sliceByteOffset = 0;
                mSliceInfo[sliceidx].sliceLength = nalu_size;
                mSliceInfo[sliceidx].sliceSize = nalu_size;

                WTRACE("sliceHeaderByte = 0x%x", mSliceInfo[sliceidx].sliceHeaderByte);
                WTRACE("sliceStartOffset = %d", mSliceInfo[sliceidx].sliceStartOffset);
                WTRACE("sliceByteOffset = %d", mSliceInfo[sliceidx].sliceByteOffset);
                WTRACE("sliceSize = %d", mSliceInfo[sliceidx].sliceSize);
                WTRACE("sliceLength = %d", mSliceInfo[sliceidx].sliceLength);
#if 0
                uint32_t testsize;
                uint8_t *testdata;
                testsize = mSliceInfo[sliceidx].sliceSize > 64 ? 64 : mSliceInfo[sliceidx].sliceSize ;
                testdata = (uint8_t *)(mFrameData);
                for (int i = 0; i < testsize; i++) {
                    VTRACE("testdata[%d] = 0x%x", i, testdata[i]);
                }
#endif
                sliceidx++;

            } else if (naluType == NAL_UNIT_TYPE_SPS || naluType == NAL_UNIT_TYPE_PPS) {
                if (nalu_data == NULL) {
                    ETRACE("Invalid parameter: nalu_data = NULL for naluType 0x%x", naluType);
                    return DECODE_MEMORY_FAIL;
                }
                memcpy(mClearData + clear_data_size,
                    nalu_data,
                    nalu_size);
                clear_data_size += nalu_size;
            } else {
                ITRACE("Nalu type = 0x%x is skipped", naluType);
                continue;
            }
        }
        clear_data = mClearData;
        mSliceNum = sliceidx;
    } else {
        VTRACE("Decoding clear video ...");
        mIsEncryptData = 0;
        mFrameSize = buffer->size;
        mFrameData = buffer->data;
        clear_data = buffer->data;
        clear_data_size = buffer->size;
    }

    if (clear_data_size > 0) {
        status =  VideoDecoderBase::parseBuffer(
                clear_data,
                clear_data_size,
                false,
                (void**)data);
        CHECK_STATUS("VideoDecoderBase::parseBuffer");
    } else {
        status =  VideoDecoderBase::queryBuffer((void**)data);
        CHECK_STATUS("VideoDecoderBase::queryBuffer");
    }
    return DECODE_SUCCESS;

}

Decode_Status VideoDecoderAVCSecure::parseModularSliceHeader(VideoDecodeBuffer *buffer,
                                                             vbp_data_h264 *data)
{
    Decode_Status status;
    VAStatus vaStatus;

    VABufferID sliceheaderbufferID;
    VABufferID pictureparameterparsingbufferID;
    VABufferID mSlicebufferID;
    int32_t sliceIdx;

    vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
    CHECK_VA_STATUS("vaBeginPicture");

    if (mSliceNum <= 0) {
        WTRACE("Not suppose to run into here (%s) --Slice Num <= 0. just return", __func__);
        return DECODE_SUCCESS;
    }

    if (!mIsEncryptData) {
        WTRACE("Not suppose to run into here (%s) --Not encrypted data. just return", __func__);
        return DECODE_SUCCESS;
    }

    void *sliceheaderbuf;
    memset(mCachedHeader, 0, MAX_SLICEHEADER_BUFFER_SIZE);
    int32_t offset = 0;
    int32_t size = 0;

    for (sliceIdx = 0; sliceIdx < mSliceNum; sliceIdx++) {
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAParseSliceHeaderGroupBufferType,
            MAX_SLICEHEADER_BUFFER_SIZE,
            1,
            NULL,
            &sliceheaderbufferID);
        CHECK_VA_STATUS("vaCreateSliceHeaderGroupBuffer");

        vaStatus = vaMapBuffer(
            mVADisplay,
            sliceheaderbufferID,
            &sliceheaderbuf);
        CHECK_VA_STATUS("vaMapBuffer");

        memset(sliceheaderbuf, 0, MAX_SLICEHEADER_BUFFER_SIZE);

        vaStatus = vaUnmapBuffer(
            mVADisplay,
            sliceheaderbufferID);
        CHECK_VA_STATUS("vaUnmapBuffer");

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAProtectedSliceDataBufferType,
            mSliceInfo[sliceIdx].sliceSize, //size
            1,        //num_elements
            (uint8_t*)mSliceInfo[sliceIdx].sliceStartOffset, // IMR offset, need (uint8_t*)?
            &mSlicebufferID);
        CHECK_VA_STATUS("vaCreateSliceDataBuffer");

        data->pic_parse_buffer->frame_buf_id = mSlicebufferID;
        data->pic_parse_buffer->slice_headers_buf_id = sliceheaderbufferID;
        data->pic_parse_buffer->frame_size = mSliceInfo[sliceIdx].sliceLength;
        data->pic_parse_buffer->slice_headers_size = MAX_SLICEHEADER_BUFFER_SIZE;
        data->pic_parse_buffer->nalu_header.value = mSliceInfo[sliceIdx].sliceHeaderByte;
        data->pic_parse_buffer->slice_offset = mSliceInfo[sliceIdx].sliceByteOffset;
#if 0
        WTRACE("data->pic_parse_buffer->slice_offset = 0x%x", data->pic_parse_buffer->slice_offset);
        WTRACE("pic_parse_buffer->nalu_header.value = %x", data->pic_parse_buffer->nalu_header.value = mSliceInfo[sliceIdx].sliceHeaderByte);
        WTRACE("flags.bits.frame_mbs_only_flag = %d", data->pic_parse_buffer->flags.bits.frame_mbs_only_flag);
        WTRACE("flags.bits.pic_order_present_flag = %d", data->pic_parse_buffer->flags.bits.pic_order_present_flag);
        WTRACE("flags.bits.delta_pic_order_always_zero_flag = %d", data->pic_parse_buffer->flags.bits.delta_pic_order_always_zero_flag);
        WTRACE("flags.bits.redundant_pic_cnt_present_flag = %d", data->pic_parse_buffer->flags.bits.redundant_pic_cnt_present_flag);
        WTRACE("flags.bits.weighted_pred_flag = %d", data->pic_parse_buffer->flags.bits.weighted_pred_flag);
        WTRACE("flags.bits.entropy_coding_mode_flag = %d", data->pic_parse_buffer->flags.bits.entropy_coding_mode_flag);
        WTRACE("flags.bits.deblocking_filter_control_present_flag = %d", data->pic_parse_buffer->flags.bits.deblocking_filter_control_present_flag);
        WTRACE("flags.bits.weighted_bipred_idc = %d", data->pic_parse_buffer->flags.bits.weighted_bipred_idc);
        WTRACE("pic_parse_buffer->expected_pic_parameter_set_id = %d", data->pic_parse_buffer->expected_pic_parameter_set_id);
        WTRACE("pic_parse_buffer->num_slice_groups_minus1 = %d", data->pic_parse_buffer->num_slice_groups_minus1);
        WTRACE("pic_parse_buffer->chroma_format_idc = %d", data->pic_parse_buffer->chroma_format_idc);
        WTRACE("pic_parse_buffer->log2_max_pic_order_cnt_lsb_minus4 = %d", data->pic_parse_buffer->log2_max_pic_order_cnt_lsb_minus4);
        WTRACE("pic_parse_buffer->pic_order_cnt_type = %d", data->pic_parse_buffer->pic_order_cnt_type);
        WTRACE("pic_parse_buffer->residual_colour_transform_flag = %d", data->pic_parse_buffer->residual_colour_transform_flag);
        WTRACE("pic_parse_buffer->num_ref_idc_l0_active_minus1 = %d", data->pic_parse_buffer->num_ref_idc_l0_active_minus1);
        WTRACE("pic_parse_buffer->num_ref_idc_l1_active_minus1 = %d", data->pic_parse_buffer->num_ref_idc_l1_active_minus1);
#endif
        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAParsePictureParameterBufferType,
            sizeof(VAParsePictureParameterBuffer),
            1,
            data->pic_parse_buffer,
            &pictureparameterparsingbufferID);
        CHECK_VA_STATUS("vaCreatePictureParameterParsingBuffer");

        vaStatus = vaRenderPicture(
            mVADisplay,
            mVAContext,
            &pictureparameterparsingbufferID,
            1);
        CHECK_VA_STATUS("vaRenderPicture");

        vaStatus = vaMapBuffer(
            mVADisplay,
            sliceheaderbufferID,
            &sliceheaderbuf);
        CHECK_VA_STATUS("vaMapBuffer");

        size = *(uint32 *)((uint8 *)sliceheaderbuf + 4) + 4;
        VTRACE("slice header size = 0x%x, offset = 0x%x", size, offset);
        if (offset + size <= MAX_SLICEHEADER_BUFFER_SIZE - 4) {
            memcpy(mCachedHeader+offset, sliceheaderbuf, size);
            offset += size;
        } else {
            ETRACE("Allocated buffer is not enough for slice header cached, return fail ...");
            return DECODE_FAIL;
        }
        vaStatus = vaUnmapBuffer(
            mVADisplay,
            sliceheaderbufferID);
        CHECK_VA_STATUS("vaUnmapBuffer");
    }
    memset(mCachedHeader + offset, 0xFF, 4);
    status = updateSliceParameter(data,mCachedHeader);
    CHECK_STATUS("processSliceHeader");
    return DECODE_SUCCESS;
}


Decode_Status VideoDecoderAVCSecure::updateSliceParameter(vbp_data_h264 *data, void *sliceheaderbuf) {
    VTRACE("VideoDecoderAVCSecure::updateSliceParameter");
    Decode_Status status;
    status =  VideoDecoderBase::updateBuffer(
            (uint8_t *)sliceheaderbuf,
            MAX_SLICEHEADER_BUFFER_SIZE,
            (void**)&data);
    CHECK_STATUS("updateBuffer");
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::getCodecSpecificConfigs(
    VAProfile profile, VAConfigID *config)
{
    VAStatus vaStatus;
    VAConfigAttrib attrib[2];

    if (config == NULL) {
        ETRACE("Invalid parameter!");
        return DECODE_FAIL;
    }

    attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].type = VAConfigAttribDecSliceMode;
    attrib[1].value = VA_DEC_SLICE_MODE_NORMAL;
    if (mModularMode) {
        attrib[1].value = VA_DEC_SLICE_MODE_SUBSAMPLE;
    }

    vaStatus = vaCreateConfig(
            mVADisplay,
            profile,
            VAEntrypointVLD,
            &attrib[0],
            2,
            config);
    CHECK_VA_STATUS("vaCreateConfig");

    return DECODE_SUCCESS;
}

