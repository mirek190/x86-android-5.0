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

#define LOG_TAG "Camera_V4L2VideoNode"

#include "LogHelper.h"

#include "v4l2device.h"
#include "PlatformData.h"
#include <fcntl.h>
#include <cutils/atomic.h>
#include "UtilityMacros.h"


namespace android {
namespace camera2 {

////////////////////////////////////////////////////////////////////
//                          PUBLIC METHODS
////////////////////////////////////////////////////////////////////

#define MAX_V4L2_BUFFERS    MAX_CAMERA_BUFFERS_NUM

V4L2VideoNode::V4L2VideoNode(const char *name, VideNodeDirection nodeDirection, VideoNodeType nodeType):
                                                        V4L2DeviceBase(name),
                                                        mState(DEVICE_CLOSED),
                                                        mBuffersInDevice(0),
                                                        mFrameCounter(0),
                                                        mInitialSkips(0),
                                                        mDirection(nodeDirection),
                                                        mType(nodeType),
                                                        mInBufType(V4L2_BUF_TYPE_VIDEO_CAPTURE),
                                                        mOutBufType(V4L2_BUF_TYPE_VIDEO_OUTPUT)
{
    LOG1("@%s: device: %s", __FUNCTION__, name);
    mBufferPool.setCapacity(MAX_V4L2_BUFFERS);
    mSetBufferPool.setCapacity(MAX_V4L2_BUFFERS);
    CLEAR(mConfig);
}

V4L2VideoNode::~V4L2VideoNode()
{
    LOG1("@%s device : %s", __FUNCTION__, mName.string());

    /**
     * Do something for the buffer pool ?
     */
}

status_t V4L2VideoNode::open()
{
    status_t status(NO_ERROR);
    status = V4L2DeviceBase::open();
    if (status == NO_ERROR)
        mState = DEVICE_OPEN;

    android_atomic_acquire_store(0,&mBuffersInDevice);
    return status;
}

status_t V4L2VideoNode::close()
{
    status_t status(NO_ERROR);

    if (mState == DEVICE_STARTED) {
        stop();
    }
    if (!mBufferPool.isEmpty()) {
        destroyBufferPool();
    }

    status = V4L2DeviceBase::close();
    if (status == NO_ERROR)
        mState = DEVICE_CLOSED;

    android_atomic_acquire_store(0,&mBuffersInDevice);
    return status;
}

/**
 * queries the capabilities of the device and it does some basic sanity checks
 * based on the direction of the video device node
 *
 * \param cap: [OUT] V4L2 capability structure
 *
 * \return NO_ERROR  if everything went ok
 * \return INVALID_OPERATION if the device was not in correct state
 * \return UNKNOWN_ERROR if IOCTL operation failed
 * \return DEAD_OBJECT if the basic checks for this object failed
 */
status_t V4L2VideoNode::queryCap(struct v4l2_capability *cap)
{
    LOG1("@%s device : %s", __FUNCTION__, mName.string());
    int ret(0);

    if (mState != DEVICE_OPEN) {
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    ret = pioctl(mFd, VIDIOC_QUERYCAP, cap);

    if (ret < 0) {
        LOGE("VIDIOC_QUERYCAP returned: %d (%s)", ret, strerror(errno));
        return UNKNOWN_ERROR;
    }

    LOG1( "driver:      '%s'", cap->driver);
    LOG1( "card:        '%s'", cap->card);
    LOG1( "bus_info:      '%s'", cap->bus_info);
    LOG1( "version:      %x", cap->version);
    LOG1( "capabilities:      %x", cap->capabilities);

    /* Do some basic sanity check */

    if (mDirection == INPUT_VIDEO_NODE) {
        if (!(cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
           LOGW("No capture devices - But this is an input video node!");
           return DEAD_OBJECT;
        }

        if (!(cap->capabilities & V4L2_CAP_STREAMING)) {
            LOGW("Is not a video streaming device");
            return DEAD_OBJECT;
        }

    } else {
        if (!(cap->capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
            LOGW("No output devices - but this is an output video node!");
            return DEAD_OBJECT;
        }
    }

    return NO_ERROR;
}

status_t V4L2VideoNode::enumerateInputs(struct v4l2_input *anInput)
{
    LOG1("@%s device : %s", __FUNCTION__, mName.string());
    int ret(0);

    if (mState == DEVICE_CLOSED) {
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    ret = pioctl(mFd, VIDIOC_ENUMINPUT, anInput);

    if (ret < 0) {
        LOGE("VIDIOC_ENUMINPUT failed returned: %d (%s)", ret, strerror(errno));
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t V4L2VideoNode::queryCapturePixelFormats(Vector<v4l2_fmtdesc> &formats)
{
    LOG1("@%s device : %s", __FUNCTION__, mName.string());
    struct v4l2_fmtdesc aFormat;

    if (mState == DEVICE_CLOSED) {
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    formats.clear();
    CLEAR(aFormat);

    aFormat.index = 0;
    aFormat.type = mInBufType;

    while (pioctl(mFd, VIDIOC_ENUM_FMT , &aFormat) == 0) {
        formats.push(aFormat);
        aFormat.index++;
    };

    aFormat.index = 0;
    aFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    while (pioctl(mFd, VIDIOC_ENUM_FMT , &aFormat) == 0) {
        formats.push(aFormat);
        aFormat.index++;
    };

    LOG1("@%s device : %s %d format retrieved", __FUNCTION__, mName.string(), formats.size());
    return NO_ERROR;
}

status_t V4L2VideoNode::setInput(int index)
{
    LOG1("@%s", __FUNCTION__);
    struct v4l2_input input;
    status_t status = NO_ERROR;
    int ret(0);

    if (mState == DEVICE_CLOSED) {
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    input.index = index;

    ret = pioctl(mFd, VIDIOC_S_INPUT, &input);

    if (ret < 0) {
       LOGE("VIDIOC_S_INPUT index %d returned: %d (%s)",
           input.index, ret, strerror(errno));
       status = UNKNOWN_ERROR;
    }

    return status;
}

/**
 * Stop the streaming of buffers of a video device
 * This method is basically a stream-off IOCTL but it has a parameter to
 * stop and destroy the current active-buffer-pool
 *
 * After this method completes the device is in state DEVICE_PREPARED
 *
 * \param leavePopulated: boolean to control the state change
 * \return   0 on success
 *          -1 on failure
 */
int V4L2VideoNode::stop(bool keepBuffers)
{
    LOG1("@%s: device = %s", __FUNCTION__, mName.string());
    int ret = 0;

    if (mState == DEVICE_STARTED) {
        enum v4l2_buf_type type = mInBufType;
        //stream off
        ret = pioctl(mFd, VIDIOC_STREAMOFF, &type);
        if (ret < 0) {
            LOGE("VIDIOC_STREAMOFF returned: %d (%s)", ret, strerror(errno));
            return ret;
        }
        mState = DEVICE_PREPARED;
    }

    if (mState == DEVICE_PREPARED) {
        if (!keepBuffers) {
            destroyBufferPool();
            mState = DEVICE_CONFIGURED;
        }
   } else {
        LOGW("Trying to stop a device not started");
        ret = -1;
    }

    return ret;
}

/**
 * Start the streaming of buffers in a video device
 *
 * This called is allowed in the following states:
 * - PREPARED
 *
 * This method just calls  call the stream on IOCTL
 */
int V4L2VideoNode::start(int initialSkips)
{
    LOG1("@%s, device = %s, initialSkips:%d", __FUNCTION__, mName.string(), initialSkips);
    int ret(0);

    if (mState != DEVICE_PREPARED) {
        LOGE("%s: Invalid state to start %d", __FUNCTION__, mState);
        return -1;
    }

    //stream on
    enum v4l2_buf_type type = mInBufType;
    ret = pioctl(mFd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        LOGE("VIDIOC_STREAMON returned: %d (%s)", ret, strerror(errno));
        return ret;
    }

    mFrameCounter = 0;
    mState = DEVICE_STARTED;
    mInitialSkips = initialSkips;

    return ret;
}

/**
 * Update the current device node configuration
 *
 * This called is allowed in the following states:
 * - OPEN
 * - CONFIGURED
 * - PREPARED
 *
 * This method is a convenience method for use in the context of video capture
 * (INPUT_VIDEO_NODE)
 * It makes use of the more detailed method that uses as input parameter the
 * v4l2_format structure
 * This method queries first the current format and updates capture format.
 *
 *
 * \param aConfig:[IN/OUT] reference to the new configuration.
 *                 This structure contains new values for width,height and format
 *                 parameters, but the stride value is not known by the caller
 *                 of this method. The stride value is retrieved from the ISP
 *                 and the value updated, so aConfig.stride is an OUTPUT parameter
 *                 The same applies for the expected size of the buffer
 *                 aConfig.size is also an OUTPUT parameter
 *
 *  \return NO_ERROR if everything went well
 *          INVALID_OPERATION if device is not in correct state (open)
 *          UNKNOW_ERROR if we get an error from the v4l2 ioctl's
 */
status_t V4L2VideoNode::setFormat(FrameInfo &aConfig)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    int ret(0);
    struct v4l2_format v4l2_fmt;
    CLEAR(v4l2_fmt);

    if ((mState != DEVICE_OPEN) &&
        (mState != DEVICE_CONFIGURED) &&
        (mState != DEVICE_PREPARED) ){
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    v4l2_fmt.type = mInBufType;
    LOG1("VIDIOC_G_FMT");
    ret = pioctl (mFd, VIDIOC_G_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("VIDIOC_G_FMT failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    v4l2_fmt.type = mInBufType;

    v4l2_fmt.fmt.pix.width = aConfig.width;
    v4l2_fmt.fmt.pix.height = aConfig.height;
    v4l2_fmt.fmt.pix.pixelformat = aConfig.format;
    v4l2_fmt.fmt.pix.bytesperline = displayBpl(aConfig.format, aConfig.width);
    v4l2_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    v4l2_fmt.fmt.pix.sizeimage = 0;

    // Update current configuration with the new one
    ret = setFormat(v4l2_fmt);
    if (ret != NO_ERROR)
        return ret;

    // .. but get the stride from ISP
    // and update the new configuration struct with it
    aConfig.stride = mConfig.stride;

    // Do the same for the frame size
    aConfig.size = mConfig.size;

    return NO_ERROR;
}

/**
 * Update the current device node configuration (low-level)
 *
 * This called is allowed in the following states:
 * - OPEN
 * - CONFIGURED
 * - PREPARED
 *
 * This methods allows more detailed control of the format than the previous one
 * It updates the internal configuration used to check for discrepancies between
 * configuration and buffer pool properties
 *
 * \param aFormat:[IN] reference to the new v4l2_format .
 *
 *  \return NO_ERROR if everything went well
 *          INVALID_OPERATION if device is not in correct state (open)
 *          UNKNOW_ERROR if we get an error from the v4l2 ioctl's
 */
status_t V4L2VideoNode::setFormat(struct v4l2_format &aFormat)
{

    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    int ret(0);

    if ((mState != DEVICE_OPEN) &&
        (mState != DEVICE_CONFIGURED) &&
        (mState != DEVICE_PREPARED) ){
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }


    LOG1("VIDIOC_S_FMT: width: %d, height: %d, bpl: %d, fourcc: %s, field: %d",
         aFormat.fmt.pix.width,
         aFormat.fmt.pix.height,
         aFormat.fmt.pix.bytesperline,
         v4l2Fmt2Str(aFormat.fmt.pix.pixelformat),
         aFormat.fmt.pix.field);

    ret = pioctl(mFd, VIDIOC_S_FMT, &aFormat);
    if (ret < 0) {
        LOGE("VIDIOC_S_FMT failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    // Update current configuration with the new one
    mConfig.originalFormat = mConfig.format = aFormat.fmt.pix.pixelformat;
    mConfig.width = aFormat.fmt.pix.width;
    mConfig.height = aFormat.fmt.pix.height;
    mConfig.stride = bytesToPixels(mConfig.format,aFormat.fmt.pix.bytesperline);
    mConfig.size = frameSize(mConfig.format, mConfig.stride, mConfig.height);

    if (mConfig.stride != mConfig.width)
        LOG1("stride: %d from ISP width: %d", mConfig.stride,mConfig.width);

    mState = DEVICE_CONFIGURED;
    mSetBufferPool.clear();
    return NO_ERROR;
}

int V4L2VideoNode::grabFrame(struct v4l2_buffer_info *buf)
{
    LOG2("@%s", __FUNCTION__);
    int ret(0);

    if (mState != DEVICE_STARTED) {
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return -1;
    }
    if (buf == NULL) {
        LOGE("%s: invalid parameter buf is NULL",__FUNCTION__);
        return -1;
    }

    ret = dqbuf(buf);

    if (ret < 0)
        return ret;

    // inc frame counter but do no wrap to negative numbers
    mFrameCounter++;
    mFrameCounter &= INT_MAX;

    // atomisp_frame_status is a proprietary extension to v4l2_buffer flags
    // that driver places into reserved keyword
    // translate error flag into corrupt frame
    if (buf->vbuffer.flags & V4L2_BUF_FLAG_ERROR)
        buf->vbuffer.reserved = (unsigned int) ATOMISP_FRAME_STATUS_CORRUPTED;
    // translate initial skips into corrupt frame
    if (mInitialSkips > 0) {
        buf->vbuffer.reserved = (unsigned int) ATOMISP_FRAME_STATUS_CORRUPTED;
        mInitialSkips--;
    }

    LOG2("@%s, index:%d, addr:%p", __FUNCTION__, buf->vbuffer.index, (void *)buf->vbuffer.m.userptr);
    return buf->vbuffer.index;
}

status_t V4L2VideoNode::putFrame(struct v4l2_buffer const *buf)
{
    int index = buf->index;
    LOG2("@%s, index:%d, addr:%p name:%s", __FUNCTION__, buf->index, (void *)buf->m.userptr, mName.string());

    mBufferPool.editItemAt(index).vbuffer.m.userptr = buf->m.userptr;
    // reserved2 indicates whether this frame requires per-frame parameters
    mBufferPool.editItemAt(index).vbuffer.reserved2 = buf->reserved2;
    if (putFrame(index) < 0)
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

int V4L2VideoNode::putFrame(unsigned int index)
{
    LOG2("@%s", __FUNCTION__);
    int ret(0);

    if (index > mBufferPool.size()) {
        LOGE("%s Invalid index %d pool size %d", __FUNCTION__, index, mBufferPool.size());
        return -1;
    }
    struct v4l2_buffer_info vbuf = mBufferPool.editItemAt(index);
    LOG2("userptr = %p, reserved2=0x%x", (void*)vbuf.vbuffer.m.userptr, vbuf.vbuffer.reserved2);
    ret = qbuf(&vbuf);

    return ret;
}

status_t V4L2VideoNode::setParameter (struct v4l2_streamparm *aParam)
{
    LOG2("@%s", __FUNCTION__);
    status_t ret = NO_ERROR;

    if (mState == DEVICE_CLOSED)
        return INVALID_OPERATION;

    ret = pioctl(mFd, VIDIOC_S_PARM, aParam);
    if (ret < 0) {
        LOGE("VIDIOC_S_PARM failed ret %d : %s", ret, strerror(errno));
        ret = UNKNOWN_ERROR;
    }
    return ret;
}

int V4L2VideoNode::getFramerate(float * framerate, int width, int height, int pix_fmt)
{
    LOG1("@%s", __FUNCTION__);
    int ret(0);
    struct v4l2_frmivalenum frm_interval;

    if (NULL == framerate)
        return BAD_VALUE;

    if (mState == DEVICE_CLOSED) {
        LOGE("Invalid state (%d) to set an attribute",mState);
        return UNKNOWN_ERROR;
    }

    CLEAR(frm_interval);
    frm_interval.pixel_format = pix_fmt;
    frm_interval.width = width;
    frm_interval.height = height;
    *framerate = -1.0;

    ret = pioctl(mFd, VIDIOC_ENUM_FRAMEINTERVALS, &frm_interval);
    if (ret < 0) {
        LOGW("ioctl VIDIOC_ENUM_FRAMEINTERVALS failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    } else if (0 == frm_interval.discrete.denominator) {
        LOGE("ioctl VIDIOC_ENUM_FRAMEINTERVALS get invalid denominator value");
        *framerate = 0;
        return UNKNOWN_ERROR;
    }

    *framerate = 1.0 / (1.0 * frm_interval.discrete.numerator / frm_interval.discrete.denominator);

    return NO_ERROR;
}

/**
 * setBufferPool
 * updates the set buffer pool with externally allocated memory
 *
 * The device has to be at least in CONFIGURED state but once configured
 * it the buffer pool can be reset in PREPARED state.
 *
 * This pool will become active after calling start()
 * \param pool: array of void* where the memory is available
 * \param poolSize: amount of buffers in the pool
 * \param aFrameInfo: description of the properties of the buffers
 *                   it should match the configuration passed during setFormat
 * \param cached: boolean to detect whether the buffers are cached or not
 *                A cached buffer in this context means that the buffer
 *                memory may be accessed through some system caches, and
 *                the V4L2 driver must do cache invalidation in case
 *                the image data source is not updating system caches
 *                in hardware.
 *                When false (not cached), V4L2 driver can assume no cache
 *                invalidation/flushes are needed for this buffer.
 */
status_t V4L2VideoNode::setBufferPool(void **pool, unsigned int poolSize,
                                     FrameInfo *aFrameInfo, bool cached)
{
    LOG1("@%s: device = %s", __FUNCTION__, mName.string());
    struct v4l2_buffer_info vinfo;
    CLEAR(vinfo);
    uint32_t cacheflags = V4L2_BUF_FLAG_NO_CACHE_INVALIDATE |
                          V4L2_BUF_FLAG_NO_CACHE_CLEAN;

    if ((mState != DEVICE_CONFIGURED) && (mState != DEVICE_PREPARED)) {
        LOGE("%s:Invalid operation, device %s not configured (state = %d)",
                __FUNCTION__, mName.string(), mState);
        return INVALID_OPERATION;
    }

    if (pool == NULL || aFrameInfo == NULL) {
        LOGE("Invalid parameters, pool %p frameInfo %p",pool, aFrameInfo);
        return BAD_TYPE;
    }

    /**
     * check that the configuration of these buffers matches what we have already
     * told the driver.
     */
    if ((aFrameInfo->width != mConfig.width) ||
        (aFrameInfo->height != mConfig.height) ||
        (aFrameInfo->stride != mConfig.stride) ||
        (aFrameInfo->format != mConfig.format) ) {
        LOGE("Pool configuration does not match device configuration: (%dx%d) s:%d f:%s Pool is: (%dx%d) s:%d f:%s ",
                mConfig.width, mConfig.height, mConfig.stride, v4l2Fmt2Str(mConfig.format),
                aFrameInfo->width, aFrameInfo->height, aFrameInfo->stride, v4l2Fmt2Str(aFrameInfo->format));
        return BAD_VALUE;
    }

    mSetBufferPool.clear();

    for (unsigned int i = 0; i < poolSize; i++) {
        vinfo.data = pool[i];
        vinfo.width = aFrameInfo->stride;
        vinfo.height = aFrameInfo->height;
        vinfo.format = aFrameInfo->format;
        vinfo.length = aFrameInfo->size;
        if (cached)
           vinfo.cache_flags = 0;
       else
           vinfo.cache_flags = cacheflags;

        mSetBufferPool.push(vinfo);
    }

    mState = DEVICE_PREPARED;
    return NO_ERROR;
}

/**
 * setBufferPool
 * Presents the pool of buffers to the device.
 *
 * The device has to be  in CONFIGURED state.
 *
 * In this stage we request the buffer slots to the device and present
 * them to the driver assigning one index to each buffer.
 *
 * After this method the device is PREPARED and ready to Q
 * buffers
 * The structures in the pool are empty.
 * After this call the buffers have been presented to the device an index is assigned.
 * The structure is updated with this index and other details  (this is the output)
 *
 * \param pool: [IN/OUT]Vector of v4l2_buffers structures
 * \param cached: [IN]boolean to detect whether the buffers are cached or not
 *                A cached buffer in this context means that the buffer
 *                memory may be accessed through some system caches, and
 *                the V4L2 driver must do cache invalidation in case
 *                the image data source is not updating system caches
 *                in hardware.
 *                When false (not cached), V4L2 driver can assume no cache
 *                invalidation/flushes are needed for this buffer.
 *\return  INVALID_OPERATION if the device was not configured
 *\return  UNKNOWN_ERROR if any of the v4l2 commands fails
 *\return  NO_ERROR if everything went AOK
 */
status_t V4L2VideoNode::setBufferPool(Vector<struct v4l2_buffer> &pool,
                                      bool cached, int memType)
{
    LOG1("@%s: device = %s", __FUNCTION__, mName.string());
    struct v4l2_buffer_info vinfo;
    CLEAR(vinfo);
    int ret;
    uint32_t cacheflags = V4L2_BUF_FLAG_NO_CACHE_INVALIDATE |
                         V4L2_BUF_FLAG_NO_CACHE_CLEAN;

    if ((mState != DEVICE_CONFIGURED)) {
        LOGE("%s:Invalid operation, device %s not configured (state = %d)",
                __FUNCTION__, mName.string(), mState);
        return INVALID_OPERATION;
    }

    mBufferPool.clear();
    int num_buffers = requestBuffers(pool.size(), memType);
    if (num_buffers <= 0) {
        LOGE("%s: Could not complete buffer request",__FUNCTION__);
        return UNKNOWN_ERROR;
    }

    vinfo.width = mConfig.stride;
    vinfo.height = mConfig.height;
    vinfo.format = mConfig.format;
    vinfo.length = mConfig.size;

    for (unsigned int i = 0; i < pool.size(); i++) {
        if (cached)
            vinfo.cache_flags = 0;
        else
            vinfo.cache_flags = cacheflags;

        //vinfo.data = (void *) pool.editItemAt(i).m.userptr;
        ret = newBuffer(i, vinfo, memType);
        if (ret < 0) {
            LOGE("Error querying buffers status");
            mBufferPool.clear();
            mState = DEVICE_ERROR;
            return UNKNOWN_ERROR;
        }
        pool.replaceAt(vinfo.vbuffer, i);
        mBufferPool.push(vinfo);
    }

    mState = DEVICE_PREPARED;
    return NO_ERROR;
}

////////////////////////////////////////////////////////////////////
//                          PRIVATE METHODS
////////////////////////////////////////////////////////////////////

void V4L2VideoNode::destroyBufferPool()
{

    LOG1("@%s: device = %s", __FUNCTION__, mName.string());

    mBufferPool.clear();

    requestBuffers(0);
}

int V4L2VideoNode::requestBuffers(size_t num_buffers, int memType)
{
    LOG1("@%s, num_buffers:%d", __FUNCTION__, num_buffers);
    struct v4l2_requestbuffers req_buf;
    int ret;
    CLEAR(req_buf);

    if (mState == DEVICE_CLOSED)
        return 0;

    req_buf.memory = memType;
    req_buf.count = num_buffers;

    if (mDirection == INPUT_VIDEO_NODE)
        req_buf.type = mInBufType;
    else if (mDirection == OUTPUT_VIDEO_NODE)
        req_buf.type = mOutBufType;
    else {
        LOGE("Unknown node direction (in/out) this should not happen");
        return -1;
    }

    LOG1("VIDIOC_REQBUFS, count=%d", req_buf.count);
    ret = pioctl(mFd, VIDIOC_REQBUFS, &req_buf);

    if (ret < 0) {
        LOGE("VIDIOC_REQBUFS(%d) returned: %d (%s)",
            num_buffers, ret, strerror(errno));
        return ret;
    }

    if (req_buf.count < num_buffers)
        LOGW("Got less buffers than requested! %d < %d",req_buf.count, num_buffers);

    return req_buf.count;
}

int V4L2VideoNode::qbuf(struct v4l2_buffer_info *buf)
{
    LOG2("@%s", __FUNCTION__);
    struct v4l2_buffer *v4l2_buf = &buf->vbuffer;
    int ret = 0;

    v4l2_buf->flags = buf->cache_flags;
    ret = pioctl(mFd, VIDIOC_QBUF, v4l2_buf);
    if (ret < 0) {
        LOGE("VIDIOC_QBUF on %s failed: %s", mName.string(), strerror(errno));
        return ret;
    }
    android_atomic_inc(&mBuffersInDevice);
    return ret;
}

int V4L2VideoNode::dqbuf(struct v4l2_buffer_info *buf)
{
    LOG2("@%s", __FUNCTION__);
    struct v4l2_buffer *v4l2_buf = &buf->vbuffer;
    int ret = 0;

    v4l2_buf->type = mInBufType;
    v4l2_buf->memory = V4L2_MEMORY_USERPTR;

    ret = pioctl(mFd, VIDIOC_DQBUF, v4l2_buf);
    if (ret < 0) {
        LOGE("VIDIOC_QBUF failed: %s", strerror(errno));
        return ret;
    }
    android_atomic_dec(&mBuffersInDevice);
    return ret;
}

/**
 * creates an active buffer pool from the set-buffer-pool that is provided
 * to the device by the call setBufferPool.
 *
 * We request to the V4L2 driver certain amount of buffer slots with the
 * buffer configuration.
 *
 * Then copy the required number from the set-buffer-pool to the active-buffer-pool
 *
 * \param buffer_count: number of buffers that the active pool contains
 * This number is at maximum the number of buffers in the set-buffer-pool
 * \return 0  success
 *        -1  failure
 */
int V4L2VideoNode::createBufferPool(unsigned int buffer_count)
{
    LOG1("@%s: device = %s buf count %d", __FUNCTION__, mName.string(), buffer_count);
    int i(0);
    int ret(0);

    if (mState != DEVICE_PREPARED) {
        LOGE("%s: Incorrect device state  %d", __FUNCTION__, mState);
        return -1;
    }

    if (buffer_count > mSetBufferPool.size()) {
        LOGE("%s: Incorrect parameter requested %d, but only %d provided",
                __FUNCTION__, buffer_count, mSetBufferPool.size());
        return -1;
    }

    int num_buffers = requestBuffers(buffer_count);
    if (num_buffers <= 0) {
        LOGE("%s: Could not complete buffer request",__FUNCTION__);
        return -1;
    }

    mBufferPool.clear();

    for (i = 0; i < num_buffers; i++) {
        ret = newBuffer(i, mSetBufferPool.editItemAt(i));
        if (ret < 0)
            goto error;
        mBufferPool.push(mSetBufferPool[i]);
    }

    return 0;

error:
    LOGE("Failed to VIDIOC_QUERYBUF some of the buffers, clearing the active buffer pool");
    mBufferPool.clear();
    return ret;
}


int V4L2VideoNode::newBuffer(int index, struct v4l2_buffer_info &buf, int memType)
{
    LOG1("@%s", __FUNCTION__);
    int ret;
    struct v4l2_buffer *vbuf = &buf.vbuffer;

    vbuf->flags = 0x0;
    vbuf->memory = memType;

    if (mDirection == INPUT_VIDEO_NODE)
        vbuf->type = mInBufType;
    else if (mDirection == OUTPUT_VIDEO_NODE)
        vbuf->type = mOutBufType;
    else {
        LOGE("Unknown node direction (in/out) this should not happen");
        return -1;
    }


    vbuf->index = index;
    ret = pioctl(mFd , VIDIOC_QUERYBUF, vbuf);

    if (ret < 0) {
        LOGE("VIDIOC_QUERYBUF failed: %s", strerror(errno));
        return ret;
    }

    if (memType == V4L2_MEMORY_USERPTR) {
        vbuf->m.userptr = (unsigned long)(buf.data);
    } // For MMAP memory the user will do the mmap to get the ptr

    buf.length = vbuf->length;
    LOG1("index %u", vbuf->index);
    LOG1("type %d", vbuf->type);
    LOG1("bytesused %u", vbuf->bytesused);
    LOG1("flags %08x", vbuf->flags);
    if (memType == V4L2_MEMORY_MMAP) {
        LOG1("memory MMAP: offset %d", vbuf->m.offset);
    } else if (memType == V4L2_MEMORY_USERPTR) {
        LOG1("memory USRPTR:  %p", (void*)vbuf->m.userptr);
    }
    LOG1("length %u", vbuf->length);
    return ret;
}

int V4L2VideoNode::freeBuffer(struct v4l2_buffer_info *buf_info)
{
    /**
     * For devices using usr ptr as data this method is a no-op
     */
    UNUSED(buf_info);
    return 0;
}

void V4L2VideoNode::setInputBufferType(enum v4l2_buf_type v4l2BufType)
{
    mInBufType = v4l2BufType;
}

void V4L2VideoNode::setOutputBufferType(enum v4l2_buf_type v4l2BufType)
{
    mOutBufType = v4l2BufType;
}

} // namespace camera2
} // namespace android
