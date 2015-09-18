/*
 * Copyright (c) 2014 Intel Corporation.
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

#define LOG_TAG "Camera_ControlUnit"

#include "LogHelper.h"
#include "GuiLogTraces.h"
#include "ControlUnit.h"
#include "CSS2600CameraHw.h"
#include "CaptureUnit.h"
#include "CameraStream.h"
#include "IPU4CameraCapInfo.h"

namespace android {
namespace camera2 {

ControlUnit::ControlUnit(ProcessingUnit *thePU, CaptureUnit *theCU, int cameraId) :
        mProcessingUnit(thePU),
        mCaptureUnit(theCU),
        mCameraId(cameraId),
        mCurrentAeTriggerId(0),
        mForceAeLock(false),
        mFirstRequest(true),
        mThreadRunning(false),
        mMessageQueue("CtrlUnitThread", static_cast<int>(MESSAGE_ID_MAX))

{
    CLEAR(mSensorDescriptor);
}

status_t
ControlUnit::init()
{
    LOG1("@%s", __FUNCTION__);
    mMessageThread = new MessageThread(this, "CtrlUThread");
    if (mMessageThread == NULL) {
        LOGE("Error creating Control Unit Thread in init");
        return NO_MEMORY;
    }

    const IPU4CameraCapInfo * cap = getIPU4CameraCapInfo(mCameraId);
    if (!cap || cap->sensorType() == SENSOR_TYPE_RAW) {
        m3aWrapper = new Intel3aPlus();
    } else {
        LOGE("SoC camera 3A control missing");
    }
    if (m3aWrapper == NULL) {
        LOGE("Error creating 3A control");
        return UNKNOWN_ERROR;
    }

    if (m3aWrapper->initAIQ() != NO_ERROR) {
        LOGE("Error initializing 3A control");
        return UNKNOWN_ERROR;
    }

    /**
     * init the pool of Request State structs
     */
    mRequestStatePool.init(MAX_REQUEST_IN_PROCESS_NUM);

    return NO_ERROR;
}

void ControlUnit::RequestCtrlState::init(Camera3Request *req)
{
    request = req;
    controlModeAuto = true;
    aiqInputParams.init();
    captureSettings.aiqResults.init();
    afState = ALGORITHM_NOT_CONFIG;
    aeState = ALGORITHM_NOT_CONFIG;
    awbState = ALGORITHM_NOT_CONFIG;
    ctrlUnitResult = request->getPartialResultBuffer(CONTROL_UNIT_PARTIAL_RESULT);

    /**
     * Apparently we need to have this tags in the results
     */
    const CameraMetadata* settings = request->getSettings();
    camera_metadata_ro_entry entry;
    entry = settings->find(ANDROID_REQUEST_ID);
    ctrlUnitResult->update(ANDROID_REQUEST_ID, entry.data.i32,
                                               entry.count);
    entry = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
    ctrlUnitResult->update(ANDROID_CONTROL_CAPTURE_INTENT, entry.data.u8,
                                                           entry.count);
    entry = settings->find(ANDROID_REQUEST_FRAME_COUNT);
    ctrlUnitResult->update(ANDROID_REQUEST_FRAME_COUNT, entry.data.i32,
                                                        entry.count);
}

ControlUnit::~ControlUnit()
{
    LOG1("@%s", __FUNCTION__);
    status_t status;
    status = requestExitAndWait();

    if (status == NO_ERROR && mMessageThread != NULL) {
        mMessageThread.clear();
        mMessageThread = NULL;
    }

    if (m3aWrapper) {
        m3aWrapper->deinitAIQ();
        delete m3aWrapper;
        m3aWrapper = NULL;
    }
}

status_t
ControlUnit::configStreamsDone(bool configChanged)
{
    LOG1("@%s: config changed: %d", __FUNCTION__, configChanged);

    if (configChanged) {
        mFirstRequest = true;
        mWaitingForStats.clear();
        mWaitingForImage.clear();
        mAvailableStatistics.clear();
        /**
         * reset the value to start
         */
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mWaitingForStats", 0);
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mWaitingForImage", 0);
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mAvailableStatistics", 0);
    }

    return NO_ERROR;
}

status_t
ControlUnit::requestExitAndWait(void)
{
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    return mMessageQueue.send(&msg, MESSAGE_ID_EXIT);
}

status_t
ControlUnit::handleMessageExit(void)
{
    mThreadRunning = false;
    return NO_ERROR;
}

status_t
ControlUnit::processRequest(Camera3Request* request)
{
    Message msg;
    status_t status = NO_ERROR;
    RequestCtrlState *state;
    LOG2("@%s: id %d", __FUNCTION__,  request->getId());

    status = mRequestStatePool.acquireItem(&state);
    if (status != NO_ERROR) {
        LOGE("Failed to acquire free request state struct - BUG");
        /**
         * This should not happen since AAL is holding clients to send more
         * requests than we can take
         */
        return UNKNOWN_ERROR;
    }

    state->init(request);

    msg.id = MESSAGE_ID_NEW_REQUEST;
    msg.data.request.state = state;
    status =  mMessageQueue.send(&msg);
    return status;
}

status_t
ControlUnit::handleNewRequest(Message &msg)
{
    status_t status = NO_ERROR;
    int index;
    int reqId = msg.data.request.state->request->getId();
    HAL_GUITRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE, reqId);

    RequestCtrlState *reqState = msg.data.request.state;
    /**
     * PHASE 1: Process the settings
     */
    const CameraMetadata *reqSettings = reqState->request->getSettings();
    status = processRequestSettings(*reqSettings, *reqState);
    if (status != NO_ERROR) {
        LOGE("Could not process all settings, reporting request as invalid");
        // notifyError()
    }
    /**
     * PHASE2: if first request just run 2A with cold values and send the image
     * for capture
     * in other captures check if we already received the statistics and pass
     * them to 3A library before running 3A.
     */
    if (mFirstRequest) {
        mFirstRequest = false;
        run2AandCapture(reqState);
    } else if (statisticsAvailable(reqState, &index)) {
        // TODO: retrieve the stats before removing
        mAvailableStatistics.removeItemsAt(index);
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mAvailableStatistics",
                                   mAvailableStatistics.size());
        /*
         * TODO: pass the statistics to 3A library
         *  m3aWrapper->setStatistics(&iaAiqStats);
         *
         */
        run2AandCapture(reqState);
    } else {
        /**
         * if no statistics are available then Q the request, when the next
         * set of statistics arrive they will be used for this request
         * This will happen in handleNewStat()
         */
        mWaitingForStats.add(reqId, reqState);
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mWaitingForStats",
                                                       mWaitingForStats.size());
    }

    return status;
}

/**
 * processRequestSettings
 *
 * Analyze the request control metadata tags and prepare the configuration for
 * the AIQ algorithm to run.
 * \param settings [IN] settings from the request
 * \param reqAiqCfg [OUT] AIQ configuration
 */
status_t
ControlUnit::processRequestSettings(const CameraMetadata  &settings,
                                    RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = OK;
    unsigned int tag = ANDROID_CONTROL_MODE;
    camera_metadata_ro_entry_t entry = settings.find(tag);
    if (entry.count == 1) {
        if (entry.data.u8[0] == ANDROID_CONTROL_MODE_OFF) {
            reqAiqCfg.controlModeAuto = false;
        } else {
            reqAiqCfg.controlModeAuto = true;
        }
    }

    if ((status = processAeSettings(settings, reqAiqCfg)) != OK)
        return status;
    reqAiqCfg.aeState = ALGORITHM_CONFIGURED;

    if ((status = processAfSettings(settings, reqAiqCfg)) != OK)
        return status;
    reqAiqCfg.afState = ALGORITHM_CONFIGURED;

    if ((status = processAwbSettings(settings, reqAiqCfg)) != OK)
        return status;
    reqAiqCfg.awbState = ALGORITHM_CONFIGURED;
    return status;
}

status_t
ControlUnit::processAeSettings(const CameraMetadata&  settings,
                               RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = OK;
    m3aWrapper->fillAeInputParams(&settings, reqAiqCfg.aiqInputParams,
                                  &mSensorDescriptor, reqAiqCfg.controlModeAuto);

    processAeStateMachine(settings, reqAiqCfg);
    // Todo: Move to the correct place
    // Fake value to keep the framework happy for now
    int32_t aePreCaptureID = 0;
    reqAiqCfg.ctrlUnitResult->update(ANDROID_CONTROL_AE_PRECAPTURE_ID,
                                     &aePreCaptureID, 1);

    return status;
}

status_t
ControlUnit::processAfSettings(const CameraMetadata  &settings,
                               RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    // we should find values from settings and update the result
    UNUSED(settings);
    status_t status = OK;
    camera_metadata_ro_entry_t entry;

    // Fake values to keep the framework happy
    // TODO: move this to the state machine handlers
    uint8_t afState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
    reqAiqCfg.ctrlUnitResult->update(ANDROID_CONTROL_AF_STATE, &afState, 1);
    int32_t afTrigerID = 0;
    reqAiqCfg.ctrlUnitResult->update(ANDROID_CONTROL_AF_TRIGGER_ID,
                                     &afTrigerID, 1);

    // Carry forward AF Mode from settings to result
    entry = settings.find(ANDROID_CONTROL_AF_MODE);
    if (entry.count == 1) {
        uint8_t aeMode = entry.data.u8[0];
        reqAiqCfg.ctrlUnitResult->update(ANDROID_CONTROL_AF_MODE,
                                         entry.data.u8, entry.count);
    }
    return status;
}

status_t
ControlUnit::processAwbSettings(const CameraMetadata  &settings,
                                RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(settings);
    status_t status = OK;
    camera_metadata_ro_entry_t entry;

    // Fake values to keep the framework happy
    // TODO: move this to the state machine handlers
    uint8_t awbState = ANDROID_CONTROL_AWB_STATE_CONVERGED;
    reqAiqCfg.ctrlUnitResult->update(ANDROID_CONTROL_AWB_STATE, &awbState, 1);

    // Carry forward AWB Mode from settings to result
    entry = settings.find(ANDROID_CONTROL_AWB_MODE);
    if (entry.count == 1) {
        uint8_t aeMode = entry.data.u8[0];
        reqAiqCfg.ctrlUnitResult->update(ANDROID_CONTROL_AWB_MODE,
                                         entry.data.u8, entry.count);
    }
    return status;
}

void
ControlUnit::writeSensorMetadata(RequestCtrlState* reqState)
{
    LOG2("@%s", __FUNCTION__);
    camera_metadata_ro_entry_t entry;
    const CameraMetadata *settings = reqState->request->getSettings();

    // Calculate frame duration from sensor descriptor
    entry = settings->find(ANDROID_SENSOR_FRAME_DURATION);
    if (entry.count == 1) {
        int64_t frameDuration = mSensorDescriptor.pixel_periods_per_line *
                                mSensorDescriptor.line_periods_per_field /
                                mSensorDescriptor.pixel_clock_freq_mhz;
        reqState->ctrlUnitResult->update(ANDROID_SENSOR_FRAME_DURATION,
                                         &frameDuration, entry.count);
    }

    // Carry forward exposure time from settings to result
    // TODO: Use the actual value when available
    entry = settings->find(ANDROID_SENSOR_EXPOSURE_TIME);
    if (entry.count == 1) {
        int64_t exposureTime = entry.data.i64[0];
        reqState->ctrlUnitResult->update(ANDROID_SENSOR_EXPOSURE_TIME,
                                         &exposureTime, entry.count);
    }

    // Carry forward sensor sensitivity from settings to result
    // TODO: Use the actual value when available
    entry = settings->find(ANDROID_SENSOR_SENSITIVITY);
    if (entry.count == 1) {
        int32_t sensitivity = entry.data.i32[0];
        reqState->ctrlUnitResult->update(ANDROID_SENSOR_SENSITIVITY,
                                         &sensitivity, entry.count);
    }
}

void
ControlUnit::writeAwbMetadata(RequestCtrlState* reqState)
{
    LOG2("@%s", __FUNCTION__);
    camera_metadata_ro_entry_t entry;
    const CameraMetadata *settings = reqState->request->getSettings();

    // Carry forward color correction gains from settings to result
    // TODO: Use the actual value when available
    entry = settings->find(ANDROID_COLOR_CORRECTION_GAINS);
    if (entry.count == 4) {
        const float *colorGains = entry.data.f;
        reqState->ctrlUnitResult->update(ANDROID_COLOR_CORRECTION_GAINS,
                                         colorGains, entry.count);
    }

    // Carry forward color transform matrix from settings to result
    // TODO: Use the actual value when available
    entry = settings->find(ANDROID_COLOR_CORRECTION_TRANSFORM);
    if (entry.count == 9) {
        const camera_metadata_rational_t *transformMatrix = entry.data.r;
        reqState->ctrlUnitResult->update(ANDROID_COLOR_CORRECTION_TRANSFORM,
                                         transformMatrix, entry.count);
    }
}

status_t
ControlUnit::handleNewImage(Message &msg)
{
    RequestCtrlState *reqState;
    int reqId = msg.data.image.buffer->buf->requestId();
    HAL_GUITRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE, reqId);

    int index = mWaitingForImage.indexOfKey(reqId);
    if (index == NAME_NOT_FOUND) {
        LOGE("unexpected new image received %d -find the bug", reqId);
        return UNKNOWN_ERROR;
    }

    reqState = mWaitingForImage.valueAt(index);
    mWaitingForImage.removeItem(reqId);
    HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mWaitingForImage",
                                                    mWaitingForImage.size());

    // HACK
    // tmp hack. This should be updated when AF stats arrive
    float focus = 1.2;
    reqState->ctrlUnitResult->update(ANDROID_LENS_FOCUS_DISTANCE, &focus, 1);
    // End HACK

    /**
     * Complete to update the results metadata and return it
     */
    reqState->request->mCallback->metadataDone(reqState->request,
                                               CONTROL_UNIT_PARTIAL_RESULT);

    /**
     * AWB re-run for this frame with the statistics for this frame. (see method
     * handleNewStat())
     * Use the results from the AWB and send that to the ProcessingUnit to
     * complete the request. This is to generate the rest of the processed
     * buffers.
     * For now those AWB are NULL
     */
    CaptureBuffer *buf = msg.data.image.buffer;
    mProcessingUnit->completeRequest(reqState->request, buf, NULL, 0);

    /* Recycle request state object  we are done with this request */
    mRequestStatePool.releaseItem(reqState);

    return NO_ERROR;
}

/**
 * AE state handling
 */
void ControlUnit::processAeStateMachine(const CameraMetadata &settings,
                                        RequestCtrlState &reqState)
{
    LOG2("@%s", __FUNCTION__);
    camera_metadata_ro_entry_t entry;

    uint8_t androidAeState= ANDROID_CONTROL_AE_STATE_INACTIVE;
    uint8_t reqTriggerState = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    {
        entry = settings.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
        if (entry.count == 1) {
            reqTriggerState = entry.data.u8[0];
        }
    }

    //get AE mode
    entry = settings.find(ANDROID_CONTROL_AE_MODE);
    if (entry.count == 1) {
        uint8_t aeMode = entry.data.u8[0];
        if (aeMode == ANDROID_CONTROL_AE_MODE_OFF) {
            /*                            mode = AE_MODE_OFF / AWB mode not AUTO
             *| state              | trans. cause  | new state          | notes            |
             *+--------------------+---------------+--------------------+------------------+
             *| INACTIVE           |               |                    | AE/AWB disabled  |
             *+--------------------+---------------+--------------------+-----------------*/
            androidAeState = ANDROID_CONTROL_AE_STATE_INACTIVE;
            LOG2("@%s AE_STATE_INACTIVE (ae on manual or not set)", __FUNCTION__);
        } else if (reqTriggerState == ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
            /*+--------------------+---------------+--------------------+------------------+
             *| All AE states      | PRECAPTURE_   | PRECAPTURE         | Start precapture |
             *|                    | START         |                    | sequence         |
             *+--------------------+---------------+--------------------+-----------------*/
            LOG1("@%s state nr. %d -> PRECAPTURE", __FUNCTION__, androidAeState);
            androidAeState = ANDROID_CONTROL_AE_STATE_PRECAPTURE;
        } else {
        // ToDo: IntelAIQ is not used for storing the states in BXT. Until the
        //       state can be checked from AE results, there is no point in
        //       running processAutoAeStates
            //        processAutoAeStates();

            // ToDo: remove once proper solution exists
            androidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
        }
        reqState.ctrlUnitResult->update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    }
    reqState.ctrlUnitResult->update(ANDROID_CONTROL_AE_STATE,
                                    &androidAeState, 1);
}


void ControlUnit::processAutoAeStates(camera_metadata_enum_android_control_ae_state androidAeState)
{
    LOG2("@%s", __FUNCTION__);

    switch (androidAeState) {
        /* look at the state transition table in camera3.h */
        case ANDROID_CONTROL_AE_STATE_INACTIVE:
            if (mForceAeLock) {
                androidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s INACTIVE -> LOCKED", __FUNCTION__);
            } else {
                androidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                LOG1("@%s INACTIVE -> SEARCHING", __FUNCTION__);
            }
            break;
        case ANDROID_CONTROL_AE_STATE_SEARCHING:
            if (mForceAeLock) {
                androidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s SEARCHING -> LOCKED", __FUNCTION__);
                //ToDo: get aeConverged value
/*
            } else if (m3AControl->isAeConverged()) {

                if (m3AControl->getAeFlashNecessary()) {
                    androidAeState = ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED;
                    LOG1("@%s SEARCHING -> FLASH_REQUIRED", __FUNCTION__);
                } else {
                    androidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                    LOG1("@%s SEARCHING -> CONVERGED", __FUNCTION__);
                }*/
            }
            break;
        case ANDROID_CONTROL_AE_STATE_CONVERGED:
            if (mForceAeLock) {
                androidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s CONVERGED -> LOCKED", __FUNCTION__);
            } /*else if (!m3AControl->isAeConverged()) {
                androidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                LOG1("@%s CONVERGED -> SEARCHING", __FUNCTION__);
            }*/
            break;
        case ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED:
            if (mForceAeLock) {
                androidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s FLASH_REQUIRED -> LOCKED", __FUNCTION__);
                //ToDo: implement else once aeConverged available
            } /*else if (!m3AControl->isAeConverged()) {
                androidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                LOG1("@%s FLASH_REQUIRED -> SEARCHING", __FUNCTION__);
            }*/
            break;
        case ANDROID_CONTROL_AE_STATE_LOCKED:
            //ToDo: implement else once aeConverged available
            /*if (!mForceAeLock) {
                if (m3AControl->isAeConverged()) {
                    if (m3AControl->getAeFlashNecessary()) {
                        androidAeState =
                                ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED;
                        LOG1("@%s LOCKED -> FLASH_REQUIRED", __FUNCTION__);
                    } else {
                        androidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                        LOG1("@%s LOCKED -> CONVERGED", __FUNCTION__);
                    }
                } else {
                    androidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                    LOG1("@%s LOCKED -> SEARCHING", __FUNCTION__);
                }
            }*/
            break;
        case ANDROID_CONTROL_AE_STATE_PRECAPTURE:
            /*ToDo: implement else once aeConverged available
            if (m3AControl->isAeConverged()) {
                if (mForceAeLock) {
                    androidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                    LOG1("@%s PRECAPTURE -> LOCKED", __FUNCTION__);
                } else {
                    androidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                    LOG1("@%s PRECAPTURE -> CONVERGED", __FUNCTION__);
                }
            }*/
            break;
        default:
            LOGE("unknown AE state!");
            break;
    }
}

status_t
ControlUnit::handleNewStat(Message &msg)
{
    RequestCtrlState *reqState = NULL;
    int reqId = msg.data.image.buffer->buf->requestId();
    HAL_GUITRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE, reqId);

    /**
     * we will use the statistics arrived now for the next request
     * we add the initial delay N. The Capture Unit send us N statistics
     * at the beginning without a frame to start the flow
     */
    int nextReqId = reqId + mCaptureUnit->getSensorSettingDelay() + 1;
    LOG2("@%s reqId: %d, mWaitingForStats size: %d", __FUNCTION__, reqId,
                                                    mWaitingForStats.size());

    int index = mWaitingForStats.indexOfKey(nextReqId);
    if (index != NAME_NOT_FOUND) {
        LOG2("Found nextReqId %d in waiting for stats", nextReqId);
        /**
         * There is are a next request where we can use the stats to
         * calculate the capture settings
         */
        reqState = mWaitingForStats.valueAt(index);
        mWaitingForStats.removeItemsAt(index);
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mWaitingForStats",
                                                       mWaitingForStats.size());
        // retrieve the statistics buffer from the message
        // pass them to the  3A and run 2A
        // m3aWrapper->setStatistics(&iaAiqStats);
        run2AandCapture(reqState);


    } else {
        LOG2("nextReqId %d not arrived yet. Store to available stats",
                nextReqId);
        /**
         * No next request available,
         * Store statistics for this request for later use
         */
        mAvailableStatistics.add(reqId, msg.data.stats.aaStats);
        HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mAvailableStatistics",
                           mAvailableStatistics.size());
    }

   /* Lets calculate AWB for the request  that are waiting for Image
    * the results of this will be used to complete the processing
    **/
    index = mWaitingForImage.indexOfKey(reqId);
    if (index != NAME_NOT_FOUND) {
        reqState = mWaitingForImage.valueAt(index);
        // runAWB(reqState);
        // results will be stored in the request state ready for the time
        // when the Image comes.
        writeAwbMetadata(reqState);

        // Write sensor metadata to the results
        // TODO: Do this in handleNewSensorMetadata
        writeSensorMetadata(reqState);
    }

    return NO_ERROR;
}

status_t
ControlUnit::handleNewShutter(Message &msg)
{
    RequestCtrlState *reqState = NULL;
    int reqId = msg.data.shutter.requestId;
    HAL_GUITRACE_CALL(CAMERA_DEBUG_LOG_REQ_STATE, reqId);

    int index = mWaitingForImage.indexOfKey(reqId);
    if (index == NAME_NOT_FOUND) {
        LOGE("unexpected shutter event received for request %d - find the bug", reqId);
        return UNKNOWN_ERROR;
    }
    reqState = mWaitingForImage.valueAt(index);

    int64_t ts = msg.data.shutter.tv_sec * 1000000000; // seconds to nanoseconds
    ts += msg.data.shutter.tv_usec * 1000; // microseconds to nanoseconds

    reqState->ctrlUnitResult->update(ANDROID_SENSOR_TIMESTAMP, &ts, 1);
    reqState->request->mCallback->shutterDone(reqState->request, ts);

    return NO_ERROR;
}

/**
 * run2AandCapture
 *
 * Runs AE and AWB for a request and submits the request for capture
 * together with the capture settings obtained after running these 2A algorithms
 *
 *\param [IN] reqState: Pointer to the request control structure to process
 *\return NO_ERROR
 */
status_t
ControlUnit::run2AandCapture(RequestCtrlState *reqState)
{
    status_t status = NO_ERROR;
    int reqId = reqState->request->getId();
    m3aWrapper->runAe(&reqState->aiqInputParams.aeInputParams,
                     &reqState->captureSettings.aiqResults.aeResults);
    reqState->aeState = ALGORITHM_RUN;
    /**
     * TODO: Add here run AWB when we are ready. We will use the results
     * to select the parameters for the ISYS
     */
    status = mCaptureUnit->doCapture(reqState->request,
                                     reqState->captureSettings);
    if (CC_UNLIKELY(status != NO_ERROR)) {
        LOGE("Failed to issue capture request for id %d",
                reqState->request->getId());
    }
    mWaitingForImage.add(reqId, reqState);
    HAL_GUITRACE_VALUE(CAMERA_DEBUG_LOG_REQ_STATE, "mWaitingForImage",
                                                    mWaitingForImage.size());
    return status;
}

status_t
ControlUnit::handleNewSensorMetadata()
{
    LOG2("@%s", __FUNCTION__);
    return NO_ERROR;
}

status_t
ControlUnit::handleNewSensorDescriptor()
{
    LOG1("@%s", __FUNCTION__);
    // ToDo: Allocate statistics here
    return mCaptureUnit->getSensorData(&mSensorDescriptor);
}

void
ControlUnit::messageThreadLoop()
{
    LOG1("@%s - Start", __FUNCTION__);

    mThreadRunning = true;
    while (mThreadRunning) {
        status_t status = NO_ERROR;

        Message msg;
        mMessageQueue.receive(&msg);
        switch (msg.id) {
        case MESSAGE_ID_EXIT:
            status = handleMessageExit();
            break;
        case MESSAGE_ID_NEW_REQUEST:
            status = handleNewRequest(msg);
            break;

        case MESSAGE_ID_NEW_IMAGE:
            status = handleNewImage(msg);
            break;
        case MESSAGE_ID_NEW_2A_STAT:
            if (handleNewStat(msg) != NO_ERROR)
                LOGE("Error handling new stats");
            break;
        case MESSAGE_ID_NEW_SENSOR_METADATA:
            if (handleNewSensorMetadata() != NO_ERROR)
                LOGE("Error handling new sensor metadata");
            break;
        case MESSAGE_ID_NEW_SENSOR_DESCRIPTOR:
            if (handleNewSensorDescriptor() != NO_ERROR)
                LOGE("Error getting sensor descriptor");
            break;
        case MESSAGE_ID_NEW_SHUTTER:
            status = handleNewShutter(msg);
            break;
        default:
            LOGE("ERROR @%s: Unknow message %d", __FUNCTION__, msg.id);
            status = BAD_VALUE;
            break;
        }
        if (status != NO_ERROR)
            LOGE("error %d in handling message: %d", status,
                 static_cast<int>(msg.id));
        mMessageQueue.reply(msg.id, status);
    }

    LOG1("%s: Exit", __FUNCTION__);
}

bool
ControlUnit::notifyCaptureEvent(CaptureMessage *captureMsg)
{
    LOG2("@%s", __FUNCTION__);

    if (captureMsg == NULL) {
        return false;
    }

    if (captureMsg->id == CAPTURE_MESSAGE_ID_ERROR) {
        // handle capture error
        return true;
    }

    Message msg;
    switch (captureMsg->data.event.type) {
        case CAPTURE_EVENT_RAW_BAYER:
            msg.id = MESSAGE_ID_NEW_IMAGE;
            msg.data.image.frame_number = captureMsg->data.event.buffer->buf->requestId();
            msg.data.image.buffer = captureMsg->data.event.buffer;
            mMessageQueue.send(&msg);
            break;
        case CAPTURE_EVENT_NEW_SENSOR_DESCRIPTOR:
            msg.id = MESSAGE_ID_NEW_SENSOR_DESCRIPTOR;
            mMessageQueue.send(&msg);
            break;
        case CAPTURE_EVENT_2A_STATISTICS:
            msg.id = MESSAGE_ID_NEW_2A_STAT;
            if (captureMsg->data.event.buffer->buf == NULL) {
                LOGE("captureMsg->data.event.buffer->buf == NULL");
                return false;
            } else {
                msg.data.image.buffer = captureMsg->data.event.buffer;
                mMessageQueue.send(&msg);
                break;
            }
        case CAPTURE_EVENT_SHUTTER:
            msg.id = MESSAGE_ID_NEW_SHUTTER;
            msg.data.shutter.requestId = captureMsg->data.event.buffer->buf->requestId();
            msg.data.shutter.tv_sec = captureMsg->data.event.timestamp.tv_sec;
            msg.data.shutter.tv_usec = captureMsg->data.event.timestamp.tv_usec;
            mMessageQueue.send(&msg);
            break;
        default:
            LOGW("Unsupported Capture event ");
            break;
    }

    return true;
}

/**
 * statisticsAvailable
 *
 * Checks whether the statistics for this request are available
 * if so it returns true and fills the index in the array
 * The request id is delayed by N
 * where N is the number of statistics that the capture unit sends at start up
 * without image frame. This is done to cope with the inherent delay in sensor
 * setting application.
 *
 * \param reqState [IN]: request to query
 * \param index [OUT]: index on the array if available
 */
bool
ControlUnit::statisticsAvailable(RequestCtrlState* reqState, int* index)
{
    int sensorDelay = mCaptureUnit->getSensorSettingDelay();
    int adjustedReqId = reqState->request->getId() -sensorDelay;
    *index = mAvailableStatistics.indexOfKey(adjustedReqId);

    return (*index == NAME_NOT_FOUND)? false:true;
}

};  // namespace camera2
};  // namespace android

