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

#ifndef CAMERA3_HAL_RAWTOYUVTASK_H_
#define CAMERA3_HAL_RAWTOYUVTASK_H_

#include "ProcUnitTask.h"
#include "HalTypes.h"

namespace android {
namespace camera2 {
/**
 * \class RawtoYUVTask
 * Handles the all the RAW to YUV conversions needed for the request buffers
 * This class creates all the psys pipelines and makes the decision on which
 * pys pipeline shall be used for optimal performance. The raw to yuv conversion
 * shall happen in the class's own thread. StreamOutputTask needed to send
 * the filled buffers to the framework is created in the streamConfigAnalyse()
 * The RawtoYUVTask class also enables the zero buffer copy  for JPEG and does
 * the pipeline decision for getting YUV data to do the JPEG encoding
 *
 */
class PSysPipeline;
class RawtoYUVTask : public ProcUnitTask {
public:
    RawtoYUVTask(FrameInfo cuFInfo, Vector<camera3_stream_t *> &yuvStreams,
                 Vector<camera3_stream_t *> &blobStreams, int cameraId);
    virtual ~RawtoYUVTask();

    virtual status_t executeTask(ProcTaskMsg &msg);
    virtual void getTaskOutputType(void);

private:
    virtual status_t handleExecuteTask(ProcTaskMsg &msg);
    status_t createPipelines(void);
    status_t createPreviewPipeline(void);
    status_t createVFPPPipelines(void);
    status_t createCapturePipeline(void);
    status_t createBXTHWPipeline(void);
    status_t executePostProcPipeline(sp<CameraBuffer> output_buffer, ProcTaskMsg &msg);
    status_t executeCapturePipeline(sp<CameraBuffer> output_buffer, ProcTaskMsg &msg);
    void analyzeIntent(ProcTaskMsg &msg);
    status_t getJPEGInterBuffer(CameraStream *stream, sp<CameraBuffer> jpegOutBuffer, ProcTaskMsg &msg);
    status_t fillJPEGOutBuffer(CameraStream *stream, sp<CameraBuffer> outBuffer, ProcTaskMsg &msg);
    unsigned int getBppForCURawV4L2Format(int fourcc);

private:
    /**
     * \struct JpegSource
     * The JPEG Output buffer and its stream.
     */
    struct JpegSource {
        sp<CameraBuffer> buffer;
        CameraStream * stream;
        int streamType;
    };

private:
    FrameInfo mCUFInfo;
    Vector<camera3_stream_t *> mYuvStreams;
    Vector<camera3_stream_t *> mBlobStreams;
    Vector<sp<PSysPipeline> > mBXTHWPipelines;
    Vector<sp<PSysPipeline> > mPostProcPipelines; /*!< Vector to store Post processing pipelines */
    Vector<sp<PSysPipeline> > mCapturePipelines; /*!< Vector to store Capture pipelines */
    sp<PSysPipeline> mPrevPipeline; /*!< prev var pipeline , converts from RAW to YUV_Line format*/
    sp<CameraBuffer> mPrevOutputyuvLine;  /*!< intermediate buffer to store yuv line data to be reused with multiple vfpp pipeline in a request.*/
    bool mPrevPipelineRunForReq; /*!< Flag to indicate prev_var pipeline run, set once per request, cleared at request complete*/
    uint8_t mRequestIntent;
    uint8_t mRequestType;
    int mCameraId;
    Vector<sp<CameraBuffer> > mJpegInterbuffers;
    Vector<sp<CameraBuffer> > mYUVCompleteBuffers;
    Vector<JpegSource> mJpegWaitForYUVQueue;
    JpegSource mJpegInput;
   };

}  // namespace camera2
}  // namespace android

#endif  // CAMERA3_HAL_RAWTOYUVTASK_H_
