/*
 * Copyright (c) 2014 Intel Corporation.
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

#ifndef CAMERA3_HAL_CAPTUREBUFFER_H_
#define CAMERA3_HAL_CAPTUREBUFFER_H_

#include "CameraBuffer.h"

namespace android {
namespace camera2 {

struct CaptureBuffer;
class IBufferOwner
{
public:
    virtual void returnBuffer(CaptureBuffer* buffer) = 0;
    virtual ~IBufferOwner(){};
};

/**
 * \struct CaptureBuffer
 * Container class for buffers captured by the input system
 * This struct can wrap internally allocated buffers or buffers
 * from the client request.
 */
struct CaptureBuffer {
    struct v4l2_buffer      v4l2Buf;
    sp<CameraBuffer>        buf;
    IBufferOwner*           owner;
};

};  // namespace camera2
};  // namespace android

#endif  // CAMERA3_HAL_CAPTUREBUFFER_H_
