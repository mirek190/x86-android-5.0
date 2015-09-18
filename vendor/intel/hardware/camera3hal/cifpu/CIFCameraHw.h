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
#ifndef _CAMERA3_HAL_CIFCAMERAHW_H_
#define _CAMERA3_HAL_CIFCAMERAHW_H_

#include "ICameraHw.h"
#include "PlatformData.h"
#include "CaptureUnit.h"
#include "ControlUnit.h"
#include "ipu4/HwStreamBase.h"

namespace android {
namespace camera2 {

class CIFCameraHw: public ICameraHw {
 public:
    explicit CIFCameraHw(int cameraId);
    virtual ~CIFCameraHw();

  // override of ICameraHw
    virtual status_t init(void);
    virtual const camera_metadata_t * getDefaultRequestSettings(int type);
    virtual status_t bindStreams(Vector<CameraStreamNode *> activeStreams);
    virtual status_t configStreams(Vector<camera3_stream_t*> &activeStreams);
    virtual status_t processRequest(Camera3Request* request, int inFlightCount);
    virtual status_t flush();
    virtual void dump(int fd);

 // prevent copy constructor and assignment operator
 private:
    CIFCameraHw(const CIFCameraHw& other);
    CIFCameraHw& operator=(const CIFCameraHw& other);

 private:  //members
    int mCameraId;
    int mPipelineDepth;  /*!< How many request we allow in the PSL at one time*/
    CaptureUnit mCaptureUnit;
    ControlUnit mControlUnit;
    Vector<sp<HwStreamBase> > mDummyStreams; // for destruction
};

} /* namespace camera2 */
} /* namespace android */
#endif /* _CAMERA3_HAL_CIFCAMERAHW_H_ */
