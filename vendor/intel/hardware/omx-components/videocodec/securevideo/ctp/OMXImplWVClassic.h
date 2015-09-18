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

#ifndef OMX_IMPL_WV_CLASSIC_INCLUDED
#define OMX_IMPL_WV_CLASSIC_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "OMXImplBase.h"

class OMXImplWVClassic : public OMXImplBase
{
public:

    OMXImplWVClassic() ;
    virtual ~OMXImplWVClassic() ;

    int32_t Init() ;

    virtual void Deinit() ;

    virtual int32_t HandleProcessorStop() ;

    virtual void HandleProcessorPause() ;

    virtual void HandleProcessorResume() ;

    virtual int32_t HandlePrepareDecodeBuffer(
        OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) ;


private:

    static void KeepAliveTimerCallback(sigval v);
    void KeepAliveTimerCallback();

    timer_t mKeepAliveTimer;
} ;
// End of OMXImplWVClassic

#endif // OMX_IMPL_WV_CLASSIC_INCLUDED

