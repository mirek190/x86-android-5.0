/*
* Copyright (c) 2014 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#define LOG_TAG "DisplayIEDControl"
#include <utils/Log.h>

extern "C"
{
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <linux/psb_drm.h>
#include "xf86drm.h"
#include "xf86drmMode.h"
}

#include "DisplayIEDControl.h"

const int INVALID_DEVICE_FD = -1 ;

DisplayIEDControl::DisplayIEDControl()
: mDrmDevFd(INVALID_DEVICE_FD)
{
    const char* const DRM_DEVICE = "/dev/card0" ;
    mDrmDevFd = open(DRM_DEVICE, O_RDWR, 0);
    if (mDrmDevFd < 0)
    {
        ALOGE("Failed to open DRM device %s (errno %d)", DRM_DEVICE, errno);
    }
}

DisplayIEDControl::~DisplayIEDControl()
{
    if (mDrmDevFd >= 0)
    {
        close(mDrmDevFd) ;
        mDrmDevFd = INVALID_DEVICE_FD ;
    }
}

bool DisplayIEDControl::Enable(bool enable)
{
    if (mDrmDevFd < 0)
    {
        ALOGE("Enable(): renderming manager device is not open") ;
        return false;
    }

    int request = enable ?  DRM_PSB_ENABLE_IED_SESSION : DRM_PSB_DISABLE_IED_SESSION;
    int ret = drmCommandNone(mDrmDevFd, request);
    if (ret != 0)
    {
        ALOGE("drmCommandNone(request=%d) returned %d", request, ret) ;
    }

    return ret == 0;
}
