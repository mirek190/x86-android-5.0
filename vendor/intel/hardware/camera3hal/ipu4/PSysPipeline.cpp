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

#define LOG_TAG "Camera_PSysPipeline"

#include "PSysPipeline.h"
#include "LogHelper.h"
#include "UtilityMacros.h"
#include "Camera3GFXFormat.h"

namespace android {
namespace camera2 {

PSysPipeline::PSysPipeline(ia_cipf_frame_format_t input,
                           ia_cipf_frame_format_t output,
                           String8 uri):
    mInputFormatInput(input)
    ,mOutputFormatInput(output)
    ,mGraphName(uri)
    ,mPipe(NULL)
    ,mInputFormatPipe(NULL)
    ,mOutputFormatPipe(NULL)
    ,mSecOutputFormatPipe(NULL)
    ,mPipeIterator(NULL)
    ,mPipe_Input_buffer(NULL)
    ,mPipe_Inter_buffer(NULL)
    ,mPipe_Output_buffer(NULL)
    ,mPipe_Param_buffer(NULL)
    ,mPipe_SecOutput_buffer(NULL)
    ,mDebugFPS(NULL)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mInterFormatInput.width = 0;
    mInterFormatInput.height = 0;
    mInterFormatInput.fourcc = 0;
    mInterFormatInput.bpl = 0;
    mInterFormatInput.bpp = 0;

    mSecOutputFormatInput.width = 0;
    mSecOutputFormatInput.height = 0;
    mSecOutputFormatInput.fourcc = 0;
    mSecOutputFormatInput.bpl = 0;
    mSecOutputFormatInput.bpp = 0;
}


PSysPipeline::PSysPipeline(ia_cipf_frame_format_t input,
                           ia_cipf_frame_format_t inter,
                           ia_cipf_frame_format_t output,
                           String8 uri):
    mInputFormatInput(input)
    ,mInterFormatInput(inter)
    ,mOutputFormatInput(output)
    ,mGraphName(uri)
    ,mPipe(NULL)
    ,mInputFormatPipe(NULL)
    ,mOutputFormatPipe(NULL)
    ,mSecOutputFormatPipe(NULL)
    ,mPipeIterator(NULL)
    ,mPipe_Input_buffer(NULL)
    ,mPipe_Inter_buffer(NULL)
    ,mPipe_Output_buffer(NULL)
    ,mPipe_Param_buffer(NULL)
    ,mPipe_SecOutput_buffer(NULL)
    ,mDebugFPS(NULL)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    mSecOutputFormatInput.width = 0;
    mSecOutputFormatInput.height = 0;
    mSecOutputFormatInput.fourcc = 0;
    mSecOutputFormatInput.bpl = 0;
    mSecOutputFormatInput.bpp = 0;
}

PSysPipeline::PSysPipeline(ia_cipf_frame_format_t input,
                           ia_cipf_frame_format_t inter,
                           ia_cipf_frame_format_t output,
                           ia_cipf_frame_format_t sec_output,
                           String8 uri):
    mInputFormatInput(input)
    ,mInterFormatInput(inter)
    ,mOutputFormatInput(output)
    ,mSecOutputFormatInput(sec_output)
    ,mGraphName(uri)
    ,mPipe(NULL)
    ,mInputFormatPipe(NULL)
    ,mOutputFormatPipe(NULL)
    ,mSecOutputFormatPipe(NULL)
    ,mPipeIterator(NULL)
    ,mPipe_Input_buffer(NULL)
    ,mPipe_Inter_buffer(NULL)
    ,mPipe_Output_buffer(NULL)
    ,mPipe_Param_buffer(NULL)
    ,mPipe_SecOutput_buffer(NULL)
    ,mDebugFPS(NULL)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
}

PSysPipeline::~PSysPipeline()
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    ia_cipf_buffer_t *ia_buffer;

    if (mPipe_Input_buffer != NULL) {
        ia_cipf_pipe_unregister_buffer(mPipe, mPipe_Input_buffer);
        ia_cipf_buffer_destroy(mPipe_Input_buffer);
    }

    if (mPipe_Inter_buffer != NULL) {
        IA_CIPR_FREE(mPipe_Inter_buffer->payload.data.cpu_ptr);
        ia_cipf_pipe_unregister_buffer(mPipe, mPipe_Inter_buffer);
        ia_cipf_buffer_destroy(mPipe_Inter_buffer);

    }

    if (mPipe_Output_buffer != NULL) {
        IA_CIPR_FREE(mPipe_Output_buffer->payload.data.cpu_ptr);
        ia_cipf_pipe_unregister_buffer(mPipe, mPipe_Output_buffer);
        ia_cipf_buffer_destroy(mPipe_Output_buffer);

    }

    if (mPipe_SecOutput_buffer != NULL) {
        IA_CIPR_FREE(mPipe_SecOutput_buffer->payload.data.cpu_ptr);
        ia_cipf_pipe_unregister_buffer(mPipe, mPipe_SecOutput_buffer);
        ia_cipf_buffer_destroy(mPipe_SecOutput_buffer);
    }

    if (!mParamBuffs.isEmpty()) {
        while (mParamBuffs.size() > 0) {
            ia_buffer = mParamBuffs.editValueAt(0);
            IA_CIPR_FREE(ia_buffer->payload.data.cpu_ptr);
            ia_cipf_pipe_unregister_buffer(mPipe, ia_buffer);
            ia_cipf_buffer_destroy(ia_buffer);
            mParamBuffs.removeItemsAt(0);
        }
    }

    if (!mRegisteredIAInputBuffs.isEmpty()) {
        while (mRegisteredIAInputBuffs.size() > 0) {
            ia_buffer = mRegisteredIAInputBuffs.editValueAt(0);
            /* memory of input is not ours */
            ia_cipf_pipe_unregister_buffer(mPipe, ia_buffer);
            ia_cipf_buffer_destroy(ia_buffer);
            mRegisteredIAInputBuffs.removeItemsAt(0);
        }
    }
    if (!mRegisteredIAOutputBuffs.isEmpty()) {
        while (mRegisteredIAOutputBuffs.size() > 0) {
            ia_buffer = mRegisteredIAOutputBuffs.editValueAt(0);
            /* memory of input is not ours */
            ia_cipf_pipe_unregister_buffer(mPipe, ia_buffer);
            ia_cipf_buffer_destroy(ia_buffer);
            mRegisteredIAOutputBuffs.removeItemsAt(0);
        }
    }

    destroyPipeline();

    if (mDebugFPS != NULL) {
        mDebugFPS->requestExitAndWait();
        mDebugFPS.clear();
    }
}

size_t PSysPipeline::ia_cipr_pagesize()
{
    return (size_t) sysconf(_SC_PAGESIZE);
}

status_t
PSysPipeline::createPipeline(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL1);
    status_t status = NO_ERROR;
    ia_cipb_t ispPipelineBuilder;
    ia_cipb_graph_t graph;
    ia_cipf_terminal_t *terminal;
    ia_cipf_property_t *property;
    ia_cipf_buffer_t *requiredBuffer;
    ia_uid uid = 0;
    uint32_t size;

    LOG1("Creating %s pipeline", mGraphName.string());
    // Create builder
    ispPipelineBuilder = ia_cipb_create();
    if (ispPipelineBuilder == NULL) {
        LOGE("@%s: Error building pipeline", __FUNCTION__);
        return NO_INIT;
    }
    graph = ia_cipb_get_native_graph(ispPipelineBuilder, mGraphName);
    if (graph == NULL) {
        LOGE("@%s: Error getting native graph", __FUNCTION__);
        return NO_INIT;
    }
    mPipe = ia_cipb_create_pipeline(ispPipelineBuilder, graph);
    if (mPipe == NULL) {
        LOGE("@%s: Error creating pipeline", __FUNCTION__);
        return NO_INIT;
    }
    // Destroy builder
    ia_err iaErr = ia_cipb_destroy(ispPipelineBuilder);
    if (iaErr != ia_err_none) {
        LOGE("@%s: Error destroying pipeline builder", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    iaErr = ia_cipf_pipe_load(mPipe);
    if (iaErr != ia_err_none) {
        LOGE("@%s: Error loading pipeline", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    /* Configure terminal of "External Source" stage,
     * this is our input of the pipeline */
    terminal = ia_cipf_pipe_get_terminal_by_uid(mPipe,
                                                ia_cipf_external_source_terminal_uid);
    if (terminal == NULL) {
        LOGE("@%s: Error getting input terminal", __FUNCTION__);
        return NO_INIT;
    }
    iaErr = ia_cipf_terminal_set_format(terminal, &mInputFormatInput);
    if (iaErr != ia_err_none) {
        LOGE("@%s: Error setting format of terminal", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    /* Get the established terminal format for later reference
     * Note: Terminal itself cannot be used as linkage between format and
     *       buffer. Due to format propagation between the connected terminals
     *       the terminal that requests memory for the given format may not be
     *       the same. By getting the format back we receive a handle we
     *       can match when allocating. */
    mInputFormatPipe = ia_cipf_terminal_get_format_ref(terminal);

    LOG1("%s: pipeline input configured to %dx%d(%s)@%dbpp\n", __FUNCTION__,
         mInputFormatPipe->width,   mInputFormatPipe->height,
         v4l2Fmt2Str(mInputFormatPipe->fourcc),
         mInputFormatPipe->bpp);

    /* Configure intermediate format
     * NOTE: now need to travel to the correct stage terminal to set it up,
     *       pipeline should either expose it explicitly with UID or then
     *       make it fully transparent to user */
    if (mInterFormatInput.width != 0 && mInterFormatInput.height != 0) {
        terminal = ia_cipf_terminal_get_remote(terminal);
        if (terminal == NULL) {
            LOGE("@%s: Error getting terminal", __FUNCTION__);
            return NO_INIT;
        }
        ia_cipf_stage_t *stage = ia_cipf_terminal_get_stage(terminal);
        if (stage == NULL) {
            LOGE("@%s: Error getting stage", __FUNCTION__);
            return NO_INIT;
        }
        // The intermediate terminal is the output of the first psys stage
        terminal = ia_cipf_stage_get_output_terminal(stage, 0);
        if (terminal == NULL) {
            LOGE("@%s: Error getting intermediate terminal", __FUNCTION__);
            return NO_INIT;
        }

        ia_cipf_terminal_set_format(terminal, &mInterFormatInput);
        mInterFormatPipe = ia_cipf_terminal_get_format_ref(terminal);

        LOG1("%s: pipeline intermediate configured to %dx%d(%s)@%dbpp\n", __FUNCTION__,
             mInterFormatPipe->width, mInterFormatPipe->height,
             v4l2Fmt2Str(mInterFormatPipe->fourcc),
             mInterFormatPipe->bpp);
    }

    /* Configure terminal of "External Sink" stage
     * This is the output of the pipeline */
    terminal = ia_cipf_pipe_get_terminal_by_uid(mPipe,
                                                ia_cipf_external_sink_terminal_uid);
    if (terminal == NULL) {
        LOGE("@%s: Error getting output terminal", __FUNCTION__);
        return NO_INIT;
    }
    iaErr = ia_cipf_terminal_set_format(terminal, &mOutputFormatInput);
    if (iaErr != ia_err_none) {
        LOGE("@%s: Error setting format of terminal", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    // Get format gives us a handle that can be used to match terminal later
    // when allocating.
    mOutputFormatPipe = ia_cipf_terminal_get_format_ref(terminal);

    LOG1("%s: pipeline output configured to %dx%d(%s)@%dbpp\n", __FUNCTION__,
         mOutputFormatPipe->width, mOutputFormatPipe->height,
         v4l2Fmt2Str(mOutputFormatPipe->fourcc),
         mOutputFormatPipe->bpp);

    /* Configure Secondary Output format */
    if (mSecOutputFormatInput.width != 0 && mSecOutputFormatInput.height != 0) {
        terminal = ia_cipf_pipe_get_terminal_by_uid(mPipe,
                                                    ia_cipf_external_source_terminal_uid);
        if (terminal == NULL) {
            LOGE("@%s: Error getting input terminal", __FUNCTION__);
            return NO_INIT;
        }
        terminal = ia_cipf_terminal_get_remote(terminal);
        if (terminal == NULL) {
            LOGE("@%s: Error getting terminal", __FUNCTION__);
            return NO_INIT;
        }
        ia_cipf_stage_t *stage = ia_cipf_terminal_get_stage(terminal);
        if (stage == NULL) {
            LOGE("@%s: Error getting stage", __FUNCTION__);
            return NO_INIT;
        }
        // The secondary terminal is the output of the second psys stage
        terminal = ia_cipf_stage_get_output_terminal(stage, 1);
        if (terminal == NULL) {
            LOGE("@%s: Error getting secondary terminal", __FUNCTION__);
            return NO_INIT;
        }

        iaErr = ia_cipf_terminal_set_format(terminal, &mSecOutputFormatInput);
        if (iaErr != ia_err_none) {
            LOGE("@%s: Error setting format of terminal", __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        // Get format gives us a handle that can be used to match terminal later
        // when allocating.
        mSecOutputFormatPipe = ia_cipf_terminal_get_format_ref(terminal);

        LOG1("%s: pipeline secondary output configured to %dx%d(%s)@%dbpp\n", __FUNCTION__,
             mSecOutputFormatPipe->width, mSecOutputFormatPipe->height,
             v4l2Fmt2Str(mSecOutputFormatPipe->fourcc),
             mSecOutputFormatPipe->bpp);
    }

    /* Identify properties */
    iaErr = ia_cipf_pipe_next_unidentified_property(mPipe, &property);
    while ((iaErr == ia_err_none) && property) {
        uid = ia_cipf_property_get_uid(property);
        LOG2("%s: property to identify %s\n", __FUNCTION__, v4l2Fmt2Str(uid));

        /* PSYS library does the identifying and calculates the size for us */
        size = psys_library_control_sizeof_property(property);
        if (size == 0) {
            LOGE("%s: identifyin property %s failed",
                 __FUNCTION__, v4l2Fmt2Str(uid));
            return UNKNOWN_ERROR;
        }
        LOG2("%s: property indentified as %s, size %u\n", __FUNCTION__,
             v4l2Fmt2Str(uid), size);

        iaErr = ia_cipf_property_set_payload_size(property, size);
        if (iaErr != ia_err_none) {
            LOGE("@%s: Error setting property payload size", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
        ia_cipf_pipe_set_property(mPipe, property);
        ia_cipf_property_destroy(property);
        iaErr = ia_cipf_pipe_next_unidentified_property(mPipe, &property);
    }

    /* Allocate & register buffers */
    iaErr = ia_cipf_pipe_next_buffer_requirement(mPipe, &requiredBuffer);
    while ((iaErr == ia_err_none) && requiredBuffer) {
        handle_buffer_requirement(requiredBuffer);
        /* we made copies out from request */
        ia_cipf_buffer_destroy(requiredBuffer);
        iaErr = ia_cipf_pipe_next_buffer_requirement(mPipe, &requiredBuffer);
    }
    if (iaErr != ia_err_none) {
        LOGE("@%s: Failed to allocate buffers to Xos", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    mPipeIterator = ia_cipf_iterator_create(mPipe);
    if (mPipeIterator == NULL) {
        LOGE("@%s: Error getting pipeline iterator", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (mPipeIterator == NULL) {
        LOGE("@%s: error creating pipeline", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    mDebugFPS = new DebugFrameRate(mGraphName.string());
    mDebugFPS->run();

    return status;
}

status_t
PSysPipeline::allocateInputFrameBuffer(ia_cipf_buffer_t *req_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    ia_err_t ret;
    ia_cipf_frame_format_t format;
    ia_cipf_frame_t *new_frame;
    uint32_t allocate_size = 0;

    /* we identified this buffer as our input frame buffer
     * so it must be associated to frame format. */
    ret = ia_cipf_buffer_get_frame_format(req_buffer, &format);
    if (ret != ia_err_none) {
        LOGE("@%s: Error getting frame format", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    /* format here should reflect what we configured
     * stage realization is careless to calculate the size for us
     */
    if (format.fourcc == css_fourcc_raw)
        allocate_size = format.height * format.bpl;
    else
        allocate_size = ALIGN8(format.width * format.height * format.bpp) / 8U;

    /* make a copy of request */
    mPipe_Input_buffer = ia_cipf_buffer_create_copy(req_buffer);
    if (mPipe_Input_buffer == NULL) {
        LOGE("@%s: Error creating buffer copy", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame = ia_cipf_buffer_get_frame_ref(mPipe_Input_buffer);
    if (new_frame == NULL) {
        LOGE("@%s: Error getting frame for buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame->id = 0;
    new_frame->uid = format.fourcc;
    new_frame->allocated = allocate_size;
    new_frame->planes = 1;
    new_frame->payload[0].size = allocate_size;

    /* Register buffer */
    ret = ia_cipf_pipe_register_buffer(mPipe, mPipe_Input_buffer);
    if (ret != ia_err_none) {
        LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    LOG2("%s: allocated preview input frame buffer %dx%d(%s)@%dbpp\n",
              __FUNCTION__, format.width, format.height,
              v4l2Fmt2Str(format.fourcc), format.bpp);

    return status;
}

status_t
PSysPipeline::allocateInterFrameBuffer(ia_cipf_buffer_t *req_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    ia_err_t ret;
    ia_cipf_frame_format_t format;
    ia_cipf_frame_t *new_frame;
    uint32_t allocate_size = 0;

    ret = ia_cipf_buffer_get_frame_format(req_buffer, &format);
    if (ret != ia_err_none) {
        LOGE("@%s: Error getting frame format", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (format.fourcc == css_fourcc_raw)
        allocate_size = format.height * format.bpl;
    else
        allocate_size = ALIGN8(format.width * format.height * format.bpp) / 8U;

    //allocate and register one buffer
     /* make a copy of request */
    mPipe_Inter_buffer = ia_cipf_buffer_create_copy(req_buffer);
    if (mPipe_Inter_buffer == NULL) {
        LOGE("@%s: Error creating buffer copy", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame = ia_cipf_buffer_get_frame_ref(mPipe_Inter_buffer);
    if (new_frame == NULL) {
        LOGE("@%s: Error getting frame for buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame->payload[0].data.cpu_ptr = IA_CIPR_ALLOC_ALIGNED(PAGE_ALIGN(allocate_size),
                                                               IA_CIPR_PAGESIZE());
    if (new_frame->payload[0].data.cpu_ptr == NULL) {
        LOGE("@%s: Error allocating buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    memset(new_frame->payload[0].data.cpu_ptr, 0, PAGE_ALIGN(allocate_size));


    new_frame->id = 0;
    new_frame->uid = format.fourcc;
    new_frame->allocated = allocate_size;
    new_frame->planes = 1;
    new_frame->payload[0].size = allocate_size;

    /* Register buffer */
    ret = ia_cipf_pipe_register_buffer(mPipe, mPipe_Inter_buffer);
    if (ret != ia_err_none) {
        LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    LOG2("%s: allocated inter frame buffer %dx%d(%s)@%dbpp\n",
              __FUNCTION__, format.width, format.height,
              v4l2Fmt2Str(format.fourcc), format.bpp);

    return status;
}

status_t
PSysPipeline::allocateSecOutputFrameBuffer(ia_cipf_buffer_t *req_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    ia_err_t ret;
    ia_cipf_frame_format_t format;
    ia_cipf_frame_t *new_frame;
    uint32_t allocate_size = 0;

    ret = ia_cipf_buffer_get_frame_format(req_buffer, &format);
    if (ret != ia_err_none) {
        LOGE("@%s: Error getting frame format", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (format.fourcc == css_fourcc_raw)
        allocate_size = format.height * format.bpl;
    else
        allocate_size = ALIGN8(format.width * format.height * format.bpp) / 8U;

    //allocate and register one buffer
     /* make a copy of request */
    mPipe_SecOutput_buffer = ia_cipf_buffer_create_copy(req_buffer);
    if (mPipe_SecOutput_buffer == NULL) {
        LOGE("@%s: Error creating buffer copy", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame = ia_cipf_buffer_get_frame_ref(mPipe_SecOutput_buffer);
    if (new_frame == NULL) {
        LOGE("@%s: Error getting frame for buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame->payload[0].data.cpu_ptr = IA_CIPR_ALLOC_ALIGNED(PAGE_ALIGN(allocate_size),
                                                               IA_CIPR_PAGESIZE());
    if (new_frame->payload[0].data.cpu_ptr == NULL) {
        LOGE("@%s: Error allocating buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    memset(new_frame->payload[0].data.cpu_ptr, 0, PAGE_ALIGN(allocate_size));


    new_frame->id = 0;
    new_frame->uid = format.fourcc;
    new_frame->allocated = allocate_size;
    new_frame->planes = 1;
    new_frame->payload[0].size = allocate_size;

    /* Register buffer */
    ret = ia_cipf_pipe_register_buffer(mPipe, mPipe_SecOutput_buffer);
    if (ret != ia_err_none) {
        LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    LOG2("%s: allocated Secondary Ouptut frame buffer %dx%d(%s)@%dbpp\n",
              __FUNCTION__, format.width, format.height,
              v4l2Fmt2Str(format.fourcc), format.bpp);

    return status;
}

status_t
PSysPipeline::allocateOutputFrameBuffer(ia_cipf_buffer_t *req_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;
    ia_err_t ret;
    ia_cipf_frame_format_t format;
    ia_cipf_frame_t *new_frame;
    uint32_t allocate_size = 0;

    ret = ia_cipf_buffer_get_frame_format(req_buffer, &format);
    if (ret != ia_err_none) {
        LOGE("@%s: Error getting frame format", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    if (format.fourcc == css_fourcc_raw)
        allocate_size = format.height * format.bpl;
    else
        allocate_size = ALIGN8(format.width * format.height * format.bpp) / 8U;

    //allocate and register one buffer
     /* make a copy of request */
    mPipe_Output_buffer = ia_cipf_buffer_create_copy(req_buffer);
    if (mPipe_Output_buffer == NULL) {
        LOGE("@%s: Error creating buffer copy", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    new_frame = ia_cipf_buffer_get_frame_ref(mPipe_Output_buffer);
    if (new_frame == NULL) {
        LOGE("@%s: Error getting frame for buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    // TODO: Replace below with late registration of buffer to use gralloc buffer from framewwork
    new_frame->payload[0].data.cpu_ptr = IA_CIPR_ALLOC_ALIGNED(PAGE_ALIGN(allocate_size),
                                                               IA_CIPR_PAGESIZE());
    if (new_frame->payload[0].data.cpu_ptr == NULL) {
        LOGE("@%s: Error allocating buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }
    memset(new_frame->payload[0].data.cpu_ptr, 0, PAGE_ALIGN(allocate_size));


    new_frame->id = 0;
    new_frame->uid = format.fourcc;
    new_frame->allocated = allocate_size;
    new_frame->planes = 1;
    new_frame->payload[0].size = allocate_size;

    /* Register buffer */
    ret = ia_cipf_pipe_register_buffer(mPipe, mPipe_Output_buffer);
    if (ret != ia_err_none) {
        LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    return status;
}

status_t
PSysPipeline::allocateParamBuffer(ia_cipf_buffer_t *req_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    ia_cipf_buffer_t *paramBuffer;
    ia_err_t ret;
    int index;

    if (!req_buffer)
        return BAD_VALUE;

    /* Right now we maintain one buffer for each param buffer requirement */
    index = mParamBuffs.indexOfKey(req_buffer->payload.uid);
    if (index != NAME_NOT_FOUND) {
        LOGE("@%s: Buffer for %s already allocated!", __FUNCTION__,
             v4l2Fmt2Str(req_buffer->payload.uid));
        return UNKNOWN_ERROR;
    }

    /* make a copy of request */
    paramBuffer = ia_cipf_buffer_create();
    if (paramBuffer == NULL) {
        LOGE("@%s: Error creating buffer copy", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    ret = ia_cipf_buffer_replicate_association(paramBuffer,
                                               req_buffer);
    if (ret != ia_err_none) {
        LOGE("@%s: Error replicate association", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    paramBuffer->payload.data.cpu_ptr =
            IA_CIPR_ALLOC_ALIGNED(PAGE_ALIGN(req_buffer->payload.size),
                                  IA_CIPR_PAGESIZE());
    if (paramBuffer->payload.data.cpu_ptr == NULL) {
        ia_cipf_buffer_destroy(paramBuffer);
        LOGE("@%s: Error allocating buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    paramBuffer->payload.size = PAGE_ALIGN(req_buffer->payload.size);
    /* We are dealing with only PSYS buffers currently.
     * Letting PSYS library to preset the buffer */
    ret = psys_library_control_preset_buffer(paramBuffer);
    if (ret != ia_err_none) {
        IA_CIPR_FREE(paramBuffer->payload.data.cpu_ptr);
        ia_cipf_buffer_destroy(paramBuffer);
        LOGE("@%s: Error presetting parameter buffer", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    /* Register buffer */
    ret = ia_cipf_pipe_register_buffer(mPipe, paramBuffer);
    if (ret != ia_err_none) {
        IA_CIPR_FREE(paramBuffer->payload.data.cpu_ptr);
        ia_cipf_buffer_destroy(paramBuffer);
        LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    mParamBuffs.add(req_buffer->payload.uid, paramBuffer);

    return NO_ERROR;
}

status_t
PSysPipeline::handle_buffer_requirement(ia_cipf_buffer_t *req_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    const ia_cipf_frame_format_t *terminal_format;
    ia_cipf_terminal_t* terminal;

    switch(req_buffer->payload.uid) {
    case ia_cipf_frame_uid:
        {
            /* we want to know for what terminal we are allocating */
            terminal = ia_cipf_buffer_get_terminal(req_buffer);
            if (!terminal) {
                /* received frame buffer requirement from non-terminal type
                 *
                 * this is possible if stage realization wants to use frame
                 * buffer definition to request client to allocate e.g
                 * intermediate frame buffers not associated with any terminal
                 * that could be connected.
                 * */
                LOGE("@%s: Received frame buffer requirement from non-terminal type",
                     __FUNCTION__);
                return UNKNOWN_ERROR;
            }
            /* we could compare the ia_cipf_frame.info.format directly, but as it
             * is a copy. We use the terminal accessor to get the actual one
             */
            terminal_format = ia_cipf_terminal_get_format_ref(terminal);
            if (terminal_format == mInputFormatPipe) {
                allocateInputFrameBuffer(req_buffer);
            } else if (terminal_format == mInterFormatPipe) {
                allocateInterFrameBuffer(req_buffer);
            } else if (terminal_format == mOutputFormatPipe) {
                allocateOutputFrameBuffer(req_buffer);
            } else if (terminal_format == mSecOutputFormatPipe) {
                allocateSecOutputFrameBuffer(req_buffer);
            } else {
                /* we have some unexpected terminal buffer request with the
                 * pipe we created
                 *  -> align code to support selected pipeline */
                LOGE("@%s: Unexpected terminal buffer request with pipe",
                     __FUNCTION__);
                return UNKNOWN_ERROR;
            }
        }
        break;
    default:
        allocateParamBuffer(req_buffer);
        break;
    }
    return NO_ERROR;
}

status_t
PSysPipeline::registerBuffer(sp<CameraBuffer> input_buffer, sp<CameraBuffer> output_buffer)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);

    status_t status = NO_ERROR;
    ia_cipf_frame_t *new_frame;
    ia_err_t ret;
    ia_cipf_buffer_t *newBuffer;
    ia_cipf_frame_format_t format;
    int index;


    if(input_buffer == NULL || output_buffer == NULL) {
        LOGE("@%s:Buffer to register not found",  __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    index = mRegisteredIAInputBuffs.indexOfKey(input_buffer->data());
    if (index == NAME_NOT_FOUND) {
        LOGE("No input buffer registered for %p, Registering ", input_buffer->data());

        // Register input buffer
        newBuffer = ia_cipf_buffer_create_copy(mPipe_Input_buffer);
        if (!newBuffer)
            return UNKNOWN_ERROR;

        new_frame = ia_cipf_buffer_get_frame_ref(newBuffer);
        if (new_frame == NULL) {
            LOGE("@%s: Error getting frame for buffer", __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        new_frame->payload[0].data.cpu_ptr = input_buffer->data();
        new_frame->payload[0].size = input_buffer->size();

        ret = ia_cipf_buffer_get_frame_format(newBuffer, &format);
        if (ret != ia_err_none) {
            LOGE("@%s: Error getting frame format", __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        LOG1("pipeline allocated input buffer = %p, for framework handle = %p",
             newBuffer, input_buffer->data());
        LOG1("pipeline allocated input buffer resolution = %d x %d, v4l2 format = %x",
             format.width, format.height, format.fourcc);
        LOG1("framework handle resolution = %d x %d, v4l2 format = %x",
             input_buffer->width(), input_buffer->height(), input_buffer->v4l2Fmt());
        /* Register buffer */
        ret = ia_cipf_pipe_register_buffer(mPipe, newBuffer);
        if (ret != ia_err_none) {
          LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
           return UNKNOWN_ERROR;
        }
        mRegisteredIAInputBuffs.add(input_buffer->data(), newBuffer);
        LOG1("Number of registered input buffers = %d", mRegisteredIAInputBuffs.size());
    }
    index = mRegisteredIAOutputBuffs.indexOfKey(output_buffer->data());
    if (index == NAME_NOT_FOUND) {
        LOGE("No output buffer registered for %p, Registering ", output_buffer->data());
        // Register output buffer
        newBuffer = ia_cipf_buffer_create_copy(mPipe_Output_buffer);
        if (!newBuffer)
            return UNKNOWN_ERROR;

        new_frame = ia_cipf_buffer_get_frame_ref(newBuffer);
        if (new_frame == NULL) {
            LOGE("@%s: Error getting frame for buffer", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
        new_frame->payload[0].data.cpu_ptr = output_buffer->data();
        new_frame->payload[0].size = output_buffer->size();


        ret = ia_cipf_buffer_get_frame_format(newBuffer, &format);
        if (ret != ia_err_none) {
            LOGE("@%s: Error getting frame format", __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        LOG1("pipeline allocated output buffer = %p, for framework handle = %p",
             newBuffer, output_buffer->data());
        LOG1("pipeline allocated output buffer resolution = %d x %d, v4l2 format = %x",
             format.width, format.height, format.fourcc);
        LOG1("framework handle resolution = %d x %d, v4l2 format = %x",
             output_buffer->width(), output_buffer->height(), output_buffer->v4l2Fmt());
        /* Register buffer */
        ret = ia_cipf_pipe_register_buffer(mPipe, newBuffer);
        if (ret != ia_err_none) {
            LOGE("@%s: Error registering buffer to pipe", __FUNCTION__);
            return UNKNOWN_ERROR;
        }
        mRegisteredIAOutputBuffs.add(output_buffer->data(), newBuffer);
        LOG1("Number of registered output buffers = %d", mRegisteredIAOutputBuffs.size());
    }

    return status;
}

status_t
PSysPipeline::iterate(sp<CameraBuffer> input_buffer, sp<CameraBuffer> output_buffer)
{
    HAL_PER_TRACE_CALL(CAMERA_DEBUG_LOG_PERF_TRACES);
    status_t status = NO_ERROR;
    ia_err_t ret;
    int index;

    if (mDebugFPS != NULL)
        mDebugFPS->update();

    // Get ia buffer corresponding to input
    index = mRegisteredIAInputBuffs.indexOfKey(input_buffer->data());
    if (index == NAME_NOT_FOUND) {
        LOGE("No input buffer registered for %p ", input_buffer->data());
        return UNKNOWN_ERROR;
    }

    ret = ia_cipf_iteration_set_buffer(mPipeIterator, mRegisteredIAInputBuffs.valueFor(input_buffer->data()));
    if (ret != ia_err_none) {
        LOGE("@%s: Error setting input buffer to iterator", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    // Get ia buffer corresponding to output
    index = mRegisteredIAOutputBuffs.indexOfKey(output_buffer->data());
    if (index == NAME_NOT_FOUND) {
        LOGE("No output buffer registered for %p ", output_buffer->data());
        return UNKNOWN_ERROR;
    }

    ret = ia_cipf_iteration_set_buffer(mPipeIterator, mRegisteredIAOutputBuffs.valueFor(output_buffer->data()));
    if (ret != ia_err_none) {
        LOGE("@%s: Error setting output buffer to iterator", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    ret = ia_cipf_iteration_execute(mPipeIterator);
    if (ret != ia_err_none) {
        LOGE("@%s: Error iterating", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    ret = ia_cipf_iteration_wait(mPipeIterator);
    if (ret != ia_err_none) {
        LOGE("@%s: Error waiting %d", __FUNCTION__, ret);
        return UNKNOWN_ERROR;
    }

    /* TODO: with pipe_iterator, enumerate through updated properties for
     *       reading */
    ret = ia_cipf_iteration_next(mPipeIterator);
    if (ret != ia_err_none) {
        LOGE("@%s: Error iterating next", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    return status;
}

status_t
PSysPipeline::destroyPipeline(void)
{
    HAL_TRACE_CALL(CAMERA_DEBUG_LOG_LEVEL2);
    status_t status = NO_ERROR;

    if (mPipeIterator)
        ia_cipf_iterator_destroy(mPipeIterator);

    if (mPipe)
        ia_cipf_pipe_destroy(mPipe);


    return status;
}

}  // namespace camera2
}  // namespace android
