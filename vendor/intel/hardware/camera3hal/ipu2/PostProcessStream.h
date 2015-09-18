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
#ifndef CAMERA3_HAL_POSTPROCESS_STREAM_H_
#define CAMERA3_HAL_POSTPROCESS_STREAM_H_
#include <utils/threads.h>
#include "MessageQueue.h"
#include "HwStream.h"
#include "IPU2CameraHw.h"
#include "CameraBuffer.h"
#include "MessageThread.h"
#include "ImgEncoder.h"
#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
#include "iVP.h"
#endif

namespace android {
namespace camera2 {

class PostProcessStream
    :public HwStream,
     public ICssIspListener,
     public IMessageHandler {
public:
    enum PostProcessMode {
        POSTPROCESSMODE_NONE,
        POSTPROCESSMODE_COPY,
        POSTPROCESSMODE_DOWNSCALING_AND_COLOER_CONVERSION,
        POSTPROCESSMODE_JPEG_ENCODING,
        POSTPROCESSMODE_JPEG_DECODING,
        POSTPROCESSMODE_ZSL  // for zsl stream
    };
public:
    PostProcessStream(sp<ImgEncoder> imgEncoder
                         , JpegMaker *jpegMaker
                         , I3AControls *i3AControls
                         , IPU2HwIsp *hwIsp
                         , IPU2HwSensor *hwSensor
                         ,int cameraid);
    static int selectPostProcessMode(FrameInfo & outInfo, FrameInfo & inInfo);

    virtual ~PostProcessStream();
    /* HwStream overloads */
    virtual status_t init();
    virtual status_t setFormat(FrameInfo *aConfig);
    virtual status_t capture(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);
    virtual status_t reprocess(sp<CameraBuffer> aBuffer,
                             Camera3Request* request);

    void dump(int fd) const;

    virtual status_t stop();
    virtual status_t flush();
    virtual const char* getName() const { return "postprocess"; }
    void setMode(int mode);
    int getMode() { return mMode; }
    // ICssIspListener override
    bool notifyIspEvent(ICssIspListener::IspMessage *msg);


// prevent copy constructor and assignment operator
private:
    PostProcessStream(const PostProcessStream& other);
    PostProcessStream& operator=(const PostProcessStream& other);

private:
    sp<V4L2VideoNode>   mVideoNode;
    IPU2CameraHw *mHw;
    int mMode;

    Mutex               mBufBookKeepingLock;
    /*!< The  mutex above ensures thread safe access to the  buffer book keeping vectors*/
    Vector<int> mRawLockInfo;  // Save exp_id for consumer buffers
    /* End of scope for mutex mBufBookKeepingLock */

    Mutex mReceivedLock;
    Vector<sp<CameraBuffer> > mReceivedBuffers;
    Vector<Camera3Request *> mReceivedRequests;
    struct {
        int exp_id;
        int request_id;
    } mReprocessInfo;
    bool mRawLockEnabled;

    // Allocate internal buffer only when it is a consumer
    Vector<sp<CameraBuffer> > mInternalBuffers;
    unsigned mInternalBufferIndex;
    unsigned mBuffersNumInDevice;
public:
    /* CameraStreamNode overloads */
    virtual status_t query(FrameInfo *info);
    virtual status_t captureDone(sp<CameraBuffer> buffer,
                                 Camera3Request* request);
private:
    // thread message id's
    enum MessageId {
        MESSAGE_ID_EXIT = 0,
        MESSAGE_ID_ENCODE,
        MESSAGE_ID_MAX
    };

    struct Message {
        MessageId id;
        ImgEncoder::EncodePackage package;
        Camera3Request* request;
    };
    MessageQueue<Message, MessageId> mMessageQueue;
    sp<MessageThread> mMessageThead;
    bool mThreadRunning;
    /* IMessageHandler overloads */
    virtual void messageThreadLoop(void);

private:
    virtual status_t configure(void);

    status_t allocateBuffers(int bufsNum);
    status_t freeBuffers();

    sp<CameraBuffer> getOneInternalBuffers();
    void releaseOneInternalBuffers();

    status_t notifyCaptureDone(sp<CameraBuffer> buffer,
                               Camera3Request* request);
    void notifyFrameToListeners(sp<CameraBuffer> buffer);
    status_t handleJpegEncode(Message &msg);

#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
    status_t cameraBuffer2iVPLayer(sp<CameraBuffer> cameraBuffer,
                                iVP_layer_t *iVPLayer);
    status_t iVPColorConversion(sp<CameraBuffer> srcBuf,
                                sp<CameraBuffer> destBuf);
#endif
    status_t handleDownscalingColorConversion(
                                sp<CameraBuffer> srcBuf,
                                sp<CameraBuffer> destBuf,
                                Camera3Request* request);
    status_t fillPicMetaDataFromHw(sp<ExifMetaData> metadata, unsigned int exp_id);
    status_t fillPicMetaDataFrom3A(sp<ExifMetaData> metadata, int requestId);
    status_t calcuAecApexTv(sp<ExifMetaData> metadata, ImgEncoder::EncodePackage & package);

    status_t captureDoneForZsl(ICssIspListener::IspMessage *msg,
                               Camera3Request* request);

    bool checkRawLockInfo(ICssIspListener::IspMessage *msg);

private:
    sp<ImgEncoder> mImgEncoder;
    JpegMaker * mJpegMaker;
    sp<CameraBuffer> mScaledBuf;
    I3AControls *m3AControls;
    IPU2HwIsp *mIsp;
    IPU2HwSensor *mSensor;
    int mCameraId;
    static const unsigned int REAL_BUF_NUM = 4;
#if defined(GRAPHIC_IS_GEN) && !defined(GRAPHIC_GEN8)
    bool miVPCtxValid;
    iVPCtxID miVPCtx;
#endif
    atomisp_makernote_info ispMakerNote;
};

} /* namespace camera2 */
} /* namespace android */
#endif /* CAMERA3_HAL_POSTPROCESS_STREAM_H_ */
