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

#ifndef _CAMERA3_HAL_MEDIACTLPIPECONFIG_H_
#define _CAMERA3_HAL_MEDIACTLPIPECONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
namespace android {
namespace camera2 {

typedef struct {
    String8 name;
    String8 type;
    int videoNodeType;
} MediaCtlElement;

typedef struct {
    int outputWidth;
    int outputHeight;
} CameraRequestProperties;

typedef struct {
    String8 srcName;
    int srcPad;
    String8 sinkName;
    int sinkPad;
    bool enable;
} MediaCtlLinkParams;

typedef struct {
    String8 entityName;
    int pad;
    int width;
    int height;
    int formatCode;
} MediaCtlFormatParams;

typedef struct {
    String8 entityName;
    int pad;
    int target;
    int top;
    int left;
    int width;
    int height;
} MediaCtlSelectionParams;

typedef struct {
    String8 entityName;
    int controlId;
    int value;
    String8 controlName;
} MediaCtlControlParams;

/**
 * \struct MediaCtlSingleConfig
 *
 * This struct is holding all the possible
 * media ctl pipe configurations for the
 * camera in use.
 * It is holding the commands parameters for
 * setting up a camera pipe.
 *
 */
typedef struct {
    CameraRequestProperties mCameraProps;
    Vector<MediaCtlLinkParams> mLinkParams;
    Vector<MediaCtlFormatParams> mFormatParams;
    Vector<MediaCtlSelectionParams> mSelectionParams;
    Vector<MediaCtlControlParams> mControlParams;
    Vector<MediaCtlElement> mVideoNodes;
} MediaCtlConfig;

typedef struct {
    Vector<MediaCtlElement> mMediaCtlElements;
    Vector<MediaCtlConfig> mMediaCtlConfigs;
} MediaCtlProperties;


}   // namespace camera2
}   // namespace android

#endif  // _CAMERA3_HAL_MEDIACTLPIPECONFIG_H_
