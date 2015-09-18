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
#include "ControlUnit.h"
#include "CIFCameraHw.h"
#include "CaptureUnit.h"
#include "CameraStream.h"

namespace android {
namespace camera2 {

ControlUnit::ControlUnit(CaptureUnit *theCU, int cameraId) :
        mJpegMaker(cameraId),
        mCaptureUnit(theCU),
        mCameraId(cameraId),
        mCurrentAeTriggerId(0),
        mForceAeLock(false),
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

    m3aWrapper = new Intel3aPlus();
    if (m3aWrapper == NULL) {
        LOGE("Error creating 3A control");
        return UNKNOWN_ERROR;
    }

    status_t status = mJpegMaker.init();
    if (status != OK) {
        ALOGE("Couldn't init jpeg maker");
        return status;
    } else
        LOG1("Successfully initialized jpeg maker");

//    if (m3aWrapper->initAIQ() != NO_ERROR) {
//        LOGE("Error initializing 3A control");
//        return UNKNOWN_ERROR;
//    }

    /**
     * init the pool of Request State structs
     */
    mRequestStatePool.init(MAX_REQUEST_IN_PROCESS_NUM);

    return NO_ERROR;
}

void ControlUnit::RequestCtrlState::init()
{
    controlModeAuto = true;
    aiqInputParams.init();
    captureSettings.aiqResults.init();
    afState = ALGORITHM_NOT_CONFIG;
    aeState = ALGORITHM_NOT_CONFIG;
    awbState = ALGORITHM_NOT_CONFIG;
    outputBuffersDone = 0;
}

status_t ControlUnit::deinit()
{
    LOG1("@%s", __FUNCTION__);
    status_t status;
    status = requestExitAndWait();

    if (status == NO_ERROR && mMessageThread != NULL) {
        mMessageThread.clear();
        mMessageThread = NULL;
    }

    if (m3aWrapper) {
//        m3aWrapper->deinitAIQ();
        delete m3aWrapper;
        m3aWrapper = NULL;
    }

    return status;
}

ControlUnit::~ControlUnit()
{
    LOG1("@%s", __FUNCTION__);
}

status_t
ControlUnit::requestExitAndWait(void)
{
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    // tell thread to exit
    // send message asynchronously
    mMessageQueue.send(&msg, MESSAGE_ID_EXIT);

    // propagate call to msg thread
    return mMessageThread->requestExitAndWait();
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

    // ItemPool acquireItem is thread-safe so touching mRequestStatePool is safe
    status = mRequestStatePool.acquireItem(&state);
    if (status != NO_ERROR) {
        LOGE("Failed to acquire free request state struct - BUG");
        /**
         * This should not happen since AAL is holding clients to send more
         * requests than we can take
         */
        return UNKNOWN_ERROR;
    }

    state->request = request;
    state->init();

    msg.id = MESSAGE_ID_NEW_REQUEST;
    msg.data.request.state = state;
    status =  mMessageQueue.send(&msg);
    return status;
}

status_t
ControlUnit::handleNewRequest(Message &msg)
{
    status_t status = NO_ERROR;
    int reqId = msg.data.request.state->request->getId();
    LOG2("@%s:id %d", __FUNCTION__,  reqId);

    RequestCtrlState *reqState = msg.data.request.state;
    ReadWriteRequest rwRequest(reqState->request);
    /**
     * PHASE 1: Process the settings
     */
    CameraMetadata &reqSettings = rwRequest.mMembers.mSettings;
    status = processRequestSettings(reqSettings, *reqState);
    if (status != NO_ERROR) {
        LOGE("Could not process all settings, reporting request as invalid");
        // notifyError()
    }

    /**
     * PHASE 2: run Algorithms if needed
     * if the control is manual or if the pipeline is not full
     */
    if (reqState->controlModeAuto == false ||
        mWaitingForCapture.size() < DEFAULT_PIPELINE_DEPTH) {
        if (mWaitingForCapture.size() < DEFAULT_PIPELINE_DEPTH) {
            LOG1("pipeline not full yet %d/%d, filling up",
                    mWaitingForCapture.size(), DEFAULT_PIPELINE_DEPTH);
            reqState->aeState = ALGORITHM_READY;
        }

        // run in manual
//        m3aWrapper->runAe(&reqState->aiqInputParams.aeInputParams,
//                         &reqState->captureSettings.aiqResults.aeResults);
        reqState->aeState = ALGORITHM_RUN;
    }

    /**
     * PHASE 3 send to capture unit if ready or store it in the queue of request
     * waiting for statistics
     */
    if (reqState->aeState == ALGORITHM_RUN) {
        //FIXME: next line is to remove when AE is working fine
        // it s now a flag to not apply exp parameter to sensor when AE
        reqState->captureSettings.aeEnabled = reqState->controlModeAuto;
        mCaptureUnit->doCapture(reqState->request, reqState->captureSettings);
        mWaitingForCapture.add(reqId, reqState);
    } else {
        /**
         * Store the AIQ configuration in the Queue to be applied later.
         */
        mPendingRequests.add(reqId, reqState);
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
ControlUnit::processRequestSettings(CameraMetadata  &settings,
                                    RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = OK;
    unsigned int tag = ANDROID_CONTROL_MODE;
    camera_metadata_entry entry = settings.find(tag);
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
ControlUnit::processAeSettings(CameraMetadata&  settings,
                               RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    status_t status = OK;

    m3aWrapper->fillAeInputParams(&settings, reqAiqCfg.aiqInputParams,
                                  &mSensorDescriptor, reqAiqCfg.controlModeAuto);

    processAeStateMachine(settings);
    // Todo: Move to the correct place
    // Fake value to keep the framework happy for now
    int32_t aePreCaptureID = 0;
    settings.update(ANDROID_CONTROL_AE_PRECAPTURE_ID, &aePreCaptureID, 1);

    // TODO: Set real value for frame duration
    int64_t frameDuration = 33333333;
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);

    return status;
}

status_t
ControlUnit::processAfSettings(CameraMetadata  &settings,
                               RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(reqAiqCfg);
    status_t status = OK;

    // Fake values to keep the framework happy
    // TODO: move this to the state machine handlers
    uint8_t afState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
    settings.update(ANDROID_CONTROL_AF_STATE, &afState, 1);
    int32_t afTrigerID = 0;
    settings.update(ANDROID_CONTROL_AF_TRIGGER_ID, &afTrigerID, 1);

    return status;
}

status_t
ControlUnit::processAwbSettings(CameraMetadata  &settings,
                                RequestCtrlState &reqAiqCfg)
{
    LOG2("@%s", __FUNCTION__);
    UNUSED(reqAiqCfg);
    status_t status = OK;

    // Fake values to keep the framework happy
    // TODO: move this to the state machine handlers
    uint8_t awbState = ANDROID_CONTROL_AWB_STATE_CONVERGED;
    settings.update(ANDROID_CONTROL_AWB_STATE, &awbState, 1);

    return status;
}

status_t ControlUnit::handleJpeg(Camera3Request *request, CaptureBuffer *buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = OK;

    ImgEncoder::EncodePackage package;
    package.jpegOut = NULL;
    package.main = buffer->buf;
    package.thumb = NULL;
    package.jpegDQTAddr = (unsigned char *)package.main->data() +  JPEG_DATA_START_OFFSET;
    package.jpegSize = buffer->v4l2Buf.bytesused;
    package.settings = &(ReadRequest(request).mMembers.mSettings);
    package.padExif = true;
    sp<ExifMetaData> metadata = new ExifMetaData;

    // hack for CTS TODO proper time from sensor data
    metadata->aeConfig = new SensorAeConfig();
    CLEAR(*metadata->aeConfig);
    metadata->aeConfig->expTime = 1;
    metadata->aeConfig->aecApexTv = 1;

    // get metadata info from platform and 3A control
    // TODO figure out how to get accurate metadata from HW and 3a.

    status = mJpegMaker.setupExifWithMetaData(package, metadata);
    mJpegMaker.makeJpegInPlace(package);

    return status;
}

status_t
ControlUnit::handleNewImage(Message &msg)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    RequestCtrlState *reqState;
    int reqId = msg.data.image.buffer->buf->requestId();
    int index = mWaitingForCapture.indexOfKey(reqId);
    if (index == NAME_NOT_FOUND) {
        LOGE("unexpected capture request received %d-find the bug", reqId);
        return UNKNOWN_ERROR;
    }
    LOG2("@%s for req %d", __FUNCTION__, reqId);

    reqState = mWaitingForCapture.valueAt(index);

    if (msg.data.image.buffer->buf->format() == HAL_PIXEL_FORMAT_BLOB)
        handleJpeg(reqState->request, msg.data.image.buffer);

    // currently, all we know is to send the buffer straight out
    CameraStream *stream = msg.data.image.buffer->buf->getOwner();
    stream->captureDone(msg.data.image.buffer->buf, reqState->request);

    reqState->outputBuffersDone++;
    if (reqState->outputBuffersDone == reqState->request->getNumberOutputBufs()) {
        mWaitingForCapture.removeItemsAt(index, 1);

        /**
         * PHASE 1: Get and run AWB for the capture request we just got the buffer
         */
        int64_t fakets = systemTime();
        { // scope of the locked object
            ReadWriteRequest rwReq(reqState->request);
            // TODO: this should come from the CaptureUnit as a separate event
            rwReq.mMembers.mSettings.update(ANDROID_SENSOR_TIMESTAMP, &fakets, 1);

            // tmp hack. This should be updated when AF stats arrive
            float focus = 1.2;
            rwReq.mMembers.mSettings.update(ANDROID_LENS_FOCUS_DISTANCE, &focus, 1);

            /* TODO Set correct real information */
            rwReq.mMembers.mSettings.update(ANDROID_REQUEST_PIPELINE_DEPTH, &DEFAULT_PIPELINE_DEPTH, 1);
        }

        reqState->request->mCallback->shutterDone(reqState->request, fakets);

        /**
         * Run AWB and get ISP parameters for processing unit
         * TODO: AWB run
         */

        /**
         * Return metadata
         */
        reqState->request->mCallback->metadataDone(reqState->request);

        CaptureBuffer *buf = msg.data.image.buffer;
        //mProcessingUnit->completeRequest(reqState->request, buf, NULL, 0);

        /* Recycle request state object  we are done with this request */
        mRequestStatePool.releaseItem(reqState);

        /**
         * PHASE 2:  use the statistics that just arrived and run AE for the next
         * request in the Pending Queue it should be the next one
         */
        if (!mPendingRequests.isEmpty()) {
            int index = mPendingRequests.indexOfKey(reqId+1);
            if (index == NAME_NOT_FOUND) {
                LOGE("Could not find the next capture request expecting %d -BUG",
                        reqId+1);
                return UNKNOWN_ERROR;
            }
            reqState = mPendingRequests.valueAt(index);
            mPendingRequests.removeItem(reqId+1);
            // run AE in auto

            // Send the request for capture
            mCaptureUnit->doCapture(reqState->request, reqState->captureSettings);
            mWaitingForCapture.add(reqState->request->getId(), reqState);
        }
    }

    return NO_ERROR;
}

/**
 * AE state handling
 */
void ControlUnit::processAeStateMachine(CameraMetadata &settings)
{
    LOG2("@%s", __FUNCTION__);
    camera_metadata_entry entry;

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
            androidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
        }
    }
    settings.update(ANDROID_CONTROL_AE_STATE, &androidAeState,1);
}


void ControlUnit::processAutoAeStates(camera_metadata_enum_android_control_ae_state_t androidAeState)
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
ControlUnit::handleNewStat()
{
    LOG2("@%s", __FUNCTION__);
    return NO_ERROR;
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
    return mCaptureUnit->getSensorData(&mSensorDescriptor);
}

void ControlUnit::dump(int fd)
{
    if (mCaptureUnit)
        mCaptureUnit->dump(fd);
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
        case MESSAGE_ID_NEW_STAT:
            if (handleNewStat() != NO_ERROR)
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
            msg.data.image.v4l2buffer = captureMsg->data.event.v4l2buffer;
            msg.data.image.frame_number = captureMsg->data.event.buffer->buf->requestId();
            msg.data.image.buffer = captureMsg->data.event.buffer;
            mMessageQueue.send(&msg);
            break;
        case CAPTURE_EVENT_NEW_SENSOR_DESCRIPTOR:
            msg.id = MESSAGE_ID_NEW_SENSOR_DESCRIPTOR;
            mMessageQueue.send(&msg);
            break;
        default:
            LOGW("Unsupported Capture event ");
            break;
    }

    return true;
}
}  // namespace camera2
}  // namespace android

