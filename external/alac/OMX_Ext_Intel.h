/*
 * Copyright (c) 2008 The Khronos Group Inc.
 *
 * Portions contributed by Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/** @file OMX_Audio_Ext_Intel.h - OpenMax IL version 1.1.2
 *  The OMX_Ext_Intel header file contains Intel vendor-specific
 *  extensions in order to support the Apple Lossless Audio Codec (ALAC).
 */


#ifndef OMX_Ext_Intel_h
#define OMX_Ext_Intel_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Each OMX header must include all required header files to allow the
 *  header to compile without errors.  The includes below are required
 *  for this header file to compile successfully
 */
#include <OMX_Types.h>


/** The OMX_INDEXTYPE enumeration is used to select a structure when either
 *  getting or setting parameters and/or configuration data.  Each entry in
 *  this enumeration maps to an OMX specified structure.  When the
 *  OMX_GetParameter, OMX_SetParameter, OMX_GetConfig or OMX_SetConfig methods
 *  are used, the second parameter will always be an entry from this enumeration
 *  and the third entry will be the structure shown in the comments for the entry.
 *  For example, if the application is initializing a cropping function, the
 *  OMX_SetConfig command would have OMX_IndexConfigCommonInputCrop as the second parameter
 *  and would send a pointer to an initialized OMX_RECTTYPE structure as the
 *  third parameter.
 *
 *  The enumeration entries named with the OMX_Config prefix are sent using
 *  the OMX_SetConfig command and the enumeration entries named with the
 *  OMX_PARAM_ prefix are sent using the OMX_SetParameter command.
 */
typedef enum OMX_INDEXTYPE_EXT_INTEL {
    OMX_IndexComponentStartIntel = OMX_IndexVendorStartUnused,
    OMX_IndexParamAudioAlac           /**< reference: OMX_AUDIO_PARAM_ALACTYPE */
} OMX_INDEXTYPE_EXT_INTEL;

/** Enumeration used to define the possible audio codings.
 *  If "OMX_AUDIO_CodingUnused" is selected, the coding selection must
 *  be done in a vendor specific way.  Since this is for an audio
 *  processing element this enum is relevant.  However, for another
 *  type of component other enums would be in this area.
 */
typedef enum OMX_AUDIO_CODINGTYPE_EXT_INTEL {
    OMX_AUDIO_CodingIntelStart = OMX_AUDIO_CodingVendorStartUnused,  /**< Placeholder value when coding is N/A  */
    OMX_AUDIO_CodingALAC           /**< Any variant of ALAC encoded data */
} OMX_AUDIO_CODINGTYPE_EXT_INTEL;

/** ALAC params */
typedef struct OMX_AUDIO_PARAM_ALACTYPE_EXT_INTEL {
    OMX_U32 nSize;                 /**< Size of this structure, in Bytes */
    OMX_VERSIONTYPE nVersion;      /**< OMX specification version information */
    OMX_U32 nPortIndex;            /**< Port that this structure applies to */
    OMX_U32 nChannels;             /**< Number of channels */
    OMX_U32 nSampleRate;           /**< Sampling rate of the source data.  Use 0 for
                                        variable or unknown sampling rate. */
    OMX_U32 nFrameLength;          /**< Frame length (in audio samples per channel) of the codec.
                                        ALAC-encoded streams typically use 4096. */
    OMX_U8 nCompatibleVersion;
    OMX_U8 nBitDepth;              /**< Bits per decoded sample, any of these are valid: 16, 20, 24, or 32 */
    OMX_U8 nPb;                    /**< 0 <= pb <= 255 */
    OMX_U8 nMb;
    OMX_U8 nKb;
    OMX_U16 nMaxRun;
    OMX_U32 nMaxFrameBytes;
    OMX_U32 nAvgBitRate;
} OMX_AUDIO_PARAM_ALACTYPE_EXT_INTEL;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
