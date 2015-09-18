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

#define LOG_TAG "Camera_V4L2Subdev"

#include "LogHelper.h"
#include "v4l2device.h"
#include "UtilityMacros.h"

namespace android {
namespace camera2 {

////////////////////////////////////////////////////////////////////
//                          PUBLIC METHODS
////////////////////////////////////////////////////////////////////


V4L2Subdevice::V4L2Subdevice(const char *name):
    V4L2DeviceBase(name),
    mState(DEVICE_CLOSED)
{
    LOG1("@%s: %s", __FUNCTION__, name);
    CLEAR(mConfig);
}

V4L2Subdevice::~V4L2Subdevice()
{
    LOG1("@%s", __FUNCTION__);

}

status_t V4L2Subdevice::open()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    status = V4L2DeviceBase::open();
    if (status == NO_ERROR)
        mState = DEVICE_OPEN;

    return status;
}

status_t V4L2Subdevice::close()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    status = V4L2DeviceBase::close();
    if (status == NO_ERROR)
        mState = DEVICE_CLOSED;

    return status;
}

status_t V4L2Subdevice::setFormat(int pad, int width, int height, int formatCode)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    struct v4l2_subdev_format format;

    CLEAR(format);
    format.pad = pad;
    format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    format.format.code = formatCode;
    format.format.width = width;
    format.format.height = height;
    return setFormat(format);
}

/**
 * Update the current subdevice configuration
 *
 * This called is allowed in the following states:
 * - OPEN
 * - CONFIGURED
 *
 * \param aFormat:[IN] reference to the new v4l2_subdev_format
 *
 *  \return NO_ERROR if everything went well
 *          INVALID_OPERATION if device is not in correct state (open)
 *          UNKNOW_ERROR if we get an error from the v4l2 ioctl
 */
status_t V4L2Subdevice::setFormat(struct v4l2_subdev_format &aFormat)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    int ret = 0;

    if ((mState != DEVICE_OPEN) &&
        (mState != DEVICE_CONFIGURED)){
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    LOG1("VIDIOC_SUBDEV_S_FMT: pad: %d, which: %d, width: %d, "
         "height: %d, format: 0x%x, field: %d, color space: %d",
         aFormat.pad,
         aFormat.which,
         aFormat.format.width,
         aFormat.format.height,
         aFormat.format.code,
         aFormat.format.field,
         aFormat.format.colorspace);

    ret = pbxioctl(VIDIOC_SUBDEV_S_FMT, &aFormat);
    if (ret < 0) {
        LOGE("VIDIOC_SUBDEV_S_FMT failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    // TODO: Update current pad configuration with the new one


    mState = DEVICE_CONFIGURED;
    return NO_ERROR;
}

status_t V4L2Subdevice::getFormat(struct v4l2_subdev_format &aFormat)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    int ret = 0;

    if ((mState != DEVICE_OPEN) &&
        (mState != DEVICE_CONFIGURED)){
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    ret = pbxioctl(VIDIOC_SUBDEV_G_FMT, &aFormat);
    if (ret < 0) {
        LOGE("VIDIOC_SUBDEV_G_FMT failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    LOG1("VIDIOC_SUBDEV_G_FMT: pad: %d, which: %d, width: %d, "
         "height: %d, format: 0x%x, field: %d, color space: %d",
         aFormat.pad,
         aFormat.which,
         aFormat.format.width,
         aFormat.format.height,
         aFormat.format.code,
         aFormat.format.field,
         aFormat.format.colorspace);

    return NO_ERROR;
}

status_t V4L2Subdevice::getPadFormat(int padIndex, int &width, int &height, int &code)
{
    LOG1("@%s pad: %d", __FUNCTION__, padIndex);
    status_t status = NO_ERROR;
    struct v4l2_subdev_format format;

    CLEAR(format);
    format.pad = padIndex;
    format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    getFormat(format);
    if (status == NO_ERROR) {
        width = format.format.width;
        height = format.format.height;
        code = format.format.code;
    }
    return status;
}

status_t V4L2Subdevice::setSelection(int pad, int target, int top, int left, int width, int height)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    struct v4l2_subdev_selection selection;

    CLEAR(selection);
    selection.pad = pad;
    selection.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    selection.target = target;
    selection.flags = 0;
    selection.r.top = top;
    selection.r.left = left;
    selection.r.width = width;
    selection.r.height = height;

    return setSelection(selection);
}

status_t V4L2Subdevice::setSelection(struct v4l2_subdev_selection &aSelection)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    int ret = 0;

    if ((mState != DEVICE_OPEN) &&
        (mState != DEVICE_CONFIGURED)){
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    LOG1("VIDIOC_SUBDEV_S_SELECTION: which: %d, pad: %d, target: 0x%x, "
         "flags: 0x%x, rect left: %d, rect top: %d, width: %d, height: %d",
        aSelection.which,
        aSelection.pad,
        aSelection.target,
        aSelection.flags,
        aSelection.r.left,
        aSelection.r.top,
        aSelection.r.width,
        aSelection.r.height);

    ret = pbxioctl(VIDIOC_SUBDEV_S_SELECTION, &aSelection);
    if (ret < 0) {
        LOGE("VIDIOC_SUBDEV_S_SELECTION failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    // TODO: Update current pad configuration with the new one

    return NO_ERROR;
}

status_t V4L2Subdevice::getSelection(struct v4l2_subdev_selection &aSelection)
{
    LOG1("@%s device = %s", __FUNCTION__, mName.string());
    int ret = 0;

    if ((mState != DEVICE_OPEN) &&
        (mState != DEVICE_CONFIGURED)){
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    ret = pbxioctl(VIDIOC_SUBDEV_G_SELECTION, &aSelection);
    if (ret < 0) {
        LOGE("VIDIOC_SUBDEV_G_SELECTION failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    LOG1("VIDIOC_SUBDEV_G_SELECTION: which: %d, pad: %d, target: 0x%x, "
         "flags: 0x%x, rect left: %d, rect top: %d, width: %d, height: %d",
        aSelection.which,
        aSelection.pad,
        aSelection.target,
        aSelection.flags,
        aSelection.r.left,
        aSelection.r.top,
        aSelection.r.width,
        aSelection.r.height);

    return NO_ERROR;
}

status_t V4L2Subdevice::queryFormats(int pad, Vector<uint32_t> &formats)
{
    LOG1("@%s device = %s, pad: %d", __FUNCTION__, mName.string(), pad);
    struct v4l2_subdev_mbus_code_enum aFormat;

    if (mState == DEVICE_CLOSED) {
        LOGE("%s invalid device state %d",__FUNCTION__, mState);
        return INVALID_OPERATION;
    }

    formats.clear();
    CLEAR(aFormat);

    aFormat.pad = pad;
    aFormat.index = 0;

    while (pbxioctl(VIDIOC_SUBDEV_ENUM_MBUS_CODE, &aFormat) == 0) {
        formats.push(aFormat.code);
        aFormat.index++;
    };

    LOG1("@%s device: %s, %d formats retrieved", __FUNCTION__, mName.string(), formats.size());
    return NO_ERROR;
}

////////////////////////////////////////////////////////////////////
//                          PRIVATE METHODS
////////////////////////////////////////////////////////////////////


} // namespace camera2
} // namespace android



