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

#ifndef _CAMERA3_HAL_SOC_3A
#define _CAMERA3_HAL_SOC_3A

#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <utils/String8.h>

#include "I3AControls.h"
#include "ICameraHwControls.h"

namespace android {
namespace camera2 {
/**
 * \macro INVALID_STUB
 *
 * Local utility macro to reduce repeated code
 * This macro is used to create the stub implementation of a method that takes
 * one input parameter
 */
#define INVALID_STUB(x) {UNUSED(x); return INVALID_OPERATION;}
/**
 * \class Soc3A
 *
 * Soc3A implement I3AControls interface for SOC sensor.
 *
 * SOC sensor driver will provide simple 3A control functionality.
 * Class Soc3A encapsulates these device operation.
 *
 * All access to sensor driver for 3A related queries will go via Soc3A.
 *
 */
class Soc3A : public I3AControls {
private:
    // constructor/destructor
    // prevent copy constructor and assignment operator
    Soc3A(const Soc3A& other);
    Soc3A& operator=(const Soc3A& other);

public:
    Soc3A(int cameraId, HWControlGroup &hwcg);
    Soc3A(int cameraId);
    virtual ~Soc3A();

// public methods
public:
    virtual status_t init3A();
    virtual status_t deinit3A();
    virtual status_t attachHwControl(HWControlGroup &hwcg);
    virtual status_t setAeMode(AeMode mode);
    virtual AeMode getAeMode();
    virtual status_t setEv(float bias);
    virtual status_t getEv(float *ret);
    virtual status_t setFrameRate(int fps) INVALID_STUB(fps)
    virtual status_t setAeSceneMode(SceneMode mode);
    virtual SceneMode getAeSceneMode();
    virtual status_t setAwbMode(AwbMode mode);
    virtual AwbMode getAwbMode();
    virtual status_t setAwbZeroHistoryMode(bool en) INVALID_STUB(en)
    virtual status_t setManualIso(int iso);
    virtual status_t getManualIso(int *ret);
    /** expose iso mode setting*/
    virtual status_t setIsoMode(IsoMode mode);
    virtual IsoMode getIsoMode(void);
    virtual status_t setAeMeteringMode(MeteringMode mode);
    virtual MeteringMode getAeMeteringMode();
    virtual status_t set3AColorEffect(EffectMode effect);
    virtual status_t setAeFlickerMode(FlickerMode flickerMode);
    FlickerMode getAeFlickerMode() { return CAM_AE_FLICKER_MODE_NOT_SET; }
    virtual status_t setAfMode(AfMode mode);
    virtual AfMode getAfMode();
    virtual status_t setAfEnabled(bool en);
    int     get3ALock(); // helper method for 3A lock setters/getters
    virtual status_t setNRMode(bool enable) INVALID_STUB(enable)
    virtual status_t setEdgeMode(bool enable) INVALID_STUB(enable)
    virtual status_t setShadingMode(bool enable) INVALID_STUB(enable)
    virtual status_t setEdgeStrength(int strength) INVALID_STUB(strength)
    virtual bool     getAeLock();
    virtual status_t setAeLock(bool en);
    virtual bool     getAfLock();
    virtual status_t setAfLock(bool en);
    virtual status_t setAwbLock(bool en);
    virtual bool     getAwbLock();
    virtual status_t applyEv(float bias);
    virtual status_t setManualShutter(float expTime);
    virtual status_t setManualFrameDuration(uint64_t nsFrameDuration);
    virtual status_t setManualFrameDurationRange(int minFps, int maxFps);
    virtual status_t setManualFocusDistance(int distance) INVALID_STUB(distance)
    virtual status_t setAeFlashMode(FlashMode mode);
    virtual FlashMode getAeFlashMode();
    virtual void setPublicAeMode(AeMode mode);
    virtual AeMode getPublicAeMode();
    virtual void setPublicAfMode(AfMode mode);
    virtual AfMode getPublicAfMode();
    virtual bool isAeConverged(void)  { return true; }
    virtual bool isAwbConverged(void)  { return true; }
    virtual void FaceDetectCallbackFor3A(ia_face_state *facestate);
    virtual void FaceDetectCallbackForApp(camera_frame_metadata_t *face_metadata);

    // Only supported by Intel 3A
    virtual bool isIntel3A();
    virtual status_t getAeManualBrightness(float *ret);
    virtual size_t   getAeMaxNumWindows();
    virtual size_t   getAfMaxNumWindows();
    virtual status_t setAeWindow(const CameraWindow *window);
    virtual status_t setAfWindows(const CameraWindow *windows,
                                  size_t numWindows);
    virtual status_t setManualFocusIncrement(int step);
    virtual status_t initAfBracketing(int stops,  AFBracketingMode mode);
    virtual status_t initAeBracketing();
    virtual status_t getExposureInfo(SensorAeConfig& sensorAeConfig);
    virtual bool getAfNeedAssistLight();
    virtual bool getAeFlashNecessary();
    virtual void setAeFlashStatus(bool flashOn) { UNUSED(flashOn); }
    virtual bool getAeFlashStatus() { return false; }
    virtual AwbMode getLightSource();
    virtual status_t setAeBacklightCorrection(bool en);
    virtual status_t apply3AProcess(bool read_stats,
                                    struct timeval sof_timestamp,
                                    unsigned int exp_id);
    virtual status_t startStillAf();
    virtual status_t stopStillAf();
    virtual AfStatus isStillAfComplete();
    virtual status_t applyPreFlashProcess(struct timeval timestamp, unsigned int exp_id, FlashStage stage);
    virtual status_t getGBCEResults(ia_aiq_gbce_results *gbce_results);
    virtual void get3aMakerNote(ia_mkn_trg mknMode, ia_binary_data& aaaMkNote);
    virtual ia_binary_data *get3aMakerNote(ia_mkn_trg mode);
    virtual void put3aMakerNote(ia_binary_data *mknData);
    virtual void reset3aMakerNote(void);
    virtual int add3aMakerNoteRecord(ia_mkn_dfid mkn_format_id,
                                     ia_mkn_dnid mkn_name_id,
                                     const void *record,
                                     unsigned short record_size);
    virtual status_t convertGeneric2SensorAE(int sensitivity, int expTimeUs,
                     unsigned short *coarse_integration_time,
                     unsigned short *fine_integration_time,
                     int *analog_gain_code, int *digital_gain_code);
    virtual status_t convertSensor2ExposureTime(unsigned short coarse_integration_time,
                     unsigned short fine_integration_time,
                     int analog_gain_code, int digital_gain_code,
                     int *exposure_time, int *iso);
    virtual status_t convertSensorExposure2GenericExposure(unsigned short  coarse_integration_time,
                                                           unsigned short  fine_integration_time,
                                                           int  analog_gain_code,
                                                           int  digital_gain_code,
                                                           ia_aiq_exposure_parameters
                                                           *exposure);

    status_t getParameters(ispInputParameters * ispParams,
                           ia_aiq_exposure_parameters * exposure);

    virtual status_t setSmartSceneDetection(bool en);
    virtual bool     getSmartSceneDetection();
    virtual status_t switchModeAndRate(IspMode mode, float fps);

    virtual AfStatus getCAFStatus();
    status_t getSmartSceneMode(int *sceneMode, bool *sceneHdr);
    status_t setFaces(const ia_face_state& faceState);
    status_t setFlash(int numFrames);
    int getCurrentFocusDistance();
    int getOpticalStabilizationMode();
    int getnsExposuretime();
    int getnsFrametime();
    virtual int32_t mapUIISO2RealISO (int32_t iso);
    virtual int32_t mapRealISO2UIISO (int32_t iso);

private:
    int mCameraId;
    IHWIspControl * mISP;
    IHWSensorControl * mSensorCI;
    IHWFlashControl * mFlashCI;
    IHWLensControl * mLensCI;
    AeMode mPublicAeMode;
    AfMode mPublicAfMode;
}; // class Soc3A

};
}; // namespace android

#endif // _CAMERA3_HAL_SOC_3A
