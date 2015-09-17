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

/******************************************************************************
 *
 *  This is the interface file for audio gateway call-out functions.
 *
 ******************************************************************************/
#ifndef BTA_HF_CLIENT_CO_H
#define BTA_HF_CLIENT_CO_H

#include "bta_hf_client_api.h"
#include "bta_ag_api.h"


/* Definitions for audio state callout function "state" parameter */
#define BTA_HF_CLIENT_CO_AUD_STATE_OFF         0
#define BTA_HF_CLIENT_CO_AUD_STATE_OFF_XFER    1   /* Closed pending transfer of audio */
#define BTA_HF_CLIENT_CO_AUD_STATE_ON          2
#define BTA_HF_CLIENT_CO_AUD_STATE_SETUP       3

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
**                      BTA_HF_CLIENT_CO_AUD_STATE_OFF_XFER - Audio is closed pending transfer
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
BTA_API extern void bta_hf_client_co_audio_state(UINT16 handle, UINT8 app_id, UINT8 state,
                                          tBTA_AG_PEER_CODEC codec);

#else
BTA_API extern void bta_hf_client_co_audio_state(UINT16 handle, UINT8 app_id, UINT8 state);
#endif

#endif /* BTA_HF_CLIENT_CO_H */

