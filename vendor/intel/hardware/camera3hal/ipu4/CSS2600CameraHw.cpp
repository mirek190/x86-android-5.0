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

#define LOG_TAG "Camera_Css2600CameraHw"

#include "CSS2600CameraHw.h"
#include "LogHelper.h"
#include "CameraMetadataHelper.h"
#include "CameraStream.h"
#include "HwStreamBase.h"
#include "IPU4CameraCapInfo.h"

namespace android {
namespace camera2 {

// Camera factory
ICameraHw *CreateIPU4Camera(int cameraId) {
    return new Css2600CameraHw(cameraId);
}

Css2600CameraHw::Css2600CameraHw(int cameraId): mCameraId(cameraId),
                                                mStaticMeta(NULL)
{

}

Css2600CameraHw::~Css2600CameraHw() {

    if (mControlUnit != NULL) {
        delete mControlUnit;
        mControlUnit = NULL;
    }

    if (mProcessingUnit != NULL) {
        delete mProcessingUnit;
        mProcessingUnit = NULL;
    }

    if (mCaptureUnit != NULL) {
        delete mCaptureUnit;
        mCaptureUnit = NULL;
    }

    if(mStaticMeta != NULL) {
        /**
         * The metadata buffer this object uses belongs to PlatformData
         * we release it before we destroy the object.
         */
        mStaticMeta->release();
        delete mStaticMeta;
        mStaticMeta = NULL;
    }
}

status_t
Css2600CameraHw::init(void)
{
    LOG1("@%s", __FUNCTION__);

    status_t status = NO_ERROR;

    mCaptureUnit = new CaptureUnit(mCameraId);
    if (mCaptureUnit == NULL) {
        LOGE("Error creating CaptureUnit");
        return NO_MEMORY;
    }

    status = mCaptureUnit->init();
    if (status != NO_ERROR) {
        LOGE("Error initializing CaptureUnit, ret code: %x", status);
        return status;
    }

    mProcessingUnit = new ProcessingUnit(mCameraId, mCaptureUnit);
    if(mProcessingUnit == NULL) {
       LOGE("Could not allocate PU class");
       return NO_MEMORY;
    }

    status = mProcessingUnit->init();
    if (status != NO_ERROR) {
        LOGE("Error initializing ProcUnit. ret code:%x", status);
        return status;
    }

    mControlUnit = new ControlUnit(mProcessingUnit, mCaptureUnit, mCameraId);
    if (mControlUnit == NULL) {
        LOGE("Error creating ControlUnit");
        return NO_MEMORY;
    }

    status = mControlUnit->init();
    if (status != NO_ERROR) {
        LOGE("Error initializing ControlUnit. ret code:%x", status);
        return status;
    }

    status = mCaptureUnit->attachListener(mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_NEW_SENSOR_DESCRIPTOR);

    status = initStaticMetadata();
    if (CC_UNLIKELY(status != NO_ERROR))
        return status;

    status = mCaptureUnit->attachListener(mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_RAW_BAYER);

    status = mCaptureUnit->attachListener(mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_2A_STATISTICS);

    status = mCaptureUnit->attachListener(mControlUnit,
                                          ICaptureEventListener::CAPTURE_EVENT_SHUTTER);

    return status;
}

const camera_metadata_t *
Css2600CameraHw::getDefaultRequestSettings(int type)
{
    LOG1("@%s", __FUNCTION__);
    return PlatformData::getDefaultMetadata(mCameraId, type);
}

status_t
Css2600CameraHw::configStreams(Vector<camera3_stream_t*> &activeStreams)
{
    LOG1("@%s", __FUNCTION__);
    uint32_t maxBufs, usage;
    status_t status = NO_ERROR;
    bool configChanged = true;

    maxBufs = mPipelineDepth + 1;  /* value from XML static metadata */
    if (maxBufs > MAX_REQUEST_IN_PROCESS_NUM)
        maxBufs = MAX_REQUEST_IN_PROCESS_NUM;

    /**
     * Here we could give different gralloc flags depending on the stream
     * format, at the moment we give the same to all
     */
    usage = GRALLOC_USAGE_SW_READ_OFTEN |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_HW_CAMERA_READ;

    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        activeStreams[i]->max_buffers = maxBufs;
        activeStreams[i]->usage = usage;
    }

    status = mCaptureUnit->configStreams(activeStreams, configChanged);
    if (status != NO_ERROR) {
        LOGE("@%s: Unable to configure stream", __FUNCTION__);
        return status;
    }

    status = mProcessingUnit->configStreams(activeStreams);
    if (status != NO_ERROR) {
        LOGE("@%s: Unable to configure stream for Proc_Unit", __FUNCTION__);
        return status;
    }
    status = mControlUnit->configStreamsDone(configChanged);
    return status;
}

status_t
Css2600CameraHw::bindStreams(Vector<CameraStreamNode *> activeStreams)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    CameraStreamNode* stream;

    mDummyHwStreamsVector.clear();
    // Need to have a producer stream as it is required by common code
    // bind all streams to dummy HW stream class.
    for (unsigned int i = 0; i < activeStreams.size(); i++) {
        stream = activeStreams.editItemAt(i);
        HwStreamBase* hwStream = new HwStreamBase();
        CameraStream::bind(stream, hwStream);
        mDummyHwStreamsVector.push_back(hwStream);
    }

    return status;
}

status_t
Css2600CameraHw::processRequest(Camera3Request* request, int inFlightCount)
{
    if (inFlightCount >= mPipelineDepth) {
        LOG2("@%s:blocking request %d", __FUNCTION__, request->getId());
        return WOULD_BLOCK;
    }
    mControlUnit->processRequest(request);
    return NO_ERROR;
}

status_t
Css2600CameraHw::flush()
{
    return NO_ERROR;
}

void
Css2600CameraHw::dump(int fd)
{
    UNUSED(fd);
}
/**
 * initStaticMetadata
 *
 * Create CameraMetadata object to retrieve the static tags used in this class
 * we cache them as members so that we do not need to query CameraMetadata class
 * every time we need them. This is more efficient since find() is not cheap
 */
status_t
Css2600CameraHw::initStaticMetadata(void)
{
    status_t status = NO_ERROR;
    /**
    * Initialize the CameraMetadata object with the static metadata tags
    */
    camera_metadata_t* staticMeta;
    staticMeta = (camera_metadata_t*)PlatformData::getStaticMetadata(mCameraId);
    mStaticMeta = new CameraMetadata(staticMeta);

    camera_metadata_entry entry;
    entry = mStaticMeta->find(ANDROID_REQUEST_PIPELINE_MAX_DEPTH);
    if(entry.count == 1) {
        mPipelineDepth = entry.data.u8[0];
    } else {
        mPipelineDepth = DEFAULT_PIPELINE_DEPTH;
    }

    /**
     * Check the consistency of the information we had in XML file.
     * partial result count is very tightly linked to the PSL
     * implementation
     */
    int32_t xmlPartialCount = PlatformData::getPartialMetadataCount(mCameraId);
    if (xmlPartialCount != PARTIAL_RESULT_COUNT) {
            LOGW("Partial result count does not match current implementation "
                 " got %d should be %d, fix the XML!", xmlPartialCount,
                 PARTIAL_RESULT_COUNT);
            status = NO_INIT;
    }
    return status;
}
}  // namespace camera2
}  // namespace android
