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

/*!
* =========================================================================== *
*                                                                             *
*                                                                             *
* \file  phHciNfc_Emulation.h                                                 *
* \brief HCI emulation management routines.                                   *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Fri Apr  8 19:47:50 2011 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.11 $                                                            *
* $Aliases:  $                                                                *
*                                                                             *
 *
 * $Developed by : Ravindra U   $                                              *
 *
* =========================================================================== *
*/


#ifndef PHHCINFC_EMULATION_H
#define PHHCINFC_EMULATION_H

/*@}*/


/**
*  \name HCI
*
* File: \ref phHciNfc_Emulation.h
*
*/
/*@{*/
#define PHHCINFC_EMULATION_FILEREVISION "$Revision: 1.11 $" /**< \ingroup grp_file_attributes */
#define PHHCINFC_EMULATION_FILEALIASES  "$Aliases:  $"     /**< \ingroup grp_file_attributes */
/*@}*/

/*
***************************** Header File Inclusion ****************************
*/

#include <phHciNfc_Generic.h>

/*
****************************** Macro Definitions *******************************
*/
/* Connectivity Gate Command Support */
#define PRO_HOST_REQUEST            (0x10U)

/* Connectivity Gate Event Support */
#define EVT_CONNECTIVITY            (0x10U)
#define EVT_END_OF_TRANSACTION      (0x11U)
#define EVT_TRANSACTION             (0x12U)
#define EVT_OPERATION_ENDED         (0x13U)

#define TRANSACTION_MIN_LEN         (0x03U)
#define TRANSACTION_AID             (0x81U)
#define TRANSACTION_PARAM           (0x82U)

#define HOST_CE_MODE_ENABLE         (0x02U)
#define HOST_CE_MODE_DISABLE        (0xFFU)

#define NXP_PIPE_CONNECTIVITY       (0x60U)


/* Card Emulation Gate Events */
#define CE_EVT_NFC_SEND_DATA        (0x10U)
#define CE_EVT_NFC_FIELD_ON         (0x11U)
#define CE_EVT_NFC_DEACTIVATED      (0x12U)
#define CE_EVT_NFC_ACTIVATED        (0x13U)
#define CE_EVT_NFC_FIELD_OFF        (0x14U)

/*
******************** Enumeration and Structure Definition **********************
*/



/*
*********************** Function Prototype Declaration *************************
*/

extern
NFCSTATUS
phHciNfc_Uicc_Update_PipeInfo(
                                phHciNfc_sContext_t     *psHciContext,
                                uint8_t                 pipe_id,
                                phHciNfc_Pipe_Info_t    *pPipeInfo
                        );

extern
NFCSTATUS
phHciNfc_EmuMgmt_Update_Seq(
                                phHciNfc_sContext_t     *psHciContext,
                                phHciNfc_eSeqType_t     seq_type
                        );


extern
NFCSTATUS
phHciNfc_EmuMgmt_Initialise(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef
                        );

extern
NFCSTATUS
phHciNfc_EmuMgmt_Info_Sequence(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef
                        );

extern
NFCSTATUS
phHciNfc_EmuMgmt_Release(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef
                        );


extern
NFCSTATUS
phHciNfc_Emulation_Cfg (
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_eConfigType_t  cfg_type
                    );

extern
NFCSTATUS
phHciNfc_Uicc_Get_PipeID(
                            phHciNfc_sContext_t     *psHciContext,
                            uint8_t                 *ppipe_id
                        );

extern
NFCSTATUS
phHciNfc_Uicc_Connect_Status(
                               phHciNfc_sContext_t      *psHciContext,
                               void                 *pHwRef,
                               uint8_t              retry
                      );

extern
NFCSTATUS
phHciNfc_Uicc_Hiz_State(
                            phHciNfc_sContext_t     *psHciContext,
                            phHal_sHwReference_t    *pHwReference,
                            uint8_t                 *p_hiz_state
                        );

extern
void
phHciNfc_Uicc_Connectivity(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef
                        );

extern
NFCSTATUS
phHciNfc_Emulation_Send (
                        phHciNfc_sContext_t             *psHciContext,
                        void                            *pHwRef,
                        phHal_sRemoteDevInformation_t   *p_remote_dev_info,
                        phHciNfc_XchgInfo_t             *p_send_param
                        );


#endif /* PHHCINFC_EMULATION_H */
