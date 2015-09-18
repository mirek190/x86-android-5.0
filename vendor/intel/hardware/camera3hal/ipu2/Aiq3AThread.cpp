/*
 * Copyright (c) 2013 Intel Corporation.
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

#define LOG_TAG "Camera_Aiq3AThread"

#include <time.h>
#include "LogHelper.h"

#include "IPU2CameraHw.h"
#include "Aiq3A.h"
#include "Aiq3ARawSensor.h"
#include "Aiq3AThread.h"
#include "ia_coordinate.h"

namespace android {
namespace camera2 {

#define FLASH_FRAME_TIMEOUT 5
// TODO: use values relative to real sensor timings or fps
#define STATISTICS_SYNC_DELTA_US 20000
#define STATISTICS_SYNC_DELTA_US_MAX 200000

Aiq3AThread::Aiq3AThread(IPU2CameraHw * hal, IAAACallback * callback,
                        I3AControls * aaa, HWControlGroup &hwcg,
                        CameraMetadata &staticMeta) :
    mHal(hal)
    ,mCallback(callback)
    ,m3AControls(aaa)
    ,mISP(hwcg.mIspCI)
    ,mSensorCI(hwcg.mSensorCI)
    ,mStaticMeta(staticMeta)
    ,mMessageQueue("Aiq3AThread", (int)MESSAGE_ID_MAX)
    ,mThreadRunning(false)
    ,mDvsRunning(false)
    ,mStartFlashSequence(false)
    ,mFramesTillAfComplete(0)
    ,mStillAfStatus(STILL_AF_IDLE)
    ,mFlashStage(FLASH_STAGE_NA)
    ,mFrameCountForFlashSeqence(0)
    ,mAfAssistLight(false)
    ,mAndroidAwbState(ANDROID_CONTROL_AWB_STATE_INACTIVE)
    ,mAndroidAeState(ANDROID_CONTROL_AE_STATE_INACTIVE)
    ,mPreviousFaceCount(0)
    ,mDropStats(0)
    ,m3ASetting(NULL)
    ,mFlashExposedNum(0)
{
    LOG2("@%s:", __FUNCTION__);

    if (mISP->isRAW()) {
        m3ASetting = new Aiq3ARawSensor(hwcg, staticMeta, aaa);
        if (m3ASetting == NULL) {
            LOGE("not enough memory for m3ASetting");
        }
    }

    if (m3ASetting)
        m3ASetting->init3ASetting();

    mMessageThead = new MessageThread(this, "Aiq3AThread", PRIORITY_NORMAL);

    if (mMessageThead == NULL) {
        LOGE("Error create Aiq3AThread in init");
    }

    mAiq3ARequestManager = new Aiq3ARequestManager(hwcg, aaa, staticMeta);
    if (mAiq3ARequestManager == NULL) {
        LOGE("Error create Aiq3ARequestManager in init");
    } else {
        mAiq3ARequestManager->init();
    }

    CLEAR(mFaceState);
    CLEAR(mFaceMetadata);
    mFaceState.faces = new ia_face[MAX_FACES_DETECTABLE];
    if (mFaceState.faces == NULL) {
        LOGE("not enough memory for mFaceState");
    }
    mFaceMetadata.faces = new camera_face[MAX_FACES_DETECTABLE];
    if (mFaceMetadata.faces == NULL) {
        LOGE("not enough memory for mFaceMetadata");
    }

    // Store the 3a state metadata.
    m3AStateLock.lock();
    CLEAR(m3AStateMetadata);
    m3AStateMetadata.aeState = mAndroidAeState;
    m3AStateMetadata.awbState = mAndroidAwbState;
    if (m3ASetting) {
        m3AStateMetadata.afState = m3ASetting->getAndroidAfState();
        m3AStateMetadata.aeTriggerId = m3ASetting->getCurrentAeTriggerId();
        m3AStateMetadata.afTriggerId = m3ASetting->getCurrentAfTriggerId();
    }
    m3AStateLock.unlock();

    mReportedAeState.clear();
    mReportedAeState.setCapacity(MAX_REQUEST_IN_PROCESS_NUM);
}

Aiq3AThread::~Aiq3AThread()
{
    LOG2("@%s:", __FUNCTION__);

    if (mMessageThead != NULL) {
        mMessageThead->requestExitAndWait();
        mMessageThead.clear();
        mMessageThead = NULL;
    }

    if (mAiq3ARequestManager != NULL) {
        delete mAiq3ARequestManager;
        mAiq3ARequestManager = NULL;
    }

    if (mFaceState.faces) {
        delete mFaceState.faces;
        mFaceState.faces = NULL;
    }
    if (mFaceMetadata.faces) {
        delete mFaceMetadata.faces;
        mFaceMetadata.faces = NULL;
    }

    if (m3ASetting != NULL) {
        delete m3ASetting;
        m3ASetting = NULL;
    }

    mReportedAeState.clear();
}

status_t Aiq3AThread::requestExitAndWait(void)
{
    Message msg;
    msg.id = MESSAGE_ID_EXIT;
    mMessageQueue.send(&msg);
    return true;
}

status_t Aiq3AThread::handleMessageExit(void)
{
    mThreadRunning = false;
    m3ASetting->set3ARunning(false);
    return NO_ERROR;
}

status_t Aiq3AThread::reset(IspMode mode)
{
    LOG1("%s", __FUNCTION__);
    int ret =0;
    mDropStats = mISP->getIspStatisticsSkip();
    mAiq3ARequestManager->reset(mode);
    return ret;
}

bool Aiq3AThread::isPreFlashUsed()
{
    bool used = mStartFlashSequence;
    LOG2("%s: used = %d",__FUNCTION__, used);
    return used;
}

status_t Aiq3AThread::enterFlashSequence(void)
{
    LOG2("@%s", __FUNCTION__);
    handleEnterFlashSequence();

    return NO_ERROR;
}

void Aiq3AThread::exitPreFlashSequence()
{
    LOG2("%s",__FUNCTION__);
    handleExitFlashSequence();
}

status_t Aiq3AThread::processRequest(Camera3Request* request, int availableStreams)
{
    LOG2("%s",__FUNCTION__);
    Message msg;
    int ret =0;

    mAiq3ARequestManager->storeRequestForShutter(request);
    bool needProcess = mAiq3ARequestManager->storeNewRequest(request, availableStreams);

    msg.id = MESSAGE_ID_NEW_REQUEST;
    msg.data.request.needProcess = needProcess;
    msg.request = request;
    ret =  mMessageQueue.send(&msg);
    return ret;
}

status_t Aiq3AThread::handleNewRequest(Message &msg)
{
    LOG2("###-- %s (req_id %d)", __FUNCTION__, msg.request->getId());

    mAiq3ARequestManager->handleNewRequest(msg.request->getId());
    if (msg.data.request.needProcess) {
        processSettingsForRequest(msg.request, (SettingType)(TRIGGER + SETTING));
    }

    return NO_ERROR;
}

status_t Aiq3AThread::handleEnterFlashSequence(void)
{
    LOG2("@%s", __FUNCTION__);
    mStartFlashSequence = true;

    return NO_ERROR;
}

status_t Aiq3AThread::handleExitFlashSequence(void)
{
    mStartFlashSequence = false;
    // Stop flash sequence
    if (mFlashStage != FLASH_STAGE_NA) {
        LOG1("    Stop flash sequence");
        mFlashStage = FLASH_STAGE_NA;
    }
    return NO_ERROR;
}


void Aiq3AThread::FaceDetectCallbackFor3A(ia_face_state *faceState)
{
    LOG2("@%s", __FUNCTION__);
    if (faceState == NULL || mFaceState.faces == NULL) {
        LOG1("faceMetadata =%p mFaceState.faces =%p",
              faceState, mFaceState.faces);
        return;
    }
    Mutex::Autolock lock(mFaceStateLock);
    if (faceState->num_faces > MAX_FACES_DETECTABLE) {
        LOGW("@%s: %d faces detected, limiting to %d", __FUNCTION__,
            faceState->num_faces, MAX_FACES_DETECTABLE);
         mFaceState.num_faces = MAX_FACES_DETECTABLE;
    } else {
         mFaceState.num_faces = faceState->num_faces;
    }

    if (mFaceState.num_faces > 0)
        memcpy(mFaceState.faces, faceState->faces,
               sizeof(ia_face) * mFaceState.num_faces);
}

void Aiq3AThread::FaceDetectCallbackForApp(camera_frame_metadata_t *faceMetadata)
{
    LOG2("@%s", __FUNCTION__);
    if (faceMetadata == NULL || mFaceMetadata.faces == NULL) {
        LOG1("faceMetadata =%p mFaceMetadata.faces =%p",
              faceMetadata, mFaceMetadata.faces);
        return;
    }
    Mutex::Autolock lock(mFaceMetadataLock);
    mFaceMetadata.number_of_faces = faceMetadata->number_of_faces;
    if (mFaceMetadata.number_of_faces > 0)
        memcpy(mFaceMetadata.faces, faceMetadata->faces,
               sizeof(ia_face) * mFaceMetadata.number_of_faces);
}

void Aiq3AThread::CombineFaceDetectInfo(int* faceIds, int* faceLandmarks, int* faceRect, uint8_t* faceScores, int& faceNum)
{
    Mutex::Autolock lock(mFaceMetadataLock);

    faceNum = mFaceMetadata.number_of_faces;
    if (faceNum > 0) {
        for (int i = 0; i < faceNum; i++) {
             LOG2("@%s top=%d, left=%d, right=%d, bottom=%d,score=%d", __FUNCTION__,
                              mFaceMetadata.faces[i].rect[0],
                              mFaceMetadata.faces[i].rect[1],
                              mFaceMetadata.faces[i].rect[2],
                              mFaceMetadata.faces[i].rect[3],
                              mFaceMetadata.faces[i].score);
             LOG2("@%s left_eye[0]=%d, left_eye[1]=%d, right_eye[0]=%d, right_eye[1]=%d, mouth[0]=%d, mouth[1]=%d, id=%d", __FUNCTION__,
                              mFaceMetadata.faces[i].left_eye[0],
                              mFaceMetadata.faces[i].left_eye[1],
                              mFaceMetadata.faces[i].right_eye[0],
                              mFaceMetadata.faces[i].right_eye[1],
                              mFaceMetadata.faces[i].mouth[0],
                              mFaceMetadata.faces[i].mouth[1],
                              mFaceMetadata.faces[i].id);

             faceRect[i * 4] = mFaceMetadata.faces[i].rect[0];
             faceRect[i * 4 + 1] = mFaceMetadata.faces[i].rect[1];
             faceRect[i * 4 + 2] = mFaceMetadata.faces[i].rect[2];
             faceRect[i * 4 + 3] = mFaceMetadata.faces[i].rect[3];

             faceScores[i] = mFaceMetadata.faces[i].score;
             faceIds[i] = mFaceMetadata.faces[i].id;
             faceLandmarks[i * 6] = mFaceMetadata.faces[i].left_eye[0];
             faceLandmarks[i * 6 + 1] = mFaceMetadata.faces[i].left_eye[1];
             faceLandmarks[i * 6 + 2] = mFaceMetadata.faces[i].right_eye[0];
             faceLandmarks[i * 6 + 3] = mFaceMetadata.faces[i].right_eye[1];
             faceLandmarks[i * 6 + 4] = mFaceMetadata.faces[i].mouth[0];
             faceLandmarks[i * 6 + 5] = mFaceMetadata.faces[i].mouth[1];

        }
        CLEAR(*mFaceMetadata.faces);
        mFaceMetadata.number_of_faces = 0;
    }
}


status_t Aiq3AThread::newFrame(MessageAAAFrame frame)
{
    LOG2("###== @%s exp_id %d, req_id %d, msgQsize %d timestamp %lld",
        __FUNCTION__, frame.exp_id, frame.requestId,
        mMessageQueue.size(), TIMEVAL2USECS(&frame.timestamp));

    Camera3Request* request = mAiq3ARequestManager->getNextRequest(frame.requestId + 1);
    if (request != NULL)
        processSettingsForRequest(request, TRIGGER);

    // Notify results
    writeAndNotifyResults(frame.requestId, frame.exp_id, frame.status);

    Message msg;
    msg.id = MESSAGE_ID_NEW_IMAGE;
    msg.data.frame = frame;
    return mMessageQueue.send(&msg);
}

status_t Aiq3AThread::handleNewImage(Message & msg)
{
    LOG2("###-- @%s (exp_id %d, req_id %d)", __FUNCTION__,
        msg.data.frame.exp_id, msg.data.frame.requestId);
    FrameBufferStatus status = msg.data.frame.status;
    if (m3ASetting->get3ARunning() && mStartFlashSequence)
        handleFlashSequence(msg.data.frame.timestamp, msg.data.frame.exp_id, status);

    // Update the 3a state metadata.
    processAfStateMachine();
    processAeStateMachine();
    processAwbStateMachine();

    // Store the 3a state metadata.
    m3AStateLock.lock();
    if (isReportedAeState(mAndroidAeState)) {
        storeReportedAeState(mAndroidAeState);
    }
    m3AStateMetadata.aeState = mAndroidAeState;
    m3AStateMetadata.awbState = mAndroidAwbState;
    m3AStateMetadata.afState = m3ASetting->getAndroidAfState();
    m3AStateMetadata.aeTriggerId = m3ASetting->getCurrentAeTriggerId();
    m3AStateMetadata.afTriggerId = m3ASetting->getCurrentAfTriggerId();
    m3AStateLock.unlock();

    // Process the next pending request
    Camera3Request* request = mAiq3ARequestManager->getNextRequest();
    if (request != NULL)
        processSettingsForRequest(request, SETTING);

    return NO_ERROR;
 }

status_t Aiq3AThread::writeAndNotifyResults(int request_id,
                                                 unsigned int exp_id,
                                                 FrameBufferStatus flashStatus)
{
    LOG2("@%s exp_id %d, req_id %d", __FUNCTION__, exp_id, request_id);
    m3AStateLock.lock();
    aaa_state_metadata aaaStates = m3AStateMetadata;
    aaaStates.aeState = loadReportedAeState();
    m3AStateLock.unlock();

    int faceIds[MAX_FACES_DETECTABLE];
    int faceLandmarks[6 * MAX_FACES_DETECTABLE];
    int faceRect[4 * MAX_FACES_DETECTABLE];
    uint8_t faceScores[MAX_FACES_DETECTABLE];
    int faceNum = 0;
    uint8_t aePrecaptureTrigger;
    CombineFaceDetectInfo(faceIds, faceLandmarks, faceRect, faceScores, faceNum);

    if ( mFlashStage == FLASH_STAGE_PRE_START || mFlashStage==FLASH_STAGE_PRE_PHASE1 ||
         mFlashStage==FLASH_STAGE_PRE_PHASE2 || mFlashStage==FLASH_STAGE_PRE_WAITING)
        aePrecaptureTrigger=1;
    else
        aePrecaptureTrigger=0;

    // workaround: now driver can't set correct flash status. Here we add one workaround
    // to estimate flash status for next 2 frame.
    if (FRAME_STATUS_FLASH_EXPOSED == flashStatus) {
        mFlashExposedNum = 2;
    } else if (mFlashExposedNum > 0) {
        --mFlashExposedNum;
        flashStatus = FRAME_STATUS_FLASH_EXPOSED;
    }

    /* workaround: gc2235 & gc0339 ae converged too slow in precapture.
     * Here to add one wa to change ae state to use pre-ae in continuous capture mode.
     */
    if (ANDROID_CONTROL_AE_STATE_PRECAPTURE == aaaStates.aeState
        && MODE_CONTINUOUS_CAPTURE == mHal->getMode()) {
        LOG1("@%s: ae state = %d", __FUNCTION__, aaaStates.aeState);
        aaaStates.aeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
        mAndroidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
    }

    mAiq3ARequestManager->notifyResult(request_id,
                                     exp_id,
                                     aaaStates,
                                     faceIds, faceLandmarks,
                                     faceRect, faceScores,
                                     faceNum, flashStatus,
                                     & aePrecaptureTrigger);

    return NO_ERROR;
}

status_t Aiq3AThread::processSettingsForRequest(Camera3Request* request, SettingType type)
{
    LOG2("@%s:", __FUNCTION__);
    status_t status = OK;
    // process 3A settings, if they have not been processed yet
    const camera3_capture_request *userReq = request->getUserRequest();

    // settings is NULL, if settings have not changed
    if (userReq->settings != NULL) {
        ReadWriteRequest rwRequest(request);
        if (type & TRIGGER) {
            status = m3ASetting->apply3ATrigger(rwRequest.mMembers.mSettings);
        }

        if (type & SETTING) {
            status = m3ASetting->apply3ASetting(rwRequest.mMembers.mSettings);
        }

        if (status != OK) {
            LOGE("@%s bad settings provided for 3A", __FUNCTION__);
        }
    }

    return status;
}

status_t Aiq3AThread::newStats(ICssIspListener::IspMessageEvent * statMsg)
{
    Message msg;
    msg.id = MESSAGE_ID_NEW_STAT;
    msg.data.stats.exp_id = statMsg->exp_id;
    msg.data.stats.timestamp = statMsg->timestamp;
    return mMessageQueue.send(&msg);
}

/**
 * Run 3A
 * We received message that new 3A statistics are ready
 */
status_t Aiq3AThread::handleNewStat(MessageStats &stats)
{
    LOG2("###-- @%s exp_id=%d", __FUNCTION__, stats.exp_id);
    status_t status = NO_ERROR;
    struct timeval capture_timestamp = stats.timestamp;

    // Multiple newFrames indicates we are late and 3A process is going
    // to read the statistics of the most recent frame.
    // We flush the queue.
    Vector<Message> messages;
    mMessageQueue.remove(MESSAGE_ID_NEW_STAT, &messages);
    if(!messages.isEmpty()) {
        Vector<Message>::iterator it = messages.begin();
        LOG1("%d frames in 3A process queue, handling timestamp "
                "%lld instead of %lld\n", messages.size(),
                ((long long)(it->data.stats.timestamp.tv_sec)*1000000LL +
                 (long long)(it->data.stats.timestamp.tv_usec)),
                ((long long)(capture_timestamp.tv_sec)*1000000LL +
                 (long long)(capture_timestamp.tv_usec)));
        capture_timestamp = it->data.stats.timestamp;
    }

    if (!mStartFlashSequence && m3ASetting->lowPowerMode()) {
        // Set flashmode to OFF if working in low battery mode
        m3AControls->setAeFlashMode(CAM_AE_FLASH_MODE_OFF);
    } else {
        // restore flashmode with enough battery
        m3AControls->setAeFlashMode(m3ASetting->getFlashMode());
    }

    // Do not run 3A if we are in the pre-flash sequence
    if (mFlashStage == FLASH_STAGE_NA) {
        // Get the exp_id for latest 3A statistics event.
        unsigned int exp_id = stats.exp_id;
        if (messages.size() > 0)
            exp_id = messages[0].data.stats.exp_id;
        LOG2("%s: exp_id=%d", __FUNCTION__, exp_id);

        // Run 3A statistics
        {
            Mutex::Autolock lock(mFaceStateLock);
            // Set face data to 3A only if there were detected faces and
            // avoid unnecessary setting with consecutive zero face count.
            if (mFaceState.num_faces != 0 || mPreviousFaceCount != 0) {
                m3AControls->setFaces(mFaceState);
                mPreviousFaceCount = mFaceState.num_faces;
            }
        }

        if (exp_id <= mDropStats) {
            LOG2("The first %d statistics should be dropped. %d dropped.", mDropStats, exp_id);
            return status;
        } else {
            // set the mDropFrames to 0 if current frame is already a valid one.
            mDropStats = 0;
        }

        status = m3AControls->apply3AProcess(m3ASetting->get3ARunning(),
                    getFrameSyncForStatistics(&capture_timestamp), exp_id);

        // Get settings from 3A
        ispInputParameters ispParams;
        ia_aiq_exposure_parameters exposure;
        CLEAR(ispParams);
        CLEAR(exposure);
        status = m3AControls->getParameters(&ispParams, &exposure);

        // Store in global set
        mAiq3ARequestManager->storeAutoParams(&ispParams, &exposure);

        // get metadata from 3A
        // get the corresponding reqId
        exp_id = (exp_id + MAX_EXP_ID + mISP->getExposureLag()) % MAX_EXP_ID;
        int reqId = mSensorCI->getExpectedReqId(exp_id);
        // since there is exposure lag, when return metadata, sometimes, it can't
        // get exposure time from 3A.
        // so the setting of the first frames(the number is equal to exposure lag)
        // are got from xml. so abandon the first frames from 3A
        if (reqId != INVALID_REQ_ID && reqId >= mISP->getExposureLag()) {
            aaaMetadataInfo *aaaMetadata;
            aaaMetadata = new aaaMetadataInfo;
            if (aaaMetadata == NULL) {
                LOG2("there is not enough memory");
                return NO_MEMORY;
            }
            CLEAR(*aaaMetadata);
            get3aMetadata(&ispParams, aaaMetadata);
            aaaMetadata->reqId = reqId;
            mISP->storeMetadata(aaaMetadata);
        }
    }

    return status;
}

void Aiq3AThread::get3aMakerNote(ia_binary_data *aaaMkNote)
{
     ia_binary_data makeNote;
     if (aaaMkNote != NULL) {
         m3AControls->get3aMakerNote(ia_mkn_trg_section_1, makeNote);
         aaaMkNote->data = new char[makeNote.size];
         aaaMkNote->size = makeNote.size;
         if (aaaMkNote->data != NULL) {
             memcpy(aaaMkNote->data, makeNote.data, makeNote.size);
         } else {
             LOGE("no memory for 3a maker note");
         }
     }
}

void Aiq3AThread::get3aMetadata(ispInputParameters *ispParams, aaaMetadataInfo *aaaMetadata)
{
    LOG2("@%s", __FUNCTION__);
    int isoSpeed = 0;
    ia_binary_data *aaaMkNote = NULL;

    aaaMkNote = new ia_binary_data;
    if (aaaMkNote == NULL) {
        LOGE("there is no memory");
        return;
    }
    get3aMakerNote(aaaMkNote);
    aaaMetadata->aaaMkNote = aaaMkNote;

    aaaMetadata->brightness = ispParams->manual_brightness;
    aaaMetadata->contrast = ispParams->manual_contrast;
    aaaMetadata->saturation = ispParams->manual_saturation;
    m3AControls->getManualIso(&isoSpeed);
    aaaMetadata->isoSpeed = isoSpeed;
    aaaMetadata->meteringMode = m3AControls->getAeMeteringMode();
    aaaMetadata->lightSource = m3AControls->getLightSource();
    aaaMetadata->aeMode = m3AControls->getAeMode();

    m3AControls->getExposureInfo(aaaMetadata->aeConfig);

}


bool Aiq3AThread::notifyIspEvent(ICssIspListener::IspMessage *msg)
{
    if (!msg || msg->id != ICssIspListener::ISP_MESSAGE_ID_EVENT)
        return true;

    if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_FRAME
        && msg->streamBuffer.get()) {
        LOG2("###==   ISP_EVENT_TYPE_FRAME(exp_id %d, req_id %d/%d, buffer %p)",
            msg->data.event.exp_id, msg->data.event.request_id,
            msg->data.event.vbuf.reserved2,
            msg->streamBuffer->data());

        MessageAAAFrame frameData;
        frameData.status = (FrameBufferStatus)(msg->data.event.vbuf.reserved & FRAME_STATUS_MASK);
        frameData.timestamp = msg->data.event.timestamp;
        frameData.requestId = msg->data.event.request_id;
        frameData.exp_id = msg->data.event.exp_id;
        newFrame(frameData);

        return true;
    } else if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_SOF) {
        LOG2("###==   ISP_EVENT_TYPE_SOF(exp_id %d)", msg->data.event.exp_id);
        newFrameSync(&msg->data.event);
        return true;
    } else if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_STATISTICS_READY) {
        LOG2("###==   ISP_EVENT_TYPE_STATISTICS_READY(exp_id %d)", msg->data.event.exp_id);
        newStats(&msg->data.event);
        return true;
    } else if (msg->data.event.type ==
                ICssIspListener::ISP_EVENT_TYPE_EMBEDDED_METADATA_READY) {
        LOG2("###==   ISP_EVENT_TYPE_EMBEDDED_METADATA_READY(exp_id %d)",
            msg->data.event.exp_id);

        mAiq3ARequestManager->checkMetadataDone(INVALID_REQ_ID, msg->data.event.exp_id);
        return true;
    }
    return false;
}

/**
 * newFrameSync event received.
 * This event is not serialized via the message queue.
 * The reason for this is that in cases where frame sync and 3A-STATS event arrive quite
 * close the order how they are processed has an impact on 3A decisions.
 * FrameSync event time is only recorded in 3A thread for information to the 3A library
 * and it is important to get the one wherefrom the statistics originate.
 *
 * For this reason we fast-track the SOF event as a FrameSync and store
 * one previous timestamp used in case where latest is not considered to be
 * the origin for statistics about to get read by 3A. This is essentially not reliable,
 * the impact of miss sync is seen as momentary IQ miss behaviour. Proper mapping of
 * frames from lower level is expected to drop the need for this eventually.
 */
status_t Aiq3AThread::newFrameSync(ICssIspListener::IspMessageEvent * sofMsg)
{
    LOG2("@%s", __FUNCTION__);

    if (sofMsg) {
        mAiq3ARequestManager->notifyShutter(sofMsg->exp_id, sofMsg->timestamp);
    }

    Mutex::Autolock _l(mFrameSyncData.lock);
    if (!sofMsg) {
        mFrameSyncData.ts.tv_sec = mFrameSyncData.ts.tv_usec = 0;
        mFrameSyncData.prevTs = mFrameSyncData.ts;
    } else {
        mFrameSyncData.prevTs = mFrameSyncData.ts;
        mFrameSyncData.ts = sofMsg->timestamp;
    }

    return NO_ERROR;
}

/**********************************************************************
 */
void Aiq3AThread::messageThreadLoop()
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
            status = handleMessageExit();
            break;

        case MESSAGE_ID_NEW_REQUEST:
            status = handleNewRequest(msg);
            break;

        case MESSAGE_ID_NEW_IMAGE:
            status = handleNewImage(msg);
            break;

        case MESSAGE_ID_NEW_STAT:
            if (handleNewStat(msg.data.stats) != NO_ERROR)
                LOGE("Error handling new stats");
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

status_t Aiq3AThread::handleFlashSequence(struct timeval timestamp, unsigned int exp_id, FrameBufferStatus frameStatus)
{
    LOG2("@%s: %d, frame status %d", __FUNCTION__, mFlashStage, frameStatus);
    status_t status = NO_ERROR;

#define SKIP_FRAMES 2

    // Flash sequence finished
    if (mFlashStage == FLASH_STAGE_PRE_EXPOSED)
        return NO_ERROR;

    // Start flash sequence
    if (mFlashStage == FLASH_STAGE_NA) {
        //params.setInt(ANDROID_CONTROL_AE_STATE, ANDROID_CONTROL_AE_STATE_PRECAPTURE);
        bool flashOn = isFlashNecessary();
        LOG1("   Start flash sequence, flash needed? %d", flashOn);
        if (flashOn) {
            mFlashStage = FLASH_STAGE_PRE_START;
            mFrameCountForFlashSeqence = SKIP_FRAMES;
        } else {
           mFlashStage = FLASH_STAGE_PRE_EXPOSED;
           return NO_ERROR;
        }
    }

    // Execute pre- flash sequence
    switch (mFlashStage) {
        case FLASH_STAGE_PRE_START:
            LOG2("     FLASH_STAGE_PRE_START");
            // Skip frames
            if (mFrameCountForFlashSeqence-- > 0)
                break;
            // Enter Stage 1
            mFrameCountForFlashSeqence = SKIP_FRAMES;
            status = m3AControls->applyPreFlashProcess(timestamp, exp_id, CAM_FLASH_STAGE_NONE);
            mFlashStage = FLASH_STAGE_PRE_PHASE1;
            break;
        case FLASH_STAGE_PRE_PHASE1:
            LOG2("     FLASH_STAGE_PRE_PHASE1");
            // Stage 1.5: Skip 2 frames to get exposure from Stage 1.
            //            First frame is for sensor to pick up the new value
            //            and second for sensor to apply it.
            if (mFrameCountForFlashSeqence-- > 0)
                break;
            // Enter Stage 2
            mFrameCountForFlashSeqence = SKIP_FRAMES;
            status = m3AControls->applyPreFlashProcess(timestamp, exp_id, CAM_FLASH_STAGE_PRE);
            mFlashStage = FLASH_STAGE_PRE_PHASE2;
            break;
        case FLASH_STAGE_PRE_PHASE2:
            LOG2("     FLASH_STAGE_PRE_PHASE2");
            // Stage 2.5: Same as above, but for Stage 2.
            if (mFrameCountForFlashSeqence-- > 0)
                break;
            // Enter Stage 3: get the flash-exposed preview frame
            // and let the 3A library calculate the exposure
            // settings for the flash-exposed still capture.
            // We check the frame status to make sure we use
            // the flash-exposed frame.
            status = m3AControls->setFlash(1);
            mFlashStage = FLASH_STAGE_PRE_WAITING;
            break;
        case FLASH_STAGE_PRE_WAITING:
            LOG2("     FLASH_STAGE_PRE_WAITING");
            mFrameCountForFlashSeqence++;
            if (frameStatus == FRAME_STATUS_FLASH_EXPOSED) {
                LOG1("    PreFlash@Frame %d: SUCCESS    (stopping...)", mFrameCountForFlashSeqence);
                mFlashStage = FLASH_STAGE_PRE_EXPOSED;
                m3AControls->setFlash(0);
                m3AControls->applyPreFlashProcess(timestamp, exp_id, CAM_FLASH_STAGE_MAIN);
            } else if(mFrameCountForFlashSeqence > FLASH_FRAME_TIMEOUT
                   || ( frameStatus != FRAME_STATUS_OK
                        && frameStatus != FRAME_STATUS_FLASH_PARTIAL) ) {
                LOG1("    PreFlash@Frame %d: FAILED     (stopping...)", mFrameCountForFlashSeqence);
                status = UNKNOWN_ERROR;
                mFlashStage = FLASH_STAGE_PRE_EXPOSED;
                m3AControls->setFlash(0);
                break;
            }
            status = NO_ERROR;
            break;
        case FLASH_STAGE_PRE_EXPOSED:
            LOG2("     FLASH_STAGE_PRE_EXPOSED");
            // staying in exposed state and not processing 3A until
            status = NO_ERROR;
            break;
        default:
        case FLASH_STAGE_EXIT:
        case FLASH_STAGE_NA:
            status = UNKNOWN_ERROR;
            break;
    }

    if (status != NO_ERROR) {
        LOGW("    Flash sequence failed!");
        mFrameCountForFlashSeqence = 0;
        mStartFlashSequence = false;
        mFlashStage = FLASH_STAGE_NA;
        //mCallback->preFlashSequenceDone(true);
    } else if (mFlashStage == FLASH_STAGE_PRE_EXPOSED) {
        LOG1("    Flash sequence succeed!");
    }

    return status;
}

bool Aiq3AThread::isFlashNecessary(void)
{
    FlashMode flashMode = m3AControls->getAeFlashMode();
    bool flashOn = (flashMode == CAM_AE_FLASH_MODE_SINGLE ||
                    flashMode == CAM_AE_FLASH_MODE_ON);

    if (!flashOn && DetermineFlash(flashMode)) {
        // Note: getAeFlashNecessary() should not be called when
        //       assist light (or TORCH) is on.
        if (mAfAssistLight)
            LOGW("Assist light on when running pre-flash sequence");

        if (m3AControls->getAeLock()) {
            bool aeLockFlash = m3ASetting->getAeLockFlash();
            LOG1("    AE was locked in %s, using old flash decision from AE "
                 "locking time (%s)", __FUNCTION__, aeLockFlash ? "ON" : "OFF");
            flashOn = aeLockFlash;
        } else
            flashOn = m3AControls->getAeFlashNecessary();
    }
    m3AControls->setAeFlashStatus(flashOn);

    return flashOn;
}

/**
 * Get timestamp for frame wherefrom statistics are expected to get read
 *
 * Statistics event contains a timestamp of event creation in driver side.
 * Driver is unable to provide proper linkage to know from which frame these
 * statistics are. On other hand, the sequence numbers are not reliable enough
 * to do the linkage based on them.
 *
 * STATISTICS_SYNC_DELTA_US is an estimated minimum latency from SOF-event
 * (start-of-frame) to statistics ready. This static threshold is used to
 * determine whether statistics belong to latest SOF or the one before.
 */
struct timeval Aiq3AThread::getFrameSyncForStatistics(struct timeval *ts)
{
    LOG2("@%s, %ld, %ld", __FUNCTION__, ts->tv_sec, ts->tv_usec);
    Mutex::Autolock _l(mFrameSyncData.lock);
    struct timeval *retTs = &mFrameSyncData.ts;

    nsecs_t syncForTs = TIMEVAL2USECS(ts);
    nsecs_t lastTs = TIMEVAL2USECS(&mFrameSyncData.ts);
    nsecs_t delta = syncForTs - lastTs;

    if (delta < STATISTICS_SYNC_DELTA_US
        && (mFrameSyncData.prevTs.tv_usec != 0 ||
            mFrameSyncData.prevTs.tv_sec != 0))
    {
        LOG2("FrameSync @%lld does not correspond to statistics %lld, using previous timestamp",
                lastTs, syncForTs);
        retTs = &mFrameSyncData.prevTs;
    }

    // validate the selected timestamp
    //  - picked frame sync cannot be more recent than statistics
    //  - picked frame sync cannot be older than STATISTICS_SYNC_DELTA_US_MAX
    nsecs_t _retTs = TIMEVAL2USECS(retTs);
    if (syncForTs < _retTs ||
        syncForTs - _retTs > STATISTICS_SYNC_DELTA_US_MAX)
    {
        LOG1("FrameSync lost (stats ts %lld, sync ts %lld)",
                syncForTs, _retTs);
        retTs = ts;
    }

    LOG2("FrameSync @%lld given for statistics %lld",
                TIMEVAL2USECS(retTs), syncForTs);

    return *retTs;
}

void Aiq3AThread::dump(int fd)
{
    LOG2("@%s", __FUNCTION__);

    String8 message;
    message.appendFormat(LOG_TAG ":@%s\n", __FUNCTION__);
    write(fd, message.string(), message.size());
}

/**
 * AF state handling
 */
void Aiq3AThread::processAfStateMachine()
{
    LOG2("@%s", __FUNCTION__);

    AfStatus afState = m3AControls->getCAFStatus();
    uint8_t requestAfMode = m3ASetting->getRequestAfMode();

    if (requestAfMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE ||
        requestAfMode == ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO)
        processContinuousAfStateMachine(m3ASetting->popRequestAfTriggerState(), requestAfMode,
                                        afState);
    else if (requestAfMode == ANDROID_CONTROL_AF_MODE_AUTO ||
            requestAfMode == ANDROID_CONTROL_AF_MODE_MACRO)
        processAutoAfStateMachine(m3ASetting->popRequestAfTriggerState(), requestAfMode, afState);

    // TODO AF_MODE_OFF and AF_MODE_EDOF handling
}
void Aiq3AThread::processContinuousAfStateMachine(uint8_t requestTriggerState,
                                                uint8_t requestAfMode,
                                                AfStatus afState)
{
    LOG2("@%s trigger state: %d latest AF state:%d AF status:%d", __FUNCTION__,
         requestTriggerState, m3ASetting->getAndroidAfState(), afState);
    UNUSED(requestAfMode);
    switch (m3ASetting->getAndroidAfState()) {
        /* This had to be split to reduce mccabe complexity, but you can look
         * at the individual process-functions and compare against the state
         * transition table in camera3.h */
        case ANDROID_CONTROL_AF_STATE_INACTIVE:
            processCAFInactiveState(requestTriggerState, afState);
            break;
        case ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN:
            processCAFPassiveScanState(requestTriggerState, afState);
            break;
        case ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED:
        case ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED:
            processCAFPassiveFocusedState(requestTriggerState, afState);
            break;
        case ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
        case ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED:
            processCAFLockedStates(requestTriggerState);
            break;
        case ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN:
            // not a valid state in continuous mode
        default:
            LOGE("Error: invalid af state!");
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
            break;
    }
}

void Aiq3AThread::processCAFInactiveState(uint8_t requestTriggerState,
                                        AfStatus afState)
{
    if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_START) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        LOG1("@%s INACTIVE -> NOT_FOCUSED_LOCKED (trigger)", __FUNCTION__);
    } else if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
    } else if (afState == CAM_AF_STATUS_SUCCESS) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
        LOG1("@%s INACTIVE -> PASSIVE_FOCUSED", __FUNCTION__);
    } else if (afState == CAM_AF_STATUS_BUSY) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
        LOG1("@%s INACTIVE -> PASSIVE_SCAN (scaning)", __FUNCTION__);
    }
}

void Aiq3AThread::processCAFPassiveScanState(uint8_t requestTriggerState,
                                           AfStatus afState)
{
    if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
        // TODO: reset lens position
        LOG1("@%s PASSIVE_SCAN -> INACTIVE (cancel)", __FUNCTION__);
    } else if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_START) {
        if (afState == CAM_AF_STATUS_FAIL) {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            LOG1("@%s PASSIVE_SCAN -> NOT_FOCUSED_LOCKED (cancel)", __FUNCTION__);
        } else if (afState == CAM_AF_STATUS_SUCCESS) {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            LOG1("@%s PASSIVE_SCAN -> FOCUSED_LOCKED (scan ended successfully, "
                 "previous trigger)", __FUNCTION__);
        } else if (afState == CAM_AF_STATUS_BUSY) {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            LOG1("@%s PASSIVE_SCAN -> NOT_FOCUSED_LOCKED", __FUNCTION__);
        }
    } else {
        if (afState == CAM_AF_STATUS_FAIL) {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED);
            LOG1("@%s PASSIVE_SCAN -> PASSIVE_UNFOCUSED (trigger, af failed)",
                 __FUNCTION__);
        } else if (afState == CAM_AF_STATUS_SUCCESS) {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            LOG1("@%s PASSIVE_SCAN -> PASSIVE_FOCUSED (trigger, af success)",
                 __FUNCTION__);
        } else {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            LOG1("@%s PASSIVE_SCAN -> PASSIVE_SCAN (af busy, keep scan)",
                 __FUNCTION__);
        }
    }
}

void Aiq3AThread::processCAFPassiveFocusedState(uint8_t requestTriggerState,
                                              AfStatus afState)
{
    if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_START) {
        camera_metadata_enum_android_control_af_state androidAfState = m3ASetting->getAndroidAfState();
        if (afState == CAM_AF_STATUS_SUCCESS && androidAfState == ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED) {
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            LOG1("@%s PASSIVE_FOCUSED -> FOCUSED_LOCKED (trigger, in focus)",
                    __FUNCTION__);
        } else if (afState == CAM_AF_STATUS_FAIL && androidAfState == ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED){
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            LOG1("@%s PASSIVE_FOCUSED -> NOT_FOCUSED_LOCKED (trigger, not focused)",
                    __FUNCTION__);
        }
    } else if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
        m3AControls->setAfLock(false);
        LOG1("@%s PASSIVE_FOCUSED_LOCKED -> INACTIVE (cancel)",
             __FUNCTION__);
    } else if (afState == CAM_AF_STATUS_BUSY) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
        LOG1("@%s PASSIVE_FOCUSED -> PASSIVE_SCAN (started scan)",
                __FUNCTION__);
    } else if (afState == CAM_AF_STATUS_FAIL) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
        LOG1("@%s PASSIVE_FOCUSED/UNFOCUSED -> INACTIVE (failed, trigger scan)",
                __FUNCTION__);
    } else if (afState == CAM_AF_STATUS_SUCCESS) {
        LOG1("@%s PASSIVE_FOCUSED -> PASSIVE_FOCUSED (still in focus)",
                __FUNCTION__);
    }
}

void Aiq3AThread::processCAFLockedStates(uint8_t requestTriggerState)
{
    LOG2("@%s", __FUNCTION__);
    if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
        m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
        m3AControls->setAfLock(false);
        LOG1("@%s (maybe NOT)_FOCUSED_LOCKED -> INACTIVE (cancel)",
             __FUNCTION__);
    } else if (requestTriggerState == ANDROID_CONTROL_AF_TRIGGER_START) {
        m3ASetting->setAndroidAfState(m3ASetting->getAndroidAfState());
    }
}

void Aiq3AThread::processAutoAfStateMachine(uint8_t reqTriggerState,
                                          uint8_t requestAfMode,
                                          AfStatus afState)
{
    LOG2("@%s trigger state: %d latest AF state:%d AF status:%d", __FUNCTION__,
         reqTriggerState, m3ASetting->getAndroidAfState(), afState);
    UNUSED(requestAfMode);
    switch (m3ASetting->getAndroidAfState()) {
        /* look at the state transition table in camera3.h */
        case ANDROID_CONTROL_AF_STATE_INACTIVE:
            if (reqTriggerState == ANDROID_CONTROL_AF_TRIGGER_START) {
                m3AControls->setAfLock(false);
                AfStatus current_af_status = m3AControls->isStillAfComplete();
                if (current_af_status == CAM_AF_STATUS_SUCCESS) {
                    //If the previous is CAF, it's possible to get focused
                    m3AControls->stopStillAf();
                    m3AControls->setAfEnabled(false);
                    m3AControls->setAfLock(true);
                    m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
                    LOG1("@%s INACTIVE -> FOCUSED_LOCKED (trigger)", __FUNCTION__);
                } else {
                    m3AControls->stopStillAf();
                    m3AControls->setAfEnabled(false);
                    m3AControls->setAfEnabled(true);
                    m3AControls->startStillAf();
                    m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN);
                    LOG1("@%s INACTIVE -> ACTIVE_SCAN (trigger)", __FUNCTION__);
                }
            } else {
                m3AControls->setAfLock(true);
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
                LOG1("@%s INACTIVE -> INACTIVE (no trigger)", __FUNCTION__);
            }
            break;
        case ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN:
            if (reqTriggerState == ANDROID_CONTROL_AF_TRIGGER_START && afState == CAM_AF_STATUS_SUCCESS) {
                m3AControls->stopStillAf();
                m3AControls->setAfEnabled(false);
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
                m3AControls->setAfLock(true);
                LOG1("@%s ACTIVE_SCAN -> FOCUSED_LOCKED "
                     "(scan ended successfully)", __FUNCTION__);
            } else if (reqTriggerState == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
                m3AControls->stopStillAf();
                m3AControls->setAfEnabled(false);
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
                LOG1("@%s ACTIVE_SCAN -> INACTIVE (cancel)", __FUNCTION__);
            } else if (afState == CAM_AF_STATUS_SUCCESS) {
                m3AControls->stopStillAf();
                m3AControls->setAfEnabled(false);
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
                m3AControls->setAfLock(true);
                LOG1("@%s ACTIVE_SCAN -> FOCUSED_LOCKED"
                     "(no trigger)", __FUNCTION__);
            } else if (afState == CAM_AF_STATUS_FAIL) {
                m3AControls->stopStillAf();
                m3AControls->setAfEnabled(false);
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
                m3AControls->setAfLock(true);
                LOG1("@%s ACTIVE_SCAN -> NOT_FOCUSED_LOCKED "
                     "(scan ended fail)", __FUNCTION__);
            } else if (afState == CAM_AF_STATUS_BUSY) {
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN);
            }
            break;
        case ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
        case ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED:
            if (reqTriggerState == ANDROID_CONTROL_AF_TRIGGER_CANCEL) {
                m3AControls->setAfEnabled(false);
                m3AControls->stopStillAf();
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
                LOG1("@%s (maybe NOT)_FOCUSED_LOCKED -> INACTIVE (cancel)",
                        __FUNCTION__);
            } else if (reqTriggerState == ANDROID_CONTROL_AF_TRIGGER_START) {
                m3AControls->setAfEnabled(true);
                m3AControls->startStillAf();
                m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN);
                m3AControls->setAfLock(false);
                LOG1("@%s (maybe NOT)_FOCUSED_LOCKED -> ACTIVE_SCAN (trigger)",
                     __FUNCTION__);
            }
            break;
        case ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN:
        case ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED:
        default:
            LOGE("Error: invalid af state!");
            m3ASetting->setAndroidAfState(ANDROID_CONTROL_AF_STATE_INACTIVE);
            break;
    }
}

/**
 * AWB state handling
 */
void Aiq3AThread::processAwbStateMachine()
{
    LOG2("@%s", __FUNCTION__);
    if (m3AControls->getAwbMode() != CAM_AWB_MODE_AUTO &&
        mAndroidAwbState != ANDROID_CONTROL_AWB_STATE_LOCKED) {
/*                            mode = AE_MODE_OFF / AWB mode not AUTO
 *| state              | trans. cause  | new state          | notes            |
 *+--------------------+---------------+--------------------+------------------+
 *| INACTIVE           |               |                    | AE/AWB disabled  |
 *+--------------------+---------------+--------------------+-----------------*/
        mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_INACTIVE;
        LOG2("@%s AWB_STATE_INACTIVE (awb not on auto)", __FUNCTION__);
    } else {
        bool forceAwbLock = m3ASetting->getForceAwbLock();
        LOG2("@%s AndroidAwbState:%d, lock status: %d", __FUNCTION__, mAndroidAwbState, forceAwbLock);
        switch (mAndroidAwbState) {
            /* look at the state transition table in camera3.h */
            case ANDROID_CONTROL_AWB_STATE_INACTIVE:
                if (forceAwbLock) {
                    mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_LOCKED;
                    LOG1("@%s INACTIVE -> LOCKED", __FUNCTION__);
                } else {
                    if (m3AControls->isAwbConverged()){
                        mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_CONVERGED;
                        LOG1("@%s SEARCHING -> CONVERGED", __FUNCTION__);
                    } else {
                        mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_SEARCHING;
                        LOG1("@%s INACTIVE -> SEARCHING", __FUNCTION__);
                    }
                }
                break;
            case ANDROID_CONTROL_AWB_STATE_SEARCHING:
                if (forceAwbLock) {
                    mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_LOCKED;
                    LOG1("@%s SEARCHING -> LOCKED", __FUNCTION__);
                } else if (m3AControls->isAwbConverged()) {
                    mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_CONVERGED;
                    LOG1("@%s SEARCHING -> CONVERGED", __FUNCTION__);
                }
                break;
            case ANDROID_CONTROL_AWB_STATE_CONVERGED:
                if (forceAwbLock) {
                    mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_LOCKED;
                    LOG1("@%s CONVERGED -> LOCKED", __FUNCTION__);
                } else if (!m3AControls->isAwbConverged()) {
                    mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_SEARCHING;
                    LOG1("@%s CONVERGED -> SEARCHING", __FUNCTION__);
                }
                break;
            case ANDROID_CONTROL_AWB_STATE_LOCKED:
                if (!forceAwbLock) {
                    if (m3AControls->isAwbConverged()) {
                        mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_CONVERGED;
                        LOG1("@%s LOCKED -> CONVERGED", __FUNCTION__);
                    } else {
                        mAndroidAwbState = ANDROID_CONTROL_AWB_STATE_SEARCHING;
                        LOG1("@%s LOCKED -> SEARCHING", __FUNCTION__);
                    }
                }
                break;
            default:
                LOGE("unknown AWB state!");
                break;
        }
    }
}

/**
 * AE state handling
 */
void Aiq3AThread::processAeStateMachine()
{
    LOG2("@%s", __FUNCTION__);

    if (m3AControls->getAeMode() == CAM_AE_MODE_MANUAL ||
        m3AControls->getAeMode() == CAM_AE_MODE_NOT_SET) {
/*                            mode = AE_MODE_OFF / AWB mode not AUTO
 *| state              | trans. cause  | new state          | notes            |
 *+--------------------+---------------+--------------------+------------------+
 *| INACTIVE           |               |                    | AE/AWB disabled  |
 *+--------------------+---------------+--------------------+-----------------*/
        mAndroidAeState = ANDROID_CONTROL_AE_STATE_INACTIVE;
        LOG2("@%s AE_STATE_INACTIVE (ae on manual or not set)", __FUNCTION__);
    } else if (m3ASetting->getRequestAeTriggerState() == ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) {
/*+--------------------+---------------+--------------------+------------------+
 *| All AE states      | PRECAPTURE_   | PRECAPTURE         | Start precapture |
 *|                    | START         |                    | sequence         |
 *+--------------------+---------------+--------------------+-----------------*/
        if (isFlashNecessary()) {
            if (!mStartFlashSequence && mFlashStage == FLASH_STAGE_NA) {
                enterFlashSequence();
            }
        }
        LOG1("@%s state nr. %d -> PRECAPTURE", __FUNCTION__, mAndroidAeState);
        mAndroidAeState = ANDROID_CONTROL_AE_STATE_PRECAPTURE;
        m3ASetting->setRequestAeTriggerState(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE);
    } else {
        processAutoAeStates();
    }
}

void Aiq3AThread::processAutoAeStates() {
    bool forceAeLock = m3ASetting->getForceAeLock();
    switch (mAndroidAeState) {
        /* look at the state transition table in camera3.h */
        case ANDROID_CONTROL_AE_STATE_INACTIVE:
            if (forceAeLock) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s INACTIVE -> LOCKED", __FUNCTION__);
            } else {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                LOG1("@%s INACTIVE -> SEARCHING", __FUNCTION__);
            }
            break;
        case ANDROID_CONTROL_AE_STATE_SEARCHING:
            if (forceAeLock) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s SEARCHING -> LOCKED", __FUNCTION__);
            } else if (m3AControls->isAeConverged()) {
                if (m3AControls->getAeFlashNecessary()) {
                    mAndroidAeState = ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED;
                    LOG1("@%s SEARCHING -> FLASH_REQUIRED", __FUNCTION__);
                } else {
                    mAndroidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                    LOG1("@%s SEARCHING -> CONVERGED", __FUNCTION__);
                }
            }
            break;
        case ANDROID_CONTROL_AE_STATE_CONVERGED:
            if (forceAeLock) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s CONVERGED -> LOCKED", __FUNCTION__);
            } else if (!m3AControls->isAeConverged()) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                LOG1("@%s CONVERGED -> SEARCHING", __FUNCTION__);
            }
            break;
        case ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED:
            if (forceAeLock) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                LOG1("@%s FLASH_REQUIRED -> LOCKED", __FUNCTION__);
            } else if (!m3AControls->isAeConverged()) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                LOG1("@%s FLASH_REQUIRED -> SEARCHING", __FUNCTION__);
            }
            break;
        case ANDROID_CONTROL_AE_STATE_LOCKED:
            if (!forceAeLock) {
                if (m3AControls->isAeConverged()) {
                    if (m3AControls->getAeFlashNecessary()) {
                        mAndroidAeState =
                                ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED;
                        LOG1("@%s LOCKED -> FLASH_REQUIRED", __FUNCTION__);
                    } else {
                        mAndroidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                        LOG1("@%s LOCKED -> CONVERGED", __FUNCTION__);
                    }
                } else {
                    mAndroidAeState = ANDROID_CONTROL_AE_STATE_SEARCHING;
                    LOG1("@%s LOCKED -> SEARCHING", __FUNCTION__);
                }
            }
            break;
        case ANDROID_CONTROL_AE_STATE_PRECAPTURE:
            if (!mStartFlashSequence && m3AControls->isAeConverged()) {
                if (forceAeLock) {
                    mAndroidAeState = ANDROID_CONTROL_AE_STATE_LOCKED;
                    LOG1("@%s PRECAPTURE -> LOCKED", __FUNCTION__);
                } else {
                    mAndroidAeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
                    LOG1("@%s PRECAPTURE -> CONVERGED", __FUNCTION__);
                }
            } else if (mStartFlashSequence && mFlashStage == FLASH_STAGE_PRE_EXPOSED) {
                mAndroidAeState = ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED;
                LOG1("@%s PRECAPTURE -> FLASH_REQUIRED", __FUNCTION__);
            }
            break;
        default:
            LOGE("unknown AE state!");
            break;
    }
}

bool Aiq3AThread::isReportedAeState(uint8_t aeState) {
    LOG2("@%s", __FUNCTION__);
    return (aeState == ANDROID_CONTROL_AE_STATE_LOCKED
            || aeState == ANDROID_CONTROL_AE_STATE_CONVERGED
            || aeState == ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED
            || aeState == ANDROID_CONTROL_AE_STATE_PRECAPTURE);
}

void Aiq3AThread::storeReportedAeState(uint8_t aeState) {
    LOG2("@%s", __FUNCTION__);
    if (mReportedAeState.size() == mReportedAeState.capacity())
        mReportedAeState.pop();
    mReportedAeState.push_front(aeState);
}

uint8_t Aiq3AThread::loadReportedAeState(void) {
    LOG2("@%s", __FUNCTION__);
    uint8_t aeState = mAndroidAeState;
    if (mReportedAeState.size() > 0) {
        aeState = mReportedAeState.top();
        mReportedAeState.pop();
    }
    return aeState;
}

};  // namespace camera2
};  // namespac
