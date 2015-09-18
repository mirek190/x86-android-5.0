/*
 * Copyright (C) 2013 The Android Open Source Project
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
#define LOG_TAG "Camera_IPU2HwSensor"


#include <linux/media.h>
#include <fcntl.h>
#include <SchedulingPolicyService.h>
#include "cameranvm.h"
#include "otp_decoder.h"
#include "ia_cmc_parser.h"
#include "ia_nvm.h"
#include "PlatformData.h"
#include "IPU2HwSensor.h"
#include "v4l2device.h"
#include "LogHelper.h"
#include "Camera3GFXFormat.h"
#include "PerformanceTraces.h"
#include "CameraMetadataHelper.h"

namespace android {
namespace camera2 {

static sensorPrivateData gSensorDataCache[MAX_CAMERAS];

IPU2HwSensor::IPU2HwSensor(int cameraId) :
    mSensorType(SENSOR_TYPE_NONE),
    mCameraId(cameraId),
    mRawBayerFormat(-1),
    mInitialModeDataValid(false),
    mPerFrameSettingsMode(false),
    mSyncOwner(NULL),
    mExpectedExpNum(INVALID_REQ_ID),
    mReceivedSOF(INVALID_EXP_ID),
    mRequestHead(INVALID_REQ_ID),
    mRequestCaptured(INVALID_REQ_ID),
    mExposureHistory(NULL),
    mMessageQueue("Camera_IPU2HwSensor", (int)MESSAGE_ID_MAX),
    mThreadRunning(false),
    mFrameDuration(33333333),
    mLastFps(0)
{
    LOG1("@%s", __FUNCTION__);
    mCapInfo = getIPU2CameraCapInfo(cameraId);
    mPendingRequestsQueue.clear();
    mAutoExposure.clear();
    CLEAR(mInitialModeData);
    CLEAR(mCameraInput);
    CLEAR(mFrameList);
    CLEAR(mExpOfRequests);

    // Get ae compensation step from the static metadata.
    mStepEV = PlatformData::getStepEv(mCameraId);
    mMessageThead = new MessageThread(this, "IPU2HwSensor", PRIORITY_CAMERA);
    if (mMessageThead == NULL) {
        LOGE("Error create IPU2HwSensor");
        return ;
    }

    pid_t pid = getpid();
    pid_t tid = mMessageThead->getTid();

    int err = requestPriority(pid, tid, kCameraPriority);
    if (err != 0) {
        LOGW("Policy SCHED_FIFO priority %d is unavailable for pid %d tid %d error %d",
             kCameraPriority , pid, tid, err);
    }
}

IPU2HwSensor::~IPU2HwSensor()
{
    LOG1("@%s", __FUNCTION__);

    mMessageQueue.remove(MESSAGE_ID_SOF);
    if (mMessageThead != NULL) {
        Message msg;
        msg.id = MESSAGE_ID_EXIT;
        mMessageQueue.send(&msg);
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }

    if (mSensorSubdevice != NULL) {
        if (mSensorSubdevice->isOpen())
            mSensorSubdevice->close();

        mSensorSubdevice.clear();
    }

    if (mDevice != NULL)
        mDevice.clear();

    mPendingRequestsQueue.clear();
    if (mExposureHistory) {
        delete mExposureHistory;
        mExposureHistory = NULL;
    }
}

status_t IPU2HwSensor::initSubDevice()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    // Open subdevice for direct IOCTL.
    status = openSubdevices();
    if (status == NO_ERROR)
        status = applySensorFlip();
    // Sensor is configured, readout the initial mode info
    updateModeData();
    mPerFrameSettingsMode = false;

    return status;
}

status_t IPU2HwSensor::openSubdevices()
{
    LOG1("@%s", __FUNCTION__);
    struct media_device_info mediaDeviceInfo;
    struct media_entity_desc mediaEntityDesc;
    status_t status = NO_ERROR;
    int ret = 0;

    sp<V4L2DeviceBase> mediaCtl = new V4L2DeviceBase("/dev/media0");
    status = mediaCtl->open();
    if (status != NO_ERROR) {
        LOGE("Failed to open media device");
        return status;
    }

    CLEAR(mediaDeviceInfo);
    ret = pxioctl(mediaCtl, MEDIA_IOC_DEVICE_INFO, &mediaDeviceInfo);
    if (ret < 0) {
        LOGE("Failed to get media device information");
        status = UNKNOWN_ERROR;
        goto exit;
    }

    LOG1("Media device : %s", mediaDeviceInfo.driver);
    status = findMediaEntityByName(mediaCtl, mCameraInput.name, mediaEntityDesc);
    if (status != NO_ERROR) {
        LOGE("Failed to find sensor subdevice");
        goto exit;
    }

    status = openSubdevice(mSensorSubdevice, mediaEntityDesc.v4l.major, mediaEntityDesc.v4l.minor);
    if (status != NO_ERROR) {
        LOGE("Failed to open sensor subdevice");
        goto exit;
    }

exit:
    mediaCtl->close();
    mediaCtl.clear();

    return status;
}

/**
 * Open device node based on device identifier
 *
 * Helper method to find the device node name for V4L2 subdevices
 * from sysfs.
 */
status_t IPU2HwSensor::openSubdevice(sp<V4L2DeviceBase> &subdev, int major, int minor)
{
    LOG1("@%s :  major %d, minor %d", __FUNCTION__, major, minor);
    status_t status = UNKNOWN_ERROR;
    int ret = 0;
    char sysname[1024];
    char devname[1024];
    sprintf(devname, "/sys/dev/char/%u:%u", major, minor);
    ret = readlink(devname, sysname, sizeof(sysname));
    if (ret < 0) {
        LOGE("Unable to find subdevice node");
    } else {
        sysname[ret] = 0;
        char *lastSlash = strrchr(sysname, '/');
        if (lastSlash == NULL) {
            LOGE("Invalid sysfs subdev path");
            return status;
        }
        sprintf(devname, "/dev/%s", lastSlash + 1);
        LOG1("Subdevide node : %s", devname);
        subdev.clear();
        subdev = new V4L2DeviceBase(devname);
        status = subdev->open();
        if (status != NO_ERROR) {
            LOGE("Failed to open subdevice");
            subdev.clear();
        }
    }
    return status;
}

/**
 * Find description for given entity name
 *
 * Using media controller temporarily here to query entity with given name.
 */
status_t IPU2HwSensor::findMediaEntityByName(sp<V4L2DeviceBase> &mediaCtl, char const* entityName,
        struct media_entity_desc &mediaEntityDesc)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = UNKNOWN_ERROR;
    for (int i = 0; ; i++) {
        status = findMediaEntityById(mediaCtl, i | MEDIA_ENT_ID_FLAG_NEXT, mediaEntityDesc);
        if (status != NO_ERROR)
            break;
        LOG2("Media entity %d : %s", i, mediaEntityDesc.name);
        if (strncmp(mediaEntityDesc.name, entityName, MAX_SENSOR_NAME_LENGTH) == 0)
            break;
    }

    return status;
}

/**
 * Find description for given entity index
 *
 * Using media controller temporarily here to query entity with given name.
 */
status_t IPU2HwSensor::findMediaEntityById(sp<V4L2DeviceBase> &mediaCtl, int index,
        struct media_entity_desc &mediaEntityDesc)
{
    LOG1("@%s", __FUNCTION__);
    int ret = 0;
    CLEAR(mediaEntityDesc);
    mediaEntityDesc.id = index;
    ret = pxioctl(mediaCtl, MEDIA_IOC_ENUM_ENTITIES, &mediaEntityDesc);
    if (ret < 0) {
        LOG1("No more media entities");
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

int IPU2HwSensor::getCurrentCameraId(void)
{
    LOG1("@%s, id: %d", __FUNCTION__, mCameraId);
    return mCameraId;
}

size_t IPU2HwSensor::enumerateInputs(Vector<struct cameraInfo> &inputs)
{
    LOG1("@%s", __FUNCTION__);
    status_t ret;
    size_t numCameras = 0;
    struct v4l2_input input;
    struct cameraInfo sCamInfo;

    for (int i = 0; i < PlatformData::numberOfCameras(); i++) {
        CLEAR(input);
        CLEAR(sCamInfo);
        input.index = i;
        ret = mDevice->enumerateInputs(&input);
        if (ret != NO_ERROR) {
            if (ret == INVALID_OPERATION || ret == BAD_INDEX)
                break;
            LOGE("Device input enumeration failed for sensor input %d", i);
        } else {
            sCamInfo.index = i;
            strncpy(sCamInfo.name, (const char *)input.name, sizeof(sCamInfo.name)-1);
            LOG1("Detected sensor \"%s\"", sCamInfo.name);
        }
        inputs.push(sCamInfo);
        numCameras++;
    }
    return numCameras;
}

status_t IPU2HwSensor::selectActiveSensor(sp<V4L2VideoNode> &device)
{
    LOG1("@%s", __FUNCTION__);
    mDevice = device;
    status_t ret = NO_ERROR;
    Vector<struct cameraInfo> camInfo;
    size_t numCameras = enumerateInputs(camInfo);

    if (numCameras < (size_t) PlatformData::numberOfCameras()) {
        LOGE("Number of detected sensors not matching static Platform data!");
    }

    if (numCameras < 1) {
        LOGE("No detected sensors!");
        return UNKNOWN_ERROR;
    }

    // Static mapping of v4l2_input.index to android camera id
    if (numCameras == 1) {
        mCameraInput = camInfo[0];
    } else if (PlatformData::facing(mCameraId) == CAMERA_FACING_BACK) {
        mCameraInput = camInfo[0];
    } else if (PlatformData::facing(mCameraId) == CAMERA_FACING_FRONT) {
        mCameraInput = camInfo[1];
    }

    // Choose the camera sensor
    LOG1("Selecting camera sensor: %s", mCameraInput.name);
    ret = mDevice->setInput(mCameraInput.index);
    if (ret != NO_ERROR) {
        ret = UNKNOWN_ERROR;
    } else {
        // Query now the supported pixel formats
        Vector<v4l2_fmtdesc> formats;
        ret = mDevice->queryCapturePixelFormats(formats);
        if (ret != NO_ERROR) {
            LOGW("Could not query capture formats from sensor: %s", mCameraInput.name);
            ret = NO_ERROR;   // This is not critical
        }
        sensorStoreRawFormat(formats);
    }

    mExposureHistory = new AtomFifo <struct atomisp_exposure>
                        (MAX_EXPOSURE_HISTORY_SIZE + mCapInfo->exposureLag());
    PERFORMANCE_TRACES_BREAKDOWN_STEP("capture_s_input");
    return ret;
}

/**
 * Helper method for the sensor to select the prefered BAYER format
 * the supported pixel formats are retrieved when the sensor is selected.
 *
 * This helper method finds the first Bayer format and saves it to mRawBayerFormat
 * so that if raw dump feature is enabled we know what is the sensor
 * preferred format.
 *
 * TODO: sanity check, who needs a preferred format
 */
status_t IPU2HwSensor::sensorStoreRawFormat(Vector<v4l2_fmtdesc> &formats)
{
    LOG1("@%s", __FUNCTION__);
    Vector<v4l2_fmtdesc>::iterator it = formats.begin();

    for (;it != formats.end(); ++it) {
        /* store it only if is one of the Bayer formats */
        if (isBayerFormat(it->pixelformat)) {
            mRawBayerFormat = it->pixelformat;
            break;  //we take the first one, sensors tend to support only one
        }
    }
    return NO_ERROR;
}

void IPU2HwSensor::getMotorData(sensorPrivateData *sensor_data)
{
    LOG2("@%s", __FUNCTION__);
    int rc;
    struct v4l2_private_int_data motorPrivateData;

    motorPrivateData.size = 0;
    motorPrivateData.data = NULL;
    motorPrivateData.reserved[0] = 0;
    motorPrivateData.reserved[1] = 0;

    sensor_data->data = NULL;
    sensor_data->size = 0;
    // First call with size = 0 will return motor private data size.
    rc = pxioctl(mDevice, ATOMISP_IOC_G_MOTOR_PRIV_INT_DATA, &motorPrivateData);
    LOG2("%s IOCTL ATOMISP_IOC_G_MOTOR_PRIV_INT_DATA to get motor private data size ret: %d\n", __FUNCTION__, rc);
    if (rc != 0 || motorPrivateData.size == 0) {
        LOGD("Failed to get motor private data size. Error: %d", rc);
        return;
    }

    motorPrivateData.data = malloc(motorPrivateData.size);
    if (motorPrivateData.data == NULL) {
        LOGD("Failed to allocate memory for motor private data.");
        return;
    }

    // Second call with correct size will return motor private data.
    rc = pxioctl(mDevice, ATOMISP_IOC_G_MOTOR_PRIV_INT_DATA, &motorPrivateData);
    LOG2("%s IOCTL ATOMISP_IOC_G_MOTOR_PRIV_INT_DATA to get motor private data ret: %d\n", __FUNCTION__, rc);

    if (rc != 0 || motorPrivateData.size == 0) {
        LOGD("Failed to read motor private data. Error: %d", rc);
        free(motorPrivateData.data);
        return;
    }

    sensor_data->data = motorPrivateData.data;
    sensor_data->size = motorPrivateData.size;
    sensor_data->fetched = true;
}

void IPU2HwSensor::getSensorData(sensorPrivateData *sensor_data)
{
    LOG2("@%s", __FUNCTION__);
    int rc;
    struct v4l2_private_int_data nvmData;
    int cameraId = getCurrentCameraId();

    sensor_data->data = NULL;
    sensor_data->size = 0;

    nvmData.size = 0;
    nvmData.data = NULL;
    nvmData.reserved[0] = 0;
    nvmData.reserved[1] = 0;

    if (gSensorDataCache[cameraId].fetched) {
        sensor_data->data = gSensorDataCache[cameraId].data;
        sensor_data->size = gSensorDataCache[cameraId].size;
        return;
    }
    // First call with size = 0 will return OTP data size.
    rc = pxioctl(mDevice, ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA, &nvmData);
    LOG2("%s IOCTL ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA to get OTP data size ret: %d\n", __FUNCTION__, rc);
    if (rc != 0 || nvmData.size == 0) {
        LOGD("Failed to get OTP size. Error: %d", rc);
        return;
    }

    nvmData.data = calloc(nvmData.size, 1);
    if (nvmData.data == NULL) {
        LOGD("Failed to allocate memory for OTP data.");
        return;
    }

    // Second call with correct size will return OTP data.
    rc = pxioctl(mDevice, ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA, &nvmData);
    LOG2("%s IOCTL ATOMISP_IOC_G_SENSOR_PRIV_INT_DATA to get OTP data ret: %d\n", __FUNCTION__, rc);

    if (rc != 0 || nvmData.size == 0) {
        LOGD("Failed to read OTP data. Error: %d", rc);
        free(nvmData.data);
        return;
    }

    sensor_data->data = nvmData.data;
    sensor_data->size = nvmData.size;
    sensor_data->fetched = true;

    gSensorDataCache[cameraId] = *sensor_data;
}

void IPU2HwSensor::generateNVM(ia_binary_data **output_nvm_data)
{
    LOG2("@%s", __FUNCTION__);

    ia_binary_data sensorData, motorData;
    getSensorData((sensorPrivateData *) &sensorData);
    getMotorData((sensorPrivateData *)&motorData);
    Lsc_table tables[LSC_TABLE_NUM];
    // Check format of the opt data
    if(strcmp(mCapInfo->NVMFormat(), "google") == 0) {
        int ret = 0;
        uint8_t LSC_width = 0;
        uint8_t LSC_height = 0;
        int color_order = 0;
        // Get the lsc grid width/height, and bayer order from CMC
        if (PlatformData::AiqConfig) {
            ia_cmc_t *cmc = PlatformData::AiqConfig.getCMCHandler();
            if (cmc && cmc->cmc_parsed_lens_shading.cmc_lens_shading) {
                LSC_width = cmc->cmc_parsed_lens_shading.cmc_lens_shading->grid_width;
                LSC_height = cmc->cmc_parsed_lens_shading.cmc_lens_shading->grid_height;
                LOG2("LSC width:%d height:%d", LSC_width, LSC_height);
            }

            if (cmc && cmc->cmc_general_data) {
                color_order = cmc->cmc_general_data->color_order;
            }
        }

        for(int i = 0; i < LSC_TABLE_NUM; i++) {
            for(int j = 0; j < IA_NVM_NUM_CHANNELS; j++) {
                tables[i].lsc_tables[j] = new uint16_t[LSC_width * LSC_height];
                if (tables[i].lsc_tables[j] == NULL)
                    goto errorFree;
            }
        }

        NVM_input compressed_nvm_input;
        compressed_nvm_input.nvm_data = (uint8_t*)sensorData.data;
        compressed_nvm_input.len = sensorData.size;
        compressed_nvm_input.width = LSC_width;
        compressed_nvm_input.height = LSC_height;
        compressed_nvm_input.table_num = LSC_TABLE_NUM;
        switch (color_order)
        {
        case ia_aiq_bayer_order_grbg:
            compressed_nvm_input.bayer_order = GRBG;
            break;
        case ia_aiq_bayer_order_rggb:
            compressed_nvm_input.bayer_order = RGGB;
            break;
        case ia_aiq_bayer_order_bggr:
            compressed_nvm_input.bayer_order = BGGR;
            break;
        case ia_aiq_bayer_order_gbrg:
            compressed_nvm_input.bayer_order = GBRG;
            break;
        default:
            LOGW("wrong color order in CPF");
            return;
        }
        //Decompress the NVM data
        ret = DecoderNVMDataTable(compressed_nvm_input, tables);
        if (ret >= 0) {
            nvm_data input_nvm_data;
            input_nvm_data.lsc_width = LSC_width;
            input_nvm_data.lsc_height = LSC_height;
            for(int i = 0; i < LSC_TABLE_NUM; i++) {
                input_nvm_data.lsc[i] = (nvm_lsc*) &tables[i];
            }
            // convert the decompressed lsc to the data in AIQ format
            if (cameranvm_convert(&input_nvm_data, output_nvm_data) != nvm_error_none)
                LOGW("convert nvm data failed");
        } else {
            LOGW("decompress nvm data failed");
        }

        for(int i = 0; i < LSC_TABLE_NUM; i++) {
            for(int j = 0; j < IA_NVM_NUM_CHANNELS; j++) {
                delete [] tables[i].lsc_tables[j];
                tables[i].lsc_tables[j] = NULL;
            }
        }
    } else {
        // NVM data is Intel format
        String8 sensorName, spIdName;
        int spacePos;
        // Combine sensor name and spId
        PlatformData::createVendorPlatformProductName(spIdName);
        sensorName = getSensorName();
        spacePos = sensorName.find(" ");
        if (spacePos >= 0){
            sensorName.setTo(sensorName, spacePos);
        }

        sensorName += "-";
        sensorName += spIdName;
        LOG1("Sensor-vendor-platform-product name: %s", sensorName.string());
        cameranvm_create(sensorName, &sensorData, &motorData, output_nvm_data);
    }
    return;

errorFree:
    for(int i = 0; i < LSC_TABLE_NUM; i++) {
        for(int j = 0; j < IA_NVM_NUM_CHANNELS; j++) {
            delete [] tables[i].lsc_tables[j];
            tables[i].lsc_tables[j] = NULL;
        }
    }
}

void IPU2HwSensor::releaseNVM(ia_binary_data *nvm_data)
{
    LOG2("@%s", __FUNCTION__);
    cameranvm_delete(nvm_data);
}

int IPU2HwSensor::setExposureMode(v4l2_exposure_auto_type v4l2Mode)
{
    LOG2("@%s: %d", __FUNCTION__, v4l2Mode);
    return mDevice->setControl(V4L2_CID_EXPOSURE_AUTO, v4l2Mode, "AE mode");
}

int IPU2HwSensor::getExposureMode(v4l2_exposure_auto_type * type)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_EXPOSURE_AUTO, (int*)type);
}

int IPU2HwSensor::setExposureBias(int bias)
{
    LOG2("@%s: bias: %d", __FUNCTION__, bias);
    return mDevice->setControl(V4L2_CID_EXPOSURE, bias, "exposure");
}

int IPU2HwSensor::getExposureBias(int * bias)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_EXPOSURE, bias);
}

int IPU2HwSensor::setSceneMode(v4l2_scene_mode mode)
{
    LOG2("@%s: %d", __FUNCTION__, mode);
    return mDevice->setControl(V4L2_CID_SCENE_MODE, mode, "scene mode");
}

int IPU2HwSensor::getSceneMode(v4l2_scene_mode * mode)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_SCENE_MODE, (int*)mode);
}

int IPU2HwSensor::setWhiteBalance(v4l2_auto_n_preset_white_balance mode)
{
    LOG2("@%s: %d", __FUNCTION__, mode);
    return mDevice->setControl(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, mode, "white balance");
}

int IPU2HwSensor::getWhiteBalance(v4l2_auto_n_preset_white_balance * mode)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, (int*)mode);
}

int IPU2HwSensor::setIso(int iso)
{
    LOG2("@%s: ISO: %d", __FUNCTION__, iso);
    return mDevice->setControl(V4L2_CID_ISO_SENSITIVITY, iso, "iso");
}

int IPU2HwSensor::getIso(int * iso)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_ISO_SENSITIVITY, iso);
}

int IPU2HwSensor::setAeMeteringMode(v4l2_exposure_metering mode)
{
    LOG2("@%s: %d", __FUNCTION__, mode);
    return mDevice->setControl(V4L2_CID_EXPOSURE_METERING, mode, "AE metering mode");
}

int IPU2HwSensor::getAeMeteringMode(v4l2_exposure_metering * mode)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_EXPOSURE_METERING, (int*)mode);
}

int IPU2HwSensor::setAeFlickerMode(v4l2_power_line_frequency mode)
{
    LOG2("@%s: %d", __FUNCTION__, (int) mode);
    return mDevice->setControl(V4L2_CID_POWER_LINE_FREQUENCY,
                                    mode, "light frequency");
}

int IPU2HwSensor::setAfMode(v4l2_auto_focus_range mode)
{
    LOG2("@%s: %d", __FUNCTION__, mode);
    return mDevice->setControl(V4L2_CID_AUTO_FOCUS_RANGE , mode, "AF mode");
}

int IPU2HwSensor::getAfMode(v4l2_auto_focus_range * mode)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_AUTO_FOCUS_RANGE, (int*)mode);
}

int IPU2HwSensor::setAfEnabled(bool enable)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->setControl(V4L2_CID_FOCUS_AUTO, enable, "Auto Focus");
}

int IPU2HwSensor::set3ALock(int aaaLock)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->setControl(V4L2_CID_3A_LOCK, aaaLock, "AE Lock");
}

int IPU2HwSensor::get3ALock(int * aaaLock)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_3A_LOCK, aaaLock);
}


int IPU2HwSensor::setAeFlashMode(v4l2_flash_led_mode mode)
{
    LOG2("@%s: %d", __FUNCTION__, mode);
    return mDevice->setControl(V4L2_CID_FLASH_LED_MODE, mode, "Flash mode");
}

int IPU2HwSensor::getAeFlashMode(v4l2_flash_led_mode * mode)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_FLASH_LED_MODE, (int*)mode);
}

int IPU2HwSensor::getModeInfo(struct atomisp_sensor_mode_data *mode_data)
{
    LOG2("@%s", __FUNCTION__);
    int ret;
    ret = pxioctl(mDevice, ATOMISP_IOC_G_SENSOR_MODE_DATA, mode_data);
    LOG2("%s IOCTL ATOMISP_IOC_G_SENSOR_MODE_DATA ret: %d\n", __FUNCTION__, ret);
    return ret;
}

int IPU2HwSensor::getFrameParameters(ia_aiq_frame_params *frame_params,
                                    ia_aiq_exposure_sensor_descriptor *sensor_descriptor,
                                    unsigned int output_width, unsigned int output_height)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(output_width);
    UNUSED(output_height);
    int ret;
    atomisp_sensor_mode_data sensor_mode_data;
    CLEAR(sensor_mode_data);

    ret = getModeInfo(&sensor_mode_data);
    LOG2("%s IOCTL ATOMISP_IOC_G_SENSOR_MODE_DATA ret: %d\n", __FUNCTION__, ret);

    if (ret < 0) {
        sensor_mode_data.crop_horizontal_start = 0;
        sensor_mode_data.crop_vertical_start = 0;
        sensor_mode_data.crop_vertical_end = 0;
        sensor_mode_data.crop_horizontal_end = 0;
    } else {
        frame_params->horizontal_crop_offset = sensor_mode_data.crop_horizontal_start;
        frame_params->vertical_crop_offset = sensor_mode_data.crop_vertical_start;
        // The +1 needed as the *_end and *_start values are index values.
        frame_params->cropped_image_height = sensor_mode_data.crop_vertical_end -
                                             sensor_mode_data.crop_vertical_start + 1;
        frame_params->cropped_image_width = sensor_mode_data.crop_horizontal_end -
                                            sensor_mode_data.crop_horizontal_start +1;

        LOG2("horizontal_crop_offset %d", frame_params->horizontal_crop_offset);
        LOG2("vertical_crop_offset %d", frame_params->vertical_crop_offset);
        LOG2("cropped_image_height %d", frame_params->cropped_image_height);
        LOG2("cropped_image_width %d", frame_params->cropped_image_width);

        /* TODO: Get scaling factors from sensor configuration parameters */
        frame_params->horizontal_scaling_denominator = 254;
        frame_params->vertical_scaling_denominator = 254;

        if ((frame_params->cropped_image_width == 0) || (frame_params->cropped_image_height == 0)) {
        // the driver gives incorrect values for the frame width or height
            frame_params->horizontal_scaling_numerator = 0;
            frame_params->vertical_scaling_numerator = 0;
            LOGE("Invalid sensor frame params. Cropped image width: %d, cropped image height: %d",
                  frame_params->cropped_image_width, frame_params->cropped_image_height);
            LOGE("This causes lens shading table not to be used.");
        } else {
            frame_params->horizontal_scaling_numerator =
                    sensor_mode_data.output_width * 254 *
                    sensor_mode_data.binning_factor_x/frame_params->cropped_image_width;
            frame_params->vertical_scaling_numerator =
                    sensor_mode_data.output_height * 254 *
                    sensor_mode_data.binning_factor_y/frame_params->cropped_image_height;
        }

        sensor_descriptor->pixel_clock_freq_mhz = sensor_mode_data.vt_pix_clk_freq_mhz/1000000.0f;
        sensor_descriptor->pixel_periods_per_line = sensor_mode_data.line_length_pck;
        sensor_descriptor->line_periods_per_field = sensor_mode_data.frame_length_lines;
        sensor_descriptor->line_periods_vertical_blanking = sensor_mode_data.frame_length_lines
                - (sensor_mode_data.crop_vertical_end - sensor_mode_data.crop_vertical_start + 1)
                / sensor_mode_data.binning_factor_y;
        sensor_descriptor->fine_integration_time_min =
                sensor_mode_data.fine_integration_time_def;
        sensor_descriptor->fine_integration_time_max_margin =
                sensor_mode_data.line_length_pck - sensor_mode_data.fine_integration_time_def;
        sensor_descriptor->coarse_integration_time_min =
                sensor_mode_data.coarse_integration_time_min;
        sensor_descriptor->coarse_integration_time_max_margin =
                sensor_mode_data.coarse_integration_time_max_margin;

        LOG2("pixel_clock_freq_mhz %f", sensor_descriptor->pixel_clock_freq_mhz);
        LOG2("pixel_periods_per_line %d", sensor_descriptor->pixel_periods_per_line);
        LOG2("line_periods_per_field %d", sensor_descriptor->line_periods_per_field);
        LOG2("coarse_integration_time_min %d", sensor_descriptor->coarse_integration_time_min);
        LOG2("coarse_integration_time_max_margin %d",
              sensor_descriptor->coarse_integration_time_max_margin);
        LOG2("sensor_descriptor->line_periods_vertical_blanking %d", sensor_descriptor->line_periods_vertical_blanking);
        LOG2("sensor descriptor: binning_factor_y %d", sensor_mode_data.binning_factor_y);
    }
    return ret;
}

int IPU2HwSensor::setExposure(unsigned short coarse_integration_time,
                             unsigned short fine_integration_time,
                             unsigned short analog_gain_global,
                             uint16_t digital_gain_global)
{
// Should set exposure in SOF event when mPerFrameSettingsMode is true  785
    if (mPerFrameSettingsMode)
        return 0;

    int ret;
    struct atomisp_exposure exposure;
    exposure.integration_time[0] = coarse_integration_time;
    exposure.integration_time[1] = fine_integration_time;
    exposure.gain[0] = analog_gain_global;
    exposure.gain[1] = digital_gain_global;

    ret = pxioctl(mDevice, ATOMISP_IOC_S_EXPOSURE, &exposure);
    LOG2("%s IOCTL ATOMISP_IOC_S_EXPOSURE ret: %d, gain %d, citg %d\n", __FUNCTION__, ret, analog_gain_global, coarse_integration_time);
    return ret;
}

void IPU2HwSensor::clearRequestQueue()
{
    LOG2("@%s", __FUNCTION__);
    Mutex::Autolock lock(mExposureLock);
    mPendingRequestsQueue.clear();
    return;
}

int IPU2HwSensor::storeRequestExposure(int request_id,
                                           bool manualExposure,
                                           struct atomisp_exposure *exposure)
{
    LOG2("@%s: req_id = %d", __FUNCTION__, request_id);
    struct requestExposure newRequestExposure;
    CLEAR(newRequestExposure);

    newRequestExposure.requestId = request_id;
    newRequestExposure.manualExposure = manualExposure;
    if (manualExposure)
        newRequestExposure.exposure = *exposure;

    Mutex::Autolock lock(mExposureLock);
    mPendingRequestsQueue.push_back(newRequestExposure);
    if (manualExposure)
        mPerFrameSettingsMode = true;

    return NO_ERROR;
}

int IPU2HwSensor::storeAutoExposure(bool clearOldValues,
                             unsigned short coarse_integration_time,
                             unsigned short fine_integration_time,
                             unsigned short analog_gain_global,
                             uint16_t digital_gain_global)
{
    LOG2("@%s clear? %d, {coarse=%d, analog_gain=%d, digital_gain=%d}",
        __FUNCTION__, clearOldValues,
        coarse_integration_time, analog_gain_global, digital_gain_global);
    Mutex::Autolock lock(mExposureLock);
    if (clearOldValues)
        mAutoExposure.clear();

    struct atomisp_exposure newAutoExposure;
    CLEAR(newAutoExposure);

    newAutoExposure.integration_time[0] = coarse_integration_time;
    newAutoExposure.integration_time[1] = fine_integration_time;
    newAutoExposure.gain[0] = analog_gain_global;
    newAutoExposure.gain[1] = digital_gain_global;

    mAutoExposure.push_back(newAutoExposure);

    return NO_ERROR;
}

int IPU2HwSensor::applyExposure(unsigned int exp_id)
{
    LOG2("@%s: exp_id=%d", __FUNCTION__, exp_id);
    int ret;
    int requestId = -1;
    struct atomisp_exposure exposure;

    {
    Mutex::Autolock lock(mExposureLock);
    exposure = mAutoExposure[0];
    if (mPendingRequestsQueue.isEmpty()) {
        LOG2("@%s: mPendingRequestsQueue is empty", __FUNCTION__);
        // Keep using mAutoExposure;
    } else if (exp_id == 0) {
        // We just get the oldest request exposure if any.
        requestId = mPendingRequestsQueue.itemAt(0).requestId;
        if (mPendingRequestsQueue.itemAt(0).manualExposure)
            exposure = mPendingRequestsQueue.itemAt(0).exposure;
        mPendingRequestsQueue.removeAt(0);
    } else {
        // Get the request id
        requestId = getExpectedReqId(exp_id + (int)getExposureDelay());
        if (requestId < 0) {
            LOG2("%s: not a valid request, just set the auto settings", __FUNCTION__);
        } else {
            while (mPendingRequestsQueue.size() > 0) {
                if (mPendingRequestsQueue.itemAt(0).requestId == requestId) {
                    LOG2("%s: Apply setting for request %d.", __FUNCTION__, requestId);
                    if (mPendingRequestsQueue.itemAt(0).manualExposure)
                        exposure = mPendingRequestsQueue.itemAt(0).exposure;
                    mPendingRequestsQueue.removeAt(0);
                    break;
                } else if (mPendingRequestsQueue.itemAt(0).requestId < requestId) {
                    LOG2("%s: Outdated request %d in the queue, remove it.",
                        __FUNCTION__, mPendingRequestsQueue.itemAt(0).requestId);
                    mPendingRequestsQueue.removeAt(0);
                } else {
                    LOG2("%s: Request %d not found.", __FUNCTION__, requestId);
                    break;
                }
            }
        }
    }

    if (mAutoExposure.size() > 1)
        mAutoExposure.removeAt(0);
    }
    ret = pxioctl(mDevice, ATOMISP_IOC_S_EXPOSURE, &exposure);
    LOG2("Set sensor exposure params exp_id %d, {coarse=%d, analog_gain=%d, digital_gain=%d}, ret=%d",
                    exp_id, exposure.integration_time[0],
                    exposure.gain[0], exposure.gain[1], ret);
    LOG2("%s: [req %d, exp_id %d(exp)/%d(apply)] Apply exposure: {coarse=%d, analog_gain=%d, digital_gain=%d}",
                __FUNCTION__, requestId, exp_id+getExposureDelay(), exp_id,
                exposure.integration_time[0],
                exposure.gain[0], exposure.gain[1]);

    produceExposureHistory(exposure);

    return requestId;
}

int IPU2HwSensor::setExposureTime(int time)
{
    LOG2("@%s", __FUNCTION__);

    return mDevice->setControl(V4L2_CID_EXPOSURE_ABSOLUTE, time, "Exposure time");
}

int IPU2HwSensor::getExposureTime(int *time)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_EXPOSURE_ABSOLUTE, time);
}

int IPU2HwSensor::getAperture(int *aperture)
{
    LOG2("@%s", __FUNCTION__);
    return mDevice->getControl(V4L2_CID_IRIS_ABSOLUTE, aperture);
}

int IPU2HwSensor::getFNumber(unsigned short *fnum_num, unsigned short *fnum_denom)
{
    LOG2("@%s", __FUNCTION__);
    int fnum = 0, ret;

    ret = mDevice->getControl(V4L2_CID_FNUMBER_ABSOLUTE, &fnum);

    *fnum_num = (unsigned short)(fnum >> 16);
    *fnum_denom = (unsigned short)(fnum & 0xFFFF);
    return ret;
}

/**
 * returns the V4L2 Bayer format preferred by the sensor
 */
int IPU2HwSensor::getRawFormat()
{
    return mRawBayerFormat;
}

const char * IPU2HwSensor::getSensorName(void)
{
    return mCameraInput.name;
}

void IPU2HwSensor::updateModeData()
{
    Mutex::Autolock l(mModeDataLock);
    int ret = getModeInfo(&mInitialModeData);
    if (ret != 0)
        LOGW("Reading initial sensor mode info failed!");

    if (mInitialModeData.frame_length_lines != 0 &&
        mInitialModeData.binning_factor_y != 0 &&
        mInitialModeData.vt_pix_clk_freq_mhz != 0) {
        mInitialModeDataValid = true;
    }
    LOG2("mInitialModeData.frame_length_lines %d", mInitialModeData.frame_length_lines);
    LOG2("mInitialModeData.binning_factor_y %d", mInitialModeData.binning_factor_y);
    LOG2("mInitialModeData.vt_pix_clk_freq_mhz %d", mInitialModeData.vt_pix_clk_freq_mhz);
}

status_t IPU2HwSensor::applySensorFlip()
{
    int sensorFlip = mCapInfo->sensorFlipping();
    LOG1("@%s, sensorFlip = %d", __FUNCTION__, sensorFlip);

    if (sensorFlip == SENSOR_FLIP_NA
        || sensorFlip == SENSOR_FLIP_OFF)
        return NO_ERROR;

    if (mSensorSubdevice->setControl(V4L2_CID_VFLIP,
        (sensorFlip & SENSOR_FLIP_V) ? 1 : 0,
        "vertical image flip")) {
        LOGE("Failed to set vertical image flip to sensor subdevice");
        return UNKNOWN_ERROR;
    }

    if (mSensorSubdevice->setControl(V4L2_CID_HFLIP,
        (sensorFlip & SENSOR_FLIP_H) ? 1 : 0,
        "horizontal image flip")) {
        LOGE("Failed to set horizontal image flip to sensor subdevice");
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

float IPU2HwSensor::getFrameDuration()
{
     LOG1("@%s,mFrameDuration:%f", __FUNCTION__,mFrameDuration);
     return mFrameDuration;
}

float IPU2HwSensor::getFrameRate()
{
    LOG1("@%s", __FUNCTION__);
    int ret = 0;
    float fps = 24.0;

    // update module data
    updateModeData();

    // Then subdev G_FRAME_INTERVAL
    if (mSensorSubdevice != NULL) {
        struct v4l2_subdev_frame_interval subdevFrameInterval;
        CLEAR(subdevFrameInterval);
        subdevFrameInterval.pad = 0;
        ret = pxioctl(mSensorSubdevice, VIDIOC_SUBDEV_G_FRAME_INTERVAL, &subdevFrameInterval);
        if (ret >= 0 && subdevFrameInterval.interval.numerator != 0) {
            LOG1("Using framerate from sensor subdevice");
            fps = ((float) subdevFrameInterval.interval.denominator) / subdevFrameInterval.interval.numerator;
            if (fps)
                mFrameDuration = 1000000000 / fps;
            return fps;
        }
    }

    // Try initial mode data first
    {
    Mutex::Autolock l(mModeDataLock);
    if (mInitialModeDataValid) {
        LOG1("Using framerate from mode data");
        fps = ((float) mInitialModeData.vt_pix_clk_freq_mhz) /
               ( mInitialModeData.line_length_pck * mInitialModeData.frame_length_lines);
        if (fps)
            mFrameDuration = 1000000000 / fps;
        return fps;
    }
    }

    return fps;
}

/**
 * Set sensor framerate
 *
 * This function shall be called only before starting the stream and
 * also before querying sensor mode data.
 *
 * TODO: Make controls not available during streaming more explicit
 *       by protecting the IOCs with streaming state.
 */
int IPU2HwSensor::setFrameRate(int fps)
{
    int ret = 0;
    LOG1("@%s: fps %d", __FUNCTION__, fps);

    if (mSensorSubdevice == NULL)
        return -1;

    struct v4l2_subdev_frame_interval subdevFrameInterval;
    CLEAR(subdevFrameInterval);
    subdevFrameInterval.pad = 0;
    subdevFrameInterval.interval.numerator = 1;
    subdevFrameInterval.interval.denominator = fps;
    ret = mSensorSubdevice->xioctl(VIDIOC_SUBDEV_S_FRAME_INTERVAL, &subdevFrameInterval);
    if (ret < 0){
        LOGE("Failed to set framerate to sensor subdevice");
        return ret;
    }
    return ret;
}

int IPU2HwSensor::setFramelines(int fl)
{
    LOG1("@%s: frame lines %d", __FUNCTION__, fl);
    int ret = -1;
    int fps = 0;

    {
    Mutex::Autolock l(mModeDataLock);
    if (mInitialModeDataValid) {
        fps = (int)(0.5 + (float)mInitialModeData.vt_pix_clk_freq_mhz / (float)(fl * mInitialModeData.line_length_pck));
    }
    }
    if (fps != mLastFps) {
        ret = setFrameRate(fps);
        mLastFps = fps;
    }

    return ret;
}

int IPU2HwSensor::getExposurelinetime()
{
    uint32_t exposure_line_time;
    exposure_line_time = (uint32_t)1000000000 * ( (float)mInitialModeData.line_length_pck /
        (float) mInitialModeData.vt_pix_clk_freq_mhz);
    return exposure_line_time;
}

void IPU2HwSensor::setCaptureSyncOwner(void * owner, int invalidFrames)
{
    LOG2("@%s, %d", __FUNCTION__, invalidFrames);
    mSyncOwner = owner;

    Mutex::Autolock l(mExpectedLock);
    mRequestHead = mRequestCaptured = INVALID_REQ_ID;
    mExpectedExpNum = MIN_EXP_ID - invalidFrames;
    mReceivedSOF = INVALID_EXP_ID;
    CLEAR(mFrameList);
    CLEAR(mExpOfRequests);
}

void IPU2HwSensor::setExpectedCaptureExpId(int reqId, void * owner)
{
    int expId = INVALID_EXP_ID;
    if (owner == mSyncOwner) {
        Mutex::Autolock l(mExpectedLock);
        if (mExpectedExpNum <= INVALID_EXP_ID) {
            mExpectedExpNum++;
            LOG2("%s: initial frames skipping, now %d", __FUNCTION__, mExpectedExpNum);
        }

        int index = EXPNUM2INDEX(mExpectedExpNum);
        expId = EXPNUM2ID(mExpectedExpNum);
        LOG2("%s: %d/%d (%d)", __FUNCTION__, reqId, expId, mExpectedExpNum);
        mFrameList[index].expId = expId;
        mFrameList[index].reqId = reqId;

        if (reqId != INVALID_REQ_ID) {
            mRequestHead = reqId;
            mExpOfRequests[REQID2INDEX(reqId)] = mExpectedExpNum;
        }
        mExpectedExpNum++;
    }
    return;
}

// Find appropriate RAW frame for still capture(without preview)
void IPU2HwSensor::findFrameForCapture(int reqId, bool rawLockEnabled)
{
    if (reqId == INVALID_REQ_ID)
        return;

    int expId = INVALID_EXP_ID;
    Mutex::Autolock l(mExpectedLock);
    int expectedExp = mReceivedSOF + 1;
    // Make sure that the sensor setting can be applied in advance.
    if (rawLockEnabled) {
        expectedExp += (int)getExposureDelay();
    }
    // The exp num of this new request should be bigger than the last req's
    int expOfLastReq = mExpOfRequests[REQID2INDEX(mRequestHead)];
    if (expectedExp <= expOfLastReq)
        expectedExp = expOfLastReq + 1;

    expId = EXPNUM2ID(expectedExp);
    if (reqId > mRequestHead) {
        LOG2("%s: %d/%d (%d)", __FUNCTION__, reqId, expId, expectedExp);
        mRequestHead = reqId;
        mExpOfRequests[REQID2INDEX(reqId)] = expectedExp;
    }
}

#define MAX_LOST_FRAME_NUMBER   16
/**
  * Verify the captured ID and the exposure ID
  * If mismtach, there should be frame lost and the mFrameList should be get updated
  */
bool IPU2HwSensor::verifyCapturedExpId(int reqId, int expId)
{
    LOG2("%s: %d/%d", __FUNCTION__, reqId, expId);
    Mutex::Autolock l(mExpectedLock);
    int rightExpId = EXPNUM2ID(mExpOfRequests[REQID2INDEX(reqId)]);

    mRequestCaptured = reqId;

    if (rightExpId == expId)
        return true;

    //FIXME the first a few Exposure ID is wrong
    if ((rightExpId > expId) && ((rightExpId - expId) < MAX_LOST_FRAME_NUMBER))
        return true;

    //handle the case rightExpid = 1, expId = 250
    if ((rightExpId < expId) && ((expId - rightExpId) > MAX_LOST_FRAME_NUMBER))
        return true;

    LOGW("%s: find wrong frame %d/%d, expected exp id %d", __FUNCTION__,
        reqId, expId, rightExpId);

    int gap = EXPNUM2ID((expId - rightExpId)); //How many frames get lost
    int start = rightExpId;
    int count = EXPNUM2INDEX(mExpectedExpNum - start); //How many slots need to be updated

    //Update the new exposure ID for all remaining request
    for (int i = 1; i <= count; i++) {
        mFrameList[EXPNUM2INDEX(start + gap + count - i)].reqId =
            mFrameList[EXPNUM2INDEX(start + count - i)].reqId;
        mFrameList[EXPNUM2INDEX(start + gap + count - i)].expId =
            EXPNUM2ID(mFrameList[EXPNUM2INDEX(start + count - i)].expId) + gap;
    }

    //Clear the lost frames
    for (int i = 0; i < gap; i++)
        CLEAR(mFrameList[EXPNUM2INDEX(start + i)]);

    //Update the saved exposure ID of the request
    for (int i = reqId; i <= mRequestHead; i++) {
        int index = REQID2INDEX(i);
        mExpOfRequests[index] += gap;
        LOG2("%s: %d/%d (%d)", __FUNCTION__, reqId, expId, mExpectedExpNum);
    }

    mExpectedExpNum +=gap;

    return false;
}

int IPU2HwSensor::getExpectedReqId(int expId)
{
    LOG2("%s: %d, [%d %d]", __FUNCTION__, expId, mRequestCaptured, mRequestHead);

    Mutex::Autolock l(mExpectedLock);
    if (mRequestHead == INVALID_REQ_ID)
      return INVALID_REQ_ID;

    int reqId = INVALID_REQ_ID;
    int searchStop = mRequestHead - FRAME_STORED_LENGTH;
    if (searchStop < 0)
        searchStop = 0;
    for (int i = mRequestHead; i >= searchStop; i--) {
        if (EXPNUM2ID(mExpOfRequests[REQID2INDEX(i)]) == expId) {
            reqId = i;
            break;
        }
    }
    LOG2("%s: find %d/%d", __FUNCTION__, reqId, expId);
    return reqId;
}

int IPU2HwSensor::getExpIdForRequest(int reqId)
{
    Mutex::Autolock l(mExpectedLock);
    return EXPNUM2ID(mExpOfRequests[REQID2INDEX(reqId)]);
}

void IPU2HwSensor::storeSOFTime(int expId, int64_t sofTime, int64_t shutterTime)
{
    Mutex::Autolock l(mExpectedLock);
    if (mReceivedSOF == INVALID_EXP_ID)
        mReceivedSOF = expId;
    else
        mReceivedSOF++;

    if (EXPNUM2ID(mReceivedSOF) != expId) {
        LOG1("%s: exp id %d is out of order! should be %d", __FUNCTION__,
            expId, EXPNUM2ID(mReceivedSOF));
        int64_t timeNs;

        int lostSOF = EXPNUM2ID(expId - EXPNUM2ID(mReceivedSOF));
        int64_t lastSOFTime = mFrameList[EXPNUM2INDEX(mReceivedSOF - 1)].sofTime;
        int64_t sofIntervals = (sofTime -lastSOFTime)/(lostSOF +1);
        int64_t lastShutterTime = mFrameList[EXPNUM2INDEX(mReceivedSOF -1)].shutterTime;
        int64_t shutterIntervals = (shutterTime - lastShutterTime)/(lostSOF+1);

        //Calculate the lost SOF time
        for (int i = 0; i < lostSOF; i++) {
            int exp_id = mReceivedSOF + i;
            int index = EXPNUM2INDEX(exp_id);
            mFrameList[index].sofTime = lastSOFTime + sofIntervals * (i + 1);
            mFrameList[index].shutterTime = lastShutterTime + shutterIntervals * (i +1);
            LOG1("Calculated %d SOF is %lld, Shutter is %lld", mReceivedSOF +i,
                 mFrameList[index].sofTime, mFrameList[index].shutterTime);

            // Clear the old EOF time
            int clearCount = mReceivedSOF + i + FRAME_LIST_LENGTH - FRAME_STORED_LENGTH;
            CLEAR(mFrameList[EXPNUM2INDEX(clearCount)]);
        }

        mReceivedSOF = mReceivedSOF + lostSOF;

        LOG1("%s: mReceivedSOF has been reset to:%d, to keep align with real exp id",
            __FUNCTION__, mReceivedSOF);

        LOG2("lastShutterTime=%lld, shutterIntervals=%lld", lastShutterTime,
             shutterIntervals);
        for (int i = 0; i < FRAME_LIST_LENGTH; i++) {
            LOG2("mFrameList[%d].shutterTime = %lld", i, mFrameList[i].shutterTime);
        }
    }

    if (expId >= MIN_EXP_ID && expId <= MAX_EXP_ID) {
        mFrameList[EXPNUM2INDEX(mReceivedSOF)].sofTime = sofTime;
        mFrameList[EXPNUM2INDEX(mReceivedSOF)].shutterTime = shutterTime;

        LOG2("mReceivedSOF = %d, mFrameList[%d].shutterTime = %lld",
             mReceivedSOF, EXPNUM2INDEX(mReceivedSOF), shutterTime);

        /* WorkAround:
         * the EOF time the HAL get from driver is not accurate (2~3 ms error),
         * so if frame 15 has short exposure time (0.1 ms)
         * and frame 16 has long exposure time (70 ms),
         * the shutter time 16 might be smaller than frame 15 after calculated.
         */
        if (mReceivedSOF > 1) {
            int64_t lastShutter =
                mFrameList[EXPNUM2INDEX(mReceivedSOF - 1)].shutterTime;
            if (lastShutter > shutterTime)
                mFrameList[EXPNUM2INDEX(mReceivedSOF)].shutterTime =
                    lastShutter + 1000;
        }

        // Clear the old EOF time
        int clearCount = mReceivedSOF + FRAME_LIST_LENGTH - FRAME_STORED_LENGTH;
        CLEAR(mFrameList[EXPNUM2INDEX(clearCount)]);
    }
}

// Return 0 CssHwSensor shutter event is not come
int64_t IPU2HwSensor::getShutterTime(int reqId)
{
    Mutex::Autolock l(mExpectedLock);
    int64_t timeNs;
    if (reqId > mRequestHead) {
        LOG1("@%s, reqId:%d, finished request Id:%d",__FUNCTION__, reqId, mRequestHead);
        return -1;
    }
    int exp = mExpOfRequests[REQID2INDEX(reqId)];
    int index = EXPNUM2INDEX(exp);

    timeNs = _getShutterTimeByExpId(exp);
    LOG2("@%s Request %d, find req id:%d, exp_id %d: shutterTime:(%lld)", __FUNCTION__,
         reqId, mFrameList[index].reqId, exp, timeNs);
    return timeNs;
}

int64_t IPU2HwSensor::getSOFTime(int reqId)
{
    Mutex::Autolock l(mExpectedLock);
    int64_t timeNs;
    if (reqId > mRequestHead) {
        LOGE("@%s, reqId:%d, finished request Id:%d",__FUNCTION__, reqId, mRequestHead);
        return 0;
    }
    int expNum = mExpOfRequests[REQID2INDEX(reqId)];
    int index = EXPNUM2INDEX(expNum);

    timeNs = _getSOFByExpNum(expNum);
    LOG2("@%s Request %d, find req id:%d, exp_id %d: sofTime:(%lld)", __FUNCTION__,
         reqId, mFrameList[index].reqId, mFrameList[index].expId, timeNs);
    return timeNs;
}


// Return 0 CssHwSensor shutter event is not come
int64_t IPU2HwSensor::getShutterTimeByExpId(int exp)
{
    Mutex::Autolock l(mExpectedLock);
    return _getShutterTimeByExpId(exp);
}

//getShutterTimeByExpId without Lock
int64_t IPU2HwSensor::_getShutterTimeByExpId(int exp)
{
    int index = EXPNUM2INDEX(exp);
    LOG2("@%s EXPNUM2ID:%d find req id:%d", __FUNCTION__,
            EXPNUM2ID(exp), mFrameList[index].reqId);

    if (exp != 0 && mFrameList[index].expId == EXPNUM2ID(exp))
        return mFrameList[index].shutterTime;
    else
        return 0;
}

//getShutterTimeByExpId without Lock
int64_t IPU2HwSensor::_getSOFByExpNum(int exp)
{
    int index = EXPNUM2INDEX(exp);
    LOG2("@%s EXPNUM2ID:%d find req id:%d", __FUNCTION__,
            EXPNUM2ID(exp), mFrameList[index].reqId);

    if (exp != 0 && mFrameList[index].expId == EXPNUM2ID(exp))
        return mFrameList[index].sofTime;
    else
        return 0;
}


/**
 * Return vbi latency for exposure history item
 */
void IPU2HwSensor::vbiIntervalForItem(unsigned int index,
                                     unsigned int & vbiOffset,
                                     unsigned int & shutterOffset)
{
    vbiOffset = 0;
    shutterOffset = 0;
    if (!mInitialModeDataValid)
        updateModeData();
    if (!mInitialModeDataValid) {
        LOG2("mInitialModeData still invalid");
        return;
    }

    Mutex::Autolock l(mExposureLock);
    LOG2("@%s, index %d, counts=%d", __FUNCTION__, index, mExposureHistory->getCount());
    if (index >= mExposureHistory->getCount())
        index = mExposureHistory->getCount()-1;

    /*
     * Relationship of SOF/EOF/shutter time:
     *                        ^ shutter time
     *      ______________    |<- ET2 ->|_____________
     * _____|     1      |______________|    2       |_____
     *      ^            ^              ^
     *      |<-SOF       |<-EOF         |
     *      |<-       1/fps           ->|
     * ET2: exposure time of frame 2

     * The real fps will be slow if ET2 > 1/fps:
     *       ^ shutter time
     *       |<-       ET2                ->|
     *      _____________|<-    vbi       ->|_____________
     * _____|     1      |__________________|    2       |_
     *      ^                               ^
     *      |<-       ET2 + T             ->|
     * T = reset time + readout of the first line (depend on sensor)
     * (ignore it during calculation)
     */
    unsigned int fll = mInitialModeData.frame_length_lines;
    struct atomisp_exposure *item = mExposureHistory->peek(index);
    if (item != NULL && item->integration_time[0] > fll)
        fll = item->integration_time[0];
    LOG2("fll=%d", fll);

    int vbiLL = fll - (mInitialModeData.crop_vertical_end -
        mInitialModeData.crop_vertical_start + 1) / mInitialModeData.binning_factor_y;
    if (vbiLL < 0) {
        LOGW("%s: the calculated vbiLL < 0, set to 0.", __FUNCTION__);
        LOGW("%s: frame_length_lines=%d, cropped_image_height=%d, binning_factor_y=%d",
            __FUNCTION__, mInitialModeData.frame_length_lines,
            mInitialModeData.crop_vertical_end - mInitialModeData.crop_vertical_start + 1,
            mInitialModeData.binning_factor_y);
        vbiLL = 0;
    }
    int shutterLL = item ? item->integration_time[0] : fll;

    vbiOffset = ((long long) mInitialModeData.line_length_pck * vbiLL * 1000000)
           / mInitialModeData.vt_pix_clk_freq_mhz;
    shutterOffset = ((long long) mInitialModeData.line_length_pck * shutterLL * 1000000)
           / mInitialModeData.vt_pix_clk_freq_mhz;
}

status_t IPU2HwSensor::produceExposureHistory(struct atomisp_exposure exposure)
{
    LOG2("%s: coarse=%d", __FUNCTION__, exposure.integration_time[0]);
    Mutex::Autolock l(mExposureLock);
    // when fifo is full drop the oldest
    if (mExposureHistory->getCount() >= mExposureHistory->getDepth())
        mExposureHistory->dequeue();

    mExposureHistory->enqueue(exposure);

    return NO_ERROR;
}

status_t IPU2HwSensor::newSOF(ICssIspListener::IspMessageEvent * sofMsg)
{
    LOG2("@%s: mMessageQueue size %d", __FUNCTION__, mMessageQueue.size());
    Message msg;
    int ret = NO_ERROR;

    int64_t sofTime = TIMEVAL2NSECS(&sofMsg->timestamp);
    storeSOFTime(sofMsg->exp_id, sofTime, sofMsg->shutterTime * 1000);

    msg.id = MESSAGE_ID_SOF;
    msg.data.sof.exp_id = sofMsg->exp_id;
    msg.data.sof.timestamp = sofMsg->timestamp;
    msg.data.sof.vbiOffset = sofMsg->vbiOffset;
    ret = mMessageQueue.send(&msg);

    return ret;
}

status_t IPU2HwSensor::handleNewSOF(Message & msg)
{
    PERFORMANCE_HAL_ATRACE_PARAM1("exp_id", msg.data.sof.exp_id);
    LOG2("@%s", __FUNCTION__);
    // For perframe settings, apply the settings at SOF.
    if (supportPerFrameSettings()) {
        usleep(msg.data.sof.vbiOffset);
        //applyExposure(msg.data.sof.exp_id);
    }

    return NO_ERROR;
}

bool IPU2HwSensor::notifyIspEvent(ICssIspListener::IspMessage *msg)
{
    if (!msg || msg->id != ICssIspListener::ISP_MESSAGE_ID_EVENT)
        return true;

    if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_SOF) {
        LOG2("IPU2HwSensor receives ISP_EVENT_TYPE_SOF(exp_id %d)", msg->data.event.exp_id);
        newSOF(&msg->data.event);
        return true;
    }

    return false;
}

void IPU2HwSensor::messageThreadLoop()
{
    LOG2("@%s: Start", __FUNCTION__);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);
        PERFORMANCE_HAL_ATRACE_PARAM1("msg", msg.id);
        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            mThreadRunning = false;
            status = NO_ERROR;
            break;

        case MESSAGE_ID_SOF:
            handleNewSOF(msg);
            break;

        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("    error %d in handling message: %d", status, (int)msg.id);
        mMessageQueue.reply(msg.id, status);
    }

    LOG2("%s: Exit", __FUNCTION__);
}

}
}
