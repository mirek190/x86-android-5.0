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
#ifndef _CAMERA3_HAL_IPU2_COMMON_TYPES_H_
#define _CAMERA3_HAL_IPU2_COMMON_TYPES_H_
/* ********************************************************************
 * ISP relevance
 */
#define DEFAULT_PREVIEW_BUFFERS_NUM 9
#define DEFAULT_VIDEO_BUFFERS_NUM DEFAULT_PREVIEW_BUFFERS_NUM
#define DEFAULT_SNAPSHOT_BUFFERS_NUM 2
#define DEFAULT_RECORDING_FPS 30


#define INVALID_REQ_ID (-1)
#define MIN_EXP_ID 1
#define MAX_EXP_ID 250
#define INVALID_EXP_ID (MIN_EXP_ID-1)

static const int kCameraPriority = 1;

/* ********************************************************************
 */
/* Camera state machine:
 * STOPPED -> PREVIEW/RECORDING/ZSL -> STOPPED
 * STOPPED -> PREVIEW -> SNAPSHOT -> STOPPED
 * STOPPED -> ZSL -> REPROCESS -> STOPPED
 */
enum camera_state_t {
    STATE_STOPPED = 0,
    STATE_PREVIEW,
    STATE_RECORDING,
    STATE_ZSL,

    STATE_SNAPSHOT,
    STATE_REPROCESS, // can't handle continuous requests in this state now
                     // Now we can't provide image for ZSL stream, which is source of reprocess (input) stream
                     // so in STATE_REPROCESS, we still get image from ISP
    STATE_MAX
};

enum IspMode {
    MODE_NONE = -1,
    MODE_PREVIEW = 0,
    MODE_CAPTURE = 1,
    MODE_VIDEO = 2,
    MODE_CONTINUOUS_CAPTURE = 3,
    MODE_CONTINUOUS_VIDEO = 4
};

enum FrameBufferStatus {
    FRAME_STATUS_NA = -1,
    FRAME_STATUS_OK = ATOMISP_FRAME_STATUS_OK,
    FRAME_STATUS_CORRUPTED = ATOMISP_FRAME_STATUS_CORRUPTED,
    FRAME_STATUS_FLASH_EXPOSED = ATOMISP_FRAME_STATUS_FLASH_EXPOSED,
    FRAME_STATUS_FLASH_PARTIAL = ATOMISP_FRAME_STATUS_FLASH_PARTIAL,
    FRAME_STATUS_FLASH_FAILED = ATOMISP_FRAME_STATUS_FLASH_FAILED,
    FRAME_STATUS_SKIPPED,
    FRAME_STATUS_MASK = 0x0000ffff, /*!< Use low 16bit of v4l2_buffer.reserved for status*/
};

#endif//  _CAMERA3_HAL_IPU2_COMMON_TYPES_H_
