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
#ifndef CAMERA3_HAL_IPU2_HWISP_H_
#define CAMERA3_HAL_IPU2_HWISP_H_

#include <ia_isp_2_2.h>
#include "utils/Vector.h"
#include "utils/KeyedVector.h"
#include "PlatformData.h"
#include "v4l2dev/v4l2device.h"
#include "ICameraHwControls.h"
#include "MessageQueue.h"
#include "MessageThread.h"
#include "SensorEmbeddedMetaData.h"
#include "I3AControls.h"
#include "IPU2CameraCapInfo.h"

namespace android {
namespace camera2 {

#define MAX_CAMERA_NODES    5
#define MAX_DEVICE_NODE_CHAR_NR 32
#define INTEL_FILE_INJECT_CAMERA_ID 2

struct devNameGroup
{
    char dev[MAX_CAMERA_NODES + 1][MAX_DEVICE_NODE_CHAR_NR];
    bool in_use;
};

/**
 * \class ICssIspListener
 *
 * Abstract interface implemented by entities interested on receiving notifications
 * from the ISP
 */
class ICssIspListener {
public:

    enum IspMessageId {
        ISP_MESSAGE_ID_EVENT = 0,
        ISP_MESSAGE_ID_ERROR
    };

    enum IspEventType {
        ISP_EVENT_TYPE_FRAME = 1,  // timestamp, exp_id, request_id, streamBuffer
        ISP_EVENT_TYPE_SOF = 2,    // timestamp, sequence, exp_id
        ISP_EVENT_TYPE_STATISTICS_READY = 1<<2,  // timestamp, sequence
        ISP_EVENT_TYPE_EMBEDDED_METADATA_READY = 1<<3,  // timestamp, sequence
        ISP_EVENT_TYPE_RAW_LOCK,  // timestamp, sequence, exp_id, request_id
        ISP_EVENT_MAX
    };

    // For MESSAGE_ID_EVENT
    struct IspMessageEvent {
        IspEventType       type;
        struct timeval  timestamp;     // For all event
        unsigned int    sequence;
        struct v4l2_buffer vbuf;       // to be removed
        unsigned int    exp_id;        // in SOF/FRAME events
        int             request_id;    // in FRAME event
        unsigned int    vbiOffset;     // us, in SOF event
        uint8_t         metadata_type; // in EMBEDDED_METADATA event
        int64_t shutterTime;             // us,in SOF event
    };

    // For MESSAGE_ID_ERROR
    struct IspMessageError {
        status_t code;
    };

    union IspMessageData {
        IspMessageEvent        event;
        IspMessageError        error;
    };

    struct IspMessage {
        IspMessageId id;
        IspMessageData data;
        // Pass the stream buffer in ISP_EVENT_TYPE_FRAME event
        // Pass NULL if the grabbed buffer is fake buffer (not belong to streams)
        sp<CameraBuffer> streamBuffer;
    };

    virtual bool notifyIspEvent(IspMessage *msg) = 0;
    virtual ~ICssIspListener() {};

};  //ICssIspListener

class ICssBufferMaintainer {
public:
    enum DeviceStreamType {
        DEVICE_STREAM_TYPE_PREVIEW = 0,
        DEVICE_STREAM_TYPE_VIDEO,
        DEVICE_STREAM_TYPE_CAPTURE,
        DEVICE_STREAM_TYPE_MAX
    };
    virtual void * queryBufferPointer(int requestId) = 0;
    virtual ~ICssBufferMaintainer() {};
};  // ICssBufferMaintainer

/**
 * This class keeps the state of the ISP related parameters.
 * This class also provides ISP events that other entities can listen to.
 *
 */
class SensorEmbeddedMetaData;
class IaDvs2;
class IPU2HwIsp:
    public RefBase,
    public IMessageHandler,
    public IHWIspControl,
    public IHWFlashControl,
    public IHWLensControl {
#define MAX_METADATA_QUEUE_SIZE 20
public: // Types
    typedef enum {
        CSS_ISP_MODE_NONE,
        CSS_ISP_MODE_PREVIEW,
        CSS_ISP_MODE_CAPTURE,
        CSS_ISP_MODE_VIDEO,
        CSS_ISP_MODE_CONTINUOUS_CAPTURE,
        CSS_ISP_MODE_CONTINUOUS_VIDEO
    } CssIspMode;
    /* PRETTY LOGGING SUPPORT */
    static const char *sMode2String[];
    static const char *sEvent2String[];
public:
    IPU2HwIsp(sp<V4L2VideoNode> &mainNode,
                sp<V4L2VideoNode> &previewNode,
                sp<V4L2VideoNode> &videoNode,
                sp<V4L2Subdevice> &eventSubdev,
                HWControlGroup &hwcg,
                I3AControls *aaa);
    virtual ~IPU2HwIsp();
    virtual status_t init(int cameraId);
    int getExposureLag(void);
    struct devNameGroup* getDevName(unsigned int index);
    void getSensorActiveArray();
    void getZoomRatio();

    bool isRAW() const;

    status_t setMode(CssIspMode aMode, bool enableRawLock = false);
    status_t start();
    status_t stop();

    bool isRawLockEnabled() { return mRawLockEnabled; }
    bool isRawLockReadyForUse() { return mRawLockReady; }
    status_t unlockRawBuffer(int expId);
    status_t triggerCaptureByLockedRaw(int expId);

    /************************************************************
     * process settings
     */
    status_t processSettings(const CameraMetadata &settings);
    status_t processSettingFocalLength(const CameraMetadata &settings);
    status_t processSettingRecordingFPS(const CameraMetadata &settings);
    status_t processSettingDVS(const CameraMetadata &settings);
    status_t processSettingZoom(const CameraMetadata &settings);
    status_t updateSettingPipelineDepth(CameraMetadata &settings);
    status_t processTestPattern(const CameraMetadata &settings);

    /************************************************************
     * Listener management methods
     */
    status_t attachListener(ICssIspListener *aListener, ICssIspListener::IspEventType event);
    status_t detachListener(ICssIspListener *aListener, ICssIspListener::IspEventType event);
    /************************************************************
     * Continuous capture configuration
     */
    status_t configureContinuousMode(ContinuousCaptureConfig &aCfg,
                                     bool enable,
                                     bool continuousVF = false);
    status_t configureContinuousRingBuffer(int numOfCapture,
                                           bool enableRawLock = false);
    status_t requestContCapture(int numCaptures, int offset, unsigned int skip);

    /* **********************************************************
     * ISP features
     */
    status_t reSetZoomRatio();
    status_t setZoomRatio(float zoomRatio);
    status_t getMakerNote(atomisp_makernote_info *info);
    status_t initDVS();
    status_t setDVS(bool enable);
    bool dvsEnabled() { return mDvsEnabled; }
    IaDvs2* getDVSHandler() { return mDvs; };

    status_t setColorEffect(v4l2_colorfx effect);
    status_t applyColorEffect();
    status_t getIspParameters(struct atomisp_parm *isp_param) const;

    status_t setIspParameters(const ispInputParameters *ispInputParams);
    status_t getIspStatistics(ia_aiq_rgbs_grid **out_rgbs_grid,
                              ia_aiq_af_grid **out_af_grid, unsigned int *exp_id);
    unsigned int getIspStatisticsSkip() { return (unsigned int)(mCapInfo->statisticsInitialSkip()); }

    bool isHighSpeed(const CameraMetadata &setting);

    /* Sensor Embedded Metadata */
    int getSensorEmbeddedMetaData(sensor_embedded_metadata *sensorMetaData,
            uint8_t metadata_type);
    status_t getDecodedExposureParams(ia_aiq_exposure_sensor_parameters* sensor_exp_p
                                               , ia_aiq_exposure_parameters* generic_exp_p,unsigned int exp_id = 0);
    bool checkSensorMetadataAvailable(unsigned int exp_id);
    bool isSensorEmbeddedMetaDataSupported() { return mSensorEmbeddedMetaDataSupported; }
    bool isIspPerframeSettingsEnabled() { return mIspPerframeSettingsEnabled; }

    void setNrEE(bool en) { mNoiseReductionEdgeEnhancement = en; }

    /* DVS */
    status_t initialDvsConfig();
    void setDvsConfigChanged(bool changed) { mDvsConfigChange = changed; }
    status_t getDvsStatistics(struct atomisp_dis_statistics *stats,
                              bool *tryAgain) const;
    status_t setDvsCoefficients(const struct atomisp_dis_coefficients *coefs) const;
    status_t getIspDvs2BqResolutions(struct atomisp_dvs2_bq_resolutions *bq_res) const;

    void initIaIspAdaptor(const ia_binary_data *cpfData,
                          unsigned int maxStatsWidth,
                          unsigned int maxStatsHeight,
                          ia_cmc_t *cmc,
                          ia_mkn *mkn);

    void sensorModeChanged();

    void setVideoInfo(FrameInfo *info);
    void setPreviewInfo(FrameInfo *info);
    void setRecordingFramerate(int fps);
    void getVideoSize(int *w, int *h) const;

    void setDeviceStreams(ICssBufferMaintainer *aDeviceStream,
                              ICssBufferMaintainer::DeviceStreamType type);
    status_t fillDefaultMetadataFromISP(isp_metadata * metadata);
    status_t getLensShading(LensShadingInfo *lensShadingInfo);
    void storeMetadata(aaaMetadataInfo *aaaMetadata);
    status_t getMetadata(int requestId, aaaMetadataInfo *aaaMetadata);

    /* IHWFlashControl overloads, */
    status_t setFlash(int numFrames);
    status_t setFlashIndicator(int intensity);
    status_t setTorch(int intensity);
    int setFlashIntensity(int intensity);
    status_t setTorchHelper(int intensity);

    /* IHWLensControl overloads, */
    int moveFocusToPosition(int position);
    int moveFocusToBySteps(int steps);
    int getFocusPosition(int * position);
    int getFocusStatus(int *status);

public:
    status_t selectCameraSensor();
    size_t setupCameraInfo();
    status_t initCameraInput();

private:
    status_t initMetadataQueue(void);
    bool validateHighSpeedResolutionFps(int width, int height, int fps);
    status_t registerToEvents(void);
    status_t notifyListeners(ICssIspListener::IspMessage *msg);
    const char *getLogString(const char *array[], unsigned int index);
    status_t init3AStatisticsQueue();
    void deinit3AStatisticsQueue();
    status_t store3AStatistics(unsigned int * exp_id);
    struct atomisp_3a_statistics * get3AStatistics(unsigned int exp_id);
    struct atomisp_3a_statistics * allocateStatistics();
    void freeStatistics(struct atomisp_3a_statistics *stats);
    status_t setRawBufferLockMode(bool enable);
    status_t setTestPattern(int32_t testPatternMode, int16_t *testPatternData = NULL);
    status_t copyLensShading(cmc_parsed_lens_shading_t *srcLenShadingInfo, cmc_parsed_lens_shading_t *destLenShadingInfo);
    void clearMetadata(aaaMetadataInfo *metadata);
private:
    static const int MAX_SENSOR_NAME_LENGTH = 32;
    struct cameraInfo {
        int port;            //!< AtomISP port type
        uint32_t index;      //!< V4L2 index
        char name[MAX_SENSOR_NAME_LENGTH];
    };

private:
    static cameraInfo sCamInfo[MAX_CAMERA_NODES];
    int mFlashTorchSetting;
    sp<V4L2VideoNode> mMainNode;
    sp<V4L2VideoNode> mPreviewNode;
    sp<V4L2VideoNode> mVideoNode;
    sp<V4L2Subdevice> mEventNode;
    int mCameraId;
    CameraMetadata mStaticMeta;
    const IPU2CameraCapInfo * mCapInfo;
    int32_t mSensorArraySize[2];
    int mMaxZoomFactor;  // depend on ISP
    float mMaxZoomRatio;

    int mCssMajorVersion;
    int mCssMinorVersion;
    int mIspHwMajorVersion;
    int mIspHwMinorVersion;

    CssIspMode mMode;
    bool mRawLockEnabled;
    bool mRawLockReady;

    ContinuousCaptureConfig mContCaptConfig;
    bool mContCaptPrepared;
    bool mContCaptPriority;

    Mutex mIspStateLock;        /*!< Protects the current ISP state  */
    bool mStarted;
    /* end of scope for lock mIspStateLock */

    /* Listeners management */
    Mutex   mListenerLock;  /*!< Protects accesses to the Listener management variables */
    KeyedVector<ICssIspListener::IspEventType, bool> mEventsProvided;
    typedef List< ICssIspListener* > listener_list_t;
    KeyedVector<ICssIspListener::IspEventType, listener_list_t > mListeners;
    /* end of scope for lock mListenerLock */

    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_POLL,
        MESSAGE_ID_FLUSH,
        MESSAGE_ID_MAX
    };

    union MessageData {
        int data;
    };

    struct Message {
        MessageId id;
        MessageData data;
    };

    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

    /**
     * ISP features
     */
    float mZoomRatio;

    v4l2_colorfx mColorEffect;
    bool mNoiseReductionEdgeEnhancement;

    /* Handle to ISP parameter adaptor */
    ia_isp *mIspHandle;

    Mutex m3AStatisticsLock;
    Vector<struct atomisp_3a_statistics *> m3aStatisticsQueue;

    // EMBEDDED METADATA
    SensorEmbeddedMetaData*  mSensorEmbeddedMetaData;
    bool mSensorEmbeddedMetaDataSupported;

    // ISP perframe settings
    bool mIspPerframeSettingsEnabled;

    int32_t mTrigger3A;
    ICssIspListener::IspMessage mCachedStatsEventMsg;
    bool mHasLensActuator;

    /* DVS */
    bool mDvsEnabled;
    IaDvs2 *mDvs;
    struct atomisp_dvs_6axis_config * mDvs6AxisCfg;
    struct atomisp_dis_coefficients * mDvsCoefs;

    IHWSensorControl*    mSensorCI;

    FrameInfo mPreviewInfo;
    FrameInfo mVideoInfo;

    int mVideoFps;
    I3AControls *m3AControls;

    // Device streams
    ICssBufferMaintainer *mPreviewHwStream;
    ICssBufferMaintainer *mVideoHwStream;
    ICssBufferMaintainer *mCaptureHwStream;

    struct atomisp_parameters mIspConfStruct;
    struct atomisp_dvs_6axis_config *mDvsConfig;
    bool mDvsConfigChange;
    int mFocalLength;

    int mAvailableBits;
    // lens shading
    // since the len shading table isn't caculated each frame,
    // so store the old one
    cmc_parsed_lens_shading_t mWeightedLsc;
    cmc_parsed_lens_shading_t mWeightedLscForIsp;
    Mutex mLscMapLock;
    Mutex mMetadataQueueLock;
    Vector<aaaMetadataInfo *> mMetadataQueue;

private:
    status_t handleMessagePoll(Message &msg);
    status_t handleMessageFlush(Message &msg);
};

} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_IPU2_HWISP_H_ */
