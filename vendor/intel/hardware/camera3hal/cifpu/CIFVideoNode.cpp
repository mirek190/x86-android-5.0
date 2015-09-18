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

#include "CIFVideoNode.h"
#include "PlatformData.h"
#include "LogHelper.h"
#include "UtilityMacros.h"

namespace android {
namespace camera2 {

CIFVideoNode::CIFVideoNode(const char *name,
        VideNodeDirection nodeDirection,
        VideoNodeType nodeType) : V4L2VideoNode(name,
                                                nodeDirection,
                                                nodeType) {
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
status_t CIFVideoNode::setFormat(FrameInfo &aConfig)
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
    v4l2_fmt.fmt.pix.width = aConfig.width;
    v4l2_fmt.fmt.pix.height = aConfig.height;
    v4l2_fmt.fmt.pix.pixelformat = aConfig.format;
    v4l2_fmt.fmt.pix.bytesperline = displayBpl(aConfig.format, aConfig.width);
    v4l2_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    v4l2_fmt.fmt.pix.sizeimage = 0;

    // Update current configuration with the new one
    ret = V4L2VideoNode::setFormat(v4l2_fmt);
    if (ret != NO_ERROR)
        return ret;

    // .. but get the stride from ISP
    // and update the new configuration struct with it
    aConfig.stride = mConfig.stride;

    // Do the same for the frame size
    aConfig.size = mConfig.size;

    return NO_ERROR;
}

void CIFVideoNode::destroyBufferPool()
{
    LOG1("@%s: device = %s", __FUNCTION__, mName.string());
    mBufferPool.clear();
}

int CIFVideoNode::stop(bool keepBuffers)
{
    LOG1("@%s: device = %s", __FUNCTION__, mName.string());
    int ret = 0;

    // Always stream off because of driver feature.
    // Driver will only turn JPEG encoding off in the streamoff.
    enum v4l2_buf_type type = mInBufType;
    ret = pioctl(mFd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("VIDIOC_STREAMOFF returned: %d (%s)", ret, strerror(errno));
        return ret;
    }
    mState = DEVICE_PREPARED;

    if (mState == DEVICE_PREPARED) {
        if (!keepBuffers) {
            destroyBufferPool();
            mState = DEVICE_CONFIGURED;
        }
   } else {
        LOGW("Trying to stop a device not started");
        ret = -1;
    }

    mBuffersInDevice = 0;

    return ret;
}

} /* namespace camera2 */
} /* namespace android */
