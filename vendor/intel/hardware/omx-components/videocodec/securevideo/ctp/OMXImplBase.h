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

#ifndef OMX_IMPL_BASE_INCLUDED
#define OMX_IMPL_BASE_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "OMXVideoDecoderBase.h"

class OMXImplBase
{
public:

    // Status codes: success is 0 (different meanings of success codes are positive),
    // failure codes are negative.  Success code numbers increase, error code
    // number decrease.
    enum
    {
        OMXDRM_SUCCESS_BASE = 1000,

        OMXDRM_SUCCESS_FORMAT_CHANGE = (OMXDRM_SUCCESS_BASE + 1),

        OMXDRM_SUCCESS = 0,

        // Error codes begin:

        // Error base value.  Custom error values count down from this value.
        OMXDRM_ERROR_BASE = -1000,

        // General error
        OMXDRM_ERROR = OMXDRM_ERROR_BASE,

        // Invalid argument was passed to a function
        OMXDRM_ERROR_INV_ARG = (OMXDRM_ERROR_BASE - 1),

        // Invalid session ID
        OMXDRM_ERROR_INV_SESSION = (OMXDRM_ERROR_BASE - 2),

        // Corresponds to OMX_ErrorNotReady
        OMXDRM_ERROR_NOT_READY = (OMXDRM_ERROR_BASE - 3),

        // Corresponds to OMX_ErrorHardware
        OMXDRM_ERROR_HW = (OMXDRM_ERROR_BASE - 4),

        // Functionality not implemented
        OMXDRM_ERROR_NOTIMPL = (OMXDRM_ERROR_BASE - 5),

        // WV Modular DRM error base value.
        // drm_wv_mod_result errors returned from drm_wv_mod* functions
        // are subtracted from the base and returned as OMXDRM errors.
        OMXDRM_ERROR_WVMOD_BASE = -2000,
    } ;

#define OMXDRM_MAKE_WVMOD_ERROR(err) (OMXDRM_ERROR_WVMOD_BASE - (err))
#define OMXDRM_EXTRACT_WVMOD_ERROR(err) (OMXDRM_ERROR_WVMOD_BASE - (err))

    // NOTE: Init() fuction is not declared, since each DRM scheme
    // may have a custom way to initialize.

    virtual void Deinit() = 0 ;

    virtual int32_t HandleProcessorStop() = 0 ;

    virtual void HandleProcessorPause() = 0 ;

    virtual void HandleProcessorResume() = 0 ;

    virtual int32_t HandlePrepareDecodeBuffer(
        OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) = 0 ;

protected:
    virtual ~OMXImplBase() { }
} ;
// End of OMXImplBase

#endif // OMX_IMPL_BASE_INCLUDED
