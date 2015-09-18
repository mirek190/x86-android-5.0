/*
* Copyright (c) 2014 Intel Corporation.  All rights reserved.
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

#ifndef IMR_DATA_BUFFER_H
#define IMR_DATA_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMR_BUFFER_SIZE         (8 * 1024 * 1024)
#define NALU_BUFFER_SIZE        8192
#define NALU_HEADER_SIZE        5

// 00 00 00 01, in little endian format.
#define NALU_PREFIX_LE          0x01000000

#define NALU_PREFIX_LENGTH      (NALU_HEADER_SIZE - 1)

#define NALU_TYPE_MASK              0x1F

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

static inline bool IsSPSorPPS(uint8_t naluType)
{
    naluType &= NALU_TYPE_MASK ;
    return naluType == NAL_UNIT_TYPE_SPS || naluType  == NAL_UNIT_TYPE_PPS ;
}

static inline void Copy4Bytes(void* dst, const void* src)
{
    // Don't check input pointers for NULL: this is internal function,
    // and if you pass NULL to it, your code deserves to crash.

    uint8_t* bdst = (uint8_t*) dst ;
    const uint8_t* bsrc = (const uint8_t*) src ;

    *bdst++ = *bsrc++ ;
    *bdst++ = *bsrc++ ;
    *bdst++ = *bsrc++ ;
    *bdst = *bsrc ;
}
// End of Copy4Bytes()

// IsNALUPrefix()
//
// Returns true, if ptr points to a memory location, which contains
// NALU prefix.
//
// Prerequisites: ptr is a valid non-NULL pointer, which points
// to a buffer that is at least NALU_PREFIX_LENGTH in size.  The
// caller is required to do error checking.
//
static inline bool IsNALUPrefix(const uint8_t* ptr)
{
    uint32_t prefix ;
    Copy4Bytes(&prefix, ptr) ;

    return prefix == NALU_PREFIX_LE ;
}
// End of IsNALUPrefix()

// This should be able to fit compressed 1080p video I-frame, use half
// of NV12 1080p frame, which on average uses 12 bits per pixel.
#define MAX_COMPRESSED_FRAME_SIZE   (1920 * 1080 * 3 / 4)

#define MAX_NALUS_IN_FRAME      64

// Integer, which "IMRB", but no 0 terminator
#define IMR_DATA_BUFFER_MAGIC   (0UL | ('B' << 24) | ('R' << 16) | ('M' << 8) | 'I')

#define DRM_SCHEME_NONE         0
#define DRM_SCHEME_WV_CLASSIC   1
#define DRM_SCHEME_WV_MODULAR   2

// Because we need to account for NALU type byte in front of the NALU data
// in IMR, we cannot start IMR at 0, but rather start it at 16.  (16 to account
// for Chaabi DMA alignment dependency.)
#define IMR_START_OFFSET        16

#pragma pack(push, 1)

typedef union NaluHeaderPrefix_tag {
    // filler provides 8 bytes of storage
    uint64_t filler;

    // NALU header prefix is stored here
    uint8_t bytes[NALU_HEADER_SIZE];
}
NaluHeaderPrefix;

typedef struct IMRDataBuffer_tag {

    // Must be set to IMR_DATA_BUFFER_MAGIC
    uint32_t magic;

    uint32_t drmScheme;

    // 1 if clear, 0 if encrypted
    uint32_t  isClear;

    // For clear content, this is the space for clear data.
    // For encrypted content, this is the space for slice/NALU headers.
    uint8_t data[MAX_COMPRESSED_FRAME_SIZE];

    // Size of the data buffer
    uint32_t size ;

    // Session Id for libsepdrm calls, ignored WV Classic
    uint32_t sessionId;

    // IMR offset to start of frame, ignored for clear buffer
    uint32_t frameStartOffset;

    // Size of frame in bytes, ignored for clear buffer
    uint32_t frameSize;

    // Number of elements in naluOffsets array, ignored for clear buffer
    uint32_t numNalus;

    // IMR Offsets for NALUs, ignored for clear buffer
    uint32_t naluOffsets[MAX_NALUS_IN_FRAME];

    // MetaData for the NALU, ignored for clear buffer
    NaluHeaderPrefix naluPrefixes[MAX_NALUS_IN_FRAME];
}
IMRDataBuffer;

#pragma pack(pop)

#define IMR_DATA_BUFFER_VERIFY_MAGIC(buf) (buf->magic == IMR_DATA_BUFFER_MAGIC)
#define IMR_DATA_BUFFER_SET_MAGIC(buf)    (buf->magic = IMR_DATA_BUFFER_MAGIC)

static inline void Init_IMRDataBuffer(IMRDataBuffer* buf)
{
    // This is internal helper function.  If you pass invalid (e.g. NULL)
    // pointer to it, you deserve to crash.

    // Perform initialization of certain members, ignore the data
    // areas, which will be overwritten in the course of the
    // normal usage.

    buf->magic = IMR_DATA_BUFFER_MAGIC ;
    buf->drmScheme = DRM_SCHEME_NONE ;
    buf->isClear = 0 ;

    buf->size = 0 ;
    buf->sessionId = 0 ;
    buf->frameStartOffset = 0 ;
    buf->frameSize = 0 ;
    buf->numNalus = 0 ;
}
// End of Init_IMRDataBuffer()

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // IMR_DATA_BUFFER_H
