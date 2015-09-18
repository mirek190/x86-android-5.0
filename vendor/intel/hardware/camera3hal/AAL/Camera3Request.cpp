/*
 * Copyright (C) 2013 Intel Corporation
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

#define LOG_TAG "Camera3_Request"


#include <ui/Fence.h>
#include "camera/CameraMetadata.h"
#include "LogHelper.h"
#include "CameraStream.h"
#include "Camera3Request.h"
#include "PlatformData.h"

namespace android {
namespace camera2 {

/**
 * \def RESULT_ENTRY_CAP
 * Maximum number of metadata entries stored in a result buffer. Used for
 * memory allocation purposes.
 */
#define RESULT_ENTRY_CAP 256
/**
 * \def RESULT_DATA_CAP
 * Maximum amount of data storage in bytes allocated in result buffers.
 */
#define RESULT_DATA_CAP 4096


Camera3Request::Camera3Request(): mCallback(NULL), mRequestId(0)
{
    LOG1("@%s Creating request with pointer %p", __FUNCTION__, this);
    deInit();
    /**
     * Allocate the CameraBuffer objects that will be recycled for each request
     * Since CameraBuf is refCounted the objects will be deleted when the Request
     * is destroyed. No need to manually delete them
     * These Camerabuffer objects are mere wrappers that will be filled when
     * we initialize the Request object
     */
    for (int i = 0; i < MAX_NUMBER_OUTPUT_STREAMS; i++) {
        mOutputBufferPool[i] = new CameraBuffer();
        if (CC_UNLIKELY(mOutputBufferPool[i] == NULL)) {
            LOGE("Not enough memory to allocate output buffer pool--Error");
        }
    }

    mInputBufferPool = new CameraBuffer();
    if (CC_UNLIKELY(mInputBufferPool== NULL)) {
        LOGE("Not enough memory to allocate input buffer pool -- Error");
    }
    mResultBufferAllocated = false;
}

Camera3Request::~Camera3Request()
{
    LOG1("@%s Destroying request with pointer %p", __FUNCTION__, this);
    mInitialized = false;
    if (mResultBufferAllocated) {
        freePartialResultBuffers();
    }
}

void
Camera3Request::deInit()
{
    mOutBuffers.clear();
    mInBuffers.clear();
    mInStreams.clear();
    mOutStreams.clear();
    mInitialized = false;
    mMembers.mSettings.clear();
    mSettings.clear();
    mOutputBuffers.clear();
    mInputBuffer.clear();
    CLEAR(mRequest3);
}

status_t
Camera3Request::init(camera3_capture_request* req,
                     IRequestCallback* cb,
                     CameraMetadata &settings, int cameraId)
{
    status_t status = NO_ERROR;
    LOG2("@%s req, framenum:%d, inputbuf:%p, outnum:%d, outputbuf:%p",
                __FUNCTION__, req->frame_number, req->input_buffer,
                req->num_output_buffers, req->output_buffers);

    if CC_UNLIKELY((cb == NULL)) {
        LOGE("@%s: Invalid callback object", __FUNCTION__);
        return BAD_VALUE;
    }
    /**
     * clean everything before we start
     */
    deInit();

    /**
     * initialize the partial metadata result buffers
     */
    status = initPartialResultBuffers(cameraId);
    if (CC_UNLIKELY(status != NO_ERROR)) {
        LOGE("@%s: failed to initialize partial results", __FUNCTION__);
        return NO_INIT;
    }
    const camera3_stream_buffer * buffer = req->output_buffers;

    if CC_UNLIKELY((req->num_output_buffers > MAX_NUMBER_OUTPUT_STREAMS)) {
        LOGE("Too many output buffers for this request %dm max is %d",
                req->num_output_buffers, MAX_NUMBER_OUTPUT_STREAMS);
        return BAD_VALUE;
    }
    for (uint32_t i = 0; i < req->num_output_buffers; i++) {
        LOG2("@%s, req, width:%d, stream type:0x%x", __FUNCTION__,
                                                   buffer->stream->width,
                                                   buffer->stream->stream_type);

        mOutputBufferPool[i]->init(buffer, cameraId);
        mOutputBuffers.push_back(mOutputBufferPool[i]);

        mOutBuffers.push_back(*buffer);// TO BE REMOVED
        mOutBuffers.editItemAt(i).release_fence = -1; // TO BE MOVED to PSL

        CameraStream *stream = reinterpret_cast<CameraStream *>(buffer->stream->priv);
        if (stream)
            stream->incOutBuffersInHal();

        buffer++;
    }
    if (req->input_buffer) {
        mInBuffers.push_back(*(req->input_buffer)); // TO BE REMOVED
        if (CC_UNLIKELY(mInputBufferPool == NULL)) {
           LOGE("Allocation of input buffer pool failed ");
           return NO_INIT;
        }
        mInputBuffer = mInputBufferPool;
        mInputBuffer->init(req->input_buffer, cameraId);
    }

    status = checkInputStreams(req);
    status |= checkOutputStreams(req);
    if (status != NO_ERROR) {
        LOGE("error with the request's buffers");
        deInit();
        return BAD_VALUE;
    }

    mRequestId = req->frame_number;
    mRequest3 = *req;
    mCallback = cb;
    mMembers.mSettings = settings;  // TO BE REMOVED
    mSettings = settings;   // Read only setting metadata buffer
    mInitialized = true;
    LOG2("<Request %d> successfully initialized", mRequestId);
    return NO_ERROR;
}

const camera3_capture_request*
Camera3Request::getUserRequest()
{
    Mutex::Autolock _l(mAccessLock);
    return &mRequest3;
}

/**
 * getNumberOutputBufs()
 *
 * returns the number of output buffers that this request has attached.
 * This determines how many buffers need to be returned to the client
 */
unsigned int
Camera3Request::getNumberOutputBufs()
{
    Mutex::Autolock _l(mAccessLock);
    return mInitialized ? mOutBuffers.size() : 0;
}

/**
 * getNumberInputBufs()
 *
 * returns the number of input buffers that this request has attached.
 */
unsigned int
Camera3Request::getNumberInputBufs()
{
    Mutex::Autolock _l(mAccessLock);
    return mInitialized ? mInBuffers.size() : 0;
}

int
Camera3Request::getId()
{
    Mutex::Autolock _l(mAccessLock);
    return mInitialized ? mRequestId : -1;
}

const Vector<CameraStreamNode*>*
Camera3Request::getInputStreams()
{
    return mInitialized ? &mInStreams : NULL;
}

const Vector<CameraStreamNode*>*
Camera3Request::getOutputStreams()
{
    return mInitialized ? &mOutStreams : NULL;
}

const Vector<camera3_stream_buffer>*
Camera3Request::getOutputBuffers()
{
    return mInitialized ? &mOutBuffers : NULL;
}

const Vector<camera3_stream_buffer>*
Camera3Request::getInputBuffers()
{
    return mInitialized ? &mInBuffers : NULL;
}

/**
 * getPartialResultBuffer
 *
 * PSL implementations that produce metadata buffers in several chunks will
 * call this method to acquire its own metadata buffer. Coordination on
 * the usage of those buffers is responsibility of the PSL
 */
CameraMetadata*
Camera3Request::getPartialResultBuffer(unsigned int index)
{
    if CC_UNLIKELY((mPartialResultBuffers.isEmpty() ||
                   (index > mPartialResultBuffers.size()-1))) {
        LOGE("Requesting a partial buffer that does not exist");
        return NULL;
    }

    return mPartialResultBuffers[index].metaBuf;
}

/**
 * getSettings
 *
 * returns the pointer to the read-only metadata buffer with the settings
 * for this request.
 */
const CameraMetadata*
Camera3Request::getSettings()
{
    return mInitialized? &mSettings : NULL;
}

/******************************************************************************
 *  PRIVATE PARTS
 *****************************************************************************/

/**
 * Check that the input buffers are associated to a known input stream
 * A known input stream is the one that the private field
 * is initialized with a pointer to the corresponding CameraStream object
 */
status_t
Camera3Request::checkInputStreams(camera3_capture_request * request3)
{
    camera3_stream_t* stream = NULL;
    CameraStreamNode* streamNode = NULL;


    if (request3->input_buffer != NULL) {
        stream = request3->input_buffer->stream;
        if (stream->stream_type != CAMERA3_STREAM_INPUT
           && stream->stream_type != CAMERA3_STREAM_BIDIRECTIONAL) {
            LOGE("%s: Request %d: Input buffer not from input stream!",
                    __FUNCTION__, request3->frame_number);
            return BAD_VALUE;
        }
        if (stream->priv == NULL) {
            LOGE("Input Stream not configured");
            return BAD_VALUE;
        }
        streamNode = reinterpret_cast<CameraStreamNode *>(stream->priv);
        mInStreams.push_back(streamNode);
    }
    return NO_ERROR;
}

/**
 * findBuffer
 * return a buffer associated with the stream passes as parameter
 * in this request
 * \param stream: CameraStream that we want to find buffers for
 * \return: sp to the CameraBuffer object
 */
sp<CameraBuffer>
Camera3Request::findBuffer(CameraStreamNode* stream)
{
    for (size_t i = 0; i< mOutputBuffers.size(); i++) {
        if (mOutputBuffers[i]->getOwner() == stream)
            return mOutputBuffers[i];
    }
    if (mInputBuffer != NULL && mInputBuffer->getOwner() == stream)
        return mInputBuffer;

    LOGW("could not find requested buffer. invalid stream?");
    return NULL;
}

/**
 * Check whether the aBuffer is an input buffer for the request
 */
bool
Camera3Request::isInputBuffer(sp<CameraBuffer> buffer)
{
    if (mInputBuffer != NULL && buffer.get() == mInputBuffer.get()) {
        return true;
    }

    return false;
}

/**
 * Check that the output buffers belong to a known stream
 */
status_t
Camera3Request::checkOutputStreams(camera3_capture_request * request3)
{
    camera3_stream_t* stream = NULL;
    CameraStream* s = NULL;
    const camera3_stream_buffer * buffer = request3->output_buffers;

    if (request3->num_output_buffers < 1 || buffer == NULL) {
        LOGE("%s: Request %d: No output buffers provided!",
                __FUNCTION__, request3->frame_number);
        return BAD_VALUE;
    }

    for (uint32_t j = 0; j < request3->num_output_buffers; j++) {
        stream = buffer->stream;
        if (stream->priv == NULL)
            return BAD_VALUE;
        s = (CameraStream *)stream->priv;
        CameraStream* tmp = NULL;
        unsigned int i = 0;
        bool hasFlag = false;
        for (i = 0; i < mOutStreams.size(); i++) {
             tmp = reinterpret_cast<CameraStream*>(mOutStreams.editItemAt(i));
             if (tmp == s) {
                 hasFlag = true;
                 break;
             }
             if (s->width()*s->height() > tmp->width()*tmp->height()) {
                 mOutStreams.insertAt(s, i);
                 break;
             } else if ((s->width()*s->height() == tmp->width()*tmp->height())
                         && (s->seqNo() < tmp->seqNo())) {
                 mOutStreams.insertAt(s, i);
                 break;
             }
        }
        if ((tmp == NULL || i == mOutStreams.size()) && !hasFlag)
             mOutStreams.push_back((CameraStreamNode *)s);
        buffer++;
    }
    return NO_ERROR;
}

/**
 * initPartialResultBuffers
 *
 * Initialize the buffers that will store the partial results for each request
 * The initialization has 2 phases:
 * - Allocation: this is done only once in the lifetime of the request
 * - Reset: this is done for all initialization. It means to clear the buffers
 * where the result metadata is stored.
 * The number of partial results is PSL specific and is queried via PlatformData
 * Different cameraId may use different PSL
 */
status_t
Camera3Request::initPartialResultBuffers(int cameraId)
{
    status_t status = NO_ERROR;

    if CC_UNLIKELY((!mResultBufferAllocated)) {
        int partialBufferCount = PlatformData::getPartialMetadataCount(cameraId);
        status = allocatePartialResultBuffers(partialBufferCount);
        if (CC_UNLIKELY(status != NO_ERROR)) {
            return status;
        }
    }
    // Reset the metadata buffers
    camera_metadata_t *m;
    for (unsigned int i = 0; i < mPartialResultBuffers.size(); i++) {
        if (mPartialResultBuffers[i].baseBuf != NULL) {
            m = mPartialResultBuffers.editItemAt(i).metaBuf->release();
            /**
             * It may happen that a PSL may resize the result buffer if the
             * originally allocated is not big enough. Check for it
             */
            if CC_UNLIKELY(m != mPartialResultBuffers[i].baseBuf) {
                if (m == NULL) {
                    LOGE("Cannot get metadata from result buffers.");
                    return UNKNOWN_ERROR;
                }
                LOGW("PSL resized result buffer (%d) in request %p", i, this);
                reAllocateResultBuffer(m, i);
            }

            memset(mPartialResultBuffers[i].baseBuf, 0 ,
                   mPartialResultBuffers[i].size);

            /* This should not fail since it worked first time when we
             * allocated */
           m = place_camera_metadata(mPartialResultBuffers[i].baseBuf,
                                     mPartialResultBuffers[i].size,
                                     mPartialResultBuffers[i].entryCap,
                                     mPartialResultBuffers[i].dataCap);
           mPartialResultBuffers.editItemAt(i).metaBuf->acquire(m);
        }
    }
    return status;
}

/**
 * reAllocateResultBuffer
 *
 * In situations when the PSL needs to re-size the result buffer we need to
 * re-allocate the result buffer to regain the ownership of the memory
 * In that case we free the metadata buffer allocated by the framework during
 * resize and we re-allocate it ourselves.
 *
 * \param m[IN]: metadata buffer allocated by the framework
 * \param index[IN]: index in the mPartialResultBuffer vector.
 */
void Camera3Request::reAllocateResultBuffer(camera_metadata_t* m, int index)
{
    MemoryManagedMetadata &mmdata = mPartialResultBuffers.editItemAt(index);

    mmdata.size = get_camera_metadata_size(m);
    mmdata.dataCap = get_camera_metadata_data_capacity(m);
    mmdata.entryCap = get_camera_metadata_entry_capacity(m);
    free_camera_metadata(m);
    mmdata.baseBuf = (void*) new char[mmdata.size];
    if CC_UNLIKELY(mmdata.baseBuf == NULL) {
        /**
         * This should not happen, if it does just change the default data
         * capacity to allocate that from the beginning.
         * It may be due to memory fragmentation
         */
        LOGE("Result metadata re-allocation failed."
             "Increase default size to %d", mmdata.dataCap);
    }
    LOGW("Need to resize meta result buffers to %d entry cap %d, data cap %d"
            ,mmdata.size, mmdata.entryCap, mmdata.dataCap);
}

/*
 * allocatePartialResultBuffers
 * Allocates the raw buffers that will be used to store the result metadata
 * buffers. The memory of these metadata buffers is managed by this class
 * so that we do not need to re-allocate the buffers for each request
 *
 *  This allows us to clean the metadata buffer without having to re-allocate.
 */
status_t
Camera3Request::allocatePartialResultBuffers(int partialResultCount)
{
    MemoryManagedMetadata  mmdata;
    size_t bufferSize = calculate_camera_metadata_size(RESULT_ENTRY_CAP,
                                                       RESULT_DATA_CAP);
    for (int i = 0; i < partialResultCount; i++) {
        CLEAR(mmdata);
        camera_metadata_t *m;
        mmdata.baseBuf = (void*) new char[bufferSize];

        m = place_camera_metadata(mmdata.baseBuf, bufferSize, RESULT_ENTRY_CAP,
                                  RESULT_DATA_CAP);
        if ((mmdata.baseBuf == NULL) || (m == NULL)) {
            LOGE("Failed to allocate memory for result metadata buffer");
            goto bailout;
        }
        mmdata.metaBuf = new CameraMetadata(m);
        mmdata.size = bufferSize;
        mmdata.entryCap = RESULT_ENTRY_CAP;
        mmdata.dataCap = RESULT_DATA_CAP;
        mPartialResultBuffers.add(mmdata);
    }

    mResultBufferAllocated = true;
    return NO_ERROR;

bailout:
    freePartialResultBuffers();
    return NO_MEMORY;
}

void
Camera3Request::freePartialResultBuffers(void)
{
    for (unsigned int i = 0; i < mPartialResultBuffers.size(); i++) {
        if (mPartialResultBuffers[i].baseBuf != NULL)
            mPartialResultBuffers[i].metaBuf->release();
            delete[] reinterpret_cast<char*>(mPartialResultBuffers[i].baseBuf);
            delete mPartialResultBuffers[i].metaBuf;
    }
    mPartialResultBuffers.clear();
    mResultBufferAllocated = false;
}

} /* namespace camera2 */
} /* namespace android */
