/*
 * Copyright (c) 2014 Intel Corporation.
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
#define LOG_TAG "Camera_DVS2"

#include "LogHelper.h"
#include "ia_cmc_parser.h"
#include "Utils.h"
#include "IaDvs2.h"

const unsigned int DVS_MAX_WIDTH = RESOLUTION_1080P_WIDTH;
const unsigned int DVS_MAX_HEIGHT = RESOLUTION_1080P_HEIGHT;

namespace android {
namespace camera2 {


#define DIGITAL_ZOOM_RATIO 1.0f
#define DVS_MIN_ENVELOPE 6
#define DUMP_DVSLOG     "/data/dvs_log"
#define TEST_FRAMES 300
#define LOG_LEN 4096

//workaround: when DVS is on, the first 2 frames are greenish, need to be skipped.
#define DVS_SKIP_FRAMES 2

bool IaDvs2::mDumpLogEnabled = false;
IaDvs2::IaDvs2(HWControlGroup &hwcg) :
    mIsp(hwcg.mIspCI)
    ,mSensorCI(hwcg.mSensorCI)
    ,mCapInfo(NULL)
    ,mDvs2stats(NULL)
    ,mState(NULL)
    ,mMorphTable(NULL)
    ,mDVSEnabled(false)
    ,mZoomRatioChanged(true)
{
    LOG1("@%s", __FUNCTION__);
    CLEAR(mStatistics);
    CLEAR(mDumpParams);
    CLEAR(mDvs2Env);
    CLEAR(mDvs2Config);

    mCapInfo = getIPU2CameraCapInfo(mSensorCI->getCurrentCameraId());
    ia_err err;
    char const *version;
    err = dvs_get_version(&version);
    if(err == ia_err_none)
        LOG2("DVS2 lib Version: %s", version);

    mDumpLogEnabled = gLogLevel & CAMERA_DUMP_DVS2;
    mDvs2Env.vdebug = debugPrint;
    mDvs2Env.verror = debugPrint;
    mDvs2Env.vinfo = debugPrint;
    mDvs2Config.num_axis = ia_dvs2_algorihm_0_axis;
    /**< effective vertical scan ratio, used for rolling correction
      (Non-blanking ration of frame interval) */
    mDvs2Config.nonblanking_ratio = 0.88f;
}

IaDvs2::~IaDvs2()
{
    LOG1("@%s", __FUNCTION__);
    if (mDumpLogEnabled) {
       if (!mDumpParams.binaryDumpFailed)
           writeBinaryDump(DUMP_DVSLOG);
       dvs_deinit_dbglog();
    }
    if(mMorphTable) {
        dvs_free_morph_table(mMorphTable);
        mMorphTable = NULL;
    }
    if (mDvs2stats) {
        dvs_free_statistics(mDvs2stats);
        mDvs2stats = NULL;
    }
    if (mState) {
        dvs_deinit(mState);
        mState = NULL;
    }
}

status_t IaDvs2::dvsInit()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    ia_err err;

    if (PlatformData::AiqConfig) {
        ia_binary_data cpfData;
        cpfData.data = PlatformData::AiqConfig.ptr();
        cpfData.size = PlatformData::AiqConfig.size();
        ia_cmc_t *cmc = PlatformData::AiqConfig.getCMCHandler();
        if (cmc != NULL) {
            err = dvs_init(&mState, &cpfData, cmc, NULL, &mDvs2Env);
        } else {
            err = dvs_init(&mState, NULL, NULL, NULL, &mDvs2Env);
        }
    } else {
        err = dvs_init(&mState, NULL, NULL, NULL, &mDvs2Env);
    }

    if (err != ia_err_none) {
        LOGE("Failed to initilize the DVS library");
        status = NO_INIT;
    }

    return status;
}

status_t IaDvs2::reconfigure()
{
    LOG1("@%s", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    return reconfigureNoLock();
}

status_t IaDvs2::allocateDvs2Statistics(atomisp_dvs_grid_info info)
{
    status_t status = NO_ERROR;
    ia_err err;
    if (mDvs2stats) {
        dvs_free_statistics(mDvs2stats);
        mDvs2stats = NULL;
    }
    err = dvs_allocate_statistics(&info, &mDvs2stats);
    if (err != ia_err_none) {
        LOGW("dvs_allocate_statistics error:%d", err);
        return UNKNOWN_ERROR;
    }
    memcpy(&(mStatistics.dvs2_stat), mDvs2stats, sizeof(struct atomisp_dvs2_statistics));
    return status;
}

status_t IaDvs2::allocateDvs2MorphTable()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    ia_err err;
    if (mMorphTable) {
        dvs_free_morph_table(mMorphTable);
        mMorphTable = NULL;
    }
    if(mState) {
        err = dvs_allocate_morph_table(mState, &mMorphTable);
        if (err != ia_err_none)
            return UNKNOWN_ERROR;
    }
    return status;
}

status_t IaDvs2::reconfigureNoLock()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    ia_err err = ia_err_none;
    struct atomisp_parm isp_params;
    struct atomisp_dvs2_bq_resolutions bq_res;
    struct atomisp_dvs_grid_info dvs_grid;
    int bq_max_width, bq_max_height;

    if (!mState)
        return status;

    status = mIsp->getIspParameters(&isp_params);
    if (status != NO_ERROR)
        return status;

    status = mIsp->getIspDvs2BqResolutions(&bq_res);
    if (status != NO_ERROR)
        return status;

    CLEAR(dvs_grid);
    dvs_grid = isp_params.dvs_grid;

    //Configure DVS
    mDvs2Config.grid_size = dvs_grid.bqs_per_grid_cell;
    mDvs2Config.source_bq.width_bq = bq_res.source_bq.width_bq;
    mDvs2Config.source_bq.height_bq = bq_res.source_bq.height_bq;
    mDvs2Config.output_bq.width_bq = bq_res.output_bq.width_bq;
    mDvs2Config.output_bq.height_bq = bq_res.output_bq.height_bq;
    mDvs2Config.ispfilter_bq.width_bq = bq_res.ispfilter_bq.width_bq;
    mDvs2Config.ispfilter_bq.height_bq = bq_res.ispfilter_bq.height_bq;
    mDvs2Config.envelope_bq.width_bq = bq_res.envelope_bq.width_bq;
    mDvs2Config.envelope_bq.height_bq = bq_res.envelope_bq.height_bq;
    mDvs2Config.gdc_shift_x = bq_res.gdc_shift_bq.width_bq;
    mDvs2Config.gdc_shift_y = bq_res.gdc_shift_bq.height_bq;
    mDvs2Config.oxdim_y = 64;
    mDvs2Config.oydim_y = 64;
    mDvs2Config.oxdim_uv = 64;
    mDvs2Config.oydim_uv = 32;

    mDvs2Config.hw_config.scan_mode = ia_dvs2_gdc_scan_mode_stb; //hardcoded
    mDvs2Config.hw_config.interpolation = ia_dvs2_gdc_interpolation_bli; //hardcoded
    mDvs2Config.hw_config.performance_point = ia_dvs2_gdc_performance_point_1x1; //hardcoded

    mDVSEnabled = mIsp->dvsEnabled();
    if(mDVSEnabled) {
        // Check if DVS is enabled in driver by the envelope value
        if(mDvs2Config.envelope_bq.width_bq && mDvs2Config.envelope_bq.height_bq) {
            mDvs2Config.num_axis = ia_dvs2_algorihm_4_axis;
        } else {
            mDvs2Config.num_axis = ia_dvs2_algorihm_0_axis;
            mDVSEnabled = false;
        }
    } else {
        mDvs2Config.num_axis = ia_dvs2_algorihm_0_axis;
    }
    LOG2("DVS enabled:%s", mDVSEnabled ? "true": "false");
    /* setup binary dump parameter */
    mDumpParams.frames = TEST_FRAMES;
    mDumpParams.endless = false;

    bq_max_width = int(maxDvs2YUVDSRatio * float(mDvs2Config.output_bq.width_bq));
    bq_max_height = int(maxDvs2YUVDSRatio * float(mDvs2Config.output_bq.height_bq));

    // scaling_ratio = (output_bq) / (source_bq - envelope_bq - ispfilter_bq)
    // do crop by envelope if the YUV downscaling ratio exceeds the limitation
    if(mDvs2Config.source_bq.width_bq - mDvs2Config.envelope_bq.width_bq - mDvs2Config.ispfilter_bq.width_bq > bq_max_width)
        mDvs2Config.envelope_bq.width_bq = mDvs2Config.source_bq.width_bq - mDvs2Config.ispfilter_bq.width_bq - bq_max_width;

    if(mDvs2Config.source_bq.height_bq - mDvs2Config.envelope_bq.height_bq - mDvs2Config.ispfilter_bq.height_bq > bq_max_height)
        mDvs2Config.envelope_bq.height_bq = mDvs2Config.source_bq.height_bq - mDvs2Config.ispfilter_bq.height_bq - bq_max_height;

    LOG2("mDvs2Config.num_axis %d", mDvs2Config.num_axis);
    LOG2("mDvs2Config.nonblanking_ratio %f", mDvs2Config.nonblanking_ratio);
    LOG2("mDvs2Config.grid_size %d", mDvs2Config.grid_size);
    LOG2("mDvs2Config.source_bq.width_bq %d", mDvs2Config.source_bq.width_bq);
    LOG2("mDvs2Config.source_bq.height_bq %d", mDvs2Config.source_bq.height_bq);
    LOG2("mDvs2Config.output_bq.width_bq %d", mDvs2Config.output_bq.width_bq);
    LOG2("mDvs2Config.output_bq.height_bq %d", mDvs2Config.output_bq.height_bq);
    LOG2("mDvs2Config.envelope_bq.width_bq %d", mDvs2Config.envelope_bq.width_bq);
    LOG2("mDvs2Config.envelope_bq.height_bq %d", mDvs2Config.envelope_bq.height_bq);
    LOG2("mDvs2Config.ispfilter_bq.width_bq %d", mDvs2Config.ispfilter_bq.width_bq);
    LOG2("mDvs2Config.ispfilter_bq.height_bq %d", mDvs2Config.ispfilter_bq.height_bq);
    LOG2("mDvs2Config.gdc_shift_x %d", mDvs2Config.gdc_shift_x);
    LOG2("mDvs2Config.gdc_shift_y %d", mDvs2Config.gdc_shift_y);
    LOG2("mDvs2Config.oxdim_y %d", mDvs2Config.oxdim_y);
    LOG2("mDvs2Config.oydim_y %d", mDvs2Config.oydim_y);
    LOG2("mDvs2Config.oxdim_uv %d", mDvs2Config.oxdim_uv);
    LOG2("mDvs2Config.oydim_uv %d", mDvs2Config.oydim_uv);
    LOG2("mDvs2Config.hw_config.scan_mode %d", mDvs2Config.hw_config.scan_mode);
    LOG2("mDvs2Config.hw_config.interpolation %d", mDvs2Config.hw_config.interpolation);
    LOG2("mDvs2Config.hw_config.performance_point %d", mDvs2Config.hw_config.performance_point);

    err = dvs_config(mState, &mDvs2Config, DIGITAL_ZOOM_RATIO, &mDumpParams);
    if (err != ia_err_none) {
        LOGW("Configure DVS failed %d", err);
        return UNKNOWN_ERROR;
    }
    LOG2("Configure DVS success");

    struct atomisp_sensor_mode_data sensor_mode_data;
    if(mSensorCI->getModeInfo(&sensor_mode_data) < 0) {
        sensor_mode_data.crop_horizontal_start = 0;
        sensor_mode_data.crop_vertical_start = 0;
        sensor_mode_data.crop_vertical_end = 0;
        sensor_mode_data.crop_horizontal_end = 0;
    }
    if (sensor_mode_data.binning_factor_y != 0 && sensor_mode_data.output_height != 0
       && sensor_mode_data.frame_length_lines != 0) {
        float downscaling = (sensor_mode_data.crop_vertical_end - sensor_mode_data.crop_vertical_start + 1)
                            / sensor_mode_data.binning_factor_y / sensor_mode_data.output_height;
        float non_blanking_ratio = (float)(mDvs2Config.output_bq.height_bq * 2 + (mDvs2Config.envelope_bq.height_bq + mDvs2Config.ispfilter_bq.height_bq) * 4 * downscaling)/sensor_mode_data.frame_length_lines;
        dvs_set_non_blank_ratio(mState, non_blanking_ratio);
    }
    //Allocate statistics
    if (mDVSEnabled) {
        status = allocateDvs2Statistics(dvs_grid);
        if (status != NO_ERROR) {
            LOGW("Allocate dvs statistics failed");
            return UNKNOWN_ERROR;
        }
    }
    status = allocateDvs2MorphTable();
    if(!mMorphTable || status != NO_ERROR) {
        LOGW("Allocate dvs morph table failed");
        return UNKNOWN_ERROR;
    }

    //Set coefficient
    if (dvs_grid.enable) {
        atomisp_dis_coefficients *dvs_coefs = NULL;
        err = dvs_allocate_coefficients(&dvs_grid, &dvs_coefs);
        if (err != ia_err_none) {
            LOGW("allocate dvs2 coeff failed:%d", err);
            return UNKNOWN_ERROR;
        }

        err = dvs_get_coefficients(mState, dvs_coefs);
        if (err != ia_err_none) {
            LOGW("get dvs2 coeff failed: %d", err);
            return UNKNOWN_ERROR;
        } else {
            mIsp->setDvsCoefficients(dvs_coefs);
        }
        if (dvs_coefs)
            dvs_free_coefficients(dvs_coefs);
        dvs_coefs = NULL;
    }

    return status;
}

struct atomisp_dvs_6axis_config *IaDvs2::getMorphTable()
{
    Mutex::Autolock lock(mLock);
    ia_err err = ia_err_none;

    if(mMorphTable) {
        err = dvs_get_morph_table(mState, mMorphTable);
        if (err != ia_err_none) {
            LOGE("%s, Failed to get MorphTable", __FUNCTION__);
            return NULL;
        }
    }
    return mMorphTable;
}

status_t IaDvs2::run()
{
    Mutex::Autolock lock(mLock);
    if(!mZoomRatioChanged && !mDVSEnabled)
        return NO_ERROR;

    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    ia_err err = ia_err_none;
    bool try_again = false;
    if (!mState)
        goto end;

    if (mDVSEnabled) {
        if (!mDvs2stats)
            goto end;
        status = mIsp->getDvsStatistics(&mStatistics, &try_again);
        if (status != NO_ERROR) {
            LOGW("%s : Failed to get DVS statistics", __FUNCTION__);
            goto end;
        }
        /* When the driver tells us to try again, it means the grid
           has changed. Because of this, we reconfigure the DVS engine
           which will use the updated grid information. */
        if (try_again) {
            reconfigureNoLock();
            status = mIsp->getDvsStatistics(&mStatistics, NULL);
            if (status != NO_ERROR) {
                LOGW("%s : Failed to get DVS statistics (again)", __FUNCTION__);
                goto end;
            }
        }
        memcpy(mDvs2stats, &(mStatistics.dvs2_stat), sizeof(struct atomisp_dvs2_statistics));
        err = dvs_set_statistics(mState, mDvs2stats);
        if (err != ia_err_none)
            LOGW(" dvs_set_statistics failed: %d", err);

        if ((err = dvs_execute(mState)) != ia_err_none) {
            LOG2("DVS2 execution failed: %d", err);
            goto end;
        }
        mMorphTable->exp_id = mStatistics.exp_id;
        LOG2("exp_id:%d", mMorphTable->exp_id);
    }
    mIsp->setDvsConfigChanged(true);
    mZoomRatioChanged = false;

end:
    return status;
}

/**
 * If the input resolution is supported in high speed mode, return true.
 */
bool IaDvs2::isHighSpeedDvsSupported(int width, int height)
{
    LOG1("@%s", __FUNCTION__);
    const char* resolution =  mCapInfo->maxHighSpeedDvsResolution();
    if (resolution != NULL) {
        int w = 0;
        int h = 0;
        int retval = parsePair(resolution, &w, &h, 'x');
        if (retval)
            return false;

        LOG1 ("max Dvs resolution = [%dX%d], current=[%dx%d] in high speed mode", w, h, width, height);
        if (w >= width && h >= height) {
            return true;
        }
    }
    return false;
}

bool IaDvs2::isDvsValid()
{
    LOG1("@%s", __FUNCTION__);
    int width = 0, height = 0;

    mIsp->getVideoSize(&width, &height);
    if (width == 0 || height == 0)
        return false;

    int sensorFps = floor(mSensorCI->getFrameRate());
    if (sensorFps > DEFAULT_RECORDING_FPS && !isHighSpeedDvsSupported(width, height)) {
        // Workaround 1: disable DVS functionality in hi-speed recording due to performance issue
        LOGW("%s:DVS cannot be set when HighSpeed Capture and the selected resolution", __FUNCTION__);
        return false;
    }

    // Workaround 2: disable DVS functionality when the recording resolution is less than VGA,
    //               because current grid size is 64, the minimum resolution for grid size 64 is
    //               384 x 384, so FW should provide grid size 32 in low resolution for DVS functionality.
    // Exception: support DVS for 360P in SAND project
    if (width * height < 640 * 360) {
        LOGW("%s:DVS cannot be set when the selected resolution is less than 360P", __FUNCTION__);
        return false;
    }
    // Workaround 3: disable DVS functionality when the recording resolution is not less than 720P for sand
    //             (BZ: 194611)
    if (width * height >= RESOLUTION_720P_WIDTH * RESOLUTION_720P_HEIGHT) {
        LOGW("%s:DVS disabled when the selected resolution is not less than 720P", __FUNCTION__);
        return false;
    }


    //ToDo : need skip the first 2 frames because the first 2 DVS stats wrong
    return true;
}

/**
 * override for IAtomIspObserver::atomIspNotify()
 *
 * IaDvs2 gets attached to receive preview stream here.
 */
bool IaDvs2::notifyIspEvent(ICssIspListener::IspMessage *msg)
{
    if (msg && msg->id == ICssIspListener::ISP_MESSAGE_ID_EVENT) {
        if (msg->data.event.type == ICssIspListener::ISP_EVENT_TYPE_FRAME)
            run();
    }

    return false;
}

status_t IaDvs2::setZoom(float zoomRatio)
{
    Mutex::Autolock lock(mLock);
    LOG1("@%s zoom:%4.2f", __FUNCTION__, zoomRatio);
    ia_err err = ia_err_none;
    err = dvs_set_digital_zoom_magnitude(mState, zoomRatio);
    if (err != ia_err_none)
        return UNKNOWN_ERROR;
    mZoomRatioChanged = true;

    return NO_ERROR;
}

void IaDvs2::writeBinaryDump(const char *binary_dump_file)
{
     FILE *fp;
     if (!binary_dump_file)
         return;
     fp = fopen(binary_dump_file, "wb");
     if (fp) {
         if (dvs_fwrite_dbglog(fp)) {
             LOG2("ia_dvs2_fwrite_dbglog: failed to fwrite into [%s]\n", binary_dump_file);
         } else {
             LOG2("ia_dvs2_fwrite_dbglog: [%s] has been created.\n", binary_dump_file);
         }
         fclose(fp);
     }
}

void IaDvs2::debugPrint(const char *fmt, va_list ap)
{
     if (!mDumpLogEnabled)
         return;
     char string[LOG_LEN] = {0};
     char* str0 = string;
     vsnprintf(str0, LOG_LEN - 1, fmt, ap);
     LOG2("%s",str0);
}

} // namespace camera2
} //  namespace android

