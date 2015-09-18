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

#ifndef CAMERA2600_LENSHW_H_
#define CAMERA2600_LENSHW_H_

#include "ICameraIPU4HwControls.h"
#include "PlatformData.h"

namespace android {
namespace camera2 {

class V4L2Subdevice;
class MediaController;
class MediaEntity;
/**
 * \class LensHw
 * This class adds the methods that are needed
 * to drive the camera lens using v4l2 commands and custom ioctl.
 *
 */
class LensHw : public IIPU4HWLensControl, public RefBase {
public:
    LensHw(int cameraId, sp<MediaController> mediaCtl);
    ~LensHw();

    status_t setLens(sp<MediaEntity> entity);

    virtual const char* getLensName(void);
    virtual int getCurrentCameraId(void);

    // FOCUS
    virtual int moveFocusToPosition(int position);
    virtual int moveFocusToBySteps(int steps);
    virtual int getFocusPosition(int &position);
    virtual int getFocusStatus(int &status);
    virtual int startAutoFocus(void);
    virtual int stopAutoFocus(void);
    virtual int getAutoFocusStatus(unsigned int &status);
    virtual int setAutoFocusRange(int value);
    virtual int getAutoFocusRange(int &value);

    // ZOOM
    virtual int moveZoomToPosition(int position);
    virtual int moveZoomToBySteps(int steps);
    virtual int getZoomPosition(int &position);
    virtual int moveZoomContinuous(int position);
    /* [END] IHWLensControl overloads, */

private:
    static const int MAX_LENS_NAME_LENGTH = 32;

    struct lensInfo {
        uint32_t index;      //!< V4L2 index
        char name[MAX_LENS_NAME_LENGTH];
    };


private:
    int mCameraId;
    sp<MediaController> mMediaCtl;
    sp<V4L2Subdevice> mLensSubdev;
    struct lensInfo mLensInput;
};  // class LensHW

};  // namespace camera2
};  // namespace android

#endif  // CAMERA2600_LENSHW_H__
