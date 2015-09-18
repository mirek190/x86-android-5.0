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

#ifndef CAMERA3_HAL_JPEGENCODETASK_H_
#define CAMERA3_HAL_JPEGENCODETASK_H_

#include "ProcUnitTask.h"
#include "ImgEncoder.h"
#include "JpegMaker.h"

namespace android {
namespace camera2 {
/**
 * \class RawtoYUVTask
 * Does the jpeg encoding of YUV input buffers.
 * This class listens for completed JPEG buffers from RawtoYUVTask. The JPEG
 * encode task shall execute in its own thread to ensure parallelism.
 * The JpegEncode task shall create its own instance of StreamOutputTask to
 * return the completed JPEG buffer to the framework.
 *
 */
class JpegEncodeTask : public ProcUnitTask, public IPUTaskListener {
public:
    explicit JpegEncodeTask(int cameraId);
    virtual ~JpegEncodeTask();

    virtual status_t executeTask(ProcTaskMsg &msg);
    virtual void getTaskOutputType(void);

private:
    virtual status_t handleExecuteTask(ProcTaskMsg &msg);
    bool notifyPUTaskEvent(PUTaskMessage *msg);
    status_t jpegTaskInit();
    status_t doJPEGEncode(PUTaskMessage *procmsg);

private:
    sp<ImgEncoder> mImgEncoder;
    JpegMaker      *mJpegMaker;
    int mCameraId;
};

}  // namespace camera2
}  // namespace android

#endif  // CAMERA3_HAL_JPEGENCODETASK_H_
