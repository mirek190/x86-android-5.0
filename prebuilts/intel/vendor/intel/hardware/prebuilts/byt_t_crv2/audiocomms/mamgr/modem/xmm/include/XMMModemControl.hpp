/*
 * INTEL CONFIDENTIAL
 * Copyright  2013-2014 Intel
 * Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and licensors. The
 * Material is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or
 * disclosed in any way without Intels prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

#pragma once

typedef enum
{
    UTA_AUDIO_ANALOG_SRC_HANDSET_MIC = 0,
    UTA_AUDIO_ANALOG_SRC_HEADSET_MIC,
    UTA_AUDIO_ANALOG_SRC_HF_MIC,
    UTA_AUDIO_ANALOG_SRC_AUX,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_1,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_2,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_3,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_4,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_5,
    UTA_AUDIO_ANALOG_SRC_TTY
} S_UTA_AUDIO_ANALOG_SRC;

typedef enum
{
    UTA_AUDIO_ANALOG_DEST_HANDSET = 0,
    UTA_AUDIO_ANALOG_DEST_HEADSET,
    UTA_AUDIO_ANALOG_DEST_BACKSPEAKER,
    UTA_AUDIO_ANALOG_DEST_HEADSET_PLUS_BACKSPEAKER,
    UTA_AUDIO_ANALOG_DEST_HEADSET_PLUS_HANDSET,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_1,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_2,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_3,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_4,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_5,
    UTA_AUDIO_ANALOG_DEST_TTY
} S_UTA_AUDIO_ANALOG_DEST;

typedef enum
{
    IFX_DEFAULT_S = 0,
    IFX_HANDSET_S = 1,
    IFX_HEADSET_S = 2,
    IFX_HF_S = 3,
    IFX_AUX_S = 4,
    IFX_TTY_S = 5,
    IFX_BLUETOOTH_S = 6,
    IFX_USER_DEFINED_1_S = 7,
    IFX_USER_DEFINED_2_S = 8,
    IFX_USER_DEFINED_3_S = 9,
    IFX_USER_DEFINED_4_S = 10,
    IFX_USER_DEFINED_5_S = 11,
    IFX_USER_DEFINED_15_S = 21,
} IFX_TRANSDUCER_MODE_SOURCE;

typedef enum
{
    IFX_DEFAULT_D = 0,
    IFX_HANDSET_D = 1,
    IFX_HEADSET_D = 2,
    IFX_BACKSPEAKER_D = 3,
    IFX_HEADSET_PLUS_BACKSPEAKER_D = 4,
    IFX_HEADSET_PLUS_HANDSET_D = 5,
    IFX_TTY_D = 6,
    IFX_BLUETOOTH_D = 7,
    IFX_USER_DEFINED_1_D = 8,
    IFX_USER_DEFINED_2_D = 9,
    IFX_USER_DEFINED_3_D = 10,
    IFX_USER_DEFINED_4_D = 11,
    IFX_USER_DEFINED_5_D = 12,
    IFX_USER_DEFINED_15_D = 22,
} IFX_TRANSDUCER_MODE_DEST;
