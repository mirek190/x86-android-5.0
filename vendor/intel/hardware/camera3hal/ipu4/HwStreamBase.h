/*
 * Copyright (C) 2014 Intel Corporation
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
#ifndef CAMERA3_HAL_HW_STREAM_BASE_H_
#define CAMERA3_HAL_HW_STREAM_BASE_H_

#include "CameraStreamNode.h"
#include "Camera3Request.h"

namespace android {
namespace camera2 {
/**
 * \class HwStreamBase
 * Dummy producer class needed for the AAL
 */
class HwStreamBase : public CameraStreamNode {
public:
    HwStreamBase();
    virtual ~HwStreamBase();

    virtual status_t query(FrameInfo * info);
    virtual status_t registerBuffers(Vector<sp<CameraBuffer> > &theBuffers);
    virtual status_t capture(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);
    virtual status_t captureDone(sp<CameraBuffer> buffer,
                                 Camera3Request* request);
    virtual status_t reprocess(sp<CameraBuffer> buffer,
                               Camera3Request* request);
    virtual void dump(int fd) const;
    virtual status_t configure(void);
};

} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_HW_STREAM_BASE_H_ */
