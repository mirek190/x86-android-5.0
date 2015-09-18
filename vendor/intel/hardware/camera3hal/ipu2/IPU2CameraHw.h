/*
 * Copyright (C) 2011,2012,2013 Intel Corporation
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

#ifndef _CAMERA3_HAL_IPU2_CAMERAHW_H_
#define _CAMERA3_HAL_IPU2_CAMERAHW_H_

#include "CameraMetadataHelper.h"
#include "PlatformData.h"
#include "Aiq3AThread.h"

#include "ipu2/IPU2HwIsp.h"
#include "ipu2/IPU2HwSensor.h"
#include "ipu2/FakeStreamManager.h"
#include "ipu2/FaceDetectionThread.h"
#include "ImgEncoder.h"
#include "JpegMaker.h"
#include "IPU2CameraCapInfo.h"

namespace android {
namespace camera2 {

class PreviewStream;
class CaptureStream;
class PostProcessStream;

#define PREVIEW_STREAM_NAME "preview"
#define VIDEO_STREAM_NAME "video"
#define CAPTURE_STREAM_NAME "capture"

/**
 * \enum
 * This enum is used as index when acquiring the partial result metadata buffer
 * Current design only returns metadata in one plane only: AAAThread
 * In the future we can split this in multiple places.
 * In theroy there should be one metadata partial result per thread context
 * that writes result
 */
enum PartialResultEnum{
    AAA_PARTIAL_RESULT = 0,
    PARTIAL_RESULT_COUNT /* keep last to use as counter */
};

/**
 * \class IPU2CameraHw
 *  IPU2CameraHw abstracts the Camera SubSystem HW
 *
 */
class IPU2CameraHw :
    public ICameraHw {

// constructor/destructor
public:
    explicit IPU2CameraHw(int cameraId);
    virtual ~IPU2CameraHw();

    status_t initDevice();
    void deInitDevice();
    bool isDeviceInitialized() const;
    IspMode getMode() { return mMode; }

// prevent copy constructor and assignment operator
private:
    IPU2CameraHw(const IPU2CameraHw& other);
    IPU2CameraHw& operator=(const IPU2CameraHw& other);

// override the ICameraHw
public:
    virtual status_t init(void);
    virtual const camera_metadata_t * getDefaultRequestSettings(int type);
    virtual status_t processRequest(Camera3Request* request, int inFlightCount);
    virtual status_t bindStreams(Vector<CameraStreamNode *> activeStreams);

    virtual status_t flush();
    virtual status_t configStreams(Vector<camera3_stream_t*> &activeStreams);
    virtual void dump(int fd);

// private types
private:
    enum V4L2_DEVICENODE {
        V4L2_MAIN_DEVICE = 0,
        V4L2_POSTVIEW_DEVICE,
        V4L2_PREVIEW_DEVICE,
        V4L2_RECORDING_DEVICE,
        V4L2_INJECT_DEVICE,
        V4L2_ISP_SUBDEV,
        V4L2_ISP_SUBDEV2,
        // max number of device
        V4L2_NODE_MAX
    };

    enum StreamConfigMode {
        CONFIG_NONE,

        CONFIG_PREVIEW,
        CONFIG_VIDEO,
        CONFIG_CAPTURE,
        CONFIG_CONTINUOUS,
        CONFIG_CONTINUOUS_VIDEO,

        // For ZSL use case, please see workaround in checkZslCapture()
        CONFIG_CONTINUOUS_ZSL,

        // For GCAM use case:
        // Metering phase: capture request with VGA YUV output (preview)
        // Payload phase: capture request with 5M YUV output   (capture)
        CONFIG_CONTINUOUS_GCAM,

    };

    // client streams that are used to configure HW video nodes
    struct StreamConfig {
        CameraStreamNode * mPreview;
        CameraStreamNode * mCapture;
        CameraStreamNode * mVideo;
        CameraStreamNode * mFakeStream;
        IspMode mIspMode;
        bool mEnableRawLock;
        StreamConfigMode mCfgMode;
    };

private:
    int mCameraId;
    int mGroupIndex;
    Mutex mISPCountLock;
    const IPU2CameraCapInfo * mCapInfo;
    CameraMetadata mStaticMeta;

    sp<IPU2HwIsp> mHwIsp;
    IPU2HwSensor mIPU2HwSensor;

    IspMode mMode;
    bool mEnableRawLock;

    int mInFlightCount;
    sp<V4L2VideoNode>  mMainDevice;
    sp<V4L2VideoNode>  mPreviewDevice;
    sp<V4L2VideoNode>  mActivePreviewDevice; // not used in MODE_VIDEO
    sp<V4L2VideoNode>  mVideoDevice;
    sp<V4L2VideoNode>  mPictureDevice;
    sp<V4L2VideoNode>  mPostviewDevice;
    sp<V4L2VideoNode>  mActivePictureDevice; // need exchange devices if postview size is bigger
    sp<V4L2VideoNode>  mActivePostviewDevice;
    sp<V4L2Subdevice>  mIspSubdevice;
    sp<V4L2Subdevice>  m3AEventSubdevice;
    sp<V4L2VideoNode>  mFileInjectDevice;

// Initialization
private:
    void destroyDevice();

    status_t initDeviceStreams();
    void destroyDeviceStreams();

    status_t init3A();
    void destroy3A();

    size_t setupCameraInfo();

    void getMaxSnapshotResolution();
    void getMaxPreviewResolution();
    void getMaxCaptureInfoWithRatio(FrameInfo *captureInfo, FrameInfo cInfo);


// Requested processing: ISP configuration & streams binding
private:
    status_t restart(StreamConfig & cfg, const CameraMetadata &settings);
    status_t stop();
    status_t contructDefaultDynamicMetadata(int type, camera_metadata_t *metadata, isp_metadata *ispMetadata);
    // Store requested streams
    void setRequestedStreams(Camera3Request* request);
    // Figure out expected ISP mode
    StreamConfigMode analyzeConfigMode(const CameraMetadata & settings);
    // Pick out client's streams that are used to configure device nodes
    bool analyzeStreams(StreamConfigMode cfgMode, StreamConfig & cfg);
    status_t checkCaptureWithoutPreview(StreamConfig & config);

    status_t processIspSettings(IspMode mode, const CameraMetadata &settings);
    status_t configureIsp(IspMode mode, bool enableEawLock = false);
    // Link stream nodes to all client's streams
    status_t bindStreams(StreamConfig & config);
    status_t bindStreamsForPreview(CameraStreamNode * clientPreview);
    status_t bindStreamsForCapture(CameraStreamNode * clientCapture);
    status_t bindStreamsForVideo(CameraStreamNode * clientVideo, CameraStreamNode * clientPreview);
    status_t bindStreamsForContinuous(CameraStreamNode * clientCapture, CameraStreamNode * clientPreview);
    status_t bindStreamsForContinuousVideo(
                                          CameraStreamNode * clientCapture,
                                          CameraStreamNode * clientVideo,
                                          CameraStreamNode * clientPreview);
    status_t bindToHwStream(CameraStreamNode * clientStream,
                                    HwStream * hwStream);
    status_t linkToPostProcessStream(CameraStreamNode * clientS, HwStream * hwS);
    bool isPreviewSizeSupported(CameraStreamNode * stream);
    bool isCurrentConfigMatchedForCapture();

    PreviewStream* selectHWStreamForVideoSnapshot(void);

    // ISP HW streams, mapping to HW video nodes
    PreviewStream* mPreviewHwStream;   // dev2
    CaptureStream* mCaptureHwStream;   // dev0 & dev1
    PreviewStream* mVideoHwStream;   // dev3
    StreamConfig mStreamConfig;

    // PostProcess streams
    Vector<sp<PostProcessStream> > mPostStreams;

    // Client streams
    bool mJpegStreamConfigured;
    Vector<CameraStreamNode *> mActiveStreams; // size <= 5
    Vector<CameraStreamNode *> mInStreams; // size <= 1
    Vector<CameraStreamNode *> mOutStreams; // size <= 5
    Vector<CameraStreamNode *> mOutYuvStreams; // size <= 3
    Vector<CameraStreamNode *> mOutJpegStreams;   // size <= 1
    Vector<CameraStreamNode *> mOutRawStreams;    // size <= 1
    // Store YUV stream with full resolution (temporarily solution)
    Vector<CameraStreamNode *> mOutZslStreams;    // size <= 1
    int mAvailableStreams;

    int mBurstLength;

    sp<ImgEncoder> mImgEncoder;
    JpegMaker *mJpegMaker;

// 3A Controls
    I3AControls *m3AControls;
    Aiq3AThread * mAAAProcessor;

    bool mLowLight;
    int mXnr;
    int mRawDataDumpSize;
    int mFrameSyncRequested;
    int m3AStatRequested;
    bool mFrameSyncEnabled;
    bool m3AStatscEnabled;
    bool mNoiseReductionEdgeEnhancement;

    FrameInfo mMaxJpegConfig;

    int mMaxPreviewWidth;
    int mMaxPreviewHeight;

    /*
     * Fake stream provides buffers for device to support special use cases:
     * 1. video without preview: provide preview stream buffers because
     *     ISP video pipe need two stream buffers: video & preview
     */
    FakeStreamManager mFakeStreamMgr;
    // face detection
    FaceDetectionThread* mFaceDetectionThread;
    int32_t* mMaxRegions;
}; // class IPU2CameraHw

}; // namespace camera2
}; // namespace android

#endif // _CAMERA3_HAL_IPU2_CAMERAHW_H_
