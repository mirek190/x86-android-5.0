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

#define LOG_TAG "Camera_CIFCameraHw"

#include "CIFCameraHw.h"
#include "LogHelper.h"
#include "CameraMetadataHelper.h"
#include "CameraStream.h"
#include <hardware/camera3.h>


namespace android {
namespace camera2 {

ICameraHw *CreateCIFCamera(int cameraId) {
    return new CIFCameraHw(cameraId);
}

CIFCameraHw::CIFCameraHw(int cameraId) :
        mCameraId(cameraId),
        mPipelineDepth(DEFAULT_PIPELINE_DEPTH),
        mCaptureUnit(cameraId),
        mControlUnit(&mCaptureUnit, cameraId)
{
    HAL_TRACE_CALL(1);
}

CIFCameraHw::~CIFCameraHw()
{
    HAL_TRACE_CALL(1);
    mControlUnit.deinit();
}

status_t
CIFCameraHw::init(void)
{
    HAL_TRACE_CALL(1);
    status_t status = mCaptureUnit.init();
    if (status == OK)
        status = mControlUnit.init();

    status |= mCaptureUnit.attachListener(&mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_NEW_SENSOR_DESCRIPTOR);

    status |= mCaptureUnit.attachListener(&mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_RAW_BAYER);

    status |= mCaptureUnit.attachListener(&mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_2A_STATISTICS);

    return status;
}

const camera_metadata_t *
CIFCameraHw::getDefaultRequestSettings(int type)
{
    LOG1("@%s", __FUNCTION__);
    return PlatformData::getDefaultMetadata(mCameraId, type);
}

status_t
CIFCameraHw::configStreams(Vector<camera3_stream_t*> &activeStreams)
{
    HAL_TRACE_CALL(1);

    uint32_t maxBufs, usage;
    status_t status = OK;

    usage = GRALLOC_USAGE_SW_READ_OFTEN |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_HW_CAMERA_WRITE;

    for (unsigned int i = 0; i < activeStreams.size(); i++) {
	maxBufs = 3; // magic
	if (maxBufs > MAX_REQUEST_IN_PROCESS_NUM)
	    maxBufs = MAX_REQUEST_IN_PROCESS_NUM;

        LOG1("@%s stream width and height %dx%d", __FUNCTION__, activeStreams[i]->width, activeStreams[i]->height);

        if (activeStreams[i]->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            // todo gralloc should be modified to handle these based on flags
            //LOG1("@%s impl. defined format -> HAL_PIXEL_FORMAT_YCrCb_420_SP", __FUNCTION__);
            //activeStreams[i]->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        } else if (activeStreams[i]->format == HAL_PIXEL_FORMAT_BLOB) {
            LOG1("@%s blob format -> no change", __FUNCTION__);
            // Conserve memory, jpeg stream buffers are BIG - ATM 6bpp 13MP.
            // using max 1 would force us to stop accepting requests when the
            // first jpeg request comes, preventing running preview / video.
            // Therefore we can't really go lower than 2 for jpeg
            if (activeStreams.size() == 2)
                maxBufs = 2;
        } else {
            ALOGW("Unknown format: %d -> no change %d", activeStreams[i]->format, HAL_PIXEL_FORMAT_YCrCb_420_SP);
            usage |= GRALLOC_USAGE_HW_CAMERA_WRITE | GRALLOC_USAGE_HW_VIDEO_ENCODER;
        }

        activeStreams[i]->max_buffers = maxBufs;
        activeStreams[i]->usage = usage;
    }

    return mCaptureUnit.configStreams(activeStreams);
}

status_t
CIFCameraHw::bindStreams(Vector<CameraStreamNode *> activeStreams)
{
    HAL_TRACE_CALL(1);
    CameraStreamNode* stream;
    mDummyStreams.clear();
    // Need to have a producer stream as it is required by common code
    // bind all streams to dummy HW stream class.
    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        stream = activeStreams.editItemAt(i);
        HwStreamBase *hwStream = new HwStreamBase();
        if (hwStream) {
            mDummyStreams.push(hwStream);
            CameraStream::bind(stream, hwStream);
        } else {
            ALOGE("Out of memory");
            return NO_MEMORY;
        }
    }

    return OK;
}

status_t
CIFCameraHw::processRequest(Camera3Request* request, int inFlightCount)
{
    HAL_TRACE_CALL(2);
    if (inFlightCount >= mPipelineDepth) {
        LOG2("@%s:blocking request %d", __FUNCTION__, request->getId());
        return WOULD_BLOCK;
    }

    return mControlUnit.processRequest(request);
}


status_t
CIFCameraHw::flush()
{
    HAL_TRACE_CALL(1);
    return NO_ERROR;
}


void
CIFCameraHw::dump(int fd)
{
    HAL_TRACE_CALL(1);
    mControlUnit.dump(fd);
}

}  // namespace camera2
}  // namespace android
