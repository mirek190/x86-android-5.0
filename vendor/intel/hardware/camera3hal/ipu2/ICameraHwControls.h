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

#ifndef _CAMERA3_HAL_ICAMERA_HW_CONTROLS_H__
#define _CAMERA3_HAL_ICAMERA_HW_CONTROLS_H__

#include <utils/String8.h>
#include <ia_mkn_encoder.h>
#include <ia_mkn_decoder.h>
#include <ia_aiq.h>
#include <linux/videodev2.h>
#include <linux/atomisp.h>

#include "CameraStream.h"
#include "ia_isp_types.h"
#include "common/3ATypes.h"


namespace android {
namespace camera2 {

class IHwStreamListener;

#define CAM_WXH_STR(w,h) STRINGIFY_(w##x##h)
#define CAM_RESO_STR(w,h) CAM_WXH_STR(w,h) // example: CAM_RESO_STR(VGA_WIDTH,VGA_HEIGHT) -> "640x480"

#define MIN_DVS_WIDTH   384
#define MIN_DVS_HEIGHT  384

#define FPS_RANGES_LEN 2

/*!
 * \brief Common structure for sensor embedded metadata from sensor
 *
 * The structure is a combination of all the parameters needed by sensor
 * metadata decoder.
 */
typedef struct {
    void* data;
    int exp_id;
    int stride;
    int height;
    int *effective_width;
} sensor_embedded_metadata;

struct sensorPrivateData
{
    void *data;
    unsigned int size;
    bool fetched; // true if data has been attempted to read, false otherwise
};

struct isp_metadata
{
    float focal_length;
    int32_t fps_range[2 * FPS_RANGES_LEN];
};

enum ObserverType {
    //OBSERVE_PREVIEW_STREAM,
    OBSERVE_FRAME_SYNC_SOF,
    OBSERVE_3A_STAT_READY,
};

struct ContinuousCaptureConfig {
    int numCaptures;        /*!< Number of captures
                             * -1 = capture continuously
                             * 0 = disabled, stop captures
                             * >0 = burst of N snapshots
                             */
    int offset;             /*!< burst start offset */
    int skip;               /*!< skip factor */
};

struct LensShadingInfo{
    uint16_t grid_width;        /*!< LSC Grid width. */
    uint16_t grid_height;       /*!< LSC Grid height. */
    uint16_t *lsc_grids;
};
/*!
 * \brief Common structure for IA ISP Configuration input parameters
 *
 * The structure is a combination of all the input parameters needed for
 * different IA ISP Configuration versions.
 *
 */
typedef struct {
    ia_aiq_frame_use frame_use;                      /*!< Target frame type of the AIC calculations (Preview, Still, video etc.). */
    ia_aiq_frame_params        *sensor_frame_params; /*!< Sensor frame parameters (crop offsets, scaling denominators etc). */
    ia_aiq_exposure_parameters *exposure_results;    /*!< Exposure parameters which are to be used to calculate next ISP parameters. */
    ia_aiq_awb_results *awb_results;                 /*!< WB results which are to be used to calculate next ISP parameters (WB gains, color matrix,etc). */
    ia_aiq_gbce_results gbce_results;               /*!< GBCE Gamma tables which are to be used to calculate next ISP parameters.
                                                          If NULL pointer is passed, AIC will use static gamma table from the CPF.  */
    ia_aiq_pa_results *pa_results;                   /*!< Parameter adaptor results from AIQ. */
    bool gbce_results_valid;
    char manual_brightness;                          /*!< Manual brightness value range [-128,127]. */
    char manual_contrast;                            /*!< Manual contrast value range [-128,127]. */
    char manual_hue;                                 /*!< Manual hue value range [-128,127]. */
    char manual_saturation;                          /*!< Manual saturation value range [-128,127]. */
    ia_isp_effect effects;                           /*!< Manual setting for special effects.*/
    bool noiseReduction;                             /*!< Noise reduction enabled or disabled.*/
    bool edgeEnhancement;                            /*!< Edge enhancement enabled or disabled.*/
    bool lensShading;                                /*!< Lens shading enabled or disabled.*/
    int edgeStrength;                                /*!< Edge enhancement strength.*/

    int isp_config_id;                               /*!< Identify for per-frame settings.*/
    int availableStreams;
} ispInputParameters;

// store the 3A info
typedef struct {
    // request info
    int reqId;
    bool needMakeNote;
    ia_binary_data *aaaMkNote;
    float brightness;
    int isoSpeed;
    AeMode aeMode;
    MeteringMode meteringMode;
    AwbMode lightSource;
    SceneMode sceneMode;
    char contrast;
    char saturation;
    SensorAeConfig aeConfig;
} aaaMetadataInfo;

/* Abstraction of HW algorithms control interface for 3A support*/
// Temporarily current AtomISP APIs are in IHWIspControl, as the top level IF for the reset of the HAL.
// Our target is APIs will be seperated to several interface classes in the near future
class IHWIspControl
{
public:
    /* **********************************************************
     * Constructor/Destructor
     */
    virtual ~IHWIspControl() {};

// public methods
public:
    /* **********************************************************
     * General
     */


    /* **********************************************************
     * ISP features
     */
    virtual status_t setColorEffect(v4l2_colorfx effect) = 0;
    virtual status_t applyColorEffect() = 0;


    /*!
     * \brief Converts the generic output results from 3A and other SW algorithms
     * into HW specific configuration for the HW ISP and sets them to ISP.
     *
     *
     * \param[in]  ispInputParams   Outcome of the 3A and other algorithms.
     *                              This structure is the generic version produced by the SW algorithms.
     *
     * \return                      Error code.
     */
    virtual int setIspParameters(const ispInputParameters *ispInputParams) = 0;

    /*!
     * \brief Gets statistics and converts them to IA_AIQ generic format.
     *
     * \param[out] outRgbsGrid   Pointer's pointer where address of converted statistics are stored.
     *                           Converted RGBS grid statistics. Output can be directly used as input in function ia_aiq_statistics_set.
     * \param[out] outAfGrid     Pointer's pointer where address of converted statistics are stored.
     *                           Converted AF grid statistics. Output can be directly used as input in function ia_aiq_statistics_set.
     * \return                   Error code.
    */
    virtual int getIspStatistics(ia_aiq_rgbs_grid **out_rgbs_grid,
                                 ia_aiq_af_grid **out_af_grid, unsigned int *exp_id) = 0;
    virtual unsigned int getIspStatisticsSkip() = 0;
    virtual int getSensorEmbeddedMetaData(sensor_embedded_metadata *sensorMetaData,
              uint8_t metadata_type) = 0;
    virtual status_t getDecodedExposureParams(ia_aiq_exposure_sensor_parameters* sensor_exp_p,
              ia_aiq_exposure_parameters* generic_exp_p, unsigned int exp_id = 0) = 0;
    virtual bool checkSensorMetadataAvailable(unsigned int exp_id) = 0;
    virtual bool isSensorEmbeddedMetaDataSupported() = 0;
    virtual bool isIspPerframeSettingsEnabled() = 0;

    virtual bool isRAW() const = 0;

    virtual status_t getIspParameters(struct atomisp_parm *isp_param) const = 0;
    virtual void setNrEE(bool en) = 0;

    /* DVS */
    virtual void setDvsConfigChanged(bool changed) = 0;
    virtual status_t getDvsStatistics(struct atomisp_dis_statistics *stats,
                              bool *tryAgain) const = 0;
    virtual status_t setDvsCoefficients(const struct atomisp_dis_coefficients *coefs) const = 0;
    virtual status_t getIspDvs2BqResolutions(struct atomisp_dvs2_bq_resolutions *bq_res) const = 0;
    virtual bool dvsEnabled() = 0;
    virtual status_t updateSettingPipelineDepth(CameraMetadata &settings) = 0;

    /* **********************************************************
     * IA_ISP specific
     */

    /*!
     * \brief Initializes IA_ISP adaptor library and its submodules.
     *
     * \param[in]     cpfData          AIQ block from CPF file. Contains ISP specific parameters.
     * \param[in]     maxStatsWidth    Maximum width of RGBS and AF statistics grids from ISP.
     * \param[in]     maxStatsHeight   Maximum height of RGBS and AF statistics grids from ISP.
     * \param[in]     cmc              Parsed camera module characterization structure.
     * \param[in,out] mkn              Makernote handle which can be initialized with ia_mkn library. If debug data from AIQ
     *                                 is needed to be stored into EXIF, this parameter is needed.
     */
    virtual void initIaIspAdaptor(const ia_binary_data *cpfData,
                                  unsigned int maxStatsWidth,
                                  unsigned int maxStatsHeight,
                                  ia_cmc_t *cmc,
                                  ia_mkn *mkn) = 0;

    virtual void sensorModeChanged() = 0;

    /* ISP generic */
    virtual void setVideoInfo(FrameInfo *info) = 0;
    virtual void setPreviewInfo(FrameInfo *info) = 0;
    virtual void getVideoSize(int *w, int *h) const = 0;

    virtual status_t getMakerNote(atomisp_makernote_info *info) = 0;
    virtual status_t fillDefaultMetadataFromISP(isp_metadata *) = 0;
    virtual status_t getLensShading(LensShadingInfo *lensShadingInfo) = 0;
    virtual void storeMetadata(aaaMetadataInfo *aaaMetadata) = 0;
    virtual status_t getMetadata(int requestId, aaaMetadataInfo *aaaMetadata) = 0;
    virtual int getExposureLag(void) = 0;
};

/* Abstraction of HW sensor control interface for 3A support */
class IHWSensorControl
{
public:
    virtual ~IHWSensorControl() { };

    virtual const char * getSensorName(void) = 0;
    virtual int getCurrentCameraId(void) = 0;

    virtual unsigned int getExposureDelay() = 0;

    virtual unsigned int getFrameSkip() = 0;

    virtual bool supportPerFrameSettings() = 0;

    virtual float getStepEV() = 0;

    virtual int setExposure(unsigned short coarse_integration_time,
                            unsigned short fine_integration_time,
                            unsigned short analog_gain_code_global,
                            uint16_t digital_gain_global) = 0;
    virtual int storeRequestExposure(int request_id,
                                      bool manualExposure,
                                      struct atomisp_exposure *exposure) = 0;
    virtual int storeAutoExposure(bool clearOldValues,
                                 unsigned short coarse_integration_time,
                                 unsigned short fine_integration_time,
                                 unsigned short analog_gain_global,
                                 uint16_t digital_gain_global) = 0;
    virtual int applyExposure(unsigned int exp_id) = 0;
    virtual void getSensorData(sensorPrivateData *sensor_data) = 0;
    virtual void vbiIntervalForItem(unsigned int index,
                                    unsigned int & vbiOffset,
                                    unsigned int & shutterOffset) = 0;

    /*!
     * \brief Gets frame parameters and exposure related restrictions.
     *
     * \param[out]     frame_params         Frame parameters that describe cropping and scaling
     * \param[out]     sensor_descriptor    Exposure related restrictions and constants
     * \param[out]     output_width         frame width after binning/scaling
     * \param[out]     output_height        frame height after binning/scaling
     */
    virtual int  getFrameParameters(ia_aiq_frame_params *frame_params,
                                    ia_aiq_exposure_sensor_descriptor *sensor_descriptor,
                                    unsigned int output_width, unsigned int output_height) = 0;
    virtual int  getExposureTime(int *exposure_time) = 0;
    virtual int  getAperture(int *aperture) = 0;
    virtual int  getFNumber(unsigned short  *fnum_num, unsigned short *fnum_denom) = 0;
    // Sensor setting, ISP bypass?
    virtual int setExposureTime(int time) = 0;
    virtual int setExposureMode(v4l2_exposure_auto_type type) = 0;
    virtual int getExposureMode(v4l2_exposure_auto_type * type) = 0;
    virtual int setExposureBias(int bias) = 0;
    virtual int getExposureBias(int * bias) = 0;
    virtual int setSceneMode(v4l2_scene_mode mode) = 0;
    virtual int getSceneMode(v4l2_scene_mode * mode) = 0;
    virtual int setWhiteBalance(v4l2_auto_n_preset_white_balance mode) = 0;
    virtual int getWhiteBalance(v4l2_auto_n_preset_white_balance * mode) = 0;
    virtual int setIso(int iso) = 0;
    virtual int getIso(int * iso) = 0;
    virtual int setAeMeteringMode(v4l2_exposure_metering mode) = 0;
    virtual int getAeMeteringMode(v4l2_exposure_metering * mode) = 0;
    virtual int setAeFlickerMode(v4l2_power_line_frequency mode) = 0;
    virtual int setAfMode(v4l2_auto_focus_range mode) = 0;
    virtual int getAfMode(v4l2_auto_focus_range * mode) = 0;
    virtual int setAfEnabled(bool enable) = 0;
    virtual int set3ALock(int aaaLock) = 0;
    virtual int get3ALock(int * aaaLock) = 0;
    virtual int setAeFlashMode(v4l2_flash_led_mode mode) = 0;
    virtual int getAeFlashMode(v4l2_flash_led_mode * mode) = 0;

    virtual int getModeInfo(struct atomisp_sensor_mode_data *mode_data) = 0;
    virtual float getFrameDuration(void) = 0;
    virtual float getFrameRate() = 0;
    virtual int setFrameRate(int fps) = 0;
    virtual int setFramelines(int fl) = 0;
    virtual int getExposurelinetime() = 0;
    virtual void getMotorData(sensorPrivateData *sensor_data) = 0;
    virtual void generateNVM(ia_binary_data **output_nvm_data) = 0;
    virtual void releaseNVM(ia_binary_data *nvm_data) = 0;

    virtual bool verifyCapturedExpId(int reqId, int expId) = 0;
    virtual int getExpectedReqId(int expId) = 0;
    virtual int getExpIdForRequest(int reqId) = 0;
    virtual int64_t getShutterTime(int reqId) = 0;
    virtual int64_t getShutterTimeByExpId(int exp) = 0;
    virtual int64_t getSOFTime(int reqId) = 0;
};

/* Abstraction of HW flash control interface for 3A support */
class IHWFlashControl
{
public:
    virtual ~IHWFlashControl() { };

    virtual status_t setFlash(int numFrames) = 0;
    virtual status_t setFlashIndicator(int intensity) = 0;
    virtual status_t setTorch(int intensity) = 0;
    /* Flash related controls */
    virtual int setFlashIntensity(int intensity) = 0;
};

/* Abstraction of HW lens control interface for 3A support */
class IHWLensControl
{
public:
    virtual ~IHWLensControl() { };

    virtual int moveFocusToPosition(int position) = 0;
    virtual int moveFocusToBySteps(int steps) = 0;
    virtual int getFocusPosition(int * position) = 0;
    virtual int  getFocusStatus(int *status) = 0;
};

/* Compound object for HW control interfaces for 3A */
class HWControlGroup
{
public:
    HWControlGroup():
        mIspCI(NULL),
        mSensorCI(NULL),
        mFlashCI(NULL),
        mLensCI(NULL) { };
    ~HWControlGroup() { };

    IHWIspControl       *mIspCI;
    IHWSensorControl    *mSensorCI;
    IHWFlashControl     *mFlashCI;
    IHWLensControl      *mLensCI;
};


} // namespace camera2
} // namespace android
#endif //_CAMERA3_HAL_ICAMERA_HW_CONTROLS_H__
