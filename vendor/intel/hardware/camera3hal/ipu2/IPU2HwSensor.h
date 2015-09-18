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

#ifndef CAMERA3_HAL_IPU2_HWSENSOR_H_
#define CAMERA3_HAL_IPU2_HWSENSOR_H_

#include "ICameraHwControls.h"
#include "PlatformData.h"
#include "AtomFifo.h"
#include "MessageThread.h"
#include "ipu2/IPU2HwIsp.h"
#include "IPU2CameraCapInfo.h"

namespace android {
namespace camera2 {

#define FRAME_STORED_LENGTH 32
#define FRAME_LIST_LENGTH (FRAME_STORED_LENGTH * 2)

#define INVALID_REQ_ID (-1)
#define MIN_EXP_ID 1
#define MAX_EXP_ID 250
#define INVALID_EXP_ID (MIN_EXP_ID-1)

#define MAX_EXPOSURE_HISTORY_SIZE 5

#define EXPNUM2ID(count) ((count-1)%MAX_EXP_ID + 1)
#define EXPNUM2INDEX(count) ((count)%FRAME_LIST_LENGTH)

#define REQID2INDEX(reqId) ((reqId)%FRAME_LIST_LENGTH)

class ICssIspListener;
class V4L2DeviceBase;
class V4L2VideoNode;
class V4L2Subdevice;

class IPU2HwSensor : public IHWSensorControl,
                         public ICssIspListener,
                         public IMessageHandler{

public:
    IPU2HwSensor(int cameraId);
    ~IPU2HwSensor();
    status_t selectActiveSensor(sp<V4L2VideoNode> &device);
    status_t initSubDevice();

    /* IHWSensorControl overloads, */
    virtual const char * getSensorName(void);
    virtual int getCurrentCameraId(void);
    virtual void getMotorData(sensorPrivateData *sensor_data);
    virtual void getSensorData(sensorPrivateData *sensor_data);
    virtual void generateNVM(ia_binary_data **output_nvm_data);
    virtual void releaseNVM(ia_binary_data *nvm_data);
    virtual int getFrameParameters(ia_aiq_frame_params *frame_params,
                                   ia_aiq_exposure_sensor_descriptor *sensor_descriptor,
                                   unsigned int output_width, unsigned int output_height);
    virtual int setExposureTime(int time);
    virtual int getExposureTime(int *exposure_time);
    virtual int getAperture(int *aperture);
    virtual int getFNumber(unsigned short  *fnum_num, unsigned short *fnum_denom);
    virtual int setExposureMode(v4l2_exposure_auto_type type);
    virtual int getExposureMode(v4l2_exposure_auto_type * type);
    virtual int setExposureBias(int bias);
    virtual int getExposureBias(int * bias);
    virtual int setSceneMode(v4l2_scene_mode mode);
    virtual int getSceneMode(v4l2_scene_mode * mode);
    virtual int setWhiteBalance(v4l2_auto_n_preset_white_balance mode);
    virtual int getWhiteBalance(v4l2_auto_n_preset_white_balance * mode);
    virtual int setIso(int iso);
    virtual int getIso(int * iso);
    virtual int setAeMeteringMode(v4l2_exposure_metering mode);
    virtual int getAeMeteringMode(v4l2_exposure_metering * mode);
    virtual int setAeFlickerMode(v4l2_power_line_frequency mode);
    virtual int setAfMode(v4l2_auto_focus_range mode);
    virtual int getAfMode(v4l2_auto_focus_range * mode);
    virtual int setAfEnabled(bool enable);
    virtual int set3ALock(int aaaLock);
    virtual int get3ALock(int * aaaLock);
    virtual int setAeFlashMode(v4l2_flash_led_mode mode);
    virtual int getAeFlashMode(v4l2_flash_led_mode * mode);
    virtual int getModeInfo(struct atomisp_sensor_mode_data *mode_data);
    virtual int getRawFormat();
    // TODO: replacing fixed value of AE_DELAY_FRAMES in Aiq3A.h in non-functional API refactory
    //       this value exists in CPF and needs awareness of frames timing.
    virtual unsigned int getExposureDelay() { return (unsigned int)(mCapInfo->exposureLag()); }

    virtual unsigned int getFrameSkip() { return (unsigned int)(mCapInfo->frameInitialSkip()); }

    virtual bool supportPerFrameSettings() { return mCapInfo->supportPerFrameSettings(); }

    virtual float getStepEV() { return mStepEV; }

    virtual int setExposure(unsigned short fine_integration_time,
                            unsigned short coarse_integration_time,
                            unsigned short analog_gain_global,
                            uint16_t digital_gain_global);
    virtual int storeRequestExposure(int request_id,
                                          bool manualExposure,
                                          struct atomisp_exposure *exposure);
    virtual int storeAutoExposure(bool clearOldValues,
                                 unsigned short coarse_integration_time,
                                 unsigned short fine_integration_time,
                                 unsigned short analog_gain_global,
                                 uint16_t digital_gain_global);
    virtual int applyExposure(unsigned int exp_id);
    virtual float getFrameDuration(void);
    virtual float getFrameRate();
    virtual int setFrameRate(int fps);
    virtual int setFramelines(int fl);
    virtual int getExposurelinetime();

    /*
     * Capture owner will match RAW buffer and requests.
     * When Capture owner enqueue buffer to ISP, it will call
     * setExpectedCaptureExpId() to bind request and RAW output frame
     * for this buffer.
     */
    void setCaptureSyncOwner(void * owner, int invalidFrames = 0);
    void setExpectedCaptureExpId(int reqId, void * owner);
    /*
     * In offline mode, preview stream is capture owner.
     * If the user send snapshot request without preview,
     * preview stream will insert fake buffers to ISP, and capture stream need
     * to decide which RAW output is used to trigger capture for this request.
     */
    void findFrameForCapture(int reqId, bool rawLockEnabled = false);
    bool verifyCapturedExpId(int reqId, int expId);
    int getExpectedReqId(int expId);
    int getExpIdForRequest(int reqId);

    int64_t getShutterTime(int reqId);
    int64_t getShutterTimeByExpId(int exp);
    int64_t _getSOFByExpNum(int exp);
    int64_t getSOFTime(int reqId);
    void clearRequestQueue();

    void vbiIntervalForItem(unsigned int index,
                            unsigned int & vbiOffset,
                            unsigned int & shutterOffset);

    /**********************************************************************
     * ICssIspListener override
     */
    bool notifyIspEvent(ICssIspListener::IspMessage *msg);

private:
    static const int MAX_SENSOR_NAME_LENGTH = 32;

    struct cameraInfo {
        uint32_t index;      //!< V4L2 index
        char name[MAX_SENSOR_NAME_LENGTH];
    };

    struct requestExposure {
        int requestId;
        bool manualExposure;
        struct atomisp_exposure exposure;
    };

    /**********************************************************************
     * Thread methods
     */
    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_SOF,
        MESSAGE_ID_MAX
    };

    // message id and message data
    struct MessageSOF {
        unsigned int exp_id;
        struct timeval timestamp;
        unsigned int vbiOffset;
    };
    union MessageData {
        MessageSOF sof;
    };
    struct Message {
        MessageId id;
        MessageData data;
    };

    size_t enumerateInputs(Vector<struct cameraInfo> &);
    status_t sensorStoreRawFormat(Vector<v4l2_fmtdesc> &formats);
    status_t openSubdevices();
    status_t openSubdevice(sp<V4L2DeviceBase> &subdev, int major, int minor);

    void updateModeData();
    status_t applySensorFlip();

    status_t findMediaEntityByName(sp<V4L2DeviceBase> &mediaCtl, char const* entityName,
                                   struct media_entity_desc &mediaEntityDesc);
    status_t findMediaEntityById(sp<V4L2DeviceBase> &mediaCtl, int index,
                                   struct media_entity_desc &mediaEntityDesc);

    status_t produceExposureHistory(struct atomisp_exposure exposure);

    void storeSOFTime(int expId, int64_t sofTime, int64_t shutterTime);

    status_t newSOF(ICssIspListener::IspMessageEvent * sofMsg);
    status_t handleNewSOF(Message & msg);

    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

private:

    Mutex mExposureLock;
    sp<V4L2DeviceBase> mSensorSubdevice;
    sp<V4L2VideoNode> mDevice;
    SensorType        mSensorType;
    int mCameraId;
    struct cameraInfo mCameraInput;
    int mRawBayerFormat;
    const IPU2CameraCapInfo *mCapInfo;
    static const int LSC_TABLE_NUM = 2;
    float mStepEV;

    // ModeData stored
    Mutex mModeDataLock;
    struct atomisp_sensor_mode_data mInitialModeData;
    bool mInitialModeDataValid;

    Vector<requestExposure> mPendingRequestsQueue;
    //struct atomisp_exposure mAutoExposure;
    Vector<struct atomisp_exposure> mAutoExposure;
    bool mPerFrameSettingsMode;

    Mutex mExpectedLock;
    void * mSyncOwner;
    struct FrameExpInfo {
        int expId;
        int reqId;
        int64_t sofTime;
        int64_t shutterTime;
    };
    /*
     *                    receivedSOF |       | expectedExp
     * mFrameList: [... * * * * * * * * * * * * * * * * * * * * ...]
     *                         SOF received <-|-> waiting for SOF
     *                    |<-STORED_LENGTH -> |
     * expNum map one-for-one to buffers of sync owner that are enquened to ISP
     */
    FrameExpInfo mFrameList[FRAME_LIST_LENGTH];
    int mExpectedExpNum;
    int mReceivedSOF;
    int mExpOfRequests[FRAME_LIST_LENGTH];// map requests to expNum
    int mRequestHead;      // the latest received request
    int mRequestCaptured;
    // ExposureHistory. Iit is always called in ISP poll thread, so no lock
    AtomFifo <struct atomisp_exposure> *mExposureHistory;

    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    float mFrameDuration;
    int mLastFps;
    int64_t _getShutterTimeByExpId(int exp);
}; // class SensorHW

}; // namespace camera2
}; // namespace android

#endif /* CAMERA3_HAL_IPU2_HWSENSOR_H_ */
