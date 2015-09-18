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

#define LOG_TAG "Camera_Conf"

#include "CameraConf.h"
#include "LogHelper.h"
#include "PlatformData.h"
#include <dirent.h>        // DIR, dirent
#include <fnmatch.h>       // fnmatch()
#include <fcntl.h>         // open(), close()
#include <math.h>          // pow, log10
#include <linux/media.h>   // media controller
#include <linux/kdev_t.h>  // MAJOR(), MINOR()
#include <utils/Errors.h>  // Error codes
#include <sys/stat.h>
#include "ia_exc.h"
#include "CameraMetadataHelper.h"


namespace android {
namespace camera2 {

static const int TRANSFORM_MATRIX_SIZE = 9;
static const float EPSILON = 0.00001;
static const int FORWARD_MATRIX_PRECISION = 65536;

using namespace CPF;

const char *cpfConfigPath = "/etc/atomisp/";  // Where CPF files are located
// FIXME: The spec for following is "dr%02d[0-9][0-9]??????????????.aiqb"
const char *cpfConfigPattern = "%02d*.aiqb";  // How CPF file name should look

CameraBlob::Blob::Blob(const int size, void *& ptr)
{
    ptr = 0;
    if (size == 0) {
        mPtr = NULL;
    } else {
        ptr = mPtr = malloc(size);
    }
}

CameraBlob::Blob::~Blob()
{
    free(mPtr);
    mPtr = NULL;
}

CameraBlob::CameraBlob(const int size)
{
    mSize = 0;
    mPtr = NULL;
    if (size) {
        mBlob = new Blob(size, mPtr);
        if (mBlob != 0) {
            if (mPtr) {
                mSize = size;
            } else {
                mBlob.clear();
            }
        }
    }
}

CameraBlob::CameraBlob(const CameraBlob& refBlob, const int offset, const int size)
{
    mSize = 0;
    mPtr = NULL;
    if (refBlob.mBlob == 0) {
        LOGE("ERROR referring to null object!");
        return;
    }
    // Must refer only to memory allocated by reference object
    if (refBlob.size() < offset + size) {
        LOGE("ERROR illegal allocation!");
        return;
    }
    mBlob = refBlob.mBlob;
    mSize = size;
    mPtr = (char *)(refBlob.ptr()) + offset;
}

CameraBlob::CameraBlob(const CameraBlob& refBlob, void * const ptr, const int size)
{
    mSize = 0;
    mPtr = NULL;
    if (refBlob.mBlob == 0) {
        LOGE("ERROR referring to null object!");
        return;
    }
    // Must refer only to memory allocated by reference object
    int offset = (char *)(ptr) - (char *)(refBlob.ptr());
    if ((offset < 0) || (offset + size > refBlob.size())) {
        LOGE("ERROR illegal allocation!");
        return;
    }
    mBlob = refBlob.mBlob;
    mSize = size;
    mPtr = ptr;
}

CameraBlob CameraBlob::copy()
{
    CameraBlob newBlob(size());
    if (newBlob && ptr() && size()) {
        memcpy(newBlob, ptr(), size());
    }
    return newBlob;
}

void CameraBlob::clear()
{
     mBlob.clear();
     mSize = 0;
     mPtr = NULL;
}

AiqConf::~AiqConf()
{
    mLightSourceMap.clear();
    clear();
}

status_t AiqConf::initCMC()
{
    if(mCMC == NULL && ptr() != NULL) {
        ia_binary_data cpfData;
        CLEAR(cpfData);
        cpfData.data = ptr();
        cpfData.size = size();
        mCMC = ia_cmc_parser_init((ia_binary_data*)&(cpfData));
        if (mCMC != NULL)
            return NO_ERROR;
    }
    LOGE("Error Initializing CMC");
    return NO_INIT;
}

status_t AiqConf::initMapping()
{
    // init light source mapping
    mLightSourceMap.clear();
    mLightSourceMap.add(CMC_Light_Source_D65, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT);
    mLightSourceMap.add(CMC_Light_Source_F11, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FLUORESCENT);
    mLightSourceMap.add(CMC_Light_Source_A, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_TUNGSTEN);
    mLightSourceMap.add(CMC_Light_Source_D65, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FLASH);
    mLightSourceMap.add(CMC_Light_Source_D65, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FINE_WEATHER);
    mLightSourceMap.add(CMC_Light_Source_D75, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_CLOUDY_WEATHER);
    mLightSourceMap.add(CMC_Light_Source_D75, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_SHADE);
    mLightSourceMap.add(CMC_Light_Source_F1, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT_FLUORESCENT);
    mLightSourceMap.add(CMC_Light_Source_F3, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAY_WHITE_FLUORESCENT);
    mLightSourceMap.add(CMC_Light_Source_F2, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_COOL_WHITE_FLUORESCENT);
    mLightSourceMap.add(CMC_Light_Source_A, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_WHITE_FLUORESCENT);
    mLightSourceMap.add(CMC_Light_Source_A, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A);
    mLightSourceMap.add(CMC_Light_Source_B, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_B);
    mLightSourceMap.add(CMC_Light_Source_C, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_C);
    mLightSourceMap.add(CMC_Light_Source_D55, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D55);
    mLightSourceMap.add(CMC_Light_Source_D65, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65);
    mLightSourceMap.add(CMC_Light_Source_D75, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D75);
    mLightSourceMap.add(CMC_Light_Source_D50, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D50);
    mLightSourceMap.add(CMC_Light_Source_A, ANDROID_SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN);
    return NO_ERROR;
}

ia_cmc_t* AiqConf::getCMCHandler()
{
    if (mCMC == NULL)
        initCMC();
    return mCMC;
}

void AiqConf::clear()
{
    if(mCMC) {
        ia_cmc_parser_deinit(mCMC);
        mCMC = NULL;
    }
}

status_t AiqConf::fillStaticMetadataFromCMC(camera_metadata_t * metadata)
{
    status_t res = OK;
    if (mCMC == NULL) {
        res = initCMC();
        if (res != NO_ERROR)
            return res;
    }

    // Check if the metadata has been filled
    if (mMetadata == metadata) {
        return res;
    } else {
        mMetadata = metadata;
    }
    initMapping();
    // Lens
    res |= fillLensStaticMetadata(metadata);
    res |= fillSensorStaticMetadata(metadata);
    res |= fillLscSizeStaticMetadata(metadata);
    return res;
}

status_t AiqConf::fillLensStaticMetadata(camera_metadata_t * metadata)
{
    status_t res = OK;
    if (mCMC->cmc_parsed_optics.cmc_optomechanics != NULL) {
        uint16_t camera_features = mCMC->cmc_parsed_optics.cmc_optomechanics->camera_actuator_features;
        // Lens: aperture
        float fn = 2.53;
        if (!(camera_features & CMC_CAM_VariableApertures)
            && mCMC->cmc_parsed_optics.cmc_optomechanics->num_apertures == 1
            && mCMC->cmc_parsed_optics.lut_apertures != NULL) {
            // fixed aperture, the fn should be divided 100 because the value is multiplied 100 in cmc
            fn = (float)mCMC->cmc_parsed_optics.lut_apertures[0] / 100;
            float av = log10(pow(fn, 2)) / log10(2.0);
            av = ((int)(av * 10 + 0.5)) / 10.0;
            res |= MetadataHelper::updateMetadata(metadata, ANDROID_LENS_INFO_AVAILABLE_APERTURES, &av, 1);
            LOG2("static ANDROID_LENS_INFO_AVAILABLE_APERTURES :%.2f", av);
        }
        // Lens: FilterDensities
        int32_t nd_gain = 0;
        if (camera_features & CMC_CAM_NdFilter)
            nd_gain = mCMC->cmc_parsed_optics.cmc_optomechanics->nd_gain;
        res |= MetadataHelper::updateMetadata(metadata, ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES, (float*)&nd_gain, 1);
        LOG2("static ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES :%d", nd_gain);
        // Lens: availableFocalLengths, only support fixed focal length
        float effect_focal_length = 3.00;
        if (!(camera_features & CMC_CAM_OpticalZoom)) {
            // focal length (mm * 100) from CMC
            effect_focal_length = (float)mCMC->cmc_parsed_optics.cmc_optomechanics->effect_focal_length / 100;
            res |= MetadataHelper::updateMetadata(metadata, ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, &effect_focal_length, 1);
            LOG2("static ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS :%.2f", effect_focal_length);
        }
        // TODO: When the VIED BZ: 2114 is fixed, add the actuator type check.
        float min_focus_distance = 0.0;

        // the unit from CMC is cm, convert to diopters (1/m)
        if (mCMC->cmc_parsed_optics.cmc_optomechanics->min_focus_distance != 0)
            min_focus_distance = 100 / (float)mCMC->cmc_parsed_optics.cmc_optomechanics->min_focus_distance;

        camera_metadata_ro_entry entry;
        find_camera_metadata_ro_entry(metadata, ANDROID_CONTROL_AF_AVAILABLE_MODES, &entry);
        // Lens: minimumFocusDistance, the metadata should be equal to zero in fixed focus case
        if (entry.count == 1 && entry.data.u8[0] == ANDROID_CONTROL_AF_MODE_OFF)
            min_focus_distance = 0.0;

        res |= MetadataHelper::updateMetadata(metadata, ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE, &min_focus_distance, 1);
        LOG2("static ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE :%.2f", min_focus_distance);
        // android.lens.info.hyperfocalDistance
        // the unit of the sensor pix size is 100 * um
        float pixel_size_NxN_um = mCMC->cmc_parsed_optics.cmc_optomechanics->sensor_pix_size_h / 100;
        int selected_circle_of_confusion_px = 2;
        float selected_circle_of_confusion_um = pixel_size_NxN_um * selected_circle_of_confusion_px;
        float hyperfocal_distance_mm = (effect_focal_length * effect_focal_length / (fn * selected_circle_of_confusion_um)) * 1000;
        float hyperfocal_distance_diopter = 1000 / hyperfocal_distance_mm;
        res |= MetadataHelper::updateMetadata(metadata, ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE, &hyperfocal_distance_diopter, 1);
        LOG2("static ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE :%.2f", hyperfocal_distance_mm);
        // TODO: android.lens.info.availableOpticalStabilization
    }
    return res;
}

status_t AiqConf:: fillLightSourceStaticMetadata(camera_metadata_t * metadata)
{
    status_t res = UNKNOWN_ERROR;
    // color matrices
    if (mCMC->cmc_parsed_color_matrices.cmc_color_matrix != NULL && mCMC->cmc_parsed_color_matrices.cmc_color_matrices != NULL) {
        int num_matrices = mCMC->cmc_parsed_color_matrices.cmc_color_matrices->num_matrices;
        // android metadata requests 2 light source
        if (num_matrices < 2)
            return res;

        res = OK;
        // calibrationTransform1
        // the below matrix should be sample specific transformation to golden module
        // in the future the matrix should be calculated by the data from NVM
        uint16_t calibrationTransform[TRANSFORM_MATRIX_SIZE] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        int16_t reference_illuminant1[2];
        int sensor_calibration_transform_tag[2] = {ANDROID_SENSOR_CALIBRATION_TRANSFORM1, ANDROID_SENSOR_CALIBRATION_TRANSFORM2};
        int sensor_reference_illuminant_tag[2] = {ANDROID_SENSOR_REFERENCE_ILLUMINANT1, ANDROID_SENSOR_REFERENCE_ILLUMINANT2};
        int sensor_color_transform_tag[2] = {ANDROID_SENSOR_COLOR_TRANSFORM1, ANDROID_SENSOR_COLOR_TRANSFORM2};
        int sensor_forward_matrix_tag[2] = {ANDROID_SENSOR_FORWARD_MATRIX1, ANDROID_SENSOR_FORWARD_MATRIX2};
        int32_t matrix_accurate[TRANSFORM_MATRIX_SIZE];
        int16_t reference_illuminant[2];
        float XYZ_to_sRGB[TRANSFORM_MATRIX_SIZE] = {3.2404542, -1.5371385, -0.4985314,
                                                    -0.9692660, 1.8760108, 0.0415560,
                                                    0.0556434, -0.2040259, 1.0572252};
        float XYZ_to_sRGB_det = XYZ_to_sRGB[0] * XYZ_to_sRGB[4] * XYZ_to_sRGB[8] +
                                XYZ_to_sRGB[1] * XYZ_to_sRGB[5] * XYZ_to_sRGB[6] +
                                XYZ_to_sRGB[2] * XYZ_to_sRGB[3] * XYZ_to_sRGB[7] -
                                XYZ_to_sRGB[2] * XYZ_to_sRGB[4] * XYZ_to_sRGB[6] -
                                XYZ_to_sRGB[0] * XYZ_to_sRGB[5] * XYZ_to_sRGB[7] -
                                XYZ_to_sRGB[1] * XYZ_to_sRGB[3] * XYZ_to_sRGB[8];
        float XYZ_to_sRGB_adj[TRANSFORM_MATRIX_SIZE];
        //adjugate matrix
        XYZ_to_sRGB_adj[0] = XYZ_to_sRGB[4] * XYZ_to_sRGB[8] - XYZ_to_sRGB[5] * XYZ_to_sRGB[7];
        XYZ_to_sRGB_adj[1] = XYZ_to_sRGB[2] * XYZ_to_sRGB[7] - XYZ_to_sRGB[1] * XYZ_to_sRGB[8];
        XYZ_to_sRGB_adj[2] = XYZ_to_sRGB[1] * XYZ_to_sRGB[5] - XYZ_to_sRGB[2] * XYZ_to_sRGB[4];
        XYZ_to_sRGB_adj[3] = XYZ_to_sRGB[5] * XYZ_to_sRGB[6] - XYZ_to_sRGB[3] * XYZ_to_sRGB[8];
        XYZ_to_sRGB_adj[4] = XYZ_to_sRGB[0] * XYZ_to_sRGB[8] - XYZ_to_sRGB[2] * XYZ_to_sRGB[6];
        XYZ_to_sRGB_adj[5] = XYZ_to_sRGB[2] * XYZ_to_sRGB[3] - XYZ_to_sRGB[0] * XYZ_to_sRGB[5];
        XYZ_to_sRGB_adj[6] = XYZ_to_sRGB[3] * XYZ_to_sRGB[7] - XYZ_to_sRGB[4] * XYZ_to_sRGB[6];
        XYZ_to_sRGB_adj[7] = XYZ_to_sRGB[1] * XYZ_to_sRGB[6] - XYZ_to_sRGB[0] * XYZ_to_sRGB[7];
        XYZ_to_sRGB_adj[8] = XYZ_to_sRGB[0] * XYZ_to_sRGB[4] - XYZ_to_sRGB[1] * XYZ_to_sRGB[3];
        // inverse matrix
        if (!(XYZ_to_sRGB_det >= -EPSILON && XYZ_to_sRGB_det <= EPSILON)) {
            for (int i = 0; i < TRANSFORM_MATRIX_SIZE; i++)
                XYZ_to_sRGB_adj[i] = XYZ_to_sRGB_adj[i] / XYZ_to_sRGB_det;
        }
        camera_metadata_rational_t forward_matrix[TRANSFORM_MATRIX_SIZE];

        for(int light_src_num = 0; light_src_num < 2; light_src_num++) {
            CMC_Light_Source light_src = (CMC_Light_Source)mCMC->cmc_parsed_color_matrices.cmc_color_matrix[light_src_num].light_src_type;
            int index = mLightSourceMap.indexOfKey(light_src);
            if (index == NAME_NOT_FOUND) {
                LOG2("light source not found, use default!!");
                reference_illuminant[light_src_num] = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
            } else {
                reference_illuminant[light_src_num] = mLightSourceMap.valueFor(light_src);
            }
            // referenceIlluminant
            res |= MetadataHelper::updateMetadata(metadata, sensor_reference_illuminant_tag[light_src_num], &reference_illuminant[light_src_num], 1);
            // calibrationTransform
            res |= MetadataHelper::updateMetadata(metadata, sensor_calibration_transform_tag[light_src_num], &calibrationTransform, TRANSFORM_MATRIX_SIZE);
            // colorTransform
            memcpy(matrix_accurate, mCMC->cmc_parsed_color_matrices.cmc_color_matrix[light_src_num].matrix_accurate,
                   (sizeof(int32_t) * TRANSFORM_MATRIX_SIZE));
            res |= MetadataHelper::updateMetadata(metadata, sensor_color_transform_tag[light_src_num], &matrix_accurate, TRANSFORM_MATRIX_SIZE);
            LOG2("matrix_accurate:%d,%d,%d,%d,%d,%d,%d,%d,%d",matrix_accurate[0],
                 matrix_accurate[1],matrix_accurate[2],matrix_accurate[3],
                 matrix_accurate[4],matrix_accurate[5],matrix_accurate[6],
                 matrix_accurate[7],matrix_accurate[8]);

            // forwardMatrix
            if (!(XYZ_to_sRGB_det >= -EPSILON && XYZ_to_sRGB_det <= EPSILON)) {
                int row = 0;
                int col = 0;
                for (int i = 0; i < TRANSFORM_MATRIX_SIZE; i++) {
                    row = i / 3;
                    col = i % 3;
                    forward_matrix[i].numerator = 0;
                    // CMC matrix is in Q16 format, so denominator is 65536
                    forward_matrix[i].denominator = FORWARD_MATRIX_PRECISION;
                    // matrix multiply
                    for (int j = 0; j < 3; j++)
                        forward_matrix[i].numerator = forward_matrix[i].numerator + XYZ_to_sRGB_adj[row * 3 + j] * matrix_accurate[j * 3 + col];
                    LOG2("forward matrix [%d] = {%d, %d}", i, forward_matrix[i].numerator, forward_matrix[i].denominator);
                }
                res |= MetadataHelper::updateMetadata(metadata, sensor_forward_matrix_tag[light_src_num], forward_matrix, TRANSFORM_MATRIX_SIZE);
            }
        }
    }
    return res;
}

status_t AiqConf::fillSensorStaticMetadata(camera_metadata_t * metadata)
{
    LOG1("%s", __FUNCTION__);
    status_t res = OK;
    // colorFilterArrangement
    if (mCMC->cmc_general_data != NULL) {
        uint16_t color_order = mCMC->cmc_general_data->color_order;
        res |= MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT, &color_order, 1);
        LOG2("color order:%d", color_order);
    }
    // whiteLevel
    if (mCMC->cmc_saturation_level != NULL) {
        int32_t saturation_level = mCMC->cmc_saturation_level->saturation_cc1;
        res |= MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_INFO_WHITE_LEVEL, &saturation_level, 1);
        LOG2("saturation_level:%d", saturation_level);
    }

    int16_t base_iso = 0;
    // baseGainFactor
    if (mCMC->cmc_sensitivity != NULL) {
        // baseGainFactor's definition is gain factor from electrons to raw units when ISO=100
        uint16_t base_gain_factor[] = {100, 0};
        base_iso = mCMC->cmc_sensitivity->base_iso;
        base_gain_factor[1] = base_iso;
        res |= MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_BASE_GAIN_FACTOR, &base_gain_factor, 2);
        LOG2("base_iso:%d", base_iso);
    }
    // blackLevelPattern
    if (mCMC->cmc_parsed_black_level.cmc_black_level_luts != NULL) {
        uint16_t cc1 = mCMC->cmc_parsed_black_level.cmc_black_level_luts->color_channels.cc1 / 256;
        uint16_t cc2 = mCMC->cmc_parsed_black_level.cmc_black_level_luts->color_channels.cc2 / 256;
        uint16_t cc3 = mCMC->cmc_parsed_black_level.cmc_black_level_luts->color_channels.cc3 / 256;
        uint16_t cc4 = mCMC->cmc_parsed_black_level.cmc_black_level_luts->color_channels.cc4 / 256;
        int32_t black_level_pattern[4];
        black_level_pattern[0] = cc1;
        black_level_pattern[1] = cc2;
        black_level_pattern[2] = cc3;
        black_level_pattern[3] = cc4;
        res |= MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_BLACK_LEVEL_PATTERN, &black_level_pattern, 4);
        LOG2("blackLevelPattern, cc1:%d,cc2:%d,cc3:%d,cc4:%d", cc1, cc2, cc3, cc4);
    }
    // fill metadata: referenceIlluminant, android.sensor.colorTransform,
    //                android.sensor.forwardMatrix, android.sensor.calibrationTransform
    fillLightSourceStaticMetadata(metadata);

    // maxAnalogSensitivity
    if (mCMC->cmc_parsed_analog_gain_conversion.cmc_analog_gain_conversion != NULL) {
        int32_t max_analog_sensitivity = 0;
        float max_analog_gain = 0.0;
        unsigned short analog_gain_code = 0;
        // we can give a large value (e.g. 1000) as input to ia_exc.
        // Output from ia_exc is a clipped sensor specific MAX.
        ia_exc_analog_gain_to_sensor_units(&(mCMC->cmc_parsed_analog_gain_conversion), 1000, &analog_gain_code);
        ia_exc_sensor_units_to_analog_gain(&(mCMC->cmc_parsed_analog_gain_conversion),
                                           analog_gain_code,
                                           &max_analog_gain);
       // caclulate the iso base on max analog gain
       max_analog_sensitivity = max_analog_gain * base_iso;
       res |= MetadataHelper::updateMetadata(metadata, ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY, &max_analog_sensitivity, 1);
    }
    // noiseProfile, this should be got from CMC in the future, currently use default value first
    return res;
}

/*
Android requires lensShadingMapSize must be smaller than 64*64,
and it is static, however, in some case, such as video recording,
the width and height got from pa_results->weighted_lsc->cmc_lens_shading->grid_width
and grid_height are variable for different resolution.
here we are calculating the size of the downscaled LSC table we will calculate for them.
Preserve the same Aspect Ratio.
*/
status_t AiqConf::fillLscSizeStaticMetadata(camera_metadata_t * metadata)
{
    status_t res = OK;
    if (mCMC->cmc_parsed_lens_shading.cmc_lens_shading != NULL) {
        int32_t lensShadingMapSize[2];
        lensShadingMapSize[0] = mCMC->cmc_parsed_lens_shading.cmc_lens_shading->grid_width;
        lensShadingMapSize[1] = mCMC->cmc_parsed_lens_shading.cmc_lens_shading->grid_height;

        int i = 1;
        uint16_t destWidth = lensShadingMapSize[0];
        uint16_t destHeight = lensShadingMapSize[1];

        while (destWidth > MAX_LSC_GRID_WIDTH || destHeight > MAX_LSC_GRID_HEIGHT) {
               i++;
               destWidth = lensShadingMapSize[0] / i;
               destHeight = lensShadingMapSize[1] / i;
        }
        lensShadingMapSize[0] = destWidth;
        lensShadingMapSize[1] = destHeight;
        LOG2("grid_width:%d, grid_height:%d", lensShadingMapSize[0], lensShadingMapSize[1]);
        res = MetadataHelper::updateMetadata(metadata, ANDROID_LENS_INFO_SHADING_MAP_SIZE, &lensShadingMapSize, 2);
    }
    return res;
}

CpfStore::CpfStore(const int cameraId)
    : mCameraId(cameraId)
    , mIsOldConfig(false)
{
    LOG1("@%s", __FUNCTION__);
    CameraBlob aiqConf;

    // If anything goes wrong here, we simply return silently.
    // CPF should merely be seen as a way to do multiple configurations
    // at once; failing in that is not a reason to return with errors
    // and terminate the camera (some cameras may not have any CPF at all)

    if (mCameraId >= PlatformData::numberOfCameras()) {
        LOGE("ERROR bad camera index!");
        mCameraId = -1;
        return;
    }

    // Find out the related file names
    if (initFileNames(mCpfPathName)) {
        // Error message given already
        return;
    }

    // Obtain the configurations
    if (initConf(aiqConf)) {
        // Error message given already
        return;
    }

    // Provide configuration data for algorithms and image
    // quality purposes, and continue further even if errors did occur.
    // Pointer to that data is cleared later, whenever seen suitable,
    // so that the memory reserved for CPF data can then be freed
    AiqConfig = aiqConf;
}

CpfStore::~CpfStore()
{
    LOG1("@%s", __FUNCTION__);
}

status_t CpfStore::initFileNames(String8& cpfPathName)
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = 0;

    // First, we see what drivers we have in the system
    if ((ret = initDriverList())) {
        // Error message given already
        return ret;
    }

    // Secondly, we will find a matching configuration file
    int drvIndex = -1;   // Index of registered driver for which CPF file exists
    String8 cpfFileName; // Name of the corresponding CPF config file
    if ((ret = findConfigWithDriver(cpfFileName, drvIndex))) {
        // Error message given already
        return ret;
    }

    // Here is the correct CPF file
    cpfPathName = cpfConfigPath;
    cpfPathName.appendPath(cpfFileName);

    LOG1("cpf config file name: %s", cpfPathName.string());

    return ret;
}

status_t CpfStore::initDriverList()
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = 0;

    if (mRegisteredDrivers.size() > 0) {
        // We only need to go through the drivers once
        return 0;
    }

    // Sensor drivers have been registered to media controller
    const char *mcPathName = "/dev/media0";
    int fd = open(mcPathName, O_RDONLY);
    if (fd == -1) {
        LOGE("ERROR in opening media controller: %s!", strerror(errno));
        return ENXIO;
    }

    struct media_entity_desc entity;
    CLEAR(entity);
    do {
        // Go through the list of media controller entities
        entity.id |= MEDIA_ENT_ID_FLAG_NEXT;
        if (ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entity) < 0) {
            if (errno == EINVAL) {
                // Ending up here when no more entities left.
                // Will simply 'break' if everything was ok
                if (mRegisteredDrivers.size() == 0) {
                    // No registered drivers found
                    LOGE("ERROR no sensor driver registered in media controller!");
                    ret = NO_INIT;
                }
            } else {
                LOGE("ERROR in browsing media controller entities: %s!", strerror(errno));
                ret = FAILED_TRANSACTION;
            }
            break;
        } else {
            if (entity.type == MEDIA_ENT_T_V4L2_SUBDEV_SENSOR) {
                // A driver has been found!
                // The driver is using sensor name when registering
                // to media controller (we will truncate that to
                // first space, if any)
                SensorDriver drvInfo;
                drvInfo.mSensorName = entity.name;
                // Cut the name to first space
                for (int i = 0; (i = drvInfo.mSensorName.find(" ")) > 0; drvInfo.mSensorName.setTo(drvInfo.mSensorName, i));

                unsigned major = entity.v4l.major;
                unsigned minor = entity.v4l.minor;

                // Go thru the subdevs one by one, see which one
                // corresponds to this driver (if there is an error,
                // the looping ends at 'while')
                ret = initDriverListHelper(major, minor, drvInfo);
            }
        }
    } while (!ret);

    if (close(fd)) {
        LOGE("ERROR in closing media controller: %s!", strerror(errno));
        if (!ret) ret = EPERM;
    }

    return ret;
}

status_t CpfStore::initDriverListHelper(unsigned major, unsigned minor, SensorDriver& drvInfo)
{
    LOG1("@%s", __FUNCTION__);
    String8 subdevPathNameN;

    const char *subdevPathName = "/dev/v4l-subdev%d";
    for (int n = 0; true; n++) {
        subdevPathNameN = String8::format(subdevPathName, n);
        struct stat fileInfo;
        if (stat(subdevPathNameN, &fileInfo) < 0) {
            // We end up here when there are no more subdevs
            if (errno == ENOENT) {
                LOGE("ERROR sensor subdev missing: \"%s\"!", subdevPathNameN.string());
                return NO_INIT;
            } else {
                LOGE("ERROR querying sensor subdev filestat for \"%s\": %s!", subdevPathNameN.string(), strerror(errno));
                return FAILED_TRANSACTION;
            }
        }
        if ((major == MAJOR(fileInfo.st_rdev)) && (minor == MINOR(fileInfo.st_rdev))) {
            drvInfo.mDeviceName = subdevPathNameN.getPathLeaf();
            mRegisteredDrivers.push(drvInfo);
            LOG1("Registered sensor driver \"%s\" found for sensor \"%s\"", drvInfo.mDeviceName.string(), drvInfo.mSensorName.string());
            // All ok
            break;
        }
    }

    return 0;
}

status_t CpfStore::findConfigWithDriver(String8& cpfName, int& drvIndex)
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = 0;
    bool anyMatch = false;

    // We go the directory containing CPF files thru one by one file,
    // and see if a particular file is something to react upon. If yes,
    // we then see if there is a corresponding driver registered. It
    // is allowed to have more than one CPF file for particular driver
    // (spId values are used for further distinguishing in that case),
    // but having more than one suitable driver registered is a strict no no...

    DIR *dir = opendir(cpfConfigPath);
    if (!dir) {
        LOGE("ERROR in opening CPF folder \"%s\": %s!", cpfConfigPath, strerror(errno));
        return ENOTDIR;
    }

    do {
        dirent *entry;
        if (errno = 0, !(entry = readdir(dir))) {
            if (errno) {
                LOGE("ERROR in browsing CPF folder \"%s\": %s!", cpfConfigPath, strerror(errno));
                ret = FAILED_TRANSACTION;
            }
            // If errno was not set, return 0 means end of directory.
            // So, let's see if we found any (suitable) CPF files
            if (drvIndex < 0) {
                if (anyMatch) {
                    LOGE("NOTE no suitable CPF files found in CPF folder \"%s\" (ok for SOC cameras)", cpfConfigPath);
                } else {
                    LOGE("NOTE not a single CPF file found in CPF folder \"%s\" (ok for SOC cameras)", cpfConfigPath);
                }
                ret = NO_INIT;
            }
            break;
        }
        if ((strcmp(entry->d_name, ".") == 0) ||
            (strcmp(entry->d_name, "..") == 0)) {
            continue;  // Skip self and parent
        }
        String8 pattern = String8::format(cpfConfigPattern, mCameraId);
        int r = fnmatch(pattern, entry->d_name, 0);
        switch (r) {
        case 0:
            // The file name looks like a valid CPF file name
            anyMatch = true;
            // See if we have corresponding driver registered
            // (if there is an error, the looping ends at 'while')
            ret = findConfigWithDriverHelper(String8(entry->d_name), cpfName, drvIndex);
            continue;
        case FNM_NOMATCH:
            // The file name did not look like a CPF file name
            continue;
        default:
            // Unknown error (the looping ends at 'while')
            LOGE("ERROR in pattern matching file name \"%s\"!", entry->d_name);
            ret = UNKNOWN_ERROR;
            continue;
        }
    } while (!ret);

    if (closedir(dir)) {
        LOGE("ERROR in closing CPF folder \"%s\": %s!", cpfConfigPath, strerror(errno));
        if (!ret) ret = EPERM;
    }

    return ret;
}

status_t CpfStore::findConfigWithDriverHelper(const String8& fileName, String8& cpfName, int& index)
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = 0;

    for (int i = mRegisteredDrivers.size(); i-- > 0; ) {
        if (fileName.find(mRegisteredDrivers[i].mSensorName) < 0) {
            // Name of this registered driver was not found
            // from within CPF looking file name -> skip it
            continue;
        } else {
            // Since we are here, we do have a registered
            // driver whose name maps to this CPF file name
            if (index < 0) {
                // No previous CPF<>driver pairs
                index = i;
                cpfName = fileName;
            } else {
                if (index == i) {
                    // Multiple CPF files match the driver
                    // If there are cpf files for different products with the same sensor name,
                    // the files are distinguished by spId
                    // Let's check for the vendor_id, platform_family_id and product_line_id
                    String8 vendorPlatformProduct;
                    if ((PlatformData::createVendorPlatformProductName(vendorPlatformProduct) == 0) &&
                        (fileName.find(vendorPlatformProduct) >= 0)) {
                            cpfName = fileName;
                    }
                } else {
                    // We just got lost:
                    // Which is the correct sensor driver?
                    LOGE("ERROR multiple driver candidates for CPF file \"%s\"!", fileName.string());
                    ret = ENOTUNIQ;
                }
            }
        }
    }

    return ret;
}

status_t CpfStore::initConf(CameraBlob& aiqConf)
{
    LOG1("@%s", __FUNCTION__);
    status_t ret = 0;

    // First, we load the correct configuration file.
    // It will be behind reference counted MemoryHeapBase
    // object "allConf", meaning that the memory will be
    // automatically freed when it is no longer being pointed at
    if ((ret = loadConf(aiqConf)))
        return ret;

    return ret;
}

status_t CpfStore::loadConf(CameraBlob& allConf)
{
    LOG1("@%s", __FUNCTION__);
    FILE *file;
    struct stat statCurrent;
    status_t ret = 0;

    LOG1("Opening CPF file \"%s\"", mCpfPathName.string());
    file = fopen(mCpfPathName, "rb");
    if (!file) {
        LOGE("ERROR in opening CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
        return NAME_NOT_FOUND;
    }

    do {
        int fileSize;
        if ((fseek(file, 0, SEEK_END) < 0) ||
            ((fileSize = ftell(file)) < 0) ||
            (fseek(file, 0, SEEK_SET) < 0)) {
            LOGE("ERROR querying properties of CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
            ret = ESPIPE;
            break;
        }

        allConf = CameraBlob(fileSize);
        if (!allConf) {
            LOGE("ERROR no memory in %s!", __func__);
            ret = NO_MEMORY;
            break;
        }

        if (fread(allConf, fileSize, 1, file) < 1) {
            LOGE("ERROR reading CPF file \"%s\"!", mCpfPathName.string());
            ret = EIO;
            break;
        }

        // We use file statistics for file identification purposes.
        // The access time was just changed (because of us!),
        // so let's nullify the access time info
        if (stat(mCpfPathName, &statCurrent) < 0) {
            LOGE("ERROR querying filestat of CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
            ret = FAILED_TRANSACTION;
            break;
        }
        statCurrent.st_atime = 0;
        statCurrent.st_atime_nsec = 0;

    } while (0);

    if (fclose(file)) {
        LOGE("ERROR in closing CPF file \"%s\": %s!", mCpfPathName.string(), strerror(errno));
        if (!ret) ret = EPERM;
    }

    return ret;
}

}; // namespace camera2

} // namespace android
