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

#define LOG_TAG "Camera3HALModule"

#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Mutex.h>
#include <hardware/camera3.h>

#include "PlatformData.h"
#include "CameraConf.h"
#include "PerformanceTraces.h"

#include "Camera3HAL.h"

using namespace android;
using namespace android::camera2;

static int hal_dev_close(hw_device_t* device);

/**********************************************************************
 * Camera Module API (C API)
 **********************************************************************/
/**
 * Global mutex used to protect device open/close
 */
static Mutex cameraHalMutex;

int openCameraHardware(int id, const hw_module_t* module, hw_device_t** device)
{
    if (Camera3HAL::sInstances[id] != NULL)
        return 0;

    /**
     * This is a temporary HACK needed until IPU2 implementation stops using
     * this member variable from PlatformData.
     * TO BE REMOVED
     */
    PlatformData::AiqConfig = *PlatformData::getAiqConfiguration(id);

    Camera3HAL* halDev = new android::camera2::Camera3HAL(id, module);

    if (halDev == NULL) {
        ALOGE("Memory allocation error for HAL!");
        return -ENOMEM;
    }

    if (halDev->init() != NO_ERROR) {
        ALOGE("HAL initialization fail!");
        delete halDev;
        return -EINVAL;
    }
    camera3_device_t *cam3Device = halDev->getDeviceStruct();

    cam3Device->common.close = hal_dev_close;
    *device = &cam3Device->common;
    return 0;
}

static int hal_get_number_of_cameras(void)
{
    LogHelper::setDebugLevel();
    int num = PlatformData::numberOfCameras();
    return (num <= MAX_CAMERAS) ? num : MAX_CAMERAS;
}

static int hal_get_camera_info(int cameraId, struct camera_info *cameraInfo)
{
    HAL_TRACE_CALL(1);
    if (cameraId < 0 || cameraId >= hal_get_number_of_cameras())
        return -ENODEV;

    PlatformData::getCameraInfo(cameraId, cameraInfo);

    return 0;
}

static int hal_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    UNUSED(callbacks);
    return 0;
}

static int hal_dev_open(const hw_module_t* module, const char* name,
                        hw_device_t** device)
{
    int status = -EINVAL;
    int camera_id;

    HAL_TRACE_CALL(1);
    LogHelper::setDebugLevel();
    PERFORMANCE_TRACES_LAUNCH_START();
    LOG1("%s, camera id: %s", __FUNCTION__, name);

    Mutex::Autolock l(cameraHalMutex);

    if (!name) {
        ALOGE("Camera name is NULL");
        return status;
    }

    camera_id = atoi(name);
    if (camera_id < 0 || camera_id >= hal_get_number_of_cameras()) {
        ALOGE("%s: Camera id %d is out of bounds, num. of cameras (%d)",
             __func__, camera_id, hal_get_number_of_cameras());
        return -ENODEV;
    }

    if ((!PlatformData::supportDualVideo()) &&
         Camera3HAL::sInstaceCount > 0 &&
         Camera3HAL::sInstances[camera_id] == NULL) {
        ALOGE("Don't support front/primary open at the same time");
        return -EUSERS;
    }

    status = openCameraHardware(camera_id, module, device);
    if (status != NO_ERROR) {
        return status;
    }

    return 0;
}

static int hal_dev_close(hw_device_t* device)
{
    HAL_TRACE_CALL(1);
    Mutex::Autolock l(cameraHalMutex);

    if (!device)
        return -EINVAL;

    camera3_device_t *camera3_dev = (struct camera3_device *)device;
    Camera3HAL* camera_priv = (Camera3HAL*)(camera3_dev->priv);

    if (camera_priv != NULL) {
        camera_priv->deinit();
        delete camera_priv;
    }
    PERFORMANCE_TRACES_BREAKDOWN_STEP("Close_HAL_Done");
    PERFORMANCE_TRACES_IO_STOP();

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    NAMED_FIELD_INITIALIZER(open) hal_dev_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    NAMED_FIELD_INITIALIZER(common) {
        NAMED_FIELD_INITIALIZER(tag) HARDWARE_MODULE_TAG,
        NAMED_FIELD_INITIALIZER(module_api_version) CAMERA_MODULE_API_VERSION_2_0,
        NAMED_FIELD_INITIALIZER(hal_api_version) 0,
        NAMED_FIELD_INITIALIZER(id) CAMERA_HARDWARE_MODULE_ID,
        NAMED_FIELD_INITIALIZER(name) "Intel Camera3HAL Module",
        NAMED_FIELD_INITIALIZER(author) "Intel",
        NAMED_FIELD_INITIALIZER(methods) &hal_module_methods,
        NAMED_FIELD_INITIALIZER(dso) NULL,
        NAMED_FIELD_INITIALIZER(reserved) {0},
    },
    NAMED_FIELD_INITIALIZER(get_number_of_cameras) hal_get_number_of_cameras,
    NAMED_FIELD_INITIALIZER(get_camera_info) hal_get_camera_info,
    NAMED_FIELD_INITIALIZER(set_callbacks) hal_set_callbacks,
    NAMED_FIELD_INITIALIZER(get_vendor_tag_ops) NULL,
    NAMED_FIELD_INITIALIZER(open_legacy) NULL,
    NAMED_FIELD_INITIALIZER(reserved) {0}
};
