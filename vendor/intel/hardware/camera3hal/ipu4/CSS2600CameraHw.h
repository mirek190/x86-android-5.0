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
#ifndef _CAMERA3_HAL_CSS2600CAMERAHW_H_
#define _CAMERA3_HAL_CSS2600CAMERAHW_H_

#include "ICameraHw.h"
#include "PlatformData.h"
#include "ProcessingUnit.h"
#include "ControlUnit.h"
#include "CaptureUnit.h"
#include "HwStreamBase.h"

namespace android {
namespace camera2 {

/**
 * \enum
 * This enum is used as index when acquiring the partial result metadata buffer
 * In theory there should be one metadata partial result per thread context
 * that writes result
 * in IPU4 ControlUnit and Capture Unit update metadata result and return it
 */
enum PartialResultEnum{
    CONTROL_UNIT_PARTIAL_RESULT = 0,
    PARTIAL_RESULT_COUNT /* keep last to use as counter */
};

class Css2600CameraHw: public ICameraHw {
 public:
    explicit Css2600CameraHw(int cameraId);
    virtual ~Css2600CameraHw();

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
    Css2600CameraHw(const Css2600CameraHw& other);
    Css2600CameraHw& operator=(const Css2600CameraHw& other);
    status_t  initStaticMetadata(void);

 private:  //members
    int mCameraId;
    CameraMetadata* mStaticMeta;
    /**
     * locally cached static metadata tag values
     */
    int mPipelineDepth;  /*!< How many request we allow in the PSL at one time*/
    ProcessingUnit *mProcessingUnit;
    ControlUnit *mControlUnit;
    CaptureUnit *mCaptureUnit;
    // Vector to store dummy Hw streams
    Vector<sp<HwStreamBase> > mDummyHwStreamsVector;
};

} /* namespace camera2 */
} /* namespace android */
#endif /* _CAMERA3_HAL_CSS2600CAMERAHW_H_ */
