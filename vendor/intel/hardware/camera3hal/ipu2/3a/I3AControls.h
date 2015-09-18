/*
 * Copyright (C) 2012,2013 Intel Corporation
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

#ifndef _I3ACONTROLS_H_
#define _I3ACONTROLS_H_

#include <ia_mkn_types.h>
#include <ia_aiq_types.h>
#include "ia_face.h"
#include "CameraAreas.h"
#include "ICameraHwControls.h"
#include "3ATypes.h"
#include "IPU2CommonTypes.h"

namespace android {
namespace camera2 {

/**
* \class IFaceDetectCallBack
* IFaceDetectCallBack defines interfaces for saving face state
* to 3A and to applocation as a part of metadata
*/
class IFaceDetectCallback {
 public:
IFaceDetectCallback () {}
virtual ~IFaceDetectCallback () {}
// send face state back to 3A
virtual void FaceDetectCallbackFor3A(ia_face_state *facestate) = 0;
// send face metadata info to application
virtual void FaceDetectCallbackForApp(camera_frame_metadata_t *face_metadata) = 0;
};

/**
 * \class IFlashCallback
 * IFlashCallback defines interfaces for letting CaptureStream
 * access preflash state and control preflash
 */
class IFlashCallback {
public:
    IFlashCallback () {}
    virtual ~IFlashCallback () {}
    // get preflash info
    virtual bool isPreFlashUsed() = 0;
    // exit preflash sequence
    virtual void exitPreFlashSequence() = 0;
    // if flash necessary
    virtual bool isFlashNecessary() = 0;
};

/**
 * \class I3AControls
 *
 * I3AControls defines an interface for 3A controls.
 * For RAW cameras the 3A controls are handled in Intel 3A library,
 * and for SoC cameras they are set via V4L2 commands and handled
 * in the driver.
 *
 * This interface is implemented by Aiq3A (for RAW) and
 * Soc3A (for SoC) classes.
 */
class I3AControls
{
public:
    virtual ~I3AControls() {}
    virtual status_t init3A() = 0;
    virtual status_t deinit3A() = 0;
    virtual status_t attachHwControl(HWControlGroup &hwcg) = 0;

    virtual status_t setAeMode(AeMode mode) = 0;
    virtual AeMode getAeMode() = 0;
    virtual status_t setEv(float bias) = 0;
    virtual status_t getEv(float *ret) = 0;
    virtual status_t setFrameRate(int fps) = 0;
    virtual status_t setAeSceneMode(SceneMode mode) = 0;
    virtual SceneMode getAeSceneMode() = 0;
    virtual status_t setAwbMode(AwbMode mode) = 0;
    virtual AwbMode getAwbMode() = 0;
    virtual status_t setAwbZeroHistoryMode(bool en) = 0;
    virtual status_t setManualIso(int iso) = 0;
    virtual status_t getManualIso(int *ret) = 0;
    /** expose iso mode setting*/
    virtual status_t setIsoMode(IsoMode mode) = 0;
    virtual IsoMode getIsoMode(void) = 0;
    virtual status_t setAeMeteringMode(MeteringMode mode) = 0;
    virtual MeteringMode getAeMeteringMode() = 0;
    virtual status_t set3AColorEffect(EffectMode effect) = 0;
    virtual status_t setAfMode(AfMode mode) = 0;
    virtual AfMode getAfMode() = 0;
    virtual status_t setAfEnabled(bool en) = 0;
    virtual status_t setAeWindow(const CameraWindow *window) = 0;
    virtual status_t setAfWindows(const CameraWindow *windows, size_t numWindows) = 0;
    virtual status_t setAeFlickerMode(FlickerMode mode) = 0;
    virtual FlickerMode getAeFlickerMode() = 0;

    // Intel 3A specific
    virtual bool isIntel3A() = 0;
    virtual status_t getAeManualBrightness(float *ret) = 0;
    virtual status_t setManualFocusIncrement(int step) = 0;
    virtual status_t initAfBracketing(int stops, AFBracketingMode mode) = 0;
    virtual status_t initAeBracketing() = 0;
    virtual status_t applyEv(float bias) = 0;
    virtual status_t getExposureInfo(SensorAeConfig& sensorAeConfig) = 0;
    virtual size_t   getAeMaxNumWindows() = 0;
    virtual size_t   getAfMaxNumWindows() = 0;
    virtual status_t setNRMode(bool enable) = 0;
    virtual status_t setEdgeMode(bool enable) = 0;
    virtual status_t setShadingMode(bool enable) = 0;
    virtual status_t setEdgeStrength(int strength) = 0;
    virtual bool     getAeLock() = 0;
    virtual status_t setAeLock(bool en) = 0;
    virtual bool     getAfLock() = 0;
    virtual status_t setAfLock(bool en) = 0;
    virtual status_t setAwbLock(bool en) = 0;
    virtual bool     getAwbLock() = 0;
    virtual status_t setAeFlashMode(FlashMode mode) = 0;
    virtual FlashMode getAeFlashMode() = 0;
    virtual bool getAfNeedAssistLight() = 0;
    virtual bool getAeFlashNecessary() = 0;
    virtual void setAeFlashStatus(bool flashOn) = 0;
    virtual bool getAeFlashStatus(void) = 0;
    virtual AwbMode getLightSource() = 0;
    virtual status_t setManualShutter(float expTime) = 0;
    virtual status_t setManualFrameDuration(uint64_t nsFrameDuration) = 0;
    virtual status_t setManualFrameDurationRange(int minFps, int maxFps) = 0;
    virtual status_t setManualFocusDistance(int distance) = 0;
    virtual status_t setSmartSceneDetection(bool en) = 0;
    virtual bool     getSmartSceneDetection() = 0;
    virtual status_t getSmartSceneMode(int *sceneMode, bool *sceneHdr) = 0;
    virtual void setPublicAeMode(AeMode mode) = 0;
    virtual AeMode getPublicAeMode() = 0;
    virtual AfStatus getCAFStatus() = 0;
    virtual status_t setFaces(const ia_face_state& faceState) = 0;
    virtual status_t setFlash(int numFrames) = 0;
    virtual bool isAeConverged(void) = 0;
    virtual bool isAwbConverged(void) = 0;
    virtual status_t getGBCEResults(ia_aiq_gbce_results *gbce_results) = 0;

    virtual status_t switchModeAndRate(IspMode mode, float fps) = 0;
    virtual status_t apply3AProcess(bool read_stats, const struct timeval sof_timestamp,
		    unsigned int exp_id) = 0;
    virtual status_t startStillAf() = 0;
    virtual status_t stopStillAf() = 0;
    virtual AfStatus isStillAfComplete() = 0;
    virtual status_t applyPreFlashProcess(struct timeval timestamp, unsigned int exp_id, FlashStage stage) = 0;

    // Makernote
    virtual void get3aMakerNote(ia_mkn_trg mknMode, ia_binary_data& aaaMkNote) = 0;
    virtual ia_binary_data *get3aMakerNote(ia_mkn_trg mode) = 0;
    virtual void put3aMakerNote(ia_binary_data *mknData) = 0;
    virtual void reset3aMakerNote(void) = 0;
    virtual int add3aMakerNoteRecord(ia_mkn_dfid mkn_format_id,
                                     ia_mkn_dnid mkn_name_id,
                                     const void *record,
                                     unsigned short record_size) = 0;
    virtual status_t convertGeneric2SensorAE(int sensitivity, int expTimeUs,
                         unsigned short *coarse_integration_time,
                         unsigned short *fine_integration_time,
                         int *analog_gain_code, int *digital_gain_code) = 0;
    virtual status_t convertSensor2ExposureTime(unsigned short coarse_integration_time,
                                        unsigned short fine_integration_time,
                                        int analog_gain_code, int digital_gain_code,
                                        int *exposure_time, int *iso) = 0;
    virtual status_t convertSensorExposure2GenericExposure(unsigned short    coarse_integration_time,
                                                           unsigned short   fine_integration_time,
                                                           int analog_gain_code,
                                                           int  digital_gain_code,
                                                           ia_aiq_exposure_parameters  *exposure) = 0;

    virtual status_t getParameters(ispInputParameters * ispParams,
                                    ia_aiq_exposure_parameters * exposure) = 0;
    virtual int32_t mapUIISO2RealISO (int32_t iso) = 0;
    virtual int32_t mapRealISO2UIISO (int32_t iso) = 0;
    virtual int getCurrentFocusDistance() = 0;
    virtual int getOpticalStabilizationMode() = 0;
    virtual int getnsExposuretime() = 0;
    virtual int getnsFrametime() = 0;
};

}
}

#endif // _I3ACONTROLS_H_
