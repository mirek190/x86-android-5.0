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
#define LOG_TAG "Camera_MediaController"

#include "Camera3GFXFormat.h"
#include "MediaController.h"
#include "MediaEntity.h"
#include "v4l2device.h"
#include "LogHelper.h"
#include <sys/stat.h>
#include <fcntl.h>

namespace android {
namespace camera2 {

const int MAX_ENTITY_NAME_LENGTH = 50;

MediaController::MediaController(const char *name) :
    mName(name),
    mFd(-1)
{
    LOG1("@%s", __FUNCTION__);
    CLEAR(mDeviceInfo);
    mEntities.clear();
    mEntityDesciptors.clear();
}

MediaController::~MediaController()
{
    LOG1("@%s", __FUNCTION__);
    mEntityDesciptors.clear();
    mEntities.clear();
    close();
}

status_t MediaController::init()
{
    LOG1("@%s %s", __FUNCTION__, mName.string());
    status_t status = NO_ERROR;

    status = open();
    if (status != NO_ERROR) {
        LOGE("Error opening media device");
        return status;
    }

    status = getDeviceInfo();
    if (status != NO_ERROR) {
        LOGE("Error getting media info");
        return status;
    }

    status = findEntities();
    if (status != NO_ERROR) {
        LOGE("Error finding media entities");
        return status;
    }

    return status;
}

status_t MediaController::open()
{
    LOG1("@%s %s", __FUNCTION__, mName.string());
    status_t ret = NO_ERROR;
    struct stat st;

    if (mFd != -1) {
        LOGW("Trying to open a device already open");
        return NO_ERROR;
    }

    if (stat (mName.string(), &st) == -1) {
        LOGE("Error stat media device %s: %s",
             mName.string(), strerror(errno));
        return UNKNOWN_ERROR;
    }

    if (!S_ISCHR (st.st_mode)) {
        LOGE("%s is not a device", mName.string());
        return UNKNOWN_ERROR;
    }

    mFd = ::open(mName.string(), O_RDWR);

    if (mFd < 0) {
        LOGE("Error opening media device %s: %s",
              mName.string(), strerror(errno));
        return UNKNOWN_ERROR;
    }

    return ret;
}

status_t MediaController::close()
{
    LOG1("@%s device : %s", __FUNCTION__, mName.string());

    if (mFd == -1) {
        LOGW("Device not opened!");
        return INVALID_OPERATION;
    }

    if (::close(mFd) < 0) {
        LOGE("Close media device failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    mFd = -1;
    return NO_ERROR;
}

int MediaController::xioctl(int request, void *arg) const
{
    int ret(0);
    if (mFd == -1) {
        LOGE("%s invalid device closed!",__FUNCTION__);
        return INVALID_OPERATION;
    }

    do {
        ret = ioctl (mFd, request, arg);
    } while (-1 == ret && EINTR == errno);

    if (ret < 0)
        LOGW ("%s: Request 0x%x failed: %s", __FUNCTION__, request, strerror(errno));

    return ret;
}

status_t MediaController::getDeviceInfo()
{
    LOG1("@%s", __FUNCTION__);
    CLEAR(mDeviceInfo);
    int ret = xioctl(MEDIA_IOC_DEVICE_INFO, &mDeviceInfo);
    if (ret < 0) {
        LOGE("Failed to get media device information");
        return UNKNOWN_ERROR;
    }

    LOG1("Media device: %s", mDeviceInfo.driver);
    return NO_ERROR;
}

status_t MediaController::findEntities()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    struct media_entity_desc entity;

    // Loop until all media entities are found
    for (int i = 0; ; i++) {
        CLEAR(entity);
        status = findMediaEntityById(i | MEDIA_ENT_ID_FLAG_NEXT, entity);
        if (status != NO_ERROR) {
            LOGD("@%s: %d media entities found", __FUNCTION__, i);
            break;
        }
        mEntityDesciptors.add(String8(entity.name), entity);
        LOGD("entity name: %s, id: %d, pads: %d, links: %d",
              entity.name, entity.id, entity.pads, entity.links);
    }

    if (mEntityDesciptors.size() > 0)
        status = NO_ERROR;

    return status;
}

status_t MediaController::enumLinks(struct media_links_enum &linkInfo)
{
    LOG1("@%s", __FUNCTION__);
    int ret = 0;
    ret = xioctl(MEDIA_IOC_ENUM_LINKS, &linkInfo);
    if (ret < 0) {
        LOGE("Enumerating entity links failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

/**
 * Find description for given entity ID
 *
 * Using media controller here to query entity with given index.
 */
status_t MediaController::findMediaEntityById(int index, struct media_entity_desc &mediaEntityDesc)
{
    LOG1("@%s", __FUNCTION__);
    int ret = 0;
    CLEAR(mediaEntityDesc);
    mediaEntityDesc.id = index;
    ret = xioctl(MEDIA_IOC_ENUM_ENTITIES, &mediaEntityDesc);
    if (ret < 0) {
        LOG1("Enumerating entities failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

/**
 * Find description for given entity name
 *
 * Using media controller to query entity with given name.
 */
status_t MediaController::findMediaEntityByName(char const* entityName, struct media_entity_desc &mediaEntityDesc)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = UNKNOWN_ERROR;
    for (int i = 0; ; i++) {
        status = findMediaEntityById(i | MEDIA_ENT_ID_FLAG_NEXT, mediaEntityDesc);
        if (status != NO_ERROR)
            break;
        LOG2("Media entity %d: %s", i, mediaEntityDesc.name);
        if (strncmp(mediaEntityDesc.name, entityName, MAX_ENTITY_NAME_LENGTH) == 0)
            break;
    }
    return status;
}

status_t MediaController::setFormat(const char* entityName, int pad, int width, int height, int formatCode)
{
    LOG1("@%s entity %s pad %d (%dx%d) format(%d)", __FUNCTION__,
                                                        entityName, pad,
                                                        width, height,
                                                        formatCode);
    sp<MediaEntity> entity;
    sp<V4L2Subdevice> subdev;
    status_t status = NO_ERROR;

    status = getMediaEntity(entity, entityName);
    if (status != NO_ERROR) {
        LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, entityName);
        return status;
    }
    if (entity->getType() == DEVICE_VIDEO) {
        sp<V4L2VideoNode> node;
        FrameInfo config;

        CLEAR(config);
        config.format = formatCode;
        config.width = width;
        config.height = height;
        config.stride = width;

        status = entity->getDevice((sp<V4L2DeviceBase>&) node);
        if (status != NO_ERROR) {
            LOGE("@%s: error opening device \"%s\"", __FUNCTION__, entityName);
            return status;
        }
        status = node->setFormat(config);
    } else {
        sp<V4L2Subdevice> subdev;
        status = entity->getDevice((sp<V4L2DeviceBase>&) subdev);
        if (status != NO_ERROR) {
            LOGE("@%s: error opening device \"%s\"", __FUNCTION__, entityName);
            return status;
        }
        status = subdev->setFormat(pad, width, height, formatCode);
    }
    return status;
}

status_t MediaController::setSelection(const char* entityName, int pad, int target, int top, int left, int width, int height)
{
    LOG1("@%s", __FUNCTION__);
    sp<MediaEntity> entity;
    sp<V4L2Subdevice> subdev;

    status_t status = getMediaEntity(entity, entityName);
    if (status != NO_ERROR) {
        LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, entityName);
        return status;
    }
    status = entity->getDevice((sp<V4L2DeviceBase>&) subdev);
    if (status != NO_ERROR) {
        LOGE("@%s: error opening device \"%s\"", __FUNCTION__, entityName);
        return status;
    }
    return subdev->setSelection(pad, target, top, left, width, height);
}

status_t MediaController::setControl(const char* entityName, int controlId, int value, const char *controlName)
{
    LOG1("@%s entity %s ctrl ID %d value %d name %s", __FUNCTION__,
                                                         entityName, controlId,
                                                         value,controlName);
    sp<MediaEntity> entity;
    sp<V4L2Subdevice> subdev;

    status_t status = getMediaEntity(entity, entityName);
    if (status != NO_ERROR) {
        LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, entityName);
        return status;
    }
    status = entity->getDevice((sp<V4L2DeviceBase>&) subdev);
    if (status != NO_ERROR) {
        LOGE("@%s: error opening device \"%s\"", __FUNCTION__, entityName);
        return status;
    }
    return subdev->setControl(controlId, value, controlName);
}

/**
 * Enable or disable link between two given media entities
 */
status_t MediaController::configureLink(const char* srcName, int srcPad, const char* sinkName, int sinkPad, bool enable)
{
    LOG1(" @%s: %s \"%s\" [%d] --> \"%s\" [%d]", __FUNCTION__, enable?"enable":"disable", srcName, srcPad, sinkName, sinkPad);
    status_t status = NO_ERROR;

    sp<MediaEntity> srcEntity, sinkEntity;
    struct media_link_desc linkDesc;
    struct media_pad_desc srcPadDesc, sinkPadDesc;

    status = getMediaEntity(srcEntity, srcName);
    if (status != NO_ERROR) {
        LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, srcName);
        return status;
    }
    status = getMediaEntity(sinkEntity, sinkName);
    if (status != NO_ERROR) {
        LOGE("@%s: getting MediaEntity \"%s\" failed", __FUNCTION__, sinkName);
        return status;
    }
    CLEAR(linkDesc);
    srcEntity->getPadDesc(srcPadDesc, srcPad);
    sinkEntity->getPadDesc(sinkPadDesc, sinkPad);
    linkDesc.source = srcPadDesc;
    linkDesc.sink = sinkPadDesc;
    if (enable)
        linkDesc.flags |= MEDIA_LNK_FL_ENABLED;
    else
        linkDesc.flags &= ~MEDIA_LNK_FL_ENABLED;

    status = setupLink(linkDesc);

    // refresh source entity links
    if (status == NO_ERROR) {
        struct media_entity_desc entityDesc;
        struct media_links_enum linksEnum;
        struct media_link_desc *links = NULL;

        srcEntity->getEntityDesc(entityDesc);
        links = new struct media_link_desc[entityDesc.links];

        linksEnum.entity = entityDesc.id;
        linksEnum.pads = NULL;
        linksEnum.links = links;
        status = enumLinks(linksEnum);
        if (status == NO_ERROR) {
            srcEntity->updateLinks(linksEnum.links);
        }
        delete[] links;
    }

    return status;
}

status_t MediaController::setupLink(struct media_link_desc &linkDesc)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    int ret = xioctl(MEDIA_IOC_SETUP_LINK, &linkDesc);
    if (ret < 0) {
        LOGE("Link setup failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }
    return status;
}

/**
 * Resets (disables) all links between entities
 */
status_t MediaController::resetLinks()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    for (size_t i = 0; i < mEntityDesciptors.size(); i++) {
        struct media_entity_desc entityDesc;
        struct media_links_enum linksEnum;
        struct media_link_desc *links = NULL;

        entityDesc = mEntityDesciptors[i];
        links = new struct media_link_desc[entityDesc.links];
        LOG1("@%s entity id: %d", __FUNCTION__, entityDesc.id);

        linksEnum.entity = entityDesc.id;
        linksEnum.pads = NULL;
        linksEnum.links = links;
        status = enumLinks(linksEnum);
        if (status != NO_ERROR) {
            break;
        }
        // Disable all links, except the immutable ones
        for (int j = 0; j < entityDesc.links; j++) {
            if (links[j].flags & MEDIA_LNK_FL_IMMUTABLE)
                continue;

            links[j].flags &= ~MEDIA_LNK_FL_ENABLED;
            setupLink(links[j]);
        }
    }
    return status;
}

status_t MediaController::getMediaEntity(sp<MediaEntity> &entity, const char* name)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    String8 entityName(name);
    struct media_entity_desc entityDesc;
    struct media_links_enum linksEnum;
    struct media_link_desc *links = NULL;
    struct media_pad_desc *pads = NULL;

    // check whether the MediaEntity object has already been created
    if (mEntities.indexOfKey(entityName) >= 0) {
        entity = mEntities.valueFor(entityName);
    } else if (mEntityDesciptors.indexOfKey(entityName) >= 0) {
        // MediaEntity object not yet created, so create it
        entityDesc = mEntityDesciptors.valueFor(entityName);
        if (entityDesc.links > 0)
            links = new struct media_link_desc[entityDesc.links];
        if (entityDesc.pads > 0)
            pads = new struct media_pad_desc[entityDesc.pads];

        LOG1("Creating entity - name: %s, id: %d, links: %d, pads: %d",
             entityDesc.name, entityDesc.id, entityDesc.links, entityDesc.pads);

        linksEnum.entity = entityDesc.id;
        linksEnum.pads = pads;
        linksEnum.links = links;
        status = enumLinks(linksEnum);

        if (status == NO_ERROR) {
            entity = new MediaEntity(entityDesc, links, pads);
            mEntities.add(String8(entityDesc.name), entity);
        }
    } else {
        LOGE("@%s: entity %s is not registered to the media device", __FUNCTION__, entityName.string());
        status = UNKNOWN_ERROR;
    }

    delete[] links;
    delete[] pads;

    return status;
}


}  // namespace camera2
}  // namespace android
