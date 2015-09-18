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

#define LOG_TAG "RawtoYUV_Task"

#include "RawtoYUVTask.h"
#include "LogHelper.h"
#include "Camera3GFXFormat.h"
#include "CameraStream.h"
#include "StreamOutputTask.h"
#include "UtilityMacros.h"
#include "PSysPipeline.h"
#include "CameraMetadataHelper.h"

namespace android {
namespace camera2 {

RawtoYUVTask::RawtoYUVTask(FrameInfo cuFInfo,
                           Vector<camera3_stream_t *> &yuvStreams,
                           Vector<camera3_stream_t *> &blobStreams,
                           int cameraId) :
    ProcUnitTask("RawtoYUVTask"),
    mCUFInfo(cuFInfo),
    mYuvStreams(yuvStreams),
    mBlobStreams(blobStreams),
    mCameraId(cameraId)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mPostProcPipelines.clear();
    mCapturePipelines.clear();
    mPrevPipeline.clear();
    mJpegInterbuffers.clear();
    mYUVCompleteBuffers.clear();
    mJpegWaitForYUVQueue.clear();

    createPipelines();
}

RawtoYUVTask::~RawtoYUVTask()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mPostProcPipelines.clear();
    mCapturePipelines.clear();
    mPrevPipeline.clear();
    mJpegInterbuffers.clear();
    mYUVCompleteBuffers.clear();
    mJpegWaitForYUVQueue.clear();
}

void RawtoYUVTask::getTaskOutputType(void)
{
}

unsigned int
RawtoYUVTask::getBppForCURawV4L2Format(int fourcc)
{
    LOG1("%s: CaptureUnit RAW format %s", __FUNCTION__, v4l2Fmt2Str(fourcc));
    if (!isBayerFormat(fourcc)) {
        LOGE("%s: CaptureUnit RAW format %s is not bayer", __FUNCTION__, v4l2Fmt2Str(fourcc));
        return 0;
    }

    const CameraFormatBridge* afb = getCameraFormatBridge(fourcc);
    // For extended bayer formats the depth value should be 10, but in the
    // format-bridge it is 16 change it here
    if (afb->depth == 16)
        return 10;
    else
        return afb->depth;
}

// N-1 pipeline
/**
 * createPreviewPipeline
 * Create the Preview_Var pipeline which convets the RAW input to
 * YUV_Line format. The input resolution will be what comes from the Capture Unit
 * and output resoultion will be the same. For N-1 need to remove the padding
 * that we get with the input resolution. Create a buffer to store the YUV_Line output
 * to be used as input for the post processing pipelines.
 *
 */
status_t
RawtoYUVTask::createPreviewPipeline(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    status_t status = NO_ERROR;
    ia_cipf_frame_format_t input_stream, prev_output_stream;
    unsigned int bitsPerPixel = 0;

    bitsPerPixel = getBppForCURawV4L2Format(mCUFInfo.format);
    LOG1("%s: Using %d bpp for RAW ", __FUNCTION__, bitsPerPixel);

    // Create Prev_Var pipeline
    input_stream.width = mCUFInfo.width;
    input_stream.height = mCUFInfo.height;
    input_stream.fourcc = css_fourcc_raw;

    if (mCUFInfo.stride % 128 != 0)
        LOGW("%s: Capture Unit input stride is not aligned to ISP requirement %d",
             __FUNCTION__, mCUFInfo.stride);

    input_stream.bpl =  mCUFInfo.stride * (ALIGN8(bitsPerPixel)/8U);
    input_stream.bpp = bitsPerPixel;

    prev_output_stream.width = input_stream.width - 12;
    prev_output_stream.height = input_stream.height - 12;
    prev_output_stream.bpl =  ALIGN128(input_stream.width - 12);
    prev_output_stream.fourcc = css_fourcc_internal_yuv_line;
    prev_output_stream.bpp = 12;

    mPrevPipeline = new PSysPipeline(input_stream,
                                     prev_output_stream,
                                     String8("video0.2401.fw"));
    if (mPrevPipeline == NULL) {
        LOGE("Error creating preview var pipeline object");
        return UNKNOWN_ERROR;
    }
    status = mPrevPipeline->createPipeline();
    if (status != NO_ERROR) {
        LOGE("Error creating pipeline ");
        return UNKNOWN_ERROR;
    }

    // Create intermediate yuv line buffer for vfpp input
    mPrevOutputyuvLine = MemoryUtils::allocateHeapBuffer(prev_output_stream.width,
                                                         prev_output_stream.height,
                                                         prev_output_stream.bpl,
                                                         V4L2_PIX_FMT_NV12,
                                                         mCameraId);
    if (mPrevOutputyuvLine == NULL) {
        LOGE("Failed to allocate yuv line buffer");
        return NO_MEMORY;
    }

    return status;
}

// N-1 pipeline
/**
 * createVFPPPipelines
 * Creates the post processing pipelines which converts the YUV_Line input to
 * NV12 format. The input resolution will be what comes from the Capture Unit
 * and output resolution will be YUV stream resolution. A pipeline is created
 * for each YUV stream. For N-1 need to remove the padding
 * that we get with the input resolution. Store the created pipelines in a vector
 *
 */
status_t
RawtoYUVTask::createVFPPPipelines(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    status_t status = NO_ERROR;
    ia_cipf_frame_format_t output_stream, prev_output_stream;
    sp<PSysPipeline> pipeline_vfpp;

    prev_output_stream.width = mCUFInfo.width - 12;
    prev_output_stream.height = mCUFInfo.height - 12;
    prev_output_stream.bpl =  ALIGN128(mCUFInfo.width - 12);
    prev_output_stream.fourcc = css_fourcc_internal_yuv_line;
    prev_output_stream.bpp = 12;

    // Create vfpp pipeline for each yuv stream resolution
    for (unsigned int i = 0; i < mYuvStreams.size(); i++) {
        output_stream.width = mYuvStreams[i]->width;
        output_stream.height = mYuvStreams[i]->height;
        output_stream.fourcc = ia_cipf_frame_fourcc_nv12;
        output_stream.bpl = ALIGN64(mYuvStreams[i]->width);
        output_stream.bpp = 12;

        pipeline_vfpp = new PSysPipeline(prev_output_stream,
                                         output_stream,
                                         String8("videopp0.2401.fw"));
        if (pipeline_vfpp == NULL) {
            LOGE("Error creating vfpp pipeline object");
            return UNKNOWN_ERROR;
        }
        status = pipeline_vfpp->createPipeline();
        if (status != NO_ERROR) {
            LOGE("Error creating pipeline ");
            return UNKNOWN_ERROR;
        }
        mPostProcPipelines.add(pipeline_vfpp);
    }

    return status;
}

// N-1 pipeline
/**
 * createCapturePipeline
 * Creates the Capture pipelines which convets the RAW input to NV12 format.
 * This pipeline is used specifically for JPEG capture streams only
 * The input resolution will be what comes from the Capture Unit
 * and output resoultion will be BLOB stream resolution. A pipeline is created
 * for each BLOB stream. Store the created pipelines in a vector
 *
 */
status_t
RawtoYUVTask::createCapturePipeline(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    ia_cipf_frame_format_t secoutput_stream, input_stream, output_stream;
    sp<PSysPipeline> pipeline_hal;
    // Empty frame struct
    const ia_cipf_frame_format_t emptyFmt = {0, 0, 0, 0, 0};
    unsigned int bitsPerPixel = 0;

    bitsPerPixel = getBppForCURawV4L2Format(mCUFInfo.format);
    LOG1("%s: Using %d bpp for RAW ", __FUNCTION__, bitsPerPixel);

    input_stream.width = mCUFInfo.width;
    input_stream.height = mCUFInfo.height;
    input_stream.fourcc = css_fourcc_raw;
    input_stream.bpl =  mCUFInfo.stride * (ALIGN8(bitsPerPixel)/8U);
    input_stream.bpp = bitsPerPixel;

    for (unsigned int i = 0; i < mBlobStreams.size(); i++) {
        output_stream.width = mBlobStreams[i]->width;
        output_stream.height = mBlobStreams[i]->height;
        output_stream.fourcc = ia_cipf_frame_fourcc_nv12;
        output_stream.bpl = ALIGN64(mBlobStreams[i]->width);
        output_stream.bpp = 12;

        secoutput_stream = output_stream;
        secoutput_stream.fourcc = css_fourcc_internal_yuv_line;
        // Do not use intermediate format for capture pipeline
        pipeline_hal = new PSysPipeline(input_stream,
                                        emptyFmt,
                                        output_stream,
                                        secoutput_stream,
                                        String8("capturepp0.2401.fw"));
        if (pipeline_hal == NULL) {
            LOGE("Error creating preview full pipeline object");
            return UNKNOWN_ERROR;
        }

        status = pipeline_hal->createPipeline();
        if (status != NO_ERROR) {
            LOGE("Error creating pipeline ");
            return UNKNOWN_ERROR;
        }
        mCapturePipelines.add(pipeline_hal);

        // Create Blob intermediate buffer to use in case YUV buffer is not avaialable
        sp<CameraBuffer> interBuffer = MemoryUtils::allocateHeapBuffer(
                    mBlobStreams[i]->width,
                    mBlobStreams[i]->height,
                    mBlobStreams[i]->width,
                    V4L2_PIX_FMT_NV12,
                    mCameraId);
        mJpegInterbuffers.push(interBuffer);
    }
    return status;
}

// BXT HW pipeline
/**
 * createBXTHWPipeline
 * Creates the pipeline available in the BXT HW which will be a preview-full
 * type of pipleine.
 *
 */
status_t
RawtoYUVTask::createBXTHWPipeline(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;

    ia_cipf_frame_format_t input_stream, output_stream;
    sp<PSysPipeline> pipeline_hal;
    // Empty frame struct
    const ia_cipf_frame_format_t emptyFmt = {0, 0, 0, 0, 0};
    unsigned int bitsPerPixel = 0;

    bitsPerPixel = getBppForCURawV4L2Format(mCUFInfo.format);
    LOG1("%s: Using %d bpp for RAW ", __FUNCTION__, bitsPerPixel);

    input_stream.width = mCUFInfo.width;
    input_stream.height = mCUFInfo.height;
    input_stream.fourcc = ia_cipf_frame_fourcc_grbg;
    input_stream.bpl =  mCUFInfo.width;
    input_stream.bpp = bitsPerPixel;

    // Create BXTHWPipeline for each YUV stream resolution
    for (unsigned int i = 0; i < mYuvStreams.size(); i++) {
        output_stream.width = mYuvStreams[i]->width;
        output_stream.height = mYuvStreams[i]->height;
        output_stream.fourcc = ia_cipf_frame_fourcc_nv12;
        output_stream.bpl = mYuvStreams[i]->width;
        output_stream.bpp = 12;

        // Do not use intermediate format for capture pipeline
        pipeline_hal = new PSysPipeline(input_stream,
                                        emptyFmt,
                                        output_stream,
                                        String8("test-4k60.2600.fw"));
        if (pipeline_hal == NULL) {
            LOGE("Error creating preview full pipeline object");
            return UNKNOWN_ERROR;
        }

        status = pipeline_hal->createPipeline();
        if (status != NO_ERROR) {
            LOGE("Error creating pipeline ");
            return UNKNOWN_ERROR;
        }

        mBXTHWPipelines.add(pipeline_hal);
    }

    // Create BXTHWPipeline for each BLOB stream resolution
    for (unsigned int i = 0; i < mBlobStreams.size(); i++) {
        output_stream.width = mBlobStreams[i]->width;
        output_stream.height = mBlobStreams[i]->height;
        output_stream.fourcc = ia_cipf_frame_fourcc_nv12;
        output_stream.bpl = mBlobStreams[i]->width;
        output_stream.bpp = 12;

        pipeline_hal = new PSysPipeline(input_stream,
                                        emptyFmt,
                                        output_stream,
                                        String8("test-4k60.2600.fw"));
        if (pipeline_hal == NULL) {
            LOGE("Error creating preview full pipeline object");
            return UNKNOWN_ERROR;
        }

        status = pipeline_hal->createPipeline();
        if (status != NO_ERROR) {
            LOGE("Error creating pipeline ");
            return UNKNOWN_ERROR;
        }

        mBXTHWPipelines.add(pipeline_hal);
        // Create Blob intermediate buffer to use in case YUV buffer is not avaialable
        sp<CameraBuffer> interBuffer = MemoryUtils::allocateHeapBuffer(
                    mBlobStreams[i]->width,
                    mBlobStreams[i]->height,
                    mBlobStreams[i]->width,
                    V4L2_PIX_FMT_NV12,
                    mCameraId);
        mJpegInterbuffers.push(interBuffer);
    }

    return status;
}

/**
 * createPipeline
 * Create pipelines based on the stream resolutions
 */
status_t
RawtoYUVTask::createPipelines(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
#ifdef BXTPOC // N-1
    status = createPreviewPipeline();
    status |= createVFPPPipelines();
    status |= createCapturePipeline();
#else
    status = createBXTHWPipeline();
#endif
    if (status != NO_ERROR)
        LOGE("%s: Error creating pipelines!", __FUNCTION__);
    return status;
}

status_t
RawtoYUVTask::executeCapturePipeline(sp<CameraBuffer> output_buffer, ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    sp<PSysPipeline> pipeline;

    unsigned int i;

#ifdef BXTPOC // N-1
    if (mRequestIntent == ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE ||
            ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) {
        for (i = 0; i < mCapturePipelines.size(); i++) {
            pipeline = mCapturePipelines.editItemAt(i);

            if (!COMPARE_RESOLUTION (pipeline, output_buffer))
                break;
        }
        if (i == mCapturePipelines.size()) {
            LOGE("@%s: Capture Pipeline not found for buffer %x", __FUNCTION__,
                 output_buffer->format());
            return UNKNOWN_ERROR;
        }

    } else {
        LOG1("@%s: non-Capture or video Snapshot Intent, using PostProc pipeline for JPEG", __FUNCTION__);
        for (i = 0; i < mPostProcPipelines.size(); i++) {
            pipeline = mPostProcPipelines.editItemAt(i);

            if (!COMPARE_RESOLUTION (pipeline, output_buffer))
                break;
        }
        if (i == mPostProcPipelines.size()) {
            LOGE("@%s: PostProc Pipeline not found for buffer %x", __FUNCTION__,
                 output_buffer->format());
            return UNKNOWN_ERROR;
        }
    }

    LOG1("Iterate Capture pipleine");
    status = pipeline->registerBuffer(msg.data.rawCapture->buf, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error registering buffer BLOB", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    status = pipeline->iterate(msg.data.rawCapture->buf, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error in capture pipleine iteration", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
#else
    for (i = 0; i < mBXTHWPipelines.size(); i++) {
        pipeline = mBXTHWPipelines.editItemAt(i);

        if (!COMPARE_RESOLUTION (pipeline, output_buffer))
            break;
    }

    if (i == mBXTHWPipelines.size()) {
        LOGE("@%s: BXTHW Pipeline not found for buffer %x", __FUNCTION__,
             output_buffer->format());
        return UNKNOWN_ERROR;
    }

    LOG2("Iterate BXT HW pipeline for Capture");
    status = pipeline->registerBuffer(mInput.rawCapture->buf, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error registering buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    status = pipeline->iterate(mInput.rawCapture->buf, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error in BXT HW pipeline iteration", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
#endif
    return status;
}

status_t
RawtoYUVTask::executePostProcPipeline(sp<CameraBuffer> output_buffer, ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    sp<PSysPipeline> pipeline;
    unsigned int i;

#ifdef BXTPOC // N-1
    for (i = 0; i < mPostProcPipelines.size(); i++) {
        pipeline = mPostProcPipelines.editItemAt(i);

        if (!COMPARE_RESOLUTION (pipeline, output_buffer))
            break;
    }

    if (i == mPostProcPipelines.size()) {
        LOGE("@%s: PostProc Pipeline not found for buffer %x", __FUNCTION__,
             output_buffer->format());
        return UNKNOWN_ERROR;
    }

    // first run prev pipeline
    if (mPrevPipelineRunForReq == false) {
        LOG2("Iterate preview var pipeline");
        status = mPrevPipeline->registerBuffer(msg.data.rawCapture->buf, mPrevOutputyuvLine);
        if (status != NO_ERROR) {
            LOGE("@%s: Error registering buffer", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
        status = mPrevPipeline->iterate(msg.data.rawCapture->buf, mPrevOutputyuvLine);
        if (status != NO_ERROR) {
            LOGE("@%s: Error in preview full pipeline iteration",
                 __FUNCTION__);
            return UNKNOWN_ERROR;
        }
        mPrevPipelineRunForReq = true;
    }

    LOG2("Iterate preview VFPP pipeline");
    status = pipeline->registerBuffer(mPrevOutputyuvLine, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error registering buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    status = pipeline->iterate(mPrevOutputyuvLine, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error in vfpp pipeline iteration", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
#else
    for (i = 0; i < mBXTHWPipelines.size(); i++) {
        pipeline = mBXTHWPipelines.editItemAt(i);

        if (!COMPARE_RESOLUTION (pipeline, output_buffer))
            break;
    }

    if (i == mBXTHWPipelines.size()) {
        LOGE("@%s: BXTHW Pipeline not found for buffer %x", __FUNCTION__,
             output_buffer->format());
        return UNKNOWN_ERROR;
    }

    LOG2("Iterate BXT HW pipeline for Preview");
    status = pipeline->registerBuffer(mInput.rawCapture->buf, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error registering buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    status = pipeline->iterate(mInput.rawCapture->buf, output_buffer);
    if (status != NO_ERROR) {
        LOGE("@%s: Error in BXT HW pipeline iteration", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
#endif
    return status;
}

/**
 * analyzeIntent
 * Analyze the intent of the request. Currently being used to determine which
 * pipeline to use to get the YUV data for the JPEG encoding
 */
void RawtoYUVTask::analyzeIntent(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    const CameraMetadata* settings = msg.data.request->getSettings();

    mRequestType = ANDROID_REQUEST_TYPE_CAPTURE;
    mRequestIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    MetadataHelper::getMetadataValue(*settings, ANDROID_REQUEST_TYPE, mRequestType);
    MetadataHelper::getMetadataValue(*settings, ANDROID_CONTROL_CAPTURE_INTENT, mRequestIntent);

    if (mRequestType == ANDROID_REQUEST_TYPE_REPROCESS) {
        LOG2("%s: Request type: ANDROID_REQUEST_TYPE_REPROCESS", __FUNCTION__);
    }

    switch (mRequestIntent) {
    case ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW:
        LOG2("%s: Request intent: ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW", __FUNCTION__);
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE:
        LOG2("%s: Request intent: ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE", __FUNCTION__);
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD:
        LOG2("%s: Request intent: ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD", __FUNCTION__);
         break;
    case ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT:
        LOG2("%s: Request intent: ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT", __FUNCTION__);
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
        LOG2("%s: Request intent: ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG", __FUNCTION__);
        break;
    case ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM:
        LOG2("%s: Request intent: ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM", __FUNCTION__);
        break;
    default:
        LOGE("%s: Not implement for %d yet!", __FUNCTION__, mRequestIntent);
        break;
    }
}

/**
 * getJPEGInterBuffer
 * Fill mJpegInput struct
 * Check for a corresponding YUV buffer for given JPEG output buffer.
 * Enable zero buffer copy by using Processed YUV buffers as JPEG input if
 * a YUV stream corresponding to the JPEG stream is available. If not use
 * internally created buffers for this purpose
 */
status_t
RawtoYUVTask::getJPEGInterBuffer(CameraStream *stream,
                                 sp<CameraBuffer> jpegOutBuffer,
                                 ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    camera3_stream_t *matchStream = NULL;
    int width = 0;
    int height = 0;
    unsigned int i;
    struct JpegSource jSrc;
    sp<CameraBuffer> streamBuffer = NULL;

    mJpegInput.buffer = NULL;
    mJpegInput.stream = NULL;
    mJpegInput.streamType = CAMERA3_STREAM_OUTPUT;

    Camera3Request* request = msg.data.request;

    // Check if YUV buffer of same size is avaialble in the request
    // First check YUV streams
    // Bidirectional YUV streams are also part of the array
    for (i = 0; i < mYuvStreams.size(); i++) {
        width = mYuvStreams[i]->width;
        height = mYuvStreams[i]->height;
        if (width == jpegOutBuffer->width() && height == jpegOutBuffer->height()) {
            matchStream = mYuvStreams[i];
            break;
        }
    }
    if (i == mYuvStreams.size()) {
        LOG2("%s: No YUV stream equivalent buffer found, Using created inter buffer", __FUNCTION__);
        goto noBuffer;
    }

    // Check if the buffer for the equivalent YUV stream is available in the request.
    streamBuffer = request->findBuffer(reinterpret_cast<CameraStream *>(matchStream->priv));
    if (streamBuffer == NULL) {
        LOG2("%s Buffer not found for corresponding YUV stream in Request, using created inter buffer", __FUNCTION__);
        goto noBuffer;
    } else {
        // If stream is bidirectional the buffer is filled, so return the buffer
        if (matchStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL &&
                mRequestType == ANDROID_REQUEST_TYPE_REPROCESS) {
            LOG2("%s: Bidirectional stream streamBuffer width = %d  height = %d format = %x",
                 __FUNCTION__, streamBuffer->width(), streamBuffer->height(), streamBuffer->format());

            if (!streamBuffer->isLocked()) {
                status = streamBuffer->lock();
                if (status != NO_ERROR) {
                    LOGE("@%s: Could not lock the  input buffer", __FUNCTION__);
                    return UNKNOWN_ERROR;
                }
            }
            status = streamBuffer->waitOnAcquireFence();
            if (status != NO_ERROR) {
                LOGW("Wait on fence for input buffer %p timed out", streamBuffer.get());
            }
            mJpegInput.buffer = streamBuffer;
            mJpegInput.streamType = CAMERA3_STREAM_BIDIRECTIONAL;
            mJpegInput.stream = reinterpret_cast<CameraStream *>(matchStream->priv);
            return status;
        } else {
            goto noBuffer;
        }
    }

    // YUV stream avaialble, buffer is also available in the request,
    // check if YUV processing is complete for the buffer
    for (unsigned int j = 0; j < mYUVCompleteBuffers.size(); j++) {
        if (!COMPARE_RESOLUTION(jpegOutBuffer, mYUVCompleteBuffers[j])) {
            LOG2("%s: Using YUV equivalent inter buffer", __FUNCTION__);
            mJpegInput.buffer = mYUVCompleteBuffers[j];
            return status;
        }
    }
    // YUV processing not complete for the equivalent YUV buffer push to pending queue
    LOG2("%s: Processing not complete for YUV equivalent inter buffer", __FUNCTION__);
    jSrc.stream = stream;
    jSrc.buffer = jpegOutBuffer;
    jSrc.streamType = CAMERA3_STREAM_OUTPUT;
    mJpegWaitForYUVQueue.add(jSrc);
    return NO_INIT;

noBuffer:
    for (unsigned int j = 0; j < mJpegInterbuffers.size(); j++) {
        if (!COMPARE_RESOLUTION(jpegOutBuffer, mJpegInterbuffers[j])) {
            mJpegInput.buffer = mJpegInterbuffers[j];
            return status;
        }
    }
    LOGE("%s Buffer not found, BUG, this should not happen!!", __FUNCTION__);
    return UNKNOWN_ERROR;
}

status_t
RawtoYUVTask::fillJPEGOutBuffer(CameraStream *stream, sp<CameraBuffer> outBuffer, ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    IPUTaskListener::PUTaskMessage outMsg;

    status = getJPEGInterBuffer(stream, outBuffer, msg);
    if (status != NO_ERROR || mJpegInput.buffer == NULL) {
        LOG2("%s JPEG inter Buffer not found, try later", __FUNCTION__);
        return NO_ERROR;
    }

    if (mJpegInput.streamType != CAMERA3_STREAM_BIDIRECTIONAL) {
        // Use the inter buffer to get the YUV data
        status = executeCapturePipeline(mJpegInput.buffer, msg);
        if (status != NO_ERROR) {
            LOGE("@%s: Error executing pipeline", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
    }
    // Notify listeners, first fill the observer message
    outMsg.id = IPUTaskListener::PU_TASK_MSG_ID_EVENT;
    outMsg.event.type  = IPUTaskListener::PU_TASK_EVENT_JPEG_BUFFER_COMPLETE;
    outMsg.event.buffer = outBuffer;
    outMsg.event.request = msg.data.request;
    outMsg.event.jpegInputbuffer = mJpegInput.buffer;
    notifyListeners(&outMsg);

    return status;
}

status_t
RawtoYUVTask::executeTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    ProcTaskMsg pMsg;
    pMsg.id = MESSAGE_ID_EXECUTE_TASK;
    pMsg.data = msg.data;
    mMessageQueue.send(&pMsg, MESSAGE_ID_EXECUTE_TASK);

    return NO_ERROR;
}

status_t
RawtoYUVTask::handleExecuteTask(ProcTaskMsg &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    CameraStream *stream = NULL;
    IPUTaskListener::PUTaskMessage outMsg;

    analyzeIntent(msg);

    Camera3Request* request = msg.data.request;
    const Vector<camera3_stream_buffer>* outBufs = request->getOutputBuffers();
    for (unsigned int i = 0; i < outBufs->size(); i++) {
        camera3_stream_buffer outputBuffer = outBufs->itemAt(i);
        stream = reinterpret_cast<CameraStream *>(outputBuffer.stream->priv);
        sp<CameraBuffer> buffer = request->findBuffer(stream);
        if (!buffer->isLocked()) {
            status = buffer->lock();
            if (status != NO_ERROR) {
                LOGE("@%s: Could not lock the buffer", __FUNCTION__);
                return UNKNOWN_ERROR;
            }
        }

        status = buffer->waitOnAcquireFence();
        if (status != NO_ERROR) {
            LOGW("Wait on fence for buffer %p timed out", buffer.get());
        }

        if (buffer->format() == HAL_PIXEL_FORMAT_BLOB) {
            status = fillJPEGOutBuffer(stream, buffer, msg);
            if (status != NO_ERROR) {
                LOGE("@%s: Error filling JPEG buffer", __FUNCTION__);
                return UNKNOWN_ERROR;
            }

         } else if (buffer->format() == HAL_PIXEL_FORMAT_YCbCr_420_888 ||
                   buffer->format() == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            status = executePostProcPipeline(buffer, msg);
            if (status != NO_ERROR) {
                LOGE("@%s: Error executing pipeline", __FUNCTION__);
                return UNKNOWN_ERROR;
            }

            mYUVCompleteBuffers.push(buffer);
            // Notify listeners, first fill the observer message
            outMsg.id = IPUTaskListener::PU_TASK_MSG_ID_EVENT;
            outMsg.event.type  = IPUTaskListener::PU_TASK_EVENT_BUFFER_COMPLETE;
            outMsg.event.buffer = buffer;
            outMsg.event.request = msg.data.request;
            notifyListeners(&outMsg);
        } else if (buffer->format() == HAL_PIXEL_FORMAT_RAW_SENSOR){
            // Handled in the RawBypassTask
        } else {
            LOGE("@%s: Unsupported buffer format in request %x", __FUNCTION__,
                 buffer->format());
            return UNKNOWN_ERROR;
        }
    }
    // All buffers for request completed
    // Check for pending JPEG buffers waiting for YUV processing to complete
    if (!mJpegWaitForYUVQueue.isEmpty()) {
        while (mJpegWaitForYUVQueue.size() > 0) {
            status = fillJPEGOutBuffer(mJpegWaitForYUVQueue[0].stream, mJpegWaitForYUVQueue[0].buffer, msg);
            if (status != NO_ERROR) {
                LOGE("@%s: Error filling JPEG buffer", __FUNCTION__);
                return UNKNOWN_ERROR;
            }
            mJpegWaitForYUVQueue.removeAt(0);
        }
    }

    // Return buffer to CU
    LOG2("@%s:returning buffer %p reqid  %d ",  __FUNCTION__,
         (void*)msg.data.rawCapture->v4l2Buf.m.userptr, msg.data.request->getId());
    msg.data.rawCapture->owner->returnBuffer(msg.data.rawCapture);

    // All buffers for request completed
    mPrevPipelineRunForReq = false;
    mYUVCompleteBuffers.clear();

    return status;
}

}  // namespace camera2
}  // namespace android
