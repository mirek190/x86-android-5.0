/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2010-2014 Intel Corporation. All Rights Reserved.

File Name: mfxpcp.h

\* ****************************************************************************** */
#ifndef __MFXPCP_H__
#define __MFXPCP_H__
#include "mfxstructures.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

enum {
    MFX_HANDLE_DECODER_DRIVER_HANDLE            =2,       /* Driver's handle for a video decoder that can be used to configure content protection*/
    MFX_HANDLE_DXVA2_DECODER_DEVICE             =MFX_HANDLE_DECODER_DRIVER_HANDLE,      /* A handle to the DirectX Video Acceleration 2 (DXVA-2) decoder device*/
    MFX_HANDLE_VIDEO_DECODER                    =5       /* Pointer to the IDirectXVideoDecoder or ID3D11VideoDecoder interface*/
}; //mfxHandleType

/* Frame Memory Types */
enum {
    MFX_MEMTYPE_PROTECTED   =   0x0080
};

/* Protected in mfxVideoParam */
enum {
    MFX_PROTECTION_PAVP         =   0x0001,
    MFX_PROTECTION_GPUCP_PAVP   =   0x0002,
    MFX_PROTECTION_GPUCP_AES128_CTR =   0x0003,
    MFX_PROTECTION_RESERVED1        =   0x0004,
};

/* EncryptionType in mfxExtPAVPOption */
enum
{
    MFX_PAVP_AES128_CTR = 2,
    MFX_PAVP_AES128_CBC = 4,
    MFX_PAVP_DECE_AES128_CTR = 16
};

/* CounterType in mfxExtPAVPOption */
enum
{
    MFX_PAVP_CTR_TYPE_A = 1,
    MFX_PAVP_CTR_TYPE_B = 2,
    MFX_PAVP_CTR_TYPE_C = 4
};

/* Extended Buffer Ids */
enum {
    MFX_EXTBUFF_PAVP_OPTION         =   MFX_MAKEFOURCC('P','V','O','P')
};

typedef struct _mfxAES128CipherCounter{
    mfxU64  IV;
    mfxU64  Count;
} mfxAES128CipherCounter;

typedef struct _mfxEncryptedData{
    mfxEncryptedData *Next;
    mfxHDL reserved1;
    mfxU8  *Data;
    mfxU32 DataOffset; /* offset, in bytes, from beginning of buffer to first byte of encrypted data*/
    mfxU32 DataLength; /* size of plain data in bytes */
    mfxU32 MaxLength; /*allocated  buffer size in bytes*/
    mfxAES128CipherCounter CipherCounter;
    mfxU32 reserved2[8];
} mfxEncryptedData;

typedef struct _mfxExtPAVPOption{
    mfxExtBuffer    Header; /* MFX_EXTBUFF_PAVP_OPTION */
    mfxAES128CipherCounter CipherCounter;
    mfxU32      CounterIncrement;
    mfxU16      EncryptionType;
    mfxU16      CounterType;
    mfxU32      reserved[8];
} mfxExtPAVPOption;

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */


#endif

