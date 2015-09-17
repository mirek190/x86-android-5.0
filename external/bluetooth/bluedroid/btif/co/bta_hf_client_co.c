/******************************************************************************
 *
 *  Copyright (C) 2014 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include "gki.h"
#include "bta_api.h"
#include "bta_sys.h"
#include "bta_ag_api.h"
#include "bta_hf_client_co.h"
#include "bte_appl.h"
#include "bt_utils.h"

#define LOG_TAG "BTA_HF_CLIENT_CO"

#ifndef LINUX_NATIVE
#include <cutils/properties.h>
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


/************************************************************************************
**  Externs
************************************************************************************/
extern int set_audio_state(UINT16 handle, UINT16 codec, UINT8 state, void *param);


/*******************************************************************************
**
** Function         bta_hf_client_co_audio_state
**
** Description      This function is called by the HF before the audio connection
**                  is brought up, after it comes up, and after it goes down.
**
** Parameters       handle - handle of the HF instance
**                  state - Audio state
**                      BTA_HF_CLIENT_CO_AUD_STATE_OFF      - Audio has been turned off
**                      BTA_HF_CLIENT_CO_AUD_STATE_OFF_XFER - Audio has been turned off (xfer)
**                      BTA_HF_CLIENT_CO_AUD_STATE_ON       - Audio has been turned on
**                      BTA_HF_CLIENT_CO_AUD_STATE_SETUP    - Audio is about to be turned on
**                  codec - if WBS support is compiled in, codec to going to be used is provided
**                      and when in BTA_HF_CLIENT_CO_AUD_STATE_SETUP, BTM_I2SPCMConfig() must be called with
**                      the correct platform parameters.
**                      in the other states codec type should not be ignored
**
** Returns          void
**
*******************************************************************************/
#if (BTM_WBS_INCLUDED == TRUE )
void bta_hf_client_co_audio_state(UINT16 handle, UINT8 app_id, UINT8 state, tBTA_AG_PEER_CODEC codec)
#else
void bta_hf_client_co_audio_state(UINT16 handle, UINT8 app_id, UINT8 state)
#endif
{
    BTIF_TRACE_DEBUG("bta_hf_client_co_audio_state: handle %d, state %d", handle, state);
    switch (state)
    {
    case BTA_HF_CLIENT_CO_AUD_STATE_OFF:
#if (BTM_WBS_INCLUDED == TRUE )
        BTIF_TRACE_DEBUG("bta_hf_client_co_audio_state(handle %d)::Closed (OFF), codec: 0x%x",
                        handle, codec);
        set_audio_state(handle, codec, state, NULL);
#else
        BTIF_TRACE_DEBUG("bta_hf_client_co_audio_state(handle %d)::Closed (OFF)",
                        handle);
#endif
        break;
    case BTA_HF_CLIENT_CO_AUD_STATE_OFF_XFER:
        BTIF_TRACE_DEBUG("bta_hf_client_co_audio_state(handle %d)::Closed (XFERRING)", handle);
        break;
    case BTA_HF_CLIENT_CO_AUD_STATE_SETUP:
#if (BTM_WBS_INCLUDED == TRUE )
        set_audio_state(handle, codec, state, NULL);
#else
        set_audio_state(handle, BTA_AG_CODEC_CVSD, state, NULL);
#endif
        break;
    default:
        break;
    }
#if (BTM_WBS_INCLUDED == TRUE )
    APPL_TRACE_DEBUG("bta_hf_client_co_audio_state(handle %d, app_id: %d, state %d, codec: 0x%x)",
                      handle, app_id, state, codec);
#else
    APPL_TRACE_DEBUG("bta_hf_client_co_audio_state(handle %d, app_id: %d, state %d)", \
    handle, app_id, state);
#endif
}
