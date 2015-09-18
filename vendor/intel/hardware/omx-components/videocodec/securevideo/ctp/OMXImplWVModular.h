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

#ifndef OMX_IMPL_WV_MODULAR_INCLUDED
#define OMX_IMPL_WV_MODULAR_INCLUDED

#include <limits.h>
#include "OMXImplBase.h"
#include "VideoFrameInfo.h"

class OMXImplWVModular : public OMXImplBase
{
public:

    enum
    {
        // NOTE: ideally,  instead of ULONG_MAX this should have been
        // std::numeric_limits<uint32_t>::max().  Unfortunately,
        // <limits> header file does't seem to be present on Android.
        INVALID_SESSION_ID = ULONG_MAX,
    } ;

    OMXImplWVModular() ;
    virtual ~OMXImplWVModular() ;

    int32_t Init(uint32_t sessioId) ;

    virtual void Deinit() ;

    virtual int32_t HandleProcessorStop() ;

    virtual void HandleProcessorPause() ;

    virtual void HandleProcessorResume() ;

    virtual int32_t HandlePrepareDecodeBuffer(
        OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) ;

private:

    uint32_t mSessionId ;

    // Right now this code is not getting used across threads.
    // To support multiple threads in future this variable
    // require different treatment. one solution could be,
    // Allocate space in IMRdatabuffer for frame_info_t structure could
    // help to take care of multiple threads.
    frame_info_t mFrameInfo;

} ;
// End of OMXImplWVModular

#endif // OMX_IMPL_WV_MODULAR_INCLUDED

