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

#ifndef _CAMERA3_HAL_AIQ_3A_THREAD_H_
#define _CAMERA3_HAL_AIQ_3A_THREAD_H_

#include <utils/threads.h>
#include <time.h>
#include "I3AControls.h"
#include "MessageQueue.h"
#include "ipu2/IPU2HwIsp.h"
#include "ipu2/HwStream.h"
#include "MessageThread.h"
#include "ICameraHwControls.h"
#include "Aiq3ASetting.h"
#include "Aiq3ARequestManager.h"
#include "ia_types.h"

namespace android {
namespace camera2 {

class ICssIspListener;
class IPU2CameraHw;
class IFaceDetectCallback;


class IAAACallback {
public:
    virtual ~IAAACallback() {};

    virtual int autoFocusDone(bool result) = 0;
    virtual int preFlashSequenceDone(bool flashNeeded) = 0;
};


/**
 * \class Aiq3AThread
 *
 * Aiq3AThread runs the actual 3A process for preview frames. In video
 * mode it also handles DVS.
 *
 * The implementation is done using I3AControls class, which has two
 * derived classes: Aiq3A for RAW cameras and Soc3A for SoC cameras.
 *
 * Aiq3AThread also checks 3A variables and updates the trigger status for
 * Ultra Low Light algorithm
 *
 */
class Aiq3AThread : public ICssIspListener, public IMessageHandler,
                       public IFlashCallback, public IFaceDetectCallback {
public:
    Aiq3AThread(IPU2CameraHw * hw, IAAACallback * callback,
                  I3AControls * aaa, HWControlGroup &hwcg,
                  CameraMetadata &staticMeta);
    virtual ~Aiq3AThread();

    status_t requestExitAndWait();

    status_t reset(IspMode mode);

    /**********************************************************************
     * APIs
     */

    status_t processRequest(Camera3Request* request, int availableStreams);

    void dump(int fd);

    /**********************************************************************
     * ICssIspListener override
     */
    bool notifyIspEvent(ICssIspListener::IspMessage *msg);
    /**********************************************************************
      * ICallbackAIQ override
      */
    void FaceDetectCallbackFor3A(ia_face_state *facestate);
    void FaceDetectCallbackForApp(camera_frame_metadata_t *results);

    /**********************************************************************
     * IFlashCallback override
     */
    bool isPreFlashUsed();
    void exitPreFlashSequence();
    bool isFlashNecessary(void);

private:
    status_t newFrameSync(ICssIspListener::IspMessageEvent *sofMsg);
    status_t newStats(ICssIspListener::IspMessageEvent * statMsg);

    void processAeStateMachine();
    void processAutoAeStates();
    void processAwbStateMachine();
    void processAfStateMachine();
    void processContinuousAfStateMachine(uint8_t requestTriggerState,
                                         uint8_t requestAfMode,
                                         AfStatus afState);
    void processCAFInactiveState(uint8_t requestTriggerState, AfStatus afState);
    void processCAFPassiveScanState(uint8_t requestTriggerState,
                                    AfStatus afState);
    void processCAFPassiveFocusedState(uint8_t requestTriggerState,
                                       AfStatus afState);
    void processCAFLockedStates(uint8_t requestTriggerState);

    void processAutoAfStateMachine(uint8_t requestTriggerState,
                                   uint8_t requestAfMode,
                                   AfStatus afState);

private:
    /**********************************************************************
     * Thread methods
     */
    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,

        MESSAGE_ID_NEW_REQUEST,
        MESSAGE_ID_NEW_IMAGE,
        MESSAGE_ID_NEW_STAT,

        MESSAGE_ID_MAX
    };

    struct MessageGeneric {
        bool enable;
    };

    struct MessageStats {
        timeval timestamp;
        unsigned int exp_id;
    };

    struct MessageRequest {
        bool needProcess;
    };
    struct MessageAAAFrame {
        FrameBufferStatus status;
        timeval timestamp;
        int requestId;
        unsigned int exp_id;
    };

    // union of all message data
    union MessageData {
        MessageGeneric generic;
        MessageRequest request;
        MessageAAAFrame frame;
        MessageStats stats;
    };

    // message id and message data
    struct Message {
        MessageId id;
        MessageData data;
        Camera3Request* request;
    };

    enum SettingType {
         TRIGGER = 1,
         SETTING = 2
    };

    status_t newFrame(MessageAAAFrame frame);

    status_t enterFlashSequence(void);

    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);
    status_t handleMessageExit(void);
    status_t handleNewRequest(Message &msg);

    status_t handleEnterFlashSequence(void);
    status_t handleExitFlashSequence(void);
    status_t handleFlashSequence(struct timeval timestamp, unsigned int exp_id, FrameBufferStatus status = FRAME_STATUS_NA);
    void CombineFaceDetectInfo(int* faceIds, int* faceLandmarks, int* faceRect, uint8_t* faceScores, int& faceNum);
    status_t handleNewImage(Message &msg);
    status_t handleNewStat(MessageStats &msg);

    status_t writeAndNotifyResults(int request_id, unsigned int exp_id, FrameBufferStatus flashStatus);
    struct timeval getFrameSyncForStatistics(struct timeval *ts);

    status_t processSettingsForRequest(Camera3Request* request, SettingType type);
    status_t processColorCorrectionSettings(const CameraMetadata &settings);

    void get3aMakerNote(ia_binary_data *aaaMkNote);
    void get3aMetadata(ispInputParameters *ispParams, aaaMetadataInfo *aaaMetadata);

    bool isReportedAeState(uint8_t aeState);
    void storeReportedAeState(uint8_t aeState);
    uint8_t loadReportedAeState(void);

    IPU2CameraHw * mHal;
    IAAACallback * mCallback;
    I3AControls *m3AControls;
    IHWIspControl *mISP;
    IHWSensorControl *mSensorCI;
    CameraMetadata mStaticMeta;
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    bool mDvsRunning;
    bool mStartFlashSequence;
    Aiq3ARequestManager * mAiq3ARequestManager;

    struct {
        Mutex lock; /*!< Frame sync event timestamp updates are not serialized, this lock is used to make the usage thread safe*/
        struct timeval prevTs;
        struct timeval ts;
    } mFrameSyncData;

    enum still_af_status_t {
        STILL_AF_IDLE,
        STILL_AF_IN_PROCESS,
        STILL_AF_DONE,
    };
    size_t mFramesTillAfComplete; // used for debugging only
    int mStillAfStatus;

    enum FlashStage {
        FLASH_STAGE_NA,
        FLASH_STAGE_EXIT,

        // Pre-flash
        FLASH_STAGE_PRE_START,
        FLASH_STAGE_PRE_PHASE1,
        FLASH_STAGE_PRE_PHASE2,
        FLASH_STAGE_PRE_WAITING,
        FLASH_STAGE_PRE_EXPOSED,
    };
    FlashStage mFlashStage;
    int mFrameCountForFlashSeqence;
    bool mAfAssistLight;


    camera_metadata_enum_android_control_awb_state mAndroidAwbState;
    camera_metadata_enum_android_control_ae_state mAndroidAeState;
    Vector<uint8_t> mReportedAeState;

    // face detection state
    ia_face_state mFaceState;
    Mutex mFaceStateLock;
    camera_frame_metadata_t mFaceMetadata;
    Mutex mFaceMetadataLock;
    int mPreviousFaceCount;
    unsigned int mDropStats;

    // aaa state metadata
    Mutex m3AStateLock;
    aaa_state_metadata m3AStateMetadata;

    AAASetting *m3ASetting;
    int mFlashExposedNum;
}; // class Aiq3AThread

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_AIQ_3A_THREAD_H_
