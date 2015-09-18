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
#define LOG_TAG "Camera_HwStreamBase"

#include "LogHelper.h"
#include "HwStreamBase.h"
#include "CameraBuffer.h"

#include <utils/Errors.h>

namespace android {
namespace camera2 {

HwStreamBase::HwStreamBase()
{
    LOG1("@%s", __FUNCTION__);
}


HwStreamBase::~HwStreamBase()
{
    LOG1("@%s", __FUNCTION__);
}

status_t HwStreamBase::query(FrameInfo *info)
{
    UNUSED(info);
    return NO_ERROR;
}

status_t HwStreamBase::registerBuffers(Vector<sp<CameraBuffer> > &theBuffers)
{
    UNUSED(theBuffers);
    return NO_ERROR;
}

status_t HwStreamBase::capture(sp<CameraBuffer> aBuffer,
                                Camera3Request* request)
{
    UNUSED(aBuffer);
    UNUSED(request);
    return NO_ERROR;
}

status_t HwStreamBase::captureDone(sp<CameraBuffer> buffer,
                                    Camera3Request* request)
{
    UNUSED(buffer);
    UNUSED(request);
    return NO_ERROR;
}

status_t HwStreamBase::reprocess(sp<CameraBuffer> buffer,
                                  Camera3Request* request)
{
    LOG1("@%s capture stream", __FUNCTION__);
    UNUSED(buffer);
    UNUSED(request);
    return OK;
}

void HwStreamBase::dump(int fd) const
{
    UNUSED(fd);
}

status_t HwStreamBase::configure(void)
{
    return NO_ERROR;
}

} /* namespace camera2 */
} /* namespace android */
