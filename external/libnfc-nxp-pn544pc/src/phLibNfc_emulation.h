/*
 * Copyright (C) 2010-2013 NXP Semiconductors
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

#ifndef LIBNFC_CARD_EMULATION_H
#define LIBNFC_CARD_EMULATION_H

#ifdef HOST_EMULATION

#include<phNfcHalTypes.h>


#define GEN_BYT_LEN  20

typedef struct phLibNfc_CEInfo
{
    phNfc_sData_t                *p_recv_data;
    uint32_t                     recv_index;

    phLibNfc_NtfRegister_RspCb_t  pCeNtfCb;
    void                         *pCeNtfCntx;

    pphLibNfc_RspCb_t            pCeTxCb;
    void                         *pCeTxCntx;

    pphLibNfc_Receive_RspCb_t    pCeRxCb;
    void                         *pCeRxCntx;

    pphLibNfc_RspCb_t           pCfgRspCb;
    void                        *pCfgRspCntx;

    phLibNfc_sEmulationCfg_t    config_data;

    phLibNfc_RemoteDevList_t    pCE_dev;

    uint8_t       Active;
}phLibNfc_CEInfo_t;



typedef struct phLibNfc_CECfgParams
{
    uint8_t                      sak;
    uint8_t                      GenBytes[GEN_BYT_LEN];
}phLibNfc_CECfgParams;


void phLibNfc_CENotification(void  *context,
                                    phHal_eNotificationType_t    type,
                                    phHal4Nfc_NotificationInfo_t  info,
                                    NFCSTATUS                   status);


#endif /* End of HOST_EMULATION*/

#endif
