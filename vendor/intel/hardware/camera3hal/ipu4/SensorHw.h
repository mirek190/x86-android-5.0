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

#ifndef CAMERA3_HAL_SENSORHW_H_
#define CAMERA3_HAL_SENSORHW_H_

#include "ICameraIPU4HwControls.h"
#include "PlatformData.h"
#include "IPU4CameraCapInfo.h"
#include "MessageThread.h"
#include "MessageQueue.h"
#include "Camera3Request.h"
#include "PollerThread.h"
#include "Intel3aPlus.h"

namespace android {
namespace camera2 {

#define FRAME_SYNC_POLL_TIMEOUT 500

class V4L2Subdevice;
class MediaController;
class MediaEntity;
class PollerThread;

typedef enum {
    SUBDEV_PIXEL_ARRAY,
    SUBDEV_BINNER,
    SUBDEV_SCALER,
    SUBDEV_ISYSBACKEND
} sensorEntityType;

/**
 * \class SensorHw
 * This class adds the methods that are needed
 * to format and control the camera sensor via v4l2 and custom ioctls
 *
 */
class SensorHw : public RefBase,
                 public IIPU4HWSensorControl,
                 public IMessageHandler,
                 public IPollEventListener {

public:
    SensorHw(int cameraId, sp<MediaController> mediaCtl);
    ~SensorHw();

    status_t init();
    status_t requestExitAndWait();
    status_t start();
    bool isStarted() { return mStarted; }
    status_t stop();
    status_t flush();
    status_t setParameters(ia_aiq_ae_results ae_results);

    status_t setSubdev(sp<MediaEntity> entity, sensorEntityType type);

    virtual int getCurrentCameraId(void);
    virtual int getActivePixelArraySize(int &width, int &height, int &code);
    virtual int getSensorOutputSize(int &width, int &height, int &code);
    virtual int getPixelRate(int &pixel_rate);
    virtual int getLinkFreq(int &bus_freq);
    virtual int getPixelClock(int64_t &pixel_clock);
    virtual int setExposure(int coarse_exposure, int fine_exposure);
    virtual int getExposure(int &coarse_exposure, int &fine_exposure);
    virtual int getExposureRange(int &exposure_min, int &exposure_max, int &exposure_step);
    virtual int setGains(int analog_gain, int digital_gain);
    virtual int getGains(int &analog_gain, int &digital_gain);
    virtual int setFrameDuration(unsigned int llp, unsigned int fll);
    virtual int getFrameDuration(unsigned int &llp, unsigned int &fll);
    virtual int getVBlank(unsigned int &vblank);
    virtual int getAperture(int &aperture);
    virtual int getFNumber(unsigned int &fnum_num,
                            unsigned int &fnum_denom);

    /* ICaptureEventListener interface*/
    status_t notifyPollEvent(PollEventMessage *pollEventMsg);
private:
    int mCameraId;
    const IPU4CameraCapInfo *mCapInfo;
    sp<MediaController> mMediaCtl;
    sp<PollerThread> mPollerThread;

    sp<V4L2Subdevice> mSensorSubdev;
    sp<V4L2Subdevice> mBinnerSubdev;
    sp<V4L2Subdevice> mScalerSubdev;
    sp<V4L2Subdevice> mIsysBackendSubdev;
    Vector<sp<V4L2Subdevice> > mDevicesToPoll;

    SensorType  mSensorType;

    //Thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0, // messages from client
        MESSAGE_ID_INIT,
        MESSAGE_ID_START,
        MESSAGE_ID_STOP,
        MESSAGE_ID_FLUSH,
        MESSAGE_ID_SET_PARAMS,
        MESSAGE_ID_SOF,     // messages from sensor
        MESSAGE_ID_EOF,
        MESSAGE_ID_MAX      // error
    };

    //frame sync
    enum FrameSyncSource {
        FRAME_SYNC_NA,
        FRAME_SYNC_SOF = V4L2_EVENT_FRAME_SYNC,
        FRAME_SYNC_EOF = V4L2_EVENT_FRAME_END
    }mFrameSyncSource;
    Mutex mFrameSyncMutex;

    int mRawBayerFormat;
    int mPixelRate;
    int mHorzBlank;
    int mVertBlank;
    int mCropWidth;
    int mCropHeight;

    // message id and message data
    struct MessageEOF {
        unsigned int exp_id;
        struct timeval timestamp;
        unsigned int vbiOffset;
    };

    struct MessageSOF {
        unsigned int exp_id;
        struct timeval timestamp;
    };

    struct MessageAeParams {
        ia_aiq_ae_results aeResults;
    };

    union MessageData {
        MessageAeParams aeParams;
        MessageEOF eof;
        MessageSOF sof;
    };

    struct Message {
        MessageId id;
        MessageData data;
    };

    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThread;
    ia_aiq_ae_results mAeResults;
    bool mFilled;
    bool mStarted;
    bool mThreadRunning;

private:
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);
    status_t handleMessageInit();
    status_t handleMessageStart();
    status_t handleMessageStop();
    status_t handleMessageFlush();
    status_t handleMessageSetParams(Message &msg);
    status_t handleMessageSOF();
    status_t handleMessageEOF();
    status_t handleMessageExit();

}; // class SensorHW

}; // namespace camera2
}; // namespace android

#endif
