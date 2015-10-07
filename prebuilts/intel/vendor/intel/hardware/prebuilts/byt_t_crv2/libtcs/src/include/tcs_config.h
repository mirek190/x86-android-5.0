/* Telephony Config Selector (TCS) - config data
**
** Copyright (C) Intel 2013
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
*/

#ifndef __TCS_CONFIG_HEADER__
#define __TCS_CONFIG_HEADER__

#include "stdbool.h"
#include "string.h"

#define NAME 64
#define MY_PATH_MAX 256

#define LINKS \
    X(UNKNOWN), \
    X(UART), \
    X(HSI), \
    X(USB), \
    X(SHM), \
    X(SPI), \

typedef enum e_link {
#undef X
#define X(a) E_LINK_ ## a
    LINKS
} e_link_t;

typedef struct mdm_core {
    char name[NAME];
    bool flashless;
    bool secured;
    e_link_t ipc_mdm;
    e_link_t ipc_bp_log;
    e_link_t ipc_cd;
    char hw_revision[NAME];
    char sw_revision[NAME];
} mdm_core_t;

typedef struct mdm_modules {
    char mmgr_xml[NAME];
    char rril_txt[NAME];
} mdm_modules_t;

typedef struct tlv_info {
    char filename[MY_PATH_MAX];
} tlv_info_t;

typedef struct tlvs_info {
    size_t nb;
    tlv_info_t *tlv;
} tlvs_info_t;

typedef struct channel {
    /* @TODO: Currently, we only have DLC channels. But if another type is
     * used, an union should be used here */
    char device[MY_PATH_MAX];
} channel_t;

typedef struct channels_mmgr {
    channel_t shutdown;
    channel_t sanity_check;
    channel_t secured;
    channel_t mdm_custo;
    channel_t cd_logs;
} channels_mmgr_t;

typedef struct channels_nvm {
    channel_t sync;
} channels_nvm_t;

typedef struct channels_rril {
    channel_t control;
    channel_t registration;
    channel_t settings;
    channel_t sim_toolkit;
    channel_t cops_commands;
    channel_t monitoring;
    channel_t field_test;
    channel_t rfcoexistence;
    channel_t sms;
    channel_t packet_data_1;
    channel_t packet_data_2;
    channel_t packet_data_3;
    channel_t packet_data_4;
    channel_t packet_data_5;
} channels_rril_t;

typedef struct channels {
    channels_mmgr_t mmgr;
    channels_nvm_t nvm;
    channels_rril_t rril;
} channel_info_t;

typedef struct channels_info {
    size_t nb;
    channel_info_t *ch;
} channels_info_t;

typedef struct mdm_info {
    mdm_core_t core;
    mdm_modules_t mod;
    tlvs_info_t tlvs;
    channels_info_t chs;
} mdm_info_t;

#endif
