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

#ifndef CAMERA3_HAL_PSYSPIPELINE_H_
#define CAMERA3_HAL_PSYSPIPELINE_H_

extern "C" {

// put libiacss into a namespace so that its types won't clash with libmfldadvci types
namespace libiacss {

// Pipeline framework (CIPF)
#include <ia_cipf/ia_cipf_pipe.h>
#include <ia_cipf/ia_cipf_stage.h>
#include <ia_cipf/ia_cipf_buffer.h>
#include <ia_cipf/ia_cipf_property.h>
#include <ia_cipf/ia_cipf_terminal.h>
#include <ia_cipf/ia_cipf_iterator.h>

// CIPF backends
#include <ia_cipf_css/ia_cipf_css.h>
#include <ia_cipf_common/ia_cipf_common.h>

// CIPR alloc interface
#include <ia_cipf_css/ia_cipf_css_uids.h>
#include <ia_cipr/ia_cipr_alloc.h>

// Pipeline builder
#include <ia_camera/ia_cipb.h>
#include <ia_camera/psys_vfpp_control.h>
#include <ia_camera/psys_preview_control.h>
#include <ia_camera/psys_capture_control.h>

}
}
using namespace libiacss;

#include "utils/RefBase.h"
#include <utils/String8.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>

#include "CameraBuffer.h"
#include "DebugFrameRate.h"

namespace android {
namespace camera2 {

class PSysPipeline : public RefBase{
public:
    PSysPipeline(ia_cipf_frame_format_t input,
                 ia_cipf_frame_format_t output,
                 String8 uri);
    PSysPipeline(ia_cipf_frame_format_t input,
                 ia_cipf_frame_format_t inter,
                 ia_cipf_frame_format_t output,
                 String8 uri);
    PSysPipeline(ia_cipf_frame_format_t input,
                 ia_cipf_frame_format_t inter,
                 ia_cipf_frame_format_t output,
                 ia_cipf_frame_format_t sec_output,
                 String8 uri);
    ~PSysPipeline();
    int width() {return mOutputFormatPipe->width;}  /*!< Pipeline Output Width, use same function as CameraBuffer to use in macros */
    int height() {return mOutputFormatPipe->height;}  /*!< Pipeline Output Height use same function as CameraBuffer to use in macros*/
    String8 pipelineName() {return mGraphName;}
    status_t createPipeline(void);
    status_t registerBuffer(sp<CameraBuffer> input_buffer, sp<CameraBuffer> output_buffer);
    status_t iterate(sp<CameraBuffer> input_buffer, sp<CameraBuffer> output_buffer);
    status_t destroyPipeline(void);

private: /* Methods */
    status_t handle_buffer_requirement(ia_cipf_buffer_t *req_buffer);
    status_t allocateInputFrameBuffer(ia_cipf_buffer_t *req_buffer);
    status_t allocateInterFrameBuffer(ia_cipf_buffer_t *req_buffer);
    status_t allocateOutputFrameBuffer(ia_cipf_buffer_t *req_buffer);
    status_t allocateParamBuffer(ia_cipf_buffer_t *req_buffer);
    status_t allocateSecOutputFrameBuffer(ia_cipf_buffer_t *req_buffer);
    size_t ia_cipr_pagesize();

private: /* Members */
    ia_cipf_frame_format_t mInputFormatInput, mInterFormatInput, mOutputFormatInput, mSecOutputFormatInput;
    String8 mGraphName;
    ia_cipf_pipe_t *mPipe;
    const ia_cipf_frame_format_t *mInputFormatPipe, *mOutputFormatPipe, *mInterFormatPipe, *mSecOutputFormatPipe;
    ia_cipf_iterator_t *mPipeIterator;
    ia_cipf_buffer_t *mPipe_Input_buffer;
    ia_cipf_buffer_t *mPipe_Inter_buffer;
    ia_cipf_buffer_t *mPipe_Output_buffer;
    ia_cipf_buffer_t *mPipe_Param_buffer;
    ia_cipf_buffer_t *mPipe_SecOutput_buffer;
    KeyedVector<void *, ia_cipf_buffer_t *> mRegisteredIAInputBuffs;
    KeyedVector<void *, ia_cipf_buffer_t *> mRegisteredIAOutputBuffs;
    KeyedVector<ia_uid, ia_cipf_buffer_t *> mParamBuffs;
    sp<DebugFrameRate>  mDebugFPS;
};  // class PSysPipeline

}  // namespace camera2
}  // namespace android

#endif  // CAMERA3_HAL_PSYSPIPELINE_H_
