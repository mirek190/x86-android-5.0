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
#ifndef CAMERA3_HAL_AAA_REQUEST_MANAGER_H_
#define CAMERA3_HAL_AAA_REQUEST_MANAGER_H_

#include "I3AControls.h"
#include "ICameraHwControls.h"
#include "ia_cmc_parser.h"


namespace android {
namespace camera2 {

// Color correction transform metadata is a 3x3 rational matrix.
// The precision of the rational is required to be 1/10000 by Android.
#define COLOR_TRANSFORM_PRECISION 10000

typedef struct {
    uint8_t aeState;
    uint8_t awbState;
    uint8_t afState;
    int32_t aeTriggerId;
    int32_t afTriggerId;
} aaa_state_metadata;

typedef struct {
    int32_t iso;
    int64_t exposureTimeNs;
    int64_t frameDuration;
    FlickerMode sceneFlicker;
} sensor_metadata;

class Aiq3ARequestManager {
public:
    Aiq3ARequestManager(HWControlGroup &hwcg,
                               I3AControls * aaa,
                               CameraMetadata &staticMeta);
    virtual ~Aiq3ARequestManager();
    void reset(IspMode mode);

    bool storeNewRequest(Camera3Request *request, int availableStreams);
    status_t handleNewRequest(int request_id);

    void storeRequestForShutter(Camera3Request* request);

    status_t applySettings(unsigned int exp_id);

    Camera3Request* getNextRequest(int reqId = INVALID_REQ_ID);

    status_t notifyResult(int request_id, unsigned int exp_id,
                          aaa_state_metadata aaa, int* faceIds, int* faceLandmarks,
                          int* faceRect, uint8_t* faceScores, int faceNum,
                          FrameBufferStatus flashStatus,
                          uint8_t* aePrecaptureTrigger);
    status_t checkMetadataDone(int reqId, int expId);
    status_t notifyShutter(int expId, struct timeval /*timestamp*/);

    status_t storeAutoParams(ispInputParameters * isp,
                                ia_aiq_exposure_parameters * exposure);

    status_t init();

private:
    enum request_state {
        AAA_REQUEST_NEW = 0,     // NEW request that has not been processed.
        AAA_REQUEST_PROCESSING,  // Request is in processing
        AAA_REQUEST_PROCESSED,   // Request is processed, waiting for the frame which
                                 // is delayed as sensor settings has delay.
    };

    typedef struct {
        float colorTransform[9];
        float colorGain[4];
    } color_params;

    typedef struct {
        // color correction params
        float colorTransform[9];
        float colorGain[4];
        // lens shading map params
        uint16_t lscWidth;
        uint16_t lscHeight;
        float *lscGridInRGGB;
        bool lscValid;
        // tonemap params
        unsigned int gammaLutSize;
        float* rGammaLut;
        float* gGammaLut;
        float* bGammaLut;
        //black level lock
        color_channels_t colorChannel;
    } isp_params;

    typedef struct {
        // request info
        Camera3Request* request;
        int mReqId;
        bool newSettings;
        bool manualControlMode;
        bool manualAeMode;
        bool noiseReduction;
        bool edgeEnhancement;
        bool lensShading;
        bool blackLevellock;
        int availableStreams;

        // Manual settings
        color_params *manualColor;

        bool tonemapContrastCurve;
        unsigned short* rGammaLut;
        unsigned short* gGammaLut;
        unsigned short* bGammaLut;
        unsigned int rGammaLutSize;
        unsigned int gGammaLutSize;
        unsigned int bGammaLutSize;

        // status
        request_state state;
        bool applied;
        int expId;
    } aaa_request;

private:
    status_t reallocateLscGrid(uint16_t width, uint16_t height);
    status_t reFormatLensShadingMap(uint16_t width, uint16_t height, uint16_t *srcLscGrid, float *destLscGridInRGGB);
    status_t processManualExposure(aaa_request &new_request);
    status_t processManualColorCorrection(aaa_request &new_request);
    status_t processManualTonemap(aaa_request &new_request);
    void    storeGbceResults(void);
    status_t storeLensShadingMap(uint16_t width, uint16_t height, uint16_t *lscGrid);
    status_t getTonemapCurve(const CameraMetadata settings,
                                     unsigned int tag,
                                     unsigned int *gammaLutSize,
                                     unsigned short ** gammaLut);
    status_t applyIspSettings(aaa_request &aaaRequest);

    // Fill metadatas
    bool getAndFillSensorMetadata(
                              /* out */
                              Camera3Request * request,
                              /* in */
                              const aaa_request & aRequest);
    void update3aRegion(CameraMetadata & settings,
                        int32_t *aeRegion);
    status_t fill3aRegion(CameraMetadata & settings,
                          const aaa_request & aaaRequest, unsigned int tag);
    status_t fillScalerCropRegion(CameraMetadata & settings,
                                  const aaa_request & aaaRequest);
    status_t fillColorCorrection(CameraMetadata & settings,
                                 const aaa_request & aaaRequest);
    status_t fillLensShadingMap(CameraMetadata & settings,
                                const aaa_request & aaaRequest);
    status_t fillTonemapCurve(CameraMetadata & settings,
                              const aaa_request & aaaRequest);
    status_t fillRollingShutterSkew(CameraMetadata & settings);
    status_t fillLensDynamic(CameraMetadata & settings);
    status_t fillFrameduration(CameraMetadata & settings);
    status_t getRollingShutterSkew(void);
    void fillBlackLevelLock(CameraMetadata & settings,
                               const aaa_request & aaaRequest);

    void dump();
    void aaaRequestClear(aaa_request & aaaRequest);
    void resultError(Camera3Request* request);

private:
    IHWIspControl *mISP;
    IHWSensorControl*    mSensorCI;
    I3AControls *m3AControls;
    CameraMetadata mStaticMeta;

    Mutex mPendingQueueLock;
    Vector<aaa_request> mPendingRequestsQueue;
    int mReqIdFrameDone;
    /* The flag to trigger metadata notification in sensor embedded event.
     * If sensor embedded event is later than frame done event, we will
     * recheck in next frame done event.
     * If it is frame for the last request, we should check in subsequent
     * sensor embedded events because of no next frame done event.
     */
    bool mOneRequestLeftWaitingForEmbedded;
    // To protect mReqQForShutter for thread-safety
    Mutex mReqShutterLock;
    Vector<Camera3Request*> mReqQForShutter;

    bool mGlobalInited;
    ispInputParameters mISPParams;
    ia_aiq_exposure_parameters mExposure;
    isp_params mAutoISPMetadata;

    int mSensorDelay;

    bool mFirstRequest;
    unsigned int mFrameSkips;

    int32_t mActiveArraySize[4];
    int32_t mMinSensitivity;
    int32_t mMaxSensitivity;
    int64_t mMinExposureTime;
    int64_t mMaxExposureTime;

    // LSC grid info
    int mColorOrder;
    float *mLscGrid;
    uint16_t *mLscResizeGrid;
    LensShadingInfo mLensShadingInfo;

    sensor_metadata mLastSensorMetadata;
    uint32_t mMaxTonemapCurvePoint;

    bool mNoiseReduction;
    bool mEdgeEnhancement;
    // rollingShutterSkew
    int64_t mRollingShutterSkew;
    Mutex mRollingShutterSkewLock;
    bool mHasFlash;
    // block level lock
    bool mBlackLevelLockOn;
    color_channels_t mColorChannel;
    float mPixelSize;
    float mFreFocusDistance;
    uint32_t mPreExposureTime;

    // lens
    float* mLensAperture;
    float* mLensFilterDensity;
    int32_t* mLensShadingMapSize;
    bool mLensShadingMapMode;

    int32_t* mMaxRegions;
    aaaMetadataInfo mAaaMetadata;
    int64_t mLastExposureTimeNs;
};

};  // namespace camera2
};  // namespace android

#endif  // CAMERA3_HAL_AAA_REQUEST_MANAGER_H_
