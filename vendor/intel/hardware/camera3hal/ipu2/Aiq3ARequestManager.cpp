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
#define LOG_TAG "Camera_AAARequestMng"

#include <utils/Errors.h>
#include <math.h>
#include "LogHelper.h"
#include "PlatformData.h"
#include "platformdata/CameraMetadataHelper.h"
#include "Aiq3ARequestManager.h"
#include "Utils.h"


namespace android {
namespace camera2 {

Aiq3ARequestManager::Aiq3ARequestManager(HWControlGroup &hwcg,
                                              I3AControls * aaa,
                                              CameraMetadata &staticMeta)
    :mISP(hwcg.mIspCI)
    ,mSensorCI(hwcg.mSensorCI)
    ,m3AControls(aaa)
    ,mStaticMeta(staticMeta)
    ,mReqIdFrameDone(-1)
    ,mOneRequestLeftWaitingForEmbedded(false)
    ,mGlobalInited(false)
    ,mSensorDelay(0)
    ,mFirstRequest(true)
    ,mFrameSkips(0)
    ,mMinSensitivity(0)
    ,mMaxSensitivity(0)
    ,mMinExposureTime(0)
    ,mMaxExposureTime(0)
    ,mColorOrder(0)
    ,mLscGrid(NULL)
    ,mLscResizeGrid(NULL)
    ,mMaxTonemapCurvePoint(0)
    ,mNoiseReduction(true)
    ,mEdgeEnhancement(true)
    ,mRollingShutterSkew(0)
    ,mBlackLevelLockOn(false)
    ,mPixelSize(0)
    ,mFreFocusDistance(10000)
    ,mPreExposureTime(0)
    ,mLensShadingMapMode(false)
    ,mMaxRegions(NULL)
    ,mLastExposureTimeNs(0)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    CLEAR(mISPParams);
    CLEAR(mExposure);
    CLEAR(mActiveArraySize);
    CLEAR(mAutoISPMetadata);
    CLEAR(mLensShadingInfo);
    CLEAR(mLastSensorMetadata);
    CLEAR(mAaaMetadata);
    CLEAR(mColorChannel);

    int count = 0;
    bool* hasFlash = (bool*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_FLASH_INFO_AVAILABLE,
                                             TYPE_BYTE,
                                             &count);
    if (hasFlash != NULL) {
        mHasFlash = *hasFlash;
    } else {
        mHasFlash = false;
    }

    if (PlatformData::AiqConfig) {
        ia_cmc_t *cmc = PlatformData::AiqConfig.getCMCHandler();
        if (cmc && cmc->cmc_parsed_optics.cmc_optomechanics) {
            mPixelSize = cmc->cmc_parsed_optics.cmc_optomechanics->sensor_pix_size_h / 100;
        }
    }
    mLensAperture = (float*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                                             TYPE_FLOAT,
                                             &count);
    mLensFilterDensity = (float*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
                                             TYPE_FLOAT,
                                             &count);
    mMaxRegions = (int32_t*)MetadataHelper::getMetadataValues(mStaticMeta,
                                             ANDROID_CONTROL_MAX_REGIONS,
                                             TYPE_INT32,
                                             &count);
    // Get the lsc grid width/height, and bayer order from CMC
    mLensShadingMapSize = (int32_t*)MetadataHelper::getMetadataValues(
                                     mStaticMeta,
                                     ANDROID_LENS_INFO_SHADING_MAP_SIZE,
                                     TYPE_INT32,
                                     &count);
}

Aiq3ARequestManager::~Aiq3ARequestManager()
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    for (unsigned int i = 0; i < mPendingRequestsQueue.size(); i++) {
        aaa_request & aaaRequest = mPendingRequestsQueue.editItemAt(i);
        aaaRequestClear(aaaRequest);
    }

    mPendingRequestsQueue.clear();
    mReqQForShutter.clear();

    if (mAutoISPMetadata.lscGridInRGGB != NULL)
        delete []mAutoISPMetadata.lscGridInRGGB;
    if (mLscGrid != NULL) {
        delete[] mLscGrid;
        mLscGrid = NULL;
    }

    delete mAutoISPMetadata.rGammaLut;
    delete mAutoISPMetadata.gGammaLut;
    delete mAutoISPMetadata.bGammaLut;

    delete[] mLensShadingInfo.lsc_grids;
    delete[] mLscResizeGrid;
}

// This function runs in request thread.
void Aiq3ARequestManager::reset(IspMode mode)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    UNUSED(mode);
    mFirstRequest = true;
    // get the rollingShutterSkew when reset, because the rolling shutter skew
    // only change when the resolution is changed
    getRollingShutterSkew();
    mBlackLevelLockOn = false;
}

void Aiq3ARequestManager::storeRequestForShutter(Camera3Request* request)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    Mutex::Autolock l(mReqShutterLock);
    mReqQForShutter.push_back(request);
}

// This function runs in request thread.
bool Aiq3ARequestManager::storeNewRequest(Camera3Request* request,
                                                int availableStreams)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    bool needProcess = false;
    // Get an empty element
    aaa_request new_request;
    CLEAR(new_request);

    // Fill in the request.
    new_request.request = request;
    new_request.mReqId = request->getId();
    const camera3_capture_request *req3 = request->getUserRequest();
    if (req3->settings)
        new_request.newSettings = true;

    if (new_request.newSettings || mISP->isIspPerframeSettingsEnabled()) {
        // Check whether manual control mode
        camera_metadata_ro_entry entry;
        entry = ReadRequest(request).mMembers.mSettings.find(ANDROID_CONTROL_MODE);
        if (entry.count == 1 && entry.data.u8[0] == ANDROID_CONTROL_MODE_OFF)
            new_request.manualControlMode = true;

#ifdef INTEL_ENHANCEMENT
        entry = ReadRequest(request).mMembers.mSettings.find(ANDROID_CONTROL_SKIP_VENDOR_3A);
        if (entry.count == 1) {
            if (entry.data.u8[0] == ANDROID_CONTROL_SKIP_VENDOR_3A_ON)
                new_request.manualControlMode = true;
        }
#endif
        // Process the manual settings
        processManualExposure(new_request);
        processManualColorCorrection(new_request);
        processManualTonemap(new_request);

        // Check lens shading map mode
        entry = ReadRequest(request).mMembers.mSettings.find(
                                    ANDROID_STATISTICS_LENS_SHADING_MAP_MODE);
        if (entry.count == 1) {
            if (entry.data.u8[0] == ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON)
                mLensShadingMapMode = true;
            else
                mLensShadingMapMode = false;
        }

        LOG2("%s: lens shading map mode is %d", __FUNCTION__, mLensShadingMapMode);

        // Check noise reduction mode
        entry = ReadRequest(request).mMembers.mSettings.find(ANDROID_NOISE_REDUCTION_MODE);
        if (entry.count == 1 &&
            entry.data.u8[0] == ANDROID_NOISE_REDUCTION_MODE_OFF)
            new_request.noiseReduction = false;
        else
            new_request.noiseReduction = true;

        // Check edge enhancement mode
        entry = ReadRequest(request).mMembers.mSettings.find(ANDROID_EDGE_MODE);
        if (entry.count == 1 &&
            entry.data.u8[0] == ANDROID_EDGE_MODE_OFF)
            new_request.edgeEnhancement = false;
        else
            new_request.edgeEnhancement = true;

        // Check shading mode
        entry = ReadRequest(request).mMembers.mSettings.find(ANDROID_SHADING_MODE);
        if (entry.count == 1 &&
            entry.data.u8[0] == ANDROID_SHADING_MODE_OFF)
            new_request.lensShading = false;
        else
            new_request.lensShading = true;

        // Check black level lock
        entry = ReadRequest(request).mMembers.mSettings.find(ANDROID_BLACK_LEVEL_LOCK);
        if (entry.count == 1 &&
            entry.data.u8[0] == ANDROID_BLACK_LEVEL_LOCK_OFF)
            new_request.blackLevellock = false;
        else
            new_request.blackLevellock = true;

        // For continus mode, use the available stream info to decide whether
        // to set isp parameters for capture and preview pipe.
        new_request.availableStreams = availableStreams;
    }

    if (mFirstRequest) {
        // For the first request set the exposure at once. So that it should be
        // set before stream on and will take effect at the first frame.
        mSensorCI->applyExposure(0);
        mFirstRequest = false;
    }

    // Check whether there is pending request to be processed
    {
    Mutex::Autolock l(mPendingQueueLock);
    unsigned int i;
    for (i = 0; i < mPendingRequestsQueue.size(); i++) {
        if (mPendingRequestsQueue.itemAt(i).state < AAA_REQUEST_PROCESSED)
            break;
    }
    if (i == mPendingRequestsQueue.size()) {
        needProcess = new_request.newSettings;
        new_request.state = AAA_REQUEST_PROCESSING;
    }

    mOneRequestLeftWaitingForEmbedded = false;
    // Store the new request in the pending Q
    mPendingRequestsQueue.push_back(new_request);
    }


    // dump
    LOG2("###--------------stored a new request %d--------------###", new_request.mReqId);
    dump();

    return needProcess;
}

status_t Aiq3ARequestManager::handleNewRequest(int request_id)
{
    LOG2("@%s, line:%d, <request %d>", __FUNCTION__, __LINE__, request_id);
    status_t ret = NO_ERROR;
    aaa_request aaaRequest;
    CLEAR(aaaRequest);
    unsigned int i = 0;

    {
    Mutex::Autolock l(mPendingQueueLock);
    for (i = 0; i < mPendingRequestsQueue.size(); i++) {
        if (mPendingRequestsQueue.itemAt(i).mReqId == request_id) {
            aaaRequest = mPendingRequestsQueue.editItemAt(i);
            mPendingRequestsQueue.editItemAt(i).applied = true;
            break;
        }
    }

    if (i >= mPendingRequestsQueue.size()) {
        LOGW("%s: request %d is not found", __FUNCTION__, request_id);
        return UNKNOWN_ERROR;
    }
    }

    ret = applyIspSettings(aaaRequest);

    return ret;
}

// This function is called by Aiq3AThread as soon as it receive the EOF event,
// so it will run in isp thread.
status_t Aiq3ARequestManager::applySettings(unsigned int exp_id)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    // Apply sensor settings.
    int request_id = mSensorCI->applyExposure(exp_id);
    LOG2("%s Apply for req %d, exp %d", __FUNCTION__, request_id, exp_id);

    return NO_ERROR;
}

Camera3Request* Aiq3ARequestManager::getNextRequest(int reqId)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    aaa_request aaaRequest;
    CLEAR(aaaRequest);

    Mutex::Autolock l(mPendingQueueLock);

    if (mPendingRequestsQueue.isEmpty()) {
        LOG2("No pending request.");
        return NULL;
    }

    unsigned int i;
    if (reqId == INVALID_REQ_ID) {
        for (i = 0; i < mPendingRequestsQueue.size(); i++) {
            if (mPendingRequestsQueue.itemAt(i).state == AAA_REQUEST_PROCESSING) {
                mPendingRequestsQueue.editItemAt(i).state = AAA_REQUEST_PROCESSED;
            } else if (mPendingRequestsQueue.itemAt(i).state == AAA_REQUEST_NEW) {
                mPendingRequestsQueue.editItemAt(i).state = AAA_REQUEST_PROCESSING;
                aaaRequest = mPendingRequestsQueue.editItemAt(i);
                break;
            }
        }
    } else {
        for (i = 0; i < mPendingRequestsQueue.size(); i++) {
            if (mPendingRequestsQueue[i].mReqId == reqId) {
                aaaRequest = mPendingRequestsQueue.editItemAt(i);
                break;
            }
        }
    }

    if (i == mPendingRequestsQueue.size()) {
        LOG2("No pending request to be processed.");
    }

    return aaaRequest.request;
}

status_t Aiq3ARequestManager::notifyResult(int request_id, unsigned int exp_id,
                                         aaa_state_metadata aaa,
                                         int* faceIds,
                                         int* faceLandmarks,
                                         int* faceRect,
                                         uint8_t* faceScores,
                                         int faceNum,
                                         FrameBufferStatus flashStatus,
                                         uint8_t* aePrecaptureTrigger)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    status_t status = NO_ERROR;

    // Verify the capture request pair
    mSensorCI->verifyCapturedExpId(request_id, exp_id);

    checkMetadataDone(request_id, exp_id);

    bool embeddedComing = mISP->checkSensorMetadataAvailable(exp_id);
    aaa_request aaaRequest;
    {
    Mutex::Autolock l(mPendingQueueLock);
    if (!mPendingRequestsQueue.size()
        || mPendingRequestsQueue[0].mReqId != request_id) {
        LOGE("%s: mismatch request %d", __FUNCTION__, request_id);
        return BAD_VALUE;
    }
    mPendingRequestsQueue.editItemAt(0).expId = exp_id;
    aaaRequest = mPendingRequestsQueue[0];
    if (embeddedComing)
        mPendingRequestsQueue.removeAt(0);
    else
        mOneRequestLeftWaitingForEmbedded = (mPendingRequestsQueue.size() == 1);
    mReqIdFrameDone = request_id;
    }

    // Fill in the metadatas
    {
    LOG2("%s: fill frame metadata %d/%d", __FUNCTION__, request_id, exp_id);
    ReadWriteRequest rwRequest(aaaRequest.request);
    CameraMetadata & settings = rwRequest.mMembers.mSettings;

    // 3a region should inside the crop region, so fill the crop region first
    fillScalerCropRegion(settings, aaaRequest);

    if (1 == mMaxRegions[0])
        fill3aRegion(settings, aaaRequest, ANDROID_CONTROL_AE_REGIONS);
    if (1 == mMaxRegions[2])
        fill3aRegion(settings, aaaRequest, ANDROID_CONTROL_AF_REGIONS);
    if (1 == mMaxRegions[1])
        fill3aRegion(settings, aaaRequest, ANDROID_CONTROL_AWB_REGIONS);

    fillColorCorrection(settings, aaaRequest);
    fillLensShadingMap(settings, aaaRequest);
    fillTonemapCurve(settings, aaaRequest);
    fillRollingShutterSkew(settings);
    fillBlackLevelLock(settings, aaaRequest);
    fillLensDynamic(settings);
    fillFrameduration(settings);

    // 3a states metadata
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AE_STATE, &aaa.aeState, 1);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AE_PRECAPTURE_ID, &aaa.aeTriggerId, 1);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AWB_STATE, &aaa.awbState, 1);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AF_STATE, &aaa.afState, 1);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AF_TRIGGER_ID, &aaa.afTriggerId, 1);

    // update sync.frameNumber
    int64_t frameNumber = request_id;
    rwRequest.mMembers.mSettings.update(ANDROID_SYNC_FRAME_NUMBER, &frameNumber, 1);

    // flash metadata
    camera_metadata_enum_android_flash_state flash_state = ANDROID_FLASH_STATE_UNAVAILABLE;
    if(mHasFlash) {
        switch(flashStatus) {
        case FRAME_STATUS_FLASH_EXPOSED:
            flash_state = ANDROID_FLASH_STATE_FIRED;
            break;
#ifdef HAL_3_2_PROPERTIES
        case FRAME_STATUS_FLASH_PARTIAL:
            flash_state = ANDROID_FLASH_STATE_PARTIAL;
            break;
#endif
        default:
            flash_state = ANDROID_FLASH_STATE_READY;
            break;
        }
        rwRequest.mMembers.mSettings.update(ANDROID_FLASH_STATE, (uint8_t*)&flash_state, 1);
    } else {
        rwRequest.mMembers.mSettings.update(ANDROID_FLASH_STATE, (uint8_t*)&flash_state, 1);
    }

    // face detection metadata
    rwRequest.mMembers.mSettings.update(
                  ANDROID_STATISTICS_FACE_IDS,
                  faceIds, faceNum);
    rwRequest.mMembers.mSettings.update(
                  ANDROID_STATISTICS_FACE_LANDMARKS,
                  faceLandmarks, 6 * faceNum);
    rwRequest.mMembers.mSettings.update(
                  ANDROID_STATISTICS_FACE_RECTANGLES,
                  faceRect, 4 * faceNum);
    rwRequest.mMembers.mSettings.update(
                  ANDROID_STATISTICS_FACE_SCORES,
                  faceScores, faceNum);

    // update ae Precapture Trigger metadata
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, aePrecaptureTrigger, 1);

    // update request metadata
    mISP->updateSettingPipelineDepth(rwRequest.mMembers.mSettings);
    // update android.statistics.hotPixelMapMode, currently we don't support hotpixel,
    // so set it to OFF
    int hotPixelMode = ANDROID_HOT_PIXEL_MODE_OFF;
    rwRequest.mMembers.mSettings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP, &hotPixelMode, 1);

#ifdef INTEL_ENHANCEMENT
    rwRequest.mMembers.mSettings.update(ANDROID_STATISTICS_WAS_SKIPPED_FOR_VENDOR_3A, (uint8_t*) &aaaRequest.manualControlMode, 1);
#endif
    }

    if (embeddedComing) {
        LOG2("%s: NotifyMetadata for %d/%d (%d)", __FUNCTION__,
            request_id, exp_id, mOneRequestLeftWaitingForEmbedded);
        getAndFillSensorMetadata(aaaRequest.request, aaaRequest);
        aaaRequest.request->mCallback->metadataDone(aaaRequest.request);
    }

    // dump
    LOG2("###-------finished a request and send result-------###");
    dump();

    return status;
}

// Check and return metadata callback for requests that is waiting for
// sensor embedded event.
status_t Aiq3ARequestManager::checkMetadataDone(int reqId, int expId)
{
    status_t status = NO_ERROR;

    UNUSED(expId);

    // When sensor embedded event comes, check and return callback only if there
    // is one request left and this request has received frame event.
    if (reqId == -1 && !mOneRequestLeftWaitingForEmbedded) {
        return NO_ERROR;
    }

    Vector<aaa_request> pending;
    aaa_request aRequest;
    Camera3Request * request;
    int requestDone = mReqIdFrameDone;
    do {
        request = NULL;
        {
        Mutex::Autolock l(mPendingQueueLock);
        if (mPendingRequestsQueue.size()
            && mPendingRequestsQueue[0].mReqId <= requestDone) {
            aRequest = mPendingRequestsQueue.editItemAt(0);
            request = aRequest.request;
            mPendingRequestsQueue.removeAt(0);
        }
        }

        if (request) {
            getAndFillSensorMetadata(request, aRequest);
            LOG2("%s: NotifyMetadata %d/%d (%d)", __FUNCTION__,
                aRequest.mReqId, aRequest.expId, reqId);
            request->mCallback->metadataDone(request);
        }
    } while (request != NULL);

    return status;
}

// Return shutter callback for requests that received related SOF events.
status_t Aiq3ARequestManager::notifyShutter(int expId, struct timeval /*timestamp*/)
{
    status_t status = NO_ERROR;
    Camera3Request * request = NULL;

    do {
        {
        Mutex::Autolock l(mReqShutterLock);
        // Done for the first request
        if (request) {
            mReqQForShutter.removeAt(0);
            request = NULL;
        }
        if (mReqQForShutter.size()){
            request = mReqQForShutter.editItemAt(0);
        }
        }

        if (request) {
            int curReqId = request->getId();
            int64_t time = mSensorCI->getSOFTime(curReqId);

            if (time > 0) {
                LOG2("%s: req %d, %lld (%d)", __FUNCTION__, curReqId, time, expId);
                ReadWriteRequest rwRequest(request);
                rwRequest.mMembers.mSettings.update(ANDROID_SENSOR_TIMESTAMP, &time, 1);
                request->mCallback->shutterDone(request, time);
            } else {
                // Related SOF does not come.
                break;
            }
        }
    } while (request);
    return status;
}

void Aiq3ARequestManager::storeGbceResults(void)
{
    unsigned int size = mISPParams.gbce_results.gamma_lut_size;

    if (size <= 1) {
        mAutoISPMetadata.gammaLutSize = 0;
        return ;
    }

    if (size > mMaxTonemapCurvePoint)
        size = mMaxTonemapCurvePoint;

    for (unsigned int i = 0; i < size; i++) {
        mAutoISPMetadata.rGammaLut[i*2] = (float) i / (size - 1);
        mAutoISPMetadata.gGammaLut[i*2] = (float) i / (size - 1);
        mAutoISPMetadata.bGammaLut[i*2] = (float) i / (size - 1);
    }
    mAutoISPMetadata.gammaLutSize = size;

    // Range gamma lut [0, 255] in AIQ, android definition for the metadata is [0,1],
    // so need mapping
    for (unsigned int i = 0; i < size; i++) {
        mAutoISPMetadata.rGammaLut[i*2 + 1] =
           (float) mISPParams.gbce_results.r_gamma_lut[i] / GAMMA_LUT_UPPER_BOUND;
        mAutoISPMetadata.gGammaLut[i*2 + 1] =
           (float) mISPParams.gbce_results.g_gamma_lut[i] / GAMMA_LUT_UPPER_BOUND;
        mAutoISPMetadata.bGammaLut[i*2 + 1] =
           (float) mISPParams.gbce_results.b_gamma_lut[i] / GAMMA_LUT_UPPER_BOUND;
    }
}

status_t Aiq3ARequestManager::storeLensShadingMap(uint16_t width, uint16_t height, uint16_t *lscGrid)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    if (lscGrid == NULL || mAutoISPMetadata.lscGridInRGGB == NULL) {
        LOGE("src or dest grid is NULL");
        return UNKNOWN_ERROR;
    }

    int destWidth = mLensShadingMapSize[0];
    int destHeight = mLensShadingMapSize[1];
    if (width != destWidth || height != destHeight) {
        if (mLscResizeGrid == NULL) {
            mLscResizeGrid = new uint16_t[destWidth * destHeight * 4];
            if (mLscResizeGrid == NULL) {
                LOGE("no memory for mLscResizeGrid,can't do resize");
                return NO_MEMORY;
            }
        }
        //Android requests lensShadingMapSize must be smaller than 64*64
        //and it is a constant size
        //however our lensShadingMapSize is dynamic base on different resolution,
        //so need to do resize, resize for 4 channel seperately
        for (int i = 0; i < 4; i++) {
             uint16_t *src = lscGrid + width * height * i;
             uint16_t *dst = mLscResizeGrid + destWidth * destHeight * i;
             resize2dArrayUint16(src, width, height, dst, destWidth, destHeight);
        }
        LOG2("resize the lens shading map from [%d,%d] to [%d,%d]",
              width, height, destWidth, destHeight);
        reFormatLensShadingMap(destWidth, destHeight,
                               mLscResizeGrid, mAutoISPMetadata.lscGridInRGGB);
    } else
        reFormatLensShadingMap(width, height,
                               lscGrid, mAutoISPMetadata.lscGridInRGGB);
    return NO_ERROR;

}

status_t Aiq3ARequestManager::storeAutoParams(ispInputParameters * isp,
                                        ia_aiq_exposure_parameters * exposure)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    status_t status = NO_ERROR;

    // Store the auto ISP params
    CLEAR(mISPParams);
    memcpy(&mISPParams, isp, sizeof(ispInputParameters));

    // Store the auto settings in the request for filling the metadata
    if (mISPParams.pa_results->color_conversion_matrix != NULL)
        memcpy(mAutoISPMetadata.colorTransform,
           mISPParams.pa_results->color_conversion_matrix, sizeof(float)*9);

    if (mISPParams.pa_results->color_gains != NULL)
        memcpy(mAutoISPMetadata.colorGain,
           mISPParams.pa_results->color_gains, sizeof(float)*4);

    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t *lscGrid = NULL;
    if (mISPParams.pa_results->weighted_lsc != NULL &&
        mAutoISPMetadata.lscGridInRGGB != NULL) {
        width = mISPParams.pa_results->weighted_lsc->cmc_lens_shading->grid_width;
        height = mISPParams.pa_results->weighted_lsc->cmc_lens_shading->grid_height;
        lscGrid = mISPParams.pa_results->weighted_lsc->lsc_grids;
        mAutoISPMetadata.lscValid = true;
    } else {
        // retrive lens shading map from ISP
        status = mISP->getLensShading(&mLensShadingInfo);
        if (status == NO_ERROR) {
            width = mLensShadingInfo.grid_width;
            height = mLensShadingInfo.grid_height;
            lscGrid = mLensShadingInfo.lsc_grids;
            mAutoISPMetadata.lscValid = true;
        } else
            mAutoISPMetadata.lscValid = false;
    }
    if (mISPParams.pa_results->black_level_coef != NULL) {
        mAutoISPMetadata.colorChannel.cc1 = mISPParams.pa_results->black_level_coef->cc1 / 256;
        mAutoISPMetadata.colorChannel.cc2 = mISPParams.pa_results->black_level_coef->cc2 / 256;
        mAutoISPMetadata.colorChannel.cc3 = mISPParams.pa_results->black_level_coef->cc3 / 256;
        mAutoISPMetadata.colorChannel.cc4 = mISPParams.pa_results->black_level_coef->cc4 / 256;
    }

    if (mISPParams.gbce_results_valid)
        storeGbceResults();
    if (mAutoISPMetadata.lscValid) {
        // resize and reformat
        status = storeLensShadingMap(width, height, lscGrid);
    }

    // Store the auto exposure params
    CLEAR(mExposure);
    memcpy(&mExposure, exposure, sizeof(ia_aiq_exposure_parameters));

    mGlobalInited = true;

    return status;
}

/*
*Android reuqests order of lensShadingMap is [R, Geven, Godd, B], interlaced.
*however the lensShadingMap from ISP is 4 width*height blocks,tiled.
*for ia_aiq_bayer_order_grbg, the four blocks are G, R, B, G
*/
status_t Aiq3ARequestManager::reFormatLensShadingMap(uint16_t width, uint16_t height, uint16_t *srcLscGrid, float *destLscGridInRGGB)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    if (srcLscGrid == NULL || destLscGridInRGGB == NULL) {
        LOGE("src or dest grid is NULL");
        return UNKNOWN_ERROR;
    }
    // Convert the color order
    uint16_t* gEvenLscGrid = NULL;
    uint16_t* rLscGrid = NULL;
    uint16_t* bLscGrid = NULL;
    uint16_t* gOddLscGrid = NULL;

    uint16_t* gEvenLscGridLineStart = NULL;
    uint16_t* rLscGridLineStart = NULL;
    uint16_t* bLscGridLineStart = NULL;
    uint16_t* gOddLscGridLineStart = NULL;
    float* dstGrid = destLscGridInRGGB;

    uint16_t rggbIndices[4][4] = {
          [ia_aiq_bayer_order_grbg] = {1, 3, 0, 2},
          [ia_aiq_bayer_order_rggb] = {0, 1, 2, 3},
          [ia_aiq_bayer_order_bggr] = {3, 1, 2, 0},
          [ia_aiq_bayer_order_gbrg] = {2, 3, 0, 1}
    };
    rLscGrid = srcLscGrid + rggbIndices[mColorOrder][0] * width * height;
    gEvenLscGrid = srcLscGrid + rggbIndices[mColorOrder][1] * width * height;
    gOddLscGrid = srcLscGrid + rggbIndices[mColorOrder][2] * width * height;
    bLscGrid = srcLscGrid + rggbIndices[mColorOrder][3] * width * height;

   // Metadata spec request order [R, Geven, Godd, B]
   // the lensShading from ISP is 4 width * height block,
   // for ia_aiq_bayer_order_grbg, the four block is G, R, B, G
   rLscGridLineStart = rLscGrid;
   gEvenLscGridLineStart = gEvenLscGrid;
   gOddLscGridLineStart = gOddLscGrid;
   bLscGridLineStart = bLscGrid;
   for (uint16_t i = 0; i < height; i++) {
       for (uint16_t j = 0; j < width; j++) {
           /* AIQ internal format is in 4.12 fixed point */
           *dstGrid++ = (float)(*(rLscGridLineStart + j)) / 4096;
           *dstGrid++ = (float)(*(gEvenLscGridLineStart + j)) / 4096;
           *dstGrid++ = (float)(*(gOddLscGridLineStart + j)) / 4096;
           *dstGrid++ = (float)(*(bLscGridLineStart + j)) / 4096;
      }
      rLscGridLineStart += width;
      gEvenLscGridLineStart += width;
      gOddLscGridLineStart += width;
      bLscGridLineStart += width;
   }

   return NO_ERROR;

}
status_t Aiq3ARequestManager::init()
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    status_t status = NO_ERROR;

    mSensorDelay = mSensorCI->getExposureDelay();
    LOG2("mSensorDelay = %d", mSensorDelay);

    {
    // Initialize the request queues
    Mutex::Autolock l(mPendingQueueLock);
    mPendingRequestsQueue.clear();
    mPendingRequestsQueue.setCapacity(mSensorDelay + 2);
    mReqIdFrameDone = -1;
    mOneRequestLeftWaitingForEmbedded = false;
    }
    mReqQForShutter.clear();

    mFrameSkips = mSensorCI->getFrameSkip();

    // Get active array size
    int count = 0;
    int32_t* activeArraySize = (int32_t*)MetadataHelper::getMetadataValues(
                                     mStaticMeta,
                                     ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                                     TYPE_INT32,
                                     &count);
    if (count == 4 && activeArraySize != NULL) {
        mActiveArraySize[0] = activeArraySize[0];
        mActiveArraySize[1] = activeArraySize[1];
        mActiveArraySize[2] = activeArraySize[2];
        mActiveArraySize[3] = activeArraySize[3];
    }
    LOG2("%s: mActiveArraySize: [%d, %d, %d, %d]", __FUNCTION__,
        mActiveArraySize[0], mActiveArraySize[1],
        mActiveArraySize[2], mActiveArraySize[3]);

    // Get sensitivity range
    int32_t* sensitivityRange = (int32_t*)MetadataHelper::getMetadataValues(
                                     mStaticMeta,
                                     ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
                                     TYPE_INT32,
                                     &count);
    if (count == 2 && sensitivityRange != NULL) {
        mMinSensitivity = sensitivityRange[0];
        mMaxSensitivity = sensitivityRange[1];
    }
    LOG2("%s: sensitivity range: [%d, %d]", __FUNCTION__,
        mMinSensitivity, mMaxSensitivity);

    // Get exposure time range
    int64_t* exposureTimeRange = (int64_t*)MetadataHelper::getMetadataValues(
                                     mStaticMeta,
                                     ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
                                     TYPE_INT64,
                                     &count);
    if (count == 2 && exposureTimeRange != NULL) {
        mMinExposureTime = exposureTimeRange[0];
        mMaxExposureTime = exposureTimeRange[1];
    }
    LOG2("%s: exposure time range: [%lld, %lld]", __FUNCTION__,
        mMinExposureTime, mMaxExposureTime);

    if (count == 2 && mLensShadingMapSize != NULL) {
        LOG2("LSC width:%d height:%d", mLensShadingMapSize[0], mLensShadingMapSize[1]);
        unsigned int size = 4 * mLensShadingMapSize[0] * mLensShadingMapSize[1];
        mLscGrid = new float[size];
        if (mLscGrid != NULL) {
            for (unsigned int i = 0; i < size; i++)
                 mLscGrid[i] = 1.0;
        } else {
            return NO_MEMORY;
        }
        mAutoISPMetadata.lscGridInRGGB = new float[size];
        if (mAutoISPMetadata.lscGridInRGGB != NULL) {
            mAutoISPMetadata.lscWidth = mLensShadingMapSize[0];
            mAutoISPMetadata.lscHeight = mLensShadingMapSize[1];
            // init the map with 1.0
            memcpy(mAutoISPMetadata.lscGridInRGGB, mLscGrid, size * sizeof(float));
        } else {
            return NO_MEMORY;
        }
        mLensShadingInfo.lsc_grids = new uint16_t [size];
        if (mLensShadingInfo.lsc_grids != NULL) {
            mLensShadingInfo.grid_width = mLensShadingMapSize[0];
            mLensShadingInfo.grid_height = mLensShadingMapSize[1];
        } else {
            return NO_MEMORY;
        }
    }

    if (PlatformData::AiqConfig) {
        ia_cmc_t *cmc = PlatformData::AiqConfig.getCMCHandler();
        if (cmc && cmc->cmc_general_data) {
            mColorOrder = cmc->cmc_general_data->color_order;
        }
    }
    LOG2("%s: color order is %d", __FUNCTION__, mColorOrder);

    // Get the max tonemap points
    int32_t* maxCurvePoints = (int32_t*)MetadataHelper::getMetadataValues(
                                     mStaticMeta,
                                     ANDROID_TONEMAP_MAX_CURVE_POINTS,
                                     TYPE_INT32,
                                     &count);
    if (count == 1)
        mMaxTonemapCurvePoint = (uint32_t)maxCurvePoints[0];
    LOG2("%s: mMaxTonemapCurvePoint is %d", __FUNCTION__, mMaxTonemapCurvePoint);

    mAutoISPMetadata.rGammaLut = new float[mMaxTonemapCurvePoint * 2 + 1];
    mAutoISPMetadata.gGammaLut = new float[mMaxTonemapCurvePoint * 2 + 1];
    mAutoISPMetadata.bGammaLut = new float[mMaxTonemapCurvePoint * 2 + 1];

    if ((mAutoISPMetadata.rGammaLut == NULL) || (mAutoISPMetadata.gGammaLut == NULL)
        || (mAutoISPMetadata.bGammaLut == NULL))
        return NO_MEMORY;

    return status;
}

status_t Aiq3ARequestManager::processManualExposure(aaa_request &new_request)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    status_t status = NO_ERROR;
    const CameraMetadata settings = ReadRequest(new_request.request).mMembers.mSettings;
    camera_metadata_ro_entry entry;
    int32_t iso = -1;
    int64_t exposureTimeUs = -1;

    entry = settings.find(ANDROID_CONTROL_AE_MODE);
    if (entry.count == 1) {
        if (entry.data.u8[0] == ANDROID_CONTROL_AE_MODE_OFF)
            new_request.manualAeMode = true;
    }
    if (new_request.manualControlMode)
        new_request.manualAeMode = true;

    if (new_request.manualAeMode) {
        entry = settings.find(ANDROID_SENSOR_SENSITIVITY);
        if (entry.count == 1)
            iso = (int32_t)entry.data.i32[0];

        entry = settings.find(ANDROID_SENSOR_EXPOSURE_TIME);
        if (entry.count == 1)
            exposureTimeUs = (int64_t)(entry.data.i64[0] / 1000);

        LOG2("%s: [req %d, exp_id --] App set: exposure time = %fms, iso = %d",
                    __FUNCTION__, new_request.mReqId, exposureTimeUs/1000.0f, iso);
        unsigned short coarse_integration_time;
        unsigned short fine_integration_time;
        int analog_gain_code;
        int digital_gain_code;
        m3AControls->convertGeneric2SensorAE(iso, exposureTimeUs,
                            &coarse_integration_time, &fine_integration_time,
                            &analog_gain_code, &digital_gain_code);

        struct atomisp_exposure exposure;
        exposure.integration_time[0] = coarse_integration_time;
        exposure.integration_time[1] = fine_integration_time;
        exposure.gain[0] = analog_gain_code;
        exposure.gain[1] = digital_gain_code;
        mSensorCI->storeRequestExposure(new_request.mReqId, true, &exposure);
        LOG2("%s: [req %d, exp_id --] Converted: {coarse=%d, analog_gain=%d, digital_gain=%d}",
                    __FUNCTION__, new_request.mReqId,
                    exposure.integration_time[0],
                    exposure.gain[0], exposure.gain[1]);
    } else {
        mSensorCI->storeRequestExposure(new_request.mReqId, false, NULL);
        LOG2("%s: store the auto exposure for req %d", __FUNCTION__, new_request.mReqId);
    }

    return status;
}

status_t Aiq3ARequestManager::processManualColorCorrection(aaa_request &new_request)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    const CameraMetadata settings = ReadRequest(new_request.request).mMembers.mSettings;
    camera_metadata_ro_entry entry;

    entry = settings.find(ANDROID_COLOR_CORRECTION_MODE);
    if (entry.count == 1) {
        uint8_t value = entry.data.u8[0];

        LOG2("@%s: Color correction mode = %d", __FUNCTION__, value);
        if (value == ANDROID_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX) {
            // Use the android.colorCorrection.transform matrix and
            // android.colorCorrection.gains to do color conversion
            float colorTransform[9] = {1.0, 0.0, 0.0,
                                       0.0, 1.0, 0.0,
                                       0.0, 0.0, 1.0};
            entry = settings.find(ANDROID_COLOR_CORRECTION_TRANSFORM);
            if (entry.count == 9) {
                for (unsigned int i = 0; i < entry.count; i++) {
                    int32_t numerator = entry.data.r[i].numerator;
                    int32_t denominator = entry.data.r[i].denominator;
                    colorTransform[i] = (float) numerator / denominator;
                }
                LOG2("%s, req_id %d: colorTransform:", __FUNCTION__,
                                                        new_request.mReqId);
                LOG2("%s,    [%f, %f, %f", __FUNCTION__,
                    colorTransform[0], colorTransform[1], colorTransform[2]);
                LOG2("%s,     %f, %f, %f", __FUNCTION__,
                    colorTransform[3], colorTransform[4], colorTransform[5]);
                LOG2("%s,     %f, %f, %f]", __FUNCTION__,
                    colorTransform[6], colorTransform[7], colorTransform[8]);
            }

            float colorGain[4] = {1.0, 1.0, 1.0, 1.0};
            entry = settings.find(ANDROID_COLOR_CORRECTION_GAINS);
            if (entry.count == 4) {
                // The color gains from application is in order of RGGB
                // While the order for aiq is GRBG
                colorGain[0] = entry.data.f[1];
                colorGain[1] = entry.data.f[0];
                colorGain[2] = entry.data.f[3];
                colorGain[3] = entry.data.f[2];
                LOG2("%s: colorGain:[%f,%f,%f,%f]", __FUNCTION__,
                    colorGain[0], colorGain[1], colorGain[2], colorGain[3]);
            }

            // Store the manual color correction
            color_params * manualColor = (color_params *) malloc(sizeof(color_params));
            memcpy(manualColor->colorTransform, colorTransform, sizeof(float)*9);
            memcpy(manualColor->colorGain, colorGain, sizeof(float)*4);

            new_request.manualColor = manualColor;
        }
    }

    return status;
}

status_t Aiq3ARequestManager::processManualTonemap(aaa_request &new_request)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    const CameraMetadata settings = ReadRequest(new_request.request).mMembers.mSettings;
    camera_metadata_ro_entry entry;

    entry = settings.find(ANDROID_TONEMAP_MODE);
    if (entry.count == 1 &&
        entry.data.u8[0] == ANDROID_TONEMAP_MODE_CONTRAST_CURVE) {
        new_request.tonemapContrastCurve = true;

        status = getTonemapCurve(settings, ANDROID_TONEMAP_CURVE_RED,
                                 &(new_request.rGammaLutSize),
                                 &(new_request.rGammaLut));
        if (status == NO_ERROR)
            status |= getTonemapCurve(settings, ANDROID_TONEMAP_CURVE_GREEN,
                                 &(new_request.gGammaLutSize),
                                 &(new_request.gGammaLut));
        if (status == NO_ERROR)
            status |= getTonemapCurve(settings, ANDROID_TONEMAP_CURVE_BLUE,
                                 &(new_request.bGammaLutSize),
                                 &(new_request.bGammaLut));
    }

    if (status != NO_ERROR) {
       delete new_request.rGammaLut;
       new_request.rGammaLut = NULL;

       delete new_request.gGammaLut;
       new_request.gGammaLut = NULL;

       delete new_request.bGammaLut;
       new_request.bGammaLut = NULL;
    }


    return status;
}

status_t Aiq3ARequestManager::getTonemapCurve(const CameraMetadata settings,
                                                    unsigned int tag,
                                                    unsigned int *gammaLutSize,
                                                    unsigned short ** gammaLut)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = NO_ERROR;
    camera_metadata_ro_entry entry;

    entry = settings.find(tag);
    if (entry.count < 2) {
        LOGE("%s: tonemap curve %d is not abailable", __FUNCTION__, tag);
        return UNKNOWN_ERROR;
    }

    *gammaLutSize = entry.count/2;
    *gammaLut = new unsigned short[entry.count/2];
    if (*gammaLut == NULL)
            return NO_MEMORY;

    // Range gamma lut [0, 255] in AIQ, android definition for the metadata is [0,1],
    // so need mapping
    for (unsigned int i = 0; i < entry.count / 2; i++) {
        (*gammaLut)[i] = entry.data.f[i*2 + 1] * GAMMA_LUT_UPPER_BOUND;
    }

    return status;
}

status_t Aiq3ARequestManager::applyIspSettings(aaa_request &aaaRequest)
{
    LOG2("@%s, line:%d <request %d>", __FUNCTION__, __LINE__, aaaRequest.mReqId);
    status_t ret = NO_ERROR;

    if (!mGlobalInited) {
        LOGW("%s: global parameters has not beed initialized!", __FUNCTION__);
        m3AControls->getParameters(&mISPParams, &mExposure);
        mGlobalInited = true;
    }

    if (aaaRequest.applied) {
        LOG2("%s: request %d is already applied", __FUNCTION__, aaaRequest.mReqId);
        return ret;
    }

    if (!aaaRequest.newSettings && !mISP->isIspPerframeSettingsEnabled()) {
        LOG2("%s: request %d is with no settings, isp supported? %d", __FUNCTION__,
            aaaRequest.mReqId, mISP->isIspPerframeSettingsEnabled());
        return ret;
    }

    // Merge the manual isp settings if any.
    if (aaaRequest.manualColor != NULL && mISPParams.pa_results != NULL) {
        if (mISPParams.pa_results->color_conversion_matrix != NULL)
            memcpy(mISPParams.pa_results->color_conversion_matrix,
               aaaRequest.manualColor->colorTransform, sizeof(float)*9);
        if (mISPParams.pa_results->color_gains != NULL)
            memcpy(mISPParams.pa_results->color_gains,
               aaaRequest.manualColor->colorGain, sizeof(float)*4);
    }
    if (aaaRequest.tonemapContrastCurve) {
        // TODO: Currently aiq supports only one lut_size which is used for
        // all three color channels.
        mISPParams.gbce_results.gamma_lut_size = aaaRequest.rGammaLutSize;
        memcpy(mISPParams.gbce_results.r_gamma_lut, aaaRequest.rGammaLut,
               aaaRequest.rGammaLutSize * sizeof(unsigned short));
        memcpy(mISPParams.gbce_results.g_gamma_lut, aaaRequest.gGammaLut,
               aaaRequest.rGammaLutSize * sizeof(unsigned short));
        memcpy(mISPParams.gbce_results.b_gamma_lut, aaaRequest.bGammaLut,
               aaaRequest.rGammaLutSize * sizeof(unsigned short));
    }

    // parameters with isp_config_id = 0 will be considered as global parameters.
    // But request_id starts from 0. So we give offset of 1 here.
    mISPParams.isp_config_id = aaaRequest.mReqId + 1;
    mISPParams.availableStreams = aaaRequest.availableStreams;

    mISPParams.noiseReduction = aaaRequest.noiseReduction;
    mISPParams.edgeEnhancement = aaaRequest.edgeEnhancement;
    mISPParams.lensShading = aaaRequest.lensShading;

    // Set the isp settings
    ret = mISP->setIspParameters(&mISPParams);

    return ret;
}

bool Aiq3ARequestManager::getAndFillSensorMetadata(
                              /* out */
                              Camera3Request * request,
                              /* in */
                              const aaa_request & aaaRequest)
{
    LOG2("@%s", __FUNCTION__);

    // Get the decoded sensor medatada
    sensor_metadata sensor;
    CLEAR(sensor);

    sensor.sceneFlicker = m3AControls->getAeFlickerMode();
    if (mISP->isSensorEmbeddedMetaDataSupported()) {
        ia_aiq_exposure_sensor_parameters sensorExposure;
        ia_aiq_exposure_parameters exposure;

        CLEAR(sensorExposure);
        CLEAR(exposure);
        status_t status = mISP->getDecodedExposureParams(&sensorExposure, &exposure, aaaRequest.expId);
        if (status == NO_ERROR) {
            LOG2("Got sensor exposure params: {coarse=%d, analog_gain=%d, digital_gain=%d}",
                        sensorExposure.coarse_integration_time,
                        sensorExposure.analog_gain_code_global,
                        sensorExposure.digital_gain_global);
            LOG2("%s: [req %d, exp_id %d] Get sensor exposure params: {coarse=%d, analog_gain=%d, digital_gain=%d}",
                        __FUNCTION__, aaaRequest.mReqId, aaaRequest.expId,
                        sensorExposure.coarse_integration_time,
                        sensorExposure.analog_gain_code_global,
                        sensorExposure.digital_gain_global);
        } else {
            LOGE("%s: use stored value for %d/%d", __FUNCTION__,
                aaaRequest.mReqId, aaaRequest.expId);
        }

        // Convert to generic exposure time
        if (exposure.exposure_time_us == 0 || exposure.iso == 0) {
            m3AControls->convertSensor2ExposureTime(sensorExposure.coarse_integration_time,
                                    sensorExposure.fine_integration_time,
                                    sensorExposure.analog_gain_code_global,
                                    sensorExposure.digital_gain_global,
                                    &exposure.exposure_time_us, &exposure.iso);
        }
        LOG2("%s: [req %d, exp_id %d] Get exposure time: %fms", __FUNCTION__,
            aaaRequest.mReqId, aaaRequest.expId, exposure.exposure_time_us/1000.0f);

        sensor.iso = m3AControls->mapRealISO2UIISO(exposure.iso);
        sensor.exposureTimeNs = (int64_t)exposure.exposure_time_us * 1000;
        sensor.frameDuration = 0;
    } else {
        status_t status = NO_ERROR;

        sensor.iso = 100;
        CLEAR(mAaaMetadata);
        status = mISP->getMetadata(aaaRequest.mReqId, &mAaaMetadata);
        if (status != NO_ERROR) {
            LOGD("fail to get the metadata, use last one");
            sensor.exposureTimeNs = mLastExposureTimeNs;
        } else {
            int expTime = (int) (pow(2.0, -mAaaMetadata.aeConfig.aecApexTv * (1.52587890625e-005)) * 10000);
            sensor.exposureTimeNs = expTime * 100000;
            mLastExposureTimeNs = sensor.exposureTimeNs;
        }
    }

    ReadWriteRequest rwRequest(request);

    // If sensor embedded metadata is not available, do not update
    if (sensor.iso != 0) {
        // Make sure that sensitivity is in the available range
        sensor.iso = sensor.iso < mMinSensitivity ? mMinSensitivity : \
                       sensor.iso > mMaxSensitivity ? mMaxSensitivity : sensor.iso;
        rwRequest.mMembers.mSettings.update(ANDROID_SENSOR_SENSITIVITY,
                                            &sensor.iso, 1);
    }
    if (sensor.exposureTimeNs != 0) {
        // Make sure that exposure time is in the available range
        sensor.exposureTimeNs = sensor.exposureTimeNs < mMinExposureTime ? \
                                    mMinExposureTime : \
                                    sensor.exposureTimeNs > mMaxExposureTime ? \
                                    mMaxExposureTime : sensor.exposureTimeNs;
        rwRequest.mMembers.mSettings.update(ANDROID_SENSOR_EXPOSURE_TIME,
                                            &sensor.exposureTimeNs, 1);
    }

    uint8_t sceneFlickerMode;
    if (sensor.sceneFlicker == CAM_AE_FLICKER_MODE_50HZ)
        sceneFlickerMode = ANDROID_STATISTICS_SCENE_FLICKER_50HZ;
    else if (sensor.sceneFlicker == CAM_AE_FLICKER_MODE_60HZ)
        sceneFlickerMode = ANDROID_STATISTICS_SCENE_FLICKER_60HZ;
    else
        sceneFlickerMode = ANDROID_STATISTICS_SCENE_FLICKER_NONE;

    rwRequest.mMembers.mSettings.update(ANDROID_STATISTICS_SCENE_FLICKER,
                                        &sceneFlickerMode, 1);
    mLastSensorMetadata = sensor;

    const camera_metadata_t * meta = rwRequest.mMembers.mSettings.getAndLock();
    rwRequest.mMembers.mSettings.unlock(meta);
    return true;
}

void Aiq3ARequestManager::update3aRegion(
                        CameraMetadata & settings,
                        int32_t *aeRegions)
{
    int32_t cropRegions[5] = {0};

    camera_metadata_entry cropEntry = settings.find(ANDROID_SCALER_CROP_REGION);
    if (cropEntry.count < 4) {
        return;
    }

    cropRegions[0] = cropEntry.data.i32[0];
    cropRegions[1] = cropEntry.data.i32[1];
    cropRegions[2] = cropEntry.data.i32[0] + cropEntry.data.i32[2];
    cropRegions[3] = cropEntry.data.i32[1] + cropEntry.data.i32[3];

    // the 3Aregion should intersect with Cropregion
    if ((aeRegions[0] < cropRegions[2]) &&
        (cropRegions[0] < aeRegions[2]) &&
        (aeRegions[1] < cropRegions[3]) &&
        (cropRegions[1] < aeRegions[3])) {
            aeRegions[0] = MAX(aeRegions[0], cropRegions[0]);
            aeRegions[1] = MAX(aeRegions[1], cropRegions[1]);
            aeRegions[2] = MIN(aeRegions[2], cropRegions[2]);
            aeRegions[3] = MIN(aeRegions[3], cropRegions[3]);
    }
}

status_t Aiq3ARequestManager::fill3aRegion(
                            CameraMetadata & settings,
                            const aaa_request & aaaRequest,
                            unsigned int tag)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    UNUSED(aaaRequest);
    int32_t aeRegions[5] = {0};

    // If ae region not available, fill active array size as the default value
    camera_metadata_entry entry = settings.find(tag);
    if (entry.count < 4) {
        aeRegions[0] = mActiveArraySize[0];
        aeRegions[1] = mActiveArraySize[1];
        aeRegions[2] = mActiveArraySize[0] + mActiveArraySize[2];
        aeRegions[3] = mActiveArraySize[1] + mActiveArraySize[3];
        aeRegions[4] = 1;

        settings.update(tag, aeRegions, 5);
        LOG2("Set ae region to [%d, %d, %d, %d]",
            aeRegions[0], aeRegions[1], aeRegions[2], aeRegions[3]);
    } else {
        aeRegions[0] = entry.data.i32[0];
        aeRegions[1] = entry.data.i32[1];
        aeRegions[2] = entry.data.i32[2];
        aeRegions[3] = entry.data.i32[3];
        // fill the default weight value as 1
        aeRegions[4] = 1;

        if (entry.count == 5) {
            aeRegions[4] = entry.data.i32[4];
        }

        update3aRegion(settings, aeRegions);
        settings.update(tag, aeRegions, 5);
        LOG2("Set ae region to [%d,%d,%d,%d,%d]",
            aeRegions[0], aeRegions[1], aeRegions[2], aeRegions[3], aeRegions[4]);
    }

    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillScalerCropRegion(CameraMetadata & settings,
                                                 const aaa_request & aaaRequest)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    UNUSED(aaaRequest);
    // If crop region not available, fill active array size as the default value
    camera_metadata_entry entry = settings.find(ANDROID_SCALER_CROP_REGION);
    if (entry.count < 4 || entry.data.i32[2] == 0) {
        int32_t cropRegions[4];
        cropRegions[0] = mActiveArraySize[0];
        cropRegions[1] = mActiveArraySize[1];
        cropRegions[2] = mActiveArraySize[2];
        cropRegions[3] = mActiveArraySize[3];

        settings.update(ANDROID_SCALER_CROP_REGION,
                                            cropRegions, 4);
        LOG2("Set scale crop region to [%d, %d, %d, %d]",
            cropRegions[0], cropRegions[1], cropRegions[2], cropRegions[3]);
    }

    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillColorCorrection(CameraMetadata & settings,
                                                const aaa_request & aaaRequest)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    UNUSED(aaaRequest);
    camera_metadata_entry entry;
    entry = settings.find(ANDROID_COLOR_CORRECTION_MODE);
    // set color correction mode as the fast mode when awb mode is not off
    // and no color correction mode setting
    if (entry.count != 1 && m3AControls->getAwbMode() != CAM_AWB_MODE_OFF) {
        LOG2("@%s: no Color correction mode setting, but awb mode is non-off", __FUNCTION__);
        uint8_t color_correction_mode = ANDROID_COLOR_CORRECTION_ABERRATION_MODE_FAST;
        settings.update(ANDROID_COLOR_CORRECTION_MODE, &color_correction_mode, 1);
    }
    camera_metadata_rational_t transform_matrix[9];
    for (int i = 0; i < 9; i++) {
        transform_matrix[i].numerator =
            (int32_t)(mAutoISPMetadata.colorTransform[i] * COLOR_TRANSFORM_PRECISION);
        transform_matrix[i].denominator = COLOR_TRANSFORM_PRECISION;
    }

    // The color gains from application is in order of RGGB
    // While the order for aiq is GRBG
    float colorGainRGGB[4];
    colorGainRGGB[0] = mAutoISPMetadata.colorGain[1];
    colorGainRGGB[1] = mAutoISPMetadata.colorGain[0];
    colorGainRGGB[2] = mAutoISPMetadata.colorGain[3];
    colorGainRGGB[3] = mAutoISPMetadata.colorGain[2];

    // If applicate does not set the color correction settings
    // fill the auto settings and update the metadata.
    entry = settings.find(ANDROID_COLOR_CORRECTION_TRANSFORM);
    if (entry.count < 9)
        settings.update(ANDROID_COLOR_CORRECTION_TRANSFORM,
                        (camera_metadata_rational_t*)transform_matrix, 9);
    entry = settings.find(ANDROID_COLOR_CORRECTION_GAINS);
    if (entry.count < 4)
        settings.update(ANDROID_COLOR_CORRECTION_GAINS, colorGainRGGB, 4);

    // Update the prediced color correction metadata
    settings.update(ANDROID_STATISTICS_PREDICTED_COLOR_TRANSFORM,
                    (camera_metadata_rational_t*)transform_matrix, 9);
    settings.update(ANDROID_STATISTICS_PREDICTED_COLOR_GAINS, colorGainRGGB, 4);

    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillLensShadingMap(CameraMetadata & settings,
                                               const aaa_request & aaaRequest)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);
    if (mLensShadingMapMode == false)
        return NO_ERROR;

    if (mAutoISPMetadata.lscGridInRGGB == NULL) {
        LOGW("%s: there is no lsc data, and this shouldn't happen", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    unsigned int size = 4 * mAutoISPMetadata.lscWidth * mAutoISPMetadata.lscHeight;
    LOG2("%s: gridWidth %d, gridHeight %d", __FUNCTION__,
                    mAutoISPMetadata.lscWidth, mAutoISPMetadata.lscHeight);

    if (aaaRequest.lensShading) {
        settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP,
                                            mAutoISPMetadata.lscGridInRGGB,
                                            size);
        settings.update(ANDROID_STATISTICS_LENS_SHADING_CORRECTION_MAP,
                                            mAutoISPMetadata.lscGridInRGGB,
                                            size);
    } else if(mLscGrid != NULL){
        settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP,
                       mLscGrid, size);
        settings.update(ANDROID_STATISTICS_LENS_SHADING_CORRECTION_MAP,
                       mLscGrid, size);
    }

#ifdef INTEL_ENHANCEMENT
    settings.update(ANDROID_STATISTICS_PREDICTED_LENS_SHADING_MAP,
                   mAutoISPMetadata.lscGridInRGGB,
                   size);
#endif
    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillTonemapCurve(CameraMetadata & settings,
                                             const aaa_request & aaaRequest)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    if (aaaRequest.tonemapContrastCurve)
        return NO_ERROR;

    if (mAutoISPMetadata.gammaLutSize != 0) {
        settings.update(ANDROID_TONEMAP_CURVE_RED,
                        mAutoISPMetadata.rGammaLut,
                        mAutoISPMetadata.gammaLutSize * 2);
        settings.update(ANDROID_TONEMAP_CURVE_GREEN,
                        mAutoISPMetadata.gGammaLut,
                        mAutoISPMetadata.gammaLutSize * 2);
        settings.update(ANDROID_TONEMAP_CURVE_BLUE,
                        mAutoISPMetadata.bGammaLut,
                        mAutoISPMetadata.gammaLutSize * 2);
    }

    return NO_ERROR;
}

status_t Aiq3ARequestManager::getRollingShutterSkew(void)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    struct atomisp_sensor_mode_data sensor_mode_data;
    int64_t rollingShutterSkew = 0;

    if(mSensorCI && mSensorCI->getModeInfo(&sensor_mode_data) < 0) {
        sensor_mode_data.line_length_pck = 0;
        sensor_mode_data.output_height = 0;
        sensor_mode_data.vt_pix_clk_freq_mhz = 0;
    }
    // calculate rolling shutter skew
    if (sensor_mode_data.output_height >= 1 && sensor_mode_data.vt_pix_clk_freq_mhz != 0)
        rollingShutterSkew = (unsigned int)(sensor_mode_data.line_length_pck * 1000000000LL *
                                 (sensor_mode_data.output_height - 1) / sensor_mode_data.vt_pix_clk_freq_mhz);

    Mutex::Autolock l(mRollingShutterSkewLock);
    mRollingShutterSkew = rollingShutterSkew;

    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillRollingShutterSkew(CameraMetadata & settings)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    Mutex::Autolock l(mRollingShutterSkewLock);
    settings.update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW,
                    &mRollingShutterSkew, 1);

    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillLensDynamic(CameraMetadata & settings)
{
    float currentfocusdistance;
    int aaaFocusDistance;
    uint8_t opticalstabilizationmode;
    camera_metadata_entry entry;
    float aperture;
    float focal_length;
    float hyper_focal;
    float focusrange[2];
    uint8_t lens_state;

    entry = settings.find(ANDROID_LENS_APERTURE);
    if (entry.count == 1)
        aperture = (float)entry.data.f[0];
    else
        aperture = 2.53;

    entry = settings.find(ANDROID_LENS_FOCAL_LENGTH);
    if (entry.count == 1)
        focal_length = (float)entry.data.f[0];
    else
        focal_length = 4.01;

    //calcalate focus range
    aaaFocusDistance = m3AControls->getCurrentFocusDistance();
    if (aaaFocusDistance == 0) {
        return NO_ERROR;
    }
    currentfocusdistance = aaaFocusDistance;

    opticalstabilizationmode = m3AControls->getOpticalStabilizationMode();

    if (mFreFocusDistance == currentfocusdistance)
        lens_state = 0;
    else {
        lens_state = 1;
        mFreFocusDistance = currentfocusdistance;
    }

    hyper_focal = focal_length * focal_length * 1000 / aperture / mPixelSize / 2;

    focusrange[0] = currentfocusdistance * ( hyper_focal - focal_length ) /
                      ( hyper_focal + currentfocusdistance - 2 * focal_length );
    if (currentfocusdistance < hyper_focal)
        focusrange[1] = currentfocusdistance * ( hyper_focal - focal_length ) /
                        ( hyper_focal - currentfocusdistance - 2 * focal_length );
    else
        focusrange[1] = 100000000;

    LOG2("@%s, line:%d  current distance near/current/far = %.2f, %.2f, %.2f, optical stabilization mode=%d, mPixelSize= %f lens state= %d", __FUNCTION__, __LINE__, focusrange[0], currentfocusdistance, focusrange[1], opticalstabilizationmode, mPixelSize, lens_state);

    float curr_focus_distance_diopter = 1000/currentfocusdistance;
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &curr_focus_distance_diopter, 1);
    settings.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &opticalstabilizationmode, 1);

    float focusrange_diopter[2];
    // the range is less than 1mm, it's invalid
    if (focusrange[0] > 1.0 && focusrange[1] > 1.0) {
        focusrange_diopter[0] = 1000/focusrange[0];
        focusrange_diopter[1] = 1000/focusrange[1];
        settings.update(ANDROID_LENS_FOCUS_RANGE, focusrange_diopter, 2);
    }
    settings.update(ANDROID_LENS_STATE, &lens_state, 1);
    if (mLensAperture != NULL)
        settings.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES, mLensAperture, 1);
    if (mLensFilterDensity != NULL)
        settings.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES, mLensFilterDensity, 1);

    return NO_ERROR;
}

status_t Aiq3ARequestManager::fillFrameduration(CameraMetadata & settings)
{
    int64_t frameduration;
    /*
     * WorkAround: CTS case android.hardware.camera2.cts.CaptureRequestTest#testNoiseReductionModeControl
     * use the fpsRange to calculate frameDurationRange, this may be incorrect. Sometimes, frameduration
     * is out of the incorrect range, so HAL needs to adjust frameduration into the range.
     * Please remove this workaround after CTS case error is confirmed and modified.
     */
    frameduration = mSensorCI->getFrameDuration();

    LOG2("@%s, line:%d frameduration =  %lld", __FUNCTION__, __LINE__, frameduration);
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &frameduration, 1);
    return NO_ERROR;
}

// update black level lock to OFF if the black level is changed
void  Aiq3ARequestManager::fillBlackLevelLock(CameraMetadata & settings,
                                             const aaa_request & aaaRequest)
{
    LOG2("@%s, line:%d", __FUNCTION__, __LINE__);

    if (!aaaRequest.blackLevellock) {
        mBlackLevelLockOn = false;
        return;
    }

    if (!mBlackLevelLockOn) {
        // if it is the first time to enable black level lock, save the black level
        mColorChannel.cc1 = mAutoISPMetadata.colorChannel.cc1;
        mColorChannel.cc2 = mAutoISPMetadata.colorChannel.cc2;
        mColorChannel.cc3 = mAutoISPMetadata.colorChannel.cc3;
        mColorChannel.cc4 = mAutoISPMetadata.colorChannel.cc4;
        mBlackLevelLockOn = true;
    }
    if (mColorChannel.cc1 != mAutoISPMetadata.colorChannel.cc1
        || mColorChannel.cc2 != mAutoISPMetadata.colorChannel.cc2
        || mColorChannel.cc3 != mAutoISPMetadata.colorChannel.cc3
        || mColorChannel.cc4 != mAutoISPMetadata.colorChannel.cc4) {
        LOG2("black level changed, set the lock to OFF");
        uint8_t blackLevelLock = ANDROID_BLACK_LEVEL_LOCK_OFF;
        settings.update(ANDROID_BLACK_LEVEL_LOCK,
                        &blackLevelLock, 1);
    }
}


void Aiq3ARequestManager::dump()
{
    Mutex::Autolock l(mPendingQueueLock);
    LOG2("### %d request pending, req_id:", mPendingRequestsQueue.size());
    for (unsigned int i = 0; i < mPendingRequestsQueue.size(); i++)
        LOG2("###       No.%d (state? %d) (applied? %d)",
             mPendingRequestsQueue.itemAt(i).mReqId,
             mPendingRequestsQueue.itemAt(i).state,
             mPendingRequestsQueue.itemAt(i).applied);

    return;
}

void Aiq3ARequestManager::aaaRequestClear(aaa_request & aaaRequest)
{
    if (aaaRequest.manualColor != NULL) {
        free(aaaRequest.manualColor);
        aaaRequest.manualColor = NULL;
    }
    aaaRequest.request = NULL;

    delete aaaRequest.rGammaLut;
    aaaRequest.rGammaLut = NULL;

    delete aaaRequest.gGammaLut;
    aaaRequest.gGammaLut = NULL;

    delete aaaRequest.bGammaLut;
    aaaRequest.bGammaLut = NULL;

    return;
}

void Aiq3ARequestManager::resultError(Camera3Request* request)
{
    uint8_t aeState = 0;
    uint8_t awbState = 0;
    uint8_t afState = 0;
    int32_t aeTriggerId = 0;
    int32_t afTriggerId = 0;

    ReadWriteRequest rwRequest(request);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AE_STATE, &aeState, 1);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AE_PRECAPTURE_ID,
                                        &aeTriggerId, 1);

    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AWB_STATE, &awbState,
                                        1);

    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AF_STATE, &afState, 1);
    rwRequest.mMembers.mSettings.update(ANDROID_CONTROL_AF_TRIGGER_ID,
                                        &afTriggerId, 1);

}

} /* namespace camera2 */
} /* namespace android */
