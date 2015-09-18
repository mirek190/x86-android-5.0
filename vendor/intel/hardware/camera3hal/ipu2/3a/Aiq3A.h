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

#ifndef _CAMERA3_HAL_AIQ_3A_H_
#define _CAMERA3_HAL_AIQ_3A_H_

#include <utils/Errors.h>
#include <utils/threads.h>
#include <time.h>
#include <ia_types.h>
#include <ia_aiq_types.h>
#include <ia_aiq.h>

#include "I3AControls.h"
#include "CameraConf.h"
#include "AtomFifo.h"
#include "ICameraHwControls.h"

#include "ia_face.h"

namespace android {
namespace camera2 {

#define DEFAULT_GBCE            true
#define DEFAULT_GBCE_STRENGTH   0
#define AIQ_MAX_TIME_FOR_AF     2500 // milliseconds
#define TORCH_INTENSITY         20   // 20%
#define MAX_NUM_AF_WINDOW       9
#define AE_DELAY_FRAMES_DEFAULT 2
const unsigned int NUM_EXPOSURES = 1;
const unsigned int NUM_FLASH_LEDS = 1;

typedef struct {
    ia_binary_data                    isp_output;
    bool                              exposure_changed;
    bool                              flash_intensity_changed;
} aiq_results;

typedef struct {
    ia_aiq_af_results              *af_results;
    ia_aiq_rect                     focus_rect;
    ia_aiq_manual_focus_parameters  focus_parameters;
    struct timespec                 lens_timestamp;
    unsigned long long              previous_sof;
    int32_t                         lens_position;
    bool                            aec_locked;
    bool                            assist_light;
    bool                            af_locked;
    AfMode                          afMode;
    int                             af_score_window_size;
} af_state;

typedef struct {
    ia_aiq_ae_results                 results;
    ia_aiq_hist_weight_grid           weight_grid;
    ia_aiq_exposure_parameters        exposures[NUM_EXPOSURES];
    ia_aiq_exposure_sensor_parameters sensor_exposures[NUM_EXPOSURES];
    ia_aiq_ae_exposure_result         exposure_result_array[NUM_EXPOSURES];
    ia_aiq_flash_parameters           flashes[NUM_FLASH_LEDS];
} stored_ae_results;

typedef struct {
    bool                              ae_locked;
    ia_aiq_ae_results                 *ae_results;
    stored_ae_results                 feedback_results;
    unsigned int                      feedback_delay;
    AtomFifo<stored_ae_results>       *stored_results;
} ae_state;

typedef struct {
    bool                            reconfigured;
    ia_face_state                  *faces;
    ia_aiq                         *ia_aiq_handle;
    ia_aiq_scene_mode               detected_scene;
    ia_aiq_rgbs_grid                rgbs_grid;
    ia_aiq_af_grid                  af_grid;
    ia_aiq_frame_params             sensor_frame_params;
    ia_aiq_awb_manual_cct_range     cct_range;
    bool                            dsd_enabled;
    bool                            aic_enabled;
    ia_aiq_frame_use                frame_use;
    ia_aiq_statistics_input_params  statistics_input_parameters;
    ia_aiq_af_results               af_results_feedback;
    ia_aiq_dsd_input_params         dsd_input_parameters;
    int                             boot_events;
    struct timespec                 lens_timestamp;
    aiq_results                     results;
} aaa_state;


/**
 * \class Aiq3A
 *
 * Aiq3A is a singleton interface to Intel 3A Library (formerly known as libia_aiq).
 *
 * The libia_aiq library provides the 3A functionality(AF, AEC, AWB, GBCE, DSD, AIC)
 * Due to this, in addition to Aiq3AThread that handles actual AAA processing, many other
 * subcomponents of HAL need to use Aiq3A.
 *
 * All access to the imaging library go via Aiq3A.
 *
 */
class Aiq3A : public I3AControls {

private:
    status_t getAiqConfig(ia_binary_data *cpfData);

    // Common functions for 3A, GBCE, AF etc.
    status_t run3aMain();

    //AE for flash
    int AeForFlash();
    int applyResults();
    bool changeSensorMode(void);

    //staticstics
    status_t getStatistics(const struct timeval *sof_timestamp,
                           unsigned int exposure_id);
    bool needStatistics();

    // GBCE
    int setGammaEffect(bool inv_gamma);
    int enableGbce(bool enable);
    void resetGBCEParams();
    status_t runGBCEMain();

    // 3A control
    status_t _init3A();
    int run3aInit();
    int processForFlash();
    status_t populateFrameInfo(const struct timeval *frame_timestamp,
                               const struct timeval *sof_timestamp);

    //AIC
    status_t runAICMain();

    // AF
    void resetAFParams();
    status_t runAfMain();
    void setAfFocusMode(ia_aiq_af_operation_mode mode);
    void setAfFocusRange(ia_aiq_af_range range);
    void setAfMeteringMode(ia_aiq_af_metering_mode mode);
    status_t moveFocusDriveToPos(long position);
    void afUpdateTimestamp(void);


    //AE
    void resetAECParams();
    status_t runAeMain();
    bool getAeResults();
    bool getAeFlashResults();
    bool needApplyExposure(ia_aiq_ae_results *new_ae_results);
    bool needApplyFlashIntensity(ia_aiq_ae_results *new_ae_results);
    int applyExposure(ia_aiq_exposure_sensor_parameters *);
    ia_aiq_ae_results* storeAeResults(ia_aiq_ae_results *, int updateIdx = -1);
    ia_aiq_ae_results* peekAeStoredResults(unsigned int offset);
    ia_aiq_ae_results* pickAeFeedbackResults();

    //AWB
    void resetAWBParams();
    void runAwbMain();
    bool getAwbResults();

    //DSD
    void resetDSDParams();
    status_t runDSDMain();

    //ISP parameters
    int enableEe(bool enable);
    int enableNr(bool enable);
    int enableDp(bool enable);
    int enableOb(bool enable);
    int enableShadingCorrection(bool enable);

    // Get exposure time for AE
    void getAeExpCfg(int *exp_time,
                     short unsigned int *aperture_num,
                     short unsigned int *aperture_denum,
                     int *aec_apex_Tv, int *aec_apex_Sv, int *aec_apex_Av,
                     float *digital_gain,
                     float *total_gain);

    void getSensorFrameParams(ia_aiq_frame_params *frame_params);
    status_t convertError(ia_err iaErr);

    // the aiqd methods
    String8 getAiqdFileName(void);
    bool loadAiqdData(ia_binary_data* aiqdData, String8 aiqdFileName);
    bool saveAiqdData(String8 aiqdFileName);

// prevent copy constructor and assignment operator
private:
    Aiq3A(const Aiq3A& other);
    Aiq3A& operator=(const Aiq3A& other);

public:
    Aiq3A(HWControlGroup &hwcg);
    Aiq3A(int cameraId);
    ~Aiq3A();

    virtual bool isIntel3A() { return true; }

    // Initialization functions
    virtual status_t init3A();
    virtual status_t deinit3A();
    virtual status_t switchModeAndRate(IspMode mode, float fps);

    virtual status_t attachHwControl(HWControlGroup &hwcg);

    // Getters and Setters
    status_t setAeWindow(const CameraWindow *window);
    status_t setAfWindow(const CameraWindow *window);
    status_t setAeFlickerMode(FlickerMode mode);
    FlickerMode getAeFlickerMode();
    status_t setAfEnabled(bool en) { UNUSED(en); return 0; }
    status_t setAeSceneMode(SceneMode mode);
    SceneMode getAeSceneMode();
    status_t setAeMode(AeMode mode);
    AeMode getAeMode();
    virtual status_t setAfMode(AfMode mode);
    virtual AfMode getAfMode();
    bool getAfNeedAssistLight();
    status_t setAeFlashMode(FlashMode mode);
    FlashMode getAeFlashMode();
    bool getAeFlashNecessary();
    void setAeFlashStatus(bool flashOn) { mFlashStatus = flashOn; }
    bool getAeFlashStatus(void) { return mFlashStatus; }
    status_t setAwbMode(AwbMode mode);
    AwbMode getAwbMode();
    status_t setAwbZeroHistoryMode(bool en);
    AwbMode getLightSource();
    status_t setAeMeteringMode(MeteringMode mode);
    MeteringMode getAeMeteringMode();
    status_t set3AColorEffect(EffectMode effect);
    virtual void setPublicAeMode(AeMode mode);
    virtual AeMode getPublicAeMode();
    virtual void setPublicAfMode(AfMode mode);
    virtual AfMode getPublicAfMode();
    int getCurrentFocusDistance();
    int getOpticalStabilizationMode();
    int getnsExposuretime();
    int getnsFrametime();
    virtual status_t setIsoMode(IsoMode mode);
    virtual IsoMode getIsoMode(void) { return CAM_AE_ISO_MODE_NOT_SET; }
    status_t getGBCEResults(ia_aiq_gbce_results *gbce_results);

    status_t setNRMode(bool enable);
    status_t setEdgeMode(bool enable);
    status_t setShadingMode(bool enable);
    status_t setEdgeStrength(int strength);
    status_t setAeLock(bool en);
    bool     getAeLock();
    status_t setAfLock(bool en);
    bool     getAfLock();
    AfStatus getCAFStatus();
    status_t setAwbLock(bool en);
    bool     getAwbLock();
    // returning an error in the following functions will cause some functions
    // not to be run in ControlThread
    size_t   getAeMaxNumWindows() { return 1; }
    size_t   getAfMaxNumWindows() { return MAX_NUM_AF_WINDOW; }
    status_t setAfWindows(const CameraWindow *windows, size_t numWindows);
    status_t getExposureInfo(SensorAeConfig& sensorAeConfig);
    status_t getAeManualBrightness(float *ret);
    status_t setManualFocus(int focus, bool applyNow);
    status_t setManualFocusIncrement(int step);
    status_t applyEv(float bias);
    status_t setEv(float bias);
    status_t getEv(float *ret);
    status_t setFrameRate(int fps);
    bool isAeConverged(void) { return mAeConverged; };
    bool isAwbConverged(void) { return mAwbConverged; };

    status_t setManualIso(int ret);
    status_t getManualIso(int *ret);
    status_t setManualShutter(float expTime);
    status_t getManualShutter(float *expTime);
    status_t setManualFrameDuration(uint64_t nsFrameDuration);
    status_t setManualFrameDurationRange(int minFps, int maxFps);
    status_t setManualFocusDistance(int distance);
    status_t setSmartSceneDetection(bool en);
    bool     getSmartSceneDetection();
    status_t getSmartSceneMode(int *sceneMode, bool *sceneHdr);
    status_t setFaces(const ia_face_state& faceState);

    //Bracketing
    status_t initAfBracketing(int stop, AFBracketingMode mode);
    virtual status_t initAeBracketing();

    // Flash control
    virtual status_t setFlash(int numFrames);

    // ISP processing functions
    status_t apply3AProcess(bool read_stats,
        struct timeval sof_timestamp,
        unsigned int exp_id);

    status_t startStillAf();
    status_t stopStillAf();
    AfStatus isStillAfComplete();
    status_t applyPreFlashProcess(struct timeval timestamp, unsigned int exp_id, FlashStage stage);

    // Makernote
    void get3aMakerNote(ia_mkn_trg mknMode, ia_binary_data& makerNote);
    ia_binary_data *get3aMakerNote(ia_mkn_trg mode);
    void put3aMakerNote(ia_binary_data *mknData);
    void reset3aMakerNote(void);
    int add3aMakerNoteRecord(ia_mkn_dfid mkn_format_id,
                             ia_mkn_dnid mkn_name_id,
                             const void *record,
                             unsigned short record_size);
    status_t convertGeneric2SensorAE(int sensitivity, int expTimeUs,
                                 unsigned short *coarse_integration_time,
                                 unsigned short *fine_integration_time,
                                 int *analog_gain_code, int *digital_gain_code);
    status_t convertSensor2ExposureTime(unsigned short coarse_integration_time,
                                unsigned short fine_integration_time,
                                int analog_gain_code, int digital_gain_code,
                                int *exposure_time, int *iso);
    status_t convertSensorExposure2GenericExposure(unsigned short coarse_integration_time,
                                                   unsigned short fine_integration_time,
                                                   int analog_gain_code,
                                                   int digital_gain_code,
                                                   ia_aiq_exposure_parameters *exposure);

    status_t getParameters(ispInputParameters * ispParams,
                                ia_aiq_exposure_parameters * exposure);
    int32_t mapUIISO2RealISO (int32_t iso);
    int32_t mapRealISO2UIISO (int32_t iso);

// private members
private:
    IHWIspControl *mISP;
    ia_env mPrintFunctions;

    aaa_state m3aState;
    const ia_aiq_rgbs_grid* mRgbsGridArray[NUM_EXPOSURES];
    const ia_aiq_af_grid* mAfGridArray[NUM_EXPOSURES];

    //STATISTICS
    ia_aiq_statistics_input_params mStatisticsInputParameters;

    //AF
    AfMode mAfMode;
    nsecs_t mStillAfStart;
    nsecs_t mContinuousAfStart;
    ia_aiq_af_input_params mAfInputParameters;
    af_state mAfState;
    int mFocusPosition;

    //AF bracketing
    ia_aiq_af_bracket_results* mAfBracketingResult;
    int mBracketingStops;

    //AE
    ia_aiq_ae_input_params mAeInputParameters;
    ia_aiq_ae_manual_limits mAeManualLimits;
    ia_aiq_exposure_sensor_descriptor mAeSensorDescriptor;
    AeMode mAeMode;
    AeMode mPublicAeMode;
    SceneMode mAeSceneMode;
    FlickerMode mAeFlickerMode;
    FlashMode mAeFlashMode;
    ae_state mAeState;
    ia_coordinate mAeCoord;
    bool mAeConverged;
    int mStatCount;
    bool mFlashStatus;

    //AE bracketing
    ia_aiq_ae_input_params mAeBracketingInputParameters;
    bool mBracketingRunning;
    ia_aiq_ae_results *mAEBracketingResult;

    //AW
    ia_aiq_awb_input_params mAwbInputParameters;
    ia_aiq_awb_results *mAwbResults;
    ia_aiq_awb_results mAwbStoredResults;
    AwbMode mAwbMode;
    bool mIsAwbZeroHistoryMode;
    bool mAwbLocked;
    bool mAwbDisabled;
    int mAwbRunCount;
    bool mAwbConverged;

    //GBCE
    ia_aiq_gbce_results *mGBCEResults;
    bool mGBCEDefault;

    //PA
    ia_aiq_pa_results *mPaResults;

    //ISP
    ispInputParameters mIspInputParams;

    //DSD
    ia_aiq_dsd_input_params mDSDInputParameters;
    ia_aiq_scene_mode mDetectedSceneMode;

    //MKN
    ia_mkn  *mMkn;

    IHWSensorControl*    mSensorCI;
    IHWFlashControl*    mFlashCI;
    IHWLensControl*    mLensCI;

    // fps setting
    enum FpsSettingType {
         FPS_SETTING_OFF = 1,
         FPS_SETTING_HIGH_SPEED = 1 << 1,
         FPS_SETTING_FIXED = 1 << 2,
         FPS_SETTING_RANGE = 1 << 3
    };
    FpsSettingType mFpsSetting;

    // last frame setting
    int mLastExposuretime;
    int mLastFrameTime;

    // NR, EE
    bool mNoiseReduction;
    bool mEdgeEnhancement;
    // lens shading
    bool mLensShading;
    int mEdgeStrength;
    // ISO
    int mPseudoIso; //pseudo ISO
    int mBaseIso;   //baseIso
    int mCameraId;

    // cmc info
    cmc_parsed_analog_gain_conversion_t mCmcParsedAG;
    cmc_parsed_digital_gain_t mCmcParsedDG;
    cmc_sensitivity_t mCmcSensitivity;

    FlashStage mFlashStage;
    ia_aiq_exposure_parameters mLastExposure;
    ia_aiq_exposure_sensor_parameters mLastSensorExposure;
}; // class Aiq3A


};
}; // namespace android

#endif // _CAMERA3_HAL_AIQ_3A_H_
