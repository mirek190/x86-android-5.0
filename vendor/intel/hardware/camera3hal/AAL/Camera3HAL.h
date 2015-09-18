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

#ifndef _CAMERA3_HAL_H_
#define _CAMERA3_HAL_H_

#include <hardware/camera3.h>

#include "CameraConf.h"
#include "PlatformData.h"
#include "RequestThread.h"
#include "MemoryDumper.h"

namespace android {
namespace camera2 {

class MemoryDumper;

/**
 * \class Camera3HAL
 *
 * This class represents a single HAL device instance. It has the following
 * roles:
 * - It implements the camera3_device_ops_t  API  defined by Android.
 * - It instantiates all the other objects that make the HAL:
 *      - PSL layer
 *      - RequesThread
 *      - StreamManager
 *      - Request manager
 */
class Camera3HAL :public camera3_device_ops_t {

public:
    Camera3HAL(int cameraId, const hw_module_t* module);
    virtual ~Camera3HAL();
    status_t init(void);
    status_t deinit(void);
    camera3_device_t* getDeviceStruct() { return &mDevice; }

    /**********************************************************************
     * camera3_device_ops_t implementation
     */
    status_t initialize(const camera3_callback_ops_t *callback_ops);

    int configure_streams(camera3_stream_configuration_t *stream_list);

    camera_metadata_t* construct_default_request_settings(int type);

    int process_capture_request(camera3_capture_request_t *request);

    void get_metadata_vendor_tag_ops(vendor_tag_query_ops_t *ops);

    void dump(int fd);

    int flush();

public: /* static members to track object count */
    static Camera3HAL* sInstances[MAX_CAMERAS];
    static int sInstaceCount;

private:
    int mCameraId;
    ICameraHw *mCameraHw;
    sp<RequestThread> mRequestThread;
    MemoryDumper mMemorydumper;
    camera3_device_t   mDevice;
};


}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_H_
