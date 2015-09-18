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

#ifndef _CAMERA3_HAL_IA_DVS2_H
#define _CAMERA3_HAL_IA_DVS2_H

#include <utils/Errors.h>
#include "ICameraHwControls.h"
#include "ipu2/IPU2HwIsp.h"
#include "IPU2CameraCapInfo.h"

extern "C" {
#include <stdlib.h>
#include <linux/atomisp.h>
#include <ia_dvs_2.h>
}

namespace android {
namespace camera2 {

class IaDvs2 : public ICssIspListener {

public:
    const static float maxDvs2YUVDSRatio = 1.3f;

    IaDvs2(HWControlGroup &hwcg);
    ~IaDvs2();

    status_t dvsInit();
    status_t reconfigure();
    bool isDvsValid();
    status_t setZoom(float zoom);
    struct atomisp_dvs_6axis_config *getMorphTable();

    /**********************************************************************
     * ICssIspListener override
     */
    bool notifyIspEvent(ICssIspListener::IspMessage *msg);

// prevent copy constructor and assignment operator
private:
    IaDvs2(const IaDvs2& other);
    IaDvs2& operator=(const IaDvs2& other);

private:
    status_t reconfigureNoLock();
    status_t run();
    status_t allocateDvs2Statistics(atomisp_dvs_grid_info info);
    status_t allocateDvs2MorphTable();
    void writeBinaryDump(const char *binary_dump_file);
    static void debugPrint(const char *fmt, va_list ap);
    bool isHighSpeedDvsSupported(int width, int height);

private:
    IHWIspControl *mIsp;
    IHWSensorControl *mSensorCI;
    Mutex mLock;
    const IPU2CameraCapInfo * mCapInfo;
    ia_dvs2_binary_dump_params mDumpParams;
    static bool mDumpLogEnabled;
    ia_env mDvs2Env;
    struct atomisp_dis_statistics mStatistics;
    struct atomisp_dvs2_statistics *mDvs2stats;
    ia_dvs2_state *mState;
    ia_dvs2_configuration mDvs2Config;
    atomisp_dvs_6axis_config *mMorphTable;
    bool mDVSEnabled;
    bool mZoomRatioChanged;
};

} /* namespace camera2 */
} /* namespace android */

#endif /* _CAMERA3_HAL_IA_DVS2_H */
