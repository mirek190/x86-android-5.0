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
#define LOG_TAG "Camera_MediaEntity"

#include "Camera3GFXFormat.h"
#include "MediaEntity.h"
#include "LogHelper.h"

namespace android {
namespace camera2 {

MediaEntity::MediaEntity(struct media_entity_desc &entity, struct media_link_desc *links,
                         struct media_pad_desc *pads) :
    mInfo(entity),
    mDevice(NULL)
{
    LOG1("@%s: %s, id: %d", __FUNCTION__, entity.name, entity.id);
    mLinks.setCapacity(entity.links);
    mPads.setCapacity(entity.pads);
    mLinks.clear();
    mPads.clear();

    if (links != NULL) {
        for (int i = 0; i < entity.links; i++) {
            LOG1("link %d: pad %d --> sink entity %d:%d (%s%s%s)", i, links[i].source.index,
                links[i].sink.entity, links[i].sink.index, (links[i].flags & MEDIA_LNK_FL_ENABLED)?"enabled":"disabled",
                (links[i].flags & MEDIA_LNK_FL_IMMUTABLE)?" immutable":"", (links[i].flags & MEDIA_LNK_FL_DYNAMIC)?" dynamic":"");
            mLinks.add(links[i]);
        }
    }

    if (pads != NULL) {
        for (int i = 0; i < entity.pads; i++) {
            LOG1("pad %d (%s)", pads[i].index, (pads[i].flags & MEDIA_PAD_FL_SINK)?"SINK":"SOURCE");
            mPads.add(pads[i]);
        }
    }
}

MediaEntity::~MediaEntity()
{
    LOG1("@%s", __FUNCTION__);
    CLEAR(mInfo);
    mLinks.clear();
    mPads.clear();

    if (mDevice != NULL) {
        if (mDevice->isOpen())
            mDevice->close();
        mDevice.clear();
        mDevice = NULL;
    }
}

status_t MediaEntity::getDevice(sp<V4L2DeviceBase> &device, VideoNodeType nodeType)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    if (mDevice == NULL || !mDevice->isOpen()) {
        LOG1("Opening device");
        status = openDevice(mDevice, nodeType);
    }
    if (status == NO_ERROR)
        device = mDevice;

    return status;
}

status_t MediaEntity::openDevice(sp<V4L2DeviceBase> &device, VideoNodeType nodeType)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = UNKNOWN_ERROR;
    int ret = 0;
    char sysname[1024];
    char devname[1024];
    int major = mInfo.v4l.major;
    int minor = mInfo.v4l.minor;
    sprintf(devname, "/sys/dev/char/%u:%u", major, minor);
    ret = readlink(devname, sysname, sizeof(sysname));
    if (ret < 0) {
        LOGE("Unable to find device node");
    } else {
        sysname[ret] = 0;
        char *lastSlash = strrchr(sysname, '/');
        if (lastSlash == NULL) {
            LOGE("Invalid sysfs device path");
            return status;
        }
        sprintf(devname, "/dev/%s", lastSlash + 1);
        LOG1("Device node : %s", devname);

        device.clear();
        if (mInfo.type == MEDIA_ENT_T_DEVNODE_V4L)
            device = new V4L2VideoNode(devname, V4L2VideoNode::INPUT_VIDEO_NODE, nodeType);
        else
            device = new V4L2Subdevice(devname);
        status = device->open();
        if (status != NO_ERROR) {
            LOGE("Failed to open device %s", devname);
            device.clear();
        }
    }
    return status;
}

void MediaEntity::updateLinks(const struct media_link_desc *links)
{
    LOG1("@%s", __FUNCTION__);

    mLinks.clear();
    for (int i = 0; i < mInfo.links; i++) {
        LOG1("link %d: pad %d --> sink entity %d:%d (%s%s%s)", i, links[i].source.index,
            links[i].sink.entity, links[i].sink.index, (links[i].flags & MEDIA_LNK_FL_ENABLED)?"enabled":"disabled",
            (links[i].flags & MEDIA_LNK_FL_IMMUTABLE)?" immutable":"", (links[i].flags & MEDIA_LNK_FL_DYNAMIC)?" dynamic":"");

        mLinks.add(links[i]);
    }
}

V4L2DeviceType MediaEntity::getType()
{
    LOG1("@%s", __FUNCTION__);

    switch (mInfo.type) {
    case MEDIA_ENT_T_DEVNODE_V4L:
        return DEVICE_VIDEO;
        break;
    case MEDIA_ENT_T_V4L2_SUBDEV:
        return SUBDEV_GENERIC;
        break;
    case MEDIA_ENT_T_V4L2_SUBDEV_SENSOR:
        return SUBDEV_SENSOR;
        break;
    case MEDIA_ENT_T_V4L2_SUBDEV_FLASH:
        return SUBDEV_FLASH;
        break;
    case MEDIA_ENT_T_V4L2_SUBDEV_LENS:
        return SUBDEV_LENS;
        break;
    default:
        LOGE("Unknown media entity type: %d", mInfo.type);
        return UNKNOWN_TYPE;
    }
}


}  // namespace camera2
}  // namespace android
