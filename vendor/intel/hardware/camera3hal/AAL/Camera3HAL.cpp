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

#define LOG_TAG "Camera3HAL"

#include "Camera3HAL.h"
#include "ICameraHw.h"
#include "PerformanceTraces.h"

namespace android {
namespace camera2 {
/**
 * Initialization of static members of the Camera3HAL class
 */
Camera3HAL* Camera3HAL::sInstances[MAX_CAMERAS] = {NULL, NULL};
int Camera3HAL::sInstaceCount = 0;

/******************************************************************************
 *  C DEVICE INTERFACE IMPLEMENTATION WRAPPER
 *****************************************************************************/

//Common check before the function call
#define FUNCTION_PREPARED_RETURN \
    if (!dev)\
        return -EINVAL;\
    Camera3HAL* camera_priv = (Camera3HAL*)(dev->priv);

static int
hal_dev_initialize(const struct camera3_device * dev,
                   const camera3_callback_ops_t *callback_ops)
{
    PERFORMANCE_HAL_ATRACE();
    HAL_TRACE_CALL(1);
    // As per interface requirement this call should not take longer than 10ms
    HAL_KPI_TRACE_CALL(1,10000000);
    FUNCTION_PREPARED_RETURN

    return camera_priv->initialize(callback_ops);
}

static int
hal_dev_configure_streams(const struct camera3_device * dev,
                          camera3_stream_configuration_t *stream_list)
{
    PERFORMANCE_HAL_ATRACE();
    HAL_TRACE_CALL(1);
    // As per interface requirement this call should not take longer than 1s
    HAL_KPI_TRACE_CALL(1,1000000000);
    FUNCTION_PREPARED_RETURN

    return camera_priv->configure_streams(stream_list);
}

static const camera_metadata_t*
hal_dev_construct_default_request_settings(const struct camera3_device * dev,
                                           int type)
{
    PERFORMANCE_HAL_ATRACE();
    HAL_TRACE_CALL(1);
    // As per interface requirement this call should not take longer than 5ms
    HAL_KPI_TRACE_CALL(1, 5000000);

    if (!dev)
        return NULL;
    Camera3HAL* camera_priv = (Camera3HAL*)(dev->priv);

    return camera_priv->construct_default_request_settings(type);
}

static int
hal_dev_process_capture_request(const struct camera3_device * dev,
                                camera3_capture_request_t *request)
{
    PERFORMANCE_HAL_ATRACE();
    HAL_TRACE_CALL(2);
    /**
     *  As per interface requirement this call should not take longer than 4
     *  frame intervals. Here we choose that value to be 4 frame intervals at
     *  30fps = 4 x 33.3 ms = 133 ms
     */
    HAL_KPI_TRACE_CALL(2, 133000000);
    FUNCTION_PREPARED_RETURN

    return camera_priv->process_capture_request(request);
}

static void
hal_dev_get_metadata_vendor_tag_ops(const struct camera3_device * dev,
                                    vendor_tag_query_ops_t *ops)
{
    PERFORMANCE_HAL_ATRACE();
    HAL_TRACE_CALL(1);
    if (!dev)
        return;

    Camera3HAL* camera_priv = (Camera3HAL*)(dev->priv);
    camera_priv->get_metadata_vendor_tag_ops(ops);
}

static void
hal_dev_dump(const struct camera3_device * dev, int fd)
{
    HAL_TRACE_CALL(1);
    // As per interface requirement this call should not take longer than 10ms
    HAL_KPI_TRACE_CALL(1, 10000000);
    if (!dev)
        return;

    Camera3HAL* camera_priv = (Camera3HAL*)(dev->priv);

    camera_priv->dump(fd);
}

static int
hal_dev_flush(const struct camera3_device * dev)
{
    HAL_TRACE_CALL(1);
    // As per interface requirement this call should not take longer than 1000ms
    HAL_KPI_TRACE_CALL(1, 1000000000);
    if (!dev)
        return -EINVAL;

    Camera3HAL* camera_priv = (Camera3HAL*)(dev->priv);
    return camera_priv->flush();
}

static camera3_device_ops hal_dev_ops = {
    NAMED_FIELD_INITIALIZER(initialize)                         hal_dev_initialize,
    NAMED_FIELD_INITIALIZER(configure_streams)                  hal_dev_configure_streams,
    NAMED_FIELD_INITIALIZER(register_stream_buffers)            NULL,
    NAMED_FIELD_INITIALIZER(construct_default_request_settings) hal_dev_construct_default_request_settings,
    NAMED_FIELD_INITIALIZER(process_capture_request)            hal_dev_process_capture_request,
    NAMED_FIELD_INITIALIZER(get_metadata_vendor_tag_ops)        hal_dev_get_metadata_vendor_tag_ops,
    NAMED_FIELD_INITIALIZER(dump)                               hal_dev_dump,
    NAMED_FIELD_INITIALIZER(flush)                              hal_dev_flush,
    NAMED_FIELD_INITIALIZER(reserved)                           {0},
};

/******************************************************************************
 *  C++ CLASS IMPLEMENTATION
 *****************************************************************************/
Camera3HAL::Camera3HAL(int cameraId, const hw_module_t* module) :
    mCameraId(cameraId),
    mCameraHw(NULL),
    mRequestThread(NULL)
{
    LOG1("@%s", __FUNCTION__);
    struct camera_info info;
    PlatformData::getCameraInfo(cameraId, &info);

    CLEAR(mDevice);
    mDevice.common.tag = HARDWARE_DEVICE_TAG;
    mDevice.common.version = info.device_version;
    mDevice.common.module = (hw_module_t *)(module);
    // hal_dev_close is kept in the module for symmetry with dev_open
    // it will be set there
    mDevice.common.close = NULL;
    mDevice.ops = &hal_dev_ops;
    mDevice.priv = this;

    sInstaceCount++;
    sInstances[cameraId] = this;
}

Camera3HAL::~Camera3HAL()
{
    sInstaceCount--;
    sInstances[mCameraId] = NULL;
}

status_t Camera3HAL::init(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = UNKNOWN_ERROR;

    mMemorydumper.dumpHeap();

    mCameraHw = ICameraHw::createCameraHW(mCameraId);
    if (mCameraHw == NULL) {
        LOGE("Error create Platform specific layer");
        goto bail;
    }

    status = mCameraHw->init();
    if (status != NO_ERROR) {
        LOGE("Error initializing Camera HW");
        goto bail;
    }

    mRequestThread = new RequestThread(mCameraId, mCameraHw);
    if (mRequestThread == NULL) {
        LOGE("Error create RequestThread");
        goto bail;
    }

    return NO_ERROR;

bail:
    deinit();
    return status;
}

status_t Camera3HAL::deinit(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    status_t status = NO_ERROR;

    // flush request first
    if (mRequestThread != NULL)
        mRequestThread->flush();

    if (mCameraHw != NULL) {
        delete mCameraHw;
        mCameraHw = NULL;
    }

    if (mRequestThread != NULL) {
        mRequestThread->deinit();
        mRequestThread.clear();
    }

    mMemorydumper.dumpHeap();
    return status;
}

/* *********************************************************************
 * Camera3 device  APIs
 * ********************************************************************/
int Camera3HAL::initialize(const camera3_callback_ops_t *callback_ops)
{
    status_t status = NO_ERROR;
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    if (callback_ops == NULL)
        return -ENODEV;

    status = mRequestThread->init(callback_ops);
    if (status != NO_ERROR) {
        LOGE("Error initializing Request Thread status = %d", status);
        return -ENODEV;
    }
    return status;
}

int Camera3HAL::configure_streams(camera3_stream_configuration_t *stream_list)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    if (!stream_list)
        return -EINVAL;

    if (!stream_list->streams || !stream_list->num_streams) {
        LOGE("%s: Bad input! streams list ptr: %p, num %d", __FUNCTION__,
            stream_list->streams, stream_list->num_streams);
        return -EINVAL;
    }
    int num = stream_list->num_streams;
    LOG2("@%s, line:%d, stream num:%d", __FUNCTION__, __LINE__, num);
    while (num--) {
        if (!stream_list->streams[num]){
            LOGE("%s: Bad input! streams (%d) 's ptr: %p", __FUNCTION__,
                num, stream_list->streams[num]);
            return -EINVAL;
        }
    };

    status_t status = mRequestThread->configureStreams(stream_list);
    return (status == NO_ERROR) ? 0 : -EINVAL;
}

camera_metadata_t* Camera3HAL::construct_default_request_settings(int type)
{
    LOG2("@%s, type:%d", __FUNCTION__, type);
    camera_metadata_t * meta;

    status_t status = mRequestThread->constructDefaultRequest(type, &meta);
    if (status != NO_ERROR)
        return NULL;

    return meta;
}

int Camera3HAL::process_capture_request(camera3_capture_request_t *request)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    if (!request || !request->num_output_buffers || !request->output_buffers) {
        LOGE("%s: Bad input!", __FUNCTION__);
        return -EINVAL;
    }

    status_t status = mRequestThread->processCaptureRequest(request);
    return (status == NO_ERROR) ? 0 : \
           (status == BAD_VALUE) ? -EINVAL : \
            -ENODEV;
}

void Camera3HAL::get_metadata_vendor_tag_ops(vendor_tag_query_ops_t *ops)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    UNUSED(ops);
//TODO FIX THIS WHEN WE KNOW BETTER
//    if (ops)
//        *ops = mParamsMgr;
}

void Camera3HAL::dump(int fd)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    String8 message;
    message.appendFormat(LOG_TAG ":@%s\n", __FUNCTION__);
    write(fd, message.string(), message.size());

    if (mRequestThread != NULL)
        mRequestThread->dump(fd);
    if (mCameraHw != NULL)
        mCameraHw->dump(fd);
}

int Camera3HAL::flush()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);

    if (mRequestThread != NULL)
        return mRequestThread->flush();
    return -ENODEV;
}

//----------------------------------------------------------------------------
}; // namespace camera2
}; // namespace android
