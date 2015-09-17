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
* \file  phHciNfc_Emulation.c                                                 *
* \brief HCI Emulation management routines.                                   *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Thu Oct 27 11:02:59 2011 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.58 $                                                           *
* $Aliases:  $
*                                                                             *
* $Developed by : Ravindra U   $                                              *
*                                                                             *
* =========================================================================== *
*/

/*
***************************** Header File Inclusion ****************************
*/
#include <phNfcConfig.h>
#include <phNfcCompId.h>
#include <phNfcHalTypes.h>
#include <phHciNfc_Pipe.h>
#include <phHciNfc_Emulation.h>
#include <phHciNfc_DevMgmt.h>
#include <phHciNfc_WI.h>
#include <phHciNfc_SWP.h>
#ifdef ENABLE_P2P
#include <phHciNfc_NfcIPMgmt.h>
#endif
#ifdef HOST_EMULATION
#include <phHciNfc_CE_A.h>
#include <phHciNfc_CE_B.h>
#endif
#include <phOsalNfc.h>
/*
****************************** Macro Definitions *******************************
*/



/*
*************************** Structure and Enumeration ***************************
*/

/** \defgroup grp_hci_nfc HCI Emulation Management Component
*
*
*/

typedef enum phHciNfc_EmulationMgmt_Seq{
    NFCIP_TARGET_PIPE_OPEN  = 0x00U,
    NFCIP_TARGET_MODE_CONFIG,
    NFCIP_TARGET_MERGE_SAK,
    NFCIP_TARGET_PIPE_CLOSE,

    HOST_CE_A_INIT,
    HOST_CE_A_RELEASE,

    HOST_CE_B_INIT,
    HOST_CE_B_RELEASE,

    WI_PIPE_OPEN,
    WI_GET_DEFAULT_MODE,
    WI_SMX_PRESENCE,
    WI_ENABLE_EMULATION,
    WI_DEFAULT_EMULATION,
    WI_DISABLE_EMULATION,

    WI_ENABLE_NOTIFICATION,
    WI_DISABLE_NOTIFICATION,

    WI_SWITCH_WIRED_MODE,
    WI_SWITCH_DEFAULT_MODE,

    WI_PIPE_CLOSE,

    SWP_PIPE_OPEN,
    SWP_GET_DEFAULT_MODE,
    SWP_GET_HIZ_STATE,
    SWP_GET_HIZ_IO,
    SWP_ENABLE_EMULATION,
    SWP_DEFAULT_EMULATION,
    SWP_DETECTION,
    SWP_DISABLE_EMULATION,
    SWP_GET_BIT_RATE,
    SWP_PIPE_CLOSE,

    CONFIG_DEFAULT_EMULATION,

    END_EMULATION_SEQ
} phHciNfc_EmulationMgmt_Seq_t;

typedef struct phHciNfc_EmulationMgmt_Info{
    phHal_eEmulationType_t          se_default;
    uint8_t                         smx_powerless;
    uint8_t                         uicc_enable;
    uint8_t                         uicc_powerless;
    uint8_t                         uicc_id;
    /* Application ID of the UICC Transaction performed */
    uint8_t                         uicc_aid[MAX_AID_LEN];
    uint8_t                         uicc_param[MAX_UICC_PARAM_LEN];
    uint8_t                         uicc_param_len;
    uint8_t                         swp_hiz_state;
    uint8_t                         swp_hiz_io;
    phHciNfc_Pipe_Info_t            *p_uicc_pipe_info;
    phHciNfc_EmulationMgmt_Seq_t    emulation_cur_seq;
    phHciNfc_EmulationMgmt_Seq_t    emulation_next_seq;


} phHciNfc_EmulationMgmt_Info_t;


/*
*************************** Static Function Declaration **************************
*/

static
NFCSTATUS
phHciNfc_Recv_Uicc_Cmd (
                        void                *psContext,
                        void                *pHwRef,
                        uint8_t             *pCmd,
                        uint16_t            length
                     );

static
NFCSTATUS
phHciNfc_Recv_Uicc_Event (
                        void                *psContext,
                        void                *pHwRef,
                        uint8_t             *pEvent,
                        uint16_t            length
                     );


/*
*************************** Function Definitions ***************************
*/

void
phHciNfc_Uicc_Connectivity(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef
                        )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t            *pPipeInfo = NULL;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info = NULL;

    if( NULL != psHciContext->p_emulation_mgmt_info )
    {
        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
                            psHciContext->p_emulation_mgmt_info ;
        pPipeInfo = psHciContext->p_pipe_list[NXP_PIPE_CONNECTIVITY];
        if( (NULL == pPipeInfo)
            && ((HCI_CUSTOM_INIT == psHciContext->init_mode)
            || (TRUE == ((phHal_sHwReference_t *)pHwRef)->uicc_connected))
            )
        {
            status = phHciNfc_Allocate_Resource((void **)&pPipeInfo,
                                                sizeof(phHciNfc_Pipe_Info_t));
            if((NULL != pPipeInfo)
                && (NFCSTATUS_SUCCESS == status))
            {
                /* The Source Host is the UICC Host */
                pPipeInfo->pipe.source.host_id =
                                    (uint8_t) phHciNfc_UICCHostID;
                /* The Source Gate is same as the Destination Gate */
                pPipeInfo->pipe.source.gate_id  =
                                    (uint8_t)   phHciNfc_ConnectivityGate;
                /* The Destination Host is the Terminal Host */
                pPipeInfo->pipe.dest.host_id =
                                    (uint8_t) phHciNfc_TerminalHostID;
                /* The Source Gate is same as the Destination Gate */
                pPipeInfo->pipe.dest.gate_id    =
                                    (uint8_t) phHciNfc_ConnectivityGate;
                /* The Pipe ID is Hardcoded to Connectivity */
                pPipeInfo->pipe.pipe_id = (uint8_t) NXP_PIPE_CONNECTIVITY;


                status = phHciNfc_Uicc_Update_PipeInfo(psHciContext,
                                        NXP_PIPE_CONNECTIVITY, pPipeInfo);
                if (NFCSTATUS_SUCCESS == status)
                {
                    psHciContext->p_pipe_list[NXP_PIPE_CONNECTIVITY] = pPipeInfo;
                    p_emulation_mgmt_info->uicc_enable = TRUE;
                }
                else
                {
                    (void)phOsalNfc_FreeMemory(pPipeInfo);
                }
            }
        }
    }
  return;
}

/*!
 * \brief Get the pipe_id of Connectivity Managment Gate.
 *
 * This function Get the pipe_id of Connectivity Managment Gate.
 *
 */


NFCSTATUS
phHciNfc_Uicc_Get_PipeID(
                            phHciNfc_sContext_t     *psHciContext,
                            uint8_t                 *ppipe_id
                        )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info = NULL;
    if( (NULL != psHciContext)
        && ( NULL != ppipe_id )
        && ( NULL != psHciContext->p_emulation_mgmt_info )
      )
    {
        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
                            psHciContext->p_emulation_mgmt_info ;
        *ppipe_id =  p_emulation_mgmt_info->uicc_id  ;
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    return status;
}


/*!
 * \brief Get the HI impedence State for the SWP interface Line .
 *
 * This function Updates the and returns the Status of the SWP Interface
 * for the High Impedence State.
 *
 */


NFCSTATUS
phHciNfc_Uicc_Hiz_State(
                            phHciNfc_sContext_t     *psHciContext,
                            phHal_sHwReference_t    *pHwReference,
                            uint8_t                 *p_hiz_state
                        )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info = NULL;
    if( (NULL != psHciContext)
        && ( NULL != pHwReference)
        && ( NULL != p_hiz_state )
        && ( NULL != psHciContext->p_emulation_mgmt_info )
      )
    {
        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
                            psHciContext->p_emulation_mgmt_info ;

        pHwReference->se_if_info.swp_hiz_io =
              p_emulation_mgmt_info->swp_hiz_io;

        if ((0x00 < p_emulation_mgmt_info->swp_hiz_state)
                && (0x01 < p_emulation_mgmt_info->swp_hiz_io))
        {
            *p_hiz_state = TRUE;
        }
        else
        {
            *p_hiz_state = FALSE;
        }

    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    return status;
}


/* Function to Update the  Pipe Information */
NFCSTATUS
phHciNfc_Uicc_Update_PipeInfo(
                                phHciNfc_sContext_t     *psHciContext,
                                uint8_t                 pipe_id,
                                phHciNfc_Pipe_Info_t    *pPipeInfo
                        )
{
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if ( NULL == psHciContext->p_emulation_mgmt_info )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_INFORMATION);
    }
    else
    {
        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
                                psHciContext->p_emulation_mgmt_info ;
        /* Update the pipe_id of the Connectivity Gate
         * obtained from the HCI Response */
        p_emulation_mgmt_info->uicc_id = pipe_id;
        p_emulation_mgmt_info->p_uicc_pipe_info = pPipeInfo;
        if ( NULL != pPipeInfo)
        {
            /* Update the Response Receive routine of the Connectivity Gate */
            /* pPipeInfo->recv_resp = phHciNfc_Recv_Uicc_Response; */
            pPipeInfo->recv_cmd = &phHciNfc_Recv_Uicc_Cmd;
            pPipeInfo->recv_event = &phHciNfc_Recv_Uicc_Event;
        }
    }

    return status;
}


/*!
* \brief Updates the Sequence of Emulation Managment Gate.
*
* This function Resets/Updates the sequence of the Emulation Management
* gate.
*
*/


NFCSTATUS
phHciNfc_EmuMgmt_Update_Seq(
                                phHciNfc_sContext_t     *psHciContext,
                                phHciNfc_eSeqType_t     seq_type
                        )
{
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if( NULL == psHciContext->p_emulation_mgmt_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            phHal_sHwReference_t *pHwRef = psHciContext->p_hw_ref;
            p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
                psHciContext->p_emulation_mgmt_info ;
            switch(seq_type)
            {
                case RESET_SEQ:
                case INIT_SEQ:
                {
#if defined (HOST_EMULATION)
                   if((NULL != pHwRef) &&
                       (NXP_NFC_ROM_INFO(((phHal_sHwReference_t *)pHwRef)->device_info.fw_version)
                      >= NFC44_ROM33_VER ))
                    {

                       p_emulation_mgmt_info->emulation_cur_seq = HOST_CE_A_INIT ;
                    }
                    else
                    {
#endif
#ifdef ENABLE_P2P
                       p_emulation_mgmt_info->emulation_cur_seq = NFCIP_TARGET_PIPE_OPEN;
#else
                       p_emulation_mgmt_info->emulation_cur_seq = WI_PIPE_OPEN;
#endif
#if defined (HOST_EMULATION)
                    }
#endif
                    p_emulation_mgmt_info->emulation_next_seq = END_EMULATION_SEQ ;
                    break;
                }
                case UPDATE_SEQ:
                {
                    p_emulation_mgmt_info->emulation_cur_seq =
                        p_emulation_mgmt_info->emulation_next_seq;
                    break;
                }
                case INFO_SEQ:
                {
                    if(HCI_SESSION == psHciContext->init_mode)
                    {
                       p_emulation_mgmt_info->emulation_cur_seq = SWP_ENABLE_EMULATION;
                    }
                    else
                    {
                        p_emulation_mgmt_info->emulation_cur_seq = WI_GET_DEFAULT_MODE;
                    }

                    p_emulation_mgmt_info->emulation_next_seq = END_EMULATION_SEQ ;
                    break;
                }
                case REL_SEQ:
                {
#if defined (ENABLE_WI)
                    p_emulation_mgmt_info->emulation_cur_seq = WI_DISABLE_EMULATION;
#else
                    p_emulation_mgmt_info->emulation_cur_seq = SWP_DISABLE_EMULATION;
#endif /* #if defined (ENABLE_WI) */
                    p_emulation_mgmt_info->emulation_next_seq = END_EMULATION_SEQ ;
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    return status;
}


/*!
* \brief Initialisation of RF Emulation Gates.
*
* This function initialses the RF Emulation Management and
* populates the Reader Management Information Structure
*
*/


NFCSTATUS
phHciNfc_EmuMgmt_Initialise(
                               phHciNfc_sContext_t      *psHciContext,
                               void                 *pHwRef
                           )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t            *p_pipe_info = NULL;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        if( ( NULL == psHciContext->p_emulation_mgmt_info )
            && (phHciNfc_Allocate_Resource((void **)(&p_emulation_mgmt_info),
            sizeof(phHciNfc_EmulationMgmt_Info_t))== NFCSTATUS_SUCCESS)
            )
        {
            psHciContext->p_emulation_mgmt_info = p_emulation_mgmt_info;

#if defined (HOST_EMULATION)
            if((NXP_NFC_ROM_INFO((
                (phHal_sHwReference_t *)psHciContext->p_hw_ref)->device_info.fw_version)
               >= NFC44_ROM33_VER ))
             {
                p_emulation_mgmt_info->emulation_cur_seq = HOST_CE_A_INIT;
             }
             else
             {

#if defined ( ENABLE_P2P)
                p_emulation_mgmt_info->emulation_cur_seq = NFCIP_TARGET_PIPE_OPEN;
#else
                p_emulation_mgmt_info->emulation_cur_seq = WI_PIPE_OPEN;
#endif
             }

#elif defined ( ENABLE_P2P)
            p_emulation_mgmt_info->emulation_cur_seq = NFCIP_TARGET_PIPE_OPEN;
#else
            p_emulation_mgmt_info->emulation_cur_seq = WI_PIPE_OPEN;
#endif
            p_emulation_mgmt_info->emulation_next_seq = END_EMULATION_SEQ;
            p_emulation_mgmt_info->uicc_id  = (uint8_t)HCI_UNKNOWN_PIPE_ID;
        }
        else
        {
            p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
                psHciContext->p_emulation_mgmt_info ;
        }

        if( NULL == psHciContext->p_emulation_mgmt_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
#ifdef ESTABLISH_SESSION
        else if(( hciMode_Session == psHciContext->hci_mode )
#if defined (HOST_EMULATION)
            && (( HOST_CE_A_INIT == p_emulation_mgmt_info->emulation_cur_seq )
              || ( NFCIP_TARGET_PIPE_OPEN == p_emulation_mgmt_info->emulation_cur_seq ))
#else
            && ( NFCIP_TARGET_PIPE_OPEN == p_emulation_mgmt_info->emulation_cur_seq )
#endif
            )
        {
            status = NFCSTATUS_SUCCESS;
        }
#endif
        else
        {
            switch(p_emulation_mgmt_info->emulation_cur_seq )
            {
#if defined (HOST_EMULATION)
                /* Card Emulation A Initialisation sequence */
                case HOST_CE_A_INIT:
                {
                    status =
                         phHciNfc_CE_A_Initialise( psHciContext, pHwRef );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        p_emulation_mgmt_info->emulation_next_seq =
                                                HOST_CE_B_INIT;
                        status = NFCSTATUS_PENDING;
                    }
                    else
                    {
                        p_emulation_mgmt_info->emulation_next_seq = HOST_CE_A_INIT;
                    }
                    break;
                }
                /* Card Emulation B Initialisation sequence */
                case HOST_CE_B_INIT:
                {
                    status =
                         phHciNfc_CE_B_Initialise( psHciContext, pHwRef );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        p_emulation_mgmt_info->emulation_next_seq =
                                                NFCIP_TARGET_PIPE_OPEN;
                        status = NFCSTATUS_PENDING;
                    }
                    else
                    {
                        p_emulation_mgmt_info->emulation_next_seq = HOST_CE_B_INIT;
                    }
                    break;
                }
#endif
#ifdef ENABLE_P2P
                /* NFCIP Target Open sequence */
                case NFCIP_TARGET_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_NfcIP_Info_t *)
                        psHciContext->p_nfcip_info)->p_tgt_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                            pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                    NFCIP_TARGET_MODE_CONFIG;
                            status = NFCSTATUS_PENDING;
                        }
                    }
                    break;
                }
                /* NFCIP Target Mode Config sequence */
                case NFCIP_TARGET_MODE_CONFIG:
                {
#define NFCIP_ACTIVE_SHIFT  0x03U
#define NFCIP_PASSIVE_MASK  0x07U
                    /* Ravi U:  Kindly note that the below check with &&
                     *   is intentional and should not be replaced with
                     *   bitwise &.
                     */
                    uint8_t mode = ( ( NXP_NFCIP_ACTIVE_DEFAULT && NFCIP_PASSIVE_MASK )
                                               << NFCIP_ACTIVE_SHIFT )
                                     | ( DEFAULT_NFCIP_MODE_SUPPORT & NFCIP_PASSIVE_MASK );
                    status = phHciNfc_NfcIP_SetMode( psHciContext, pHwRef,
                                                NFCIP_TARGET, mode);
                    if(status == NFCSTATUS_PENDING )
                    {
#if defined(TGT_MERGE_SAK)
                        p_emulation_mgmt_info->emulation_next_seq =
                                            NFCIP_TARGET_MERGE_SAK;
#elif defined (ENABLE_WI)
                        p_emulation_mgmt_info->emulation_next_seq =
                                            WI_PIPE_OPEN;
#else
                        p_emulation_mgmt_info->emulation_next_seq =
                                            SWP_PIPE_OPEN;
#endif /* #ifdef TGT_MERGE_SAK */
                        /* status = NFCSTATUS_SUCCESS; */
                    }
                    break;
                }
#if defined(TGT_MERGE_SAK)
                /* NFCIP Target SAK Merge sequence */
                case NFCIP_TARGET_MERGE_SAK:
                {
                    status = phHciNfc_NfcIP_SetMergeSak( psHciContext, pHwRef,
                                                TRUE );
                    if(status == NFCSTATUS_PENDING )
                    {
#if defined (ENABLE_WI)
                        p_emulation_mgmt_info->emulation_next_seq =
                                            WI_PIPE_OPEN;
#else
                        p_emulation_mgmt_info->emulation_next_seq =
                                            SWP_PIPE_OPEN;
#endif /* #ifdef ENABLE_WI */
                        /* status = NFCSTATUS_SUCCESS; */
                    }
                    break;
                }
#endif /* #ifdef TGT_MERGE_SAK */
#endif /* #ifdef ENABLE_P2P */
#ifdef ENABLE_WI
                /* Secure Element WI pipe open sequence */
                case WI_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_WI_Info_t *)
                        psHciContext->p_wi_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                                            pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
#ifdef DISABLE_WI_NOTIFICATION
                            p_emulation_mgmt_info->emulation_next_seq =
                                                            SWP_PIPE_OPEN;
#else
                            p_emulation_mgmt_info->emulation_next_seq =
                                                            WI_ENABLE_NOTIFICATION;
#endif
                            status = NFCSTATUS_PENDING;
                        }
                    }
                    break;
                }
                /* Enable the SmartMx Notifications through WI */
                case WI_ENABLE_NOTIFICATION:
                {
                    p_pipe_info = ((phHciNfc_WI_Info_t *)
                                psHciContext->p_wi_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_WI_Configure_Notifications(
                                    psHciContext, pHwRef, eEnableEvents );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                                SWP_PIPE_OPEN;
                        }
                    }
                    break;
                }
                /* Enable the SmartMx Emulation by Default through WI */
                case WI_ENABLE_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_WI_Info_t *)
                                psHciContext->p_wi_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_WI_Configure_Default( psHciContext,
                            pHwRef, TRUE );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                    SWP_PIPE_OPEN;
                        }
                    }
                    break;
                }
#endif /* #ifdef ENABLE_WI */
                /* SWP pipe open sequence */
                case SWP_PIPE_OPEN:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_Open_Pipe( psHciContext,
                            pHwRef, p_pipe_info );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                            SWP_ENABLE_EMULATION;
#ifndef ESTABLISH_SESSION
                            status = NFCSTATUS_PENDING;
#endif
                        }
                    }
                    break;
                }
                /* Enable the UICC Emulation through SWP */
                case SWP_ENABLE_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
#ifdef SWP_EVENT_USAGE
                    status = phHciNfc_SWP_Configure_Mode( psHciContext, pHwRef,
                        UICC_SWITCH_MODE_ON );
                       /* UICC_SWITCH_MODE_DEFAULT */
#else
                        status = phHciNfc_SWP_Configure_Default( psHciContext,
                            pHwRef,   TRUE );
#endif
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                    SWP_DETECTION;
                            /* status = NFCSTATUS_SUCCESS; */
                        }
                    }
                    break;
                }
                /* Disable the UICC Emulation through SWP */
                case SWP_DETECTION:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                        break;
                    }
                    else
                    {
                        status = phHciNfc_Uicc_Connect_Status(
                                                    psHciContext, pHwRef, TRUE );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            uint8_t uicc_connect = ((phHciNfc_SWP_Info_t *)
                                            psHciContext->p_swp_info)->uicc_status;
                            if((UICC_CONNECTED == uicc_connect)
                                || (UICC_CONNECTION_INACTIVE == uicc_connect))
                            {
#if defined (SWP_EVENT_USAGE)
                                p_emulation_mgmt_info->emulation_next_seq =
                                                    SWP_DISABLE_EMULATION;
#elif defined (ENABLE_WI)
                                p_emulation_mgmt_info->emulation_next_seq =
                                                    WI_DISABLE_EMULATION;
/* #ifdef ENABLE_WI */
#else
                                p_emulation_mgmt_info->emulation_next_seq =
                                                    SWP_DISABLE_EMULATION;
#endif
                                status = NFCSTATUS_PENDING;
                            }
                            else
                            {
                                status = phHciNfc_SWP_Configure_Default( psHciContext,
                                                pHwRef,   FALSE );
                                (NFCSTATUS_PENDING == status)?
                                    (p_emulation_mgmt_info->emulation_next_seq =
#if defined (ENABLE_WI)
                                    WI_DISABLE_EMULATION):
/* #ifdef ENABLE_WI */
#else
                                    SWP_DISABLE_EMULATION):
#endif
                                    (p_emulation_mgmt_info->emulation_next_seq =
                                        SWP_DETECTION);
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
#if defined (ENABLE_WI)
                /* fall through */
                /* Disable the SmartMx Emulation through WI */
                case WI_DISABLE_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_WI_Info_t *)
                                psHciContext->p_wi_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_WI_Configure_Mode( psHciContext,
                            pHwRef, eSmartMx_Default );
                        if(status == NFCSTATUS_SUCCESS)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                    SWP_DISABLE_EMULATION;
                        }
                    }
                    break;
                }
#endif /* #ifdef ENABLE_WI */
#ifndef SWP_EVENT_USAGE
                /* fall through */
                /* Get the UICC Baud Rate Status */
                case SWP_GET_BIT_RATE:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_SWP_Get_Bitrate(
                                                    psHciContext, pHwRef );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                    SWP_DISABLE_EMULATION;
                            status = NFCSTATUS_SUCCESS;
                        }
                    }
                    break;
                }
#endif
                /* fall through */
                /* Disable the UICC Emulation through SWP */
                case SWP_DISABLE_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_SWP_Configure_Mode( psHciContext,
                            pHwRef,   UICC_SWITCH_MODE_DEFAULT );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
#if defined (ENABLE_WI)
                                    WI_DISABLE_EMULATION;
/* #ifdef ENABLE_WI */
#else
                                    SWP_DISABLE_EMULATION;
#endif
                            /* Disable WI Emulation for Previous Wired
                             * Mode Set */
                            /* status = NFCSTATUS_SUCCESS; */
                        }
                    }
                    break;
                }
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                    break;
                }

            }/* End of the Sequence Switch */

        }/* End of the Reader Info Memory Check */

    } /* End of Null Context Check */

    return status;
}

/*!
* \brief Connection Routine for the Uicc.
*
* This function tries to enable and initialise the UICC connected
* through SWP.
*
*/


NFCSTATUS
phHciNfc_Uicc_Connect_Status(
                               phHciNfc_sContext_t  *psHciContext,
                               void                 *pHwRef,
                               uint8_t              retry
                      )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    /* phHciNfc_Pipe_Info_t         *p_pipe_info = NULL; */
    /* phHciNfc_EmulationMgmt_Info_t    *p_emulation_mgmt_info=NULL; */
    static uint32_t                  uicc_connection_retry = 0;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        phHciNfc_SWP_Status_t uicc_status =
            ((phHciNfc_SWP_Info_t *)
            psHciContext->p_swp_info)->uicc_status;
        if(uicc_connection_retry == 0)
        {
#ifdef UICC_STATUS_DELAY
            for( ;uicc_connection_retry < UICC_STATUS_DELAY_COUNT;
                            uicc_connection_retry ++ );
            uicc_connection_retry = 0;
#endif
            status = phHciNfc_SWP_Get_Status(
                                psHciContext, pHwRef , NULL);
            if (NFCSTATUS_PENDING == status)
            {
                uicc_connection_retry++;
            }
        }
        else if ( FALSE == retry )
        {
            uicc_connection_retry = 0;
            HCI_DEBUG("HCI: SWP Connection Status = %u \n", uicc_status)
        }
        else
        {
            switch(uicc_status)
            {
                case UICC_CONNECTION_ONGOING:
                case UICC_DISCONNECTION_ONGOING:
                case UICC_NOT_CONNECTED:
                {
                    if(uicc_connection_retry <
                                    UICC_MAX_CONNECT_RETRY)
                    {
                        status = phHciNfc_SWP_Get_Status(
                                            psHciContext, pHwRef ,NULL);
                        if (NFCSTATUS_PENDING == status)
                        {
                            uicc_connection_retry++;
                        }
                    }
                    break;
                }
                case UICC_CONNECTION_INACTIVE:
                case UICC_CONNECTED:
                {
                    break;
                }
                case UICC_CONNECTION_LOST:
                case UICC_CONNECTION_FAILED:
                default:
                {
                    uicc_connection_retry = 0;
                    break;
                }
            } /* End of the Status Switch */
        }

        if( NFCSTATUS_PENDING != status )
        {
            uicc_connection_retry = 0;
            /* Error Scenario due to SWP Disable Config */
        }

    } /* End of Null Context Check */

    return status;
}


/*!
* \brief Release of RF Emulation Gate Configuration.
*
* This function initialses the RF Emulation Management and
* populates the Reader Management Information Structure
*
*/


NFCSTATUS
phHciNfc_EmuMgmt_Info_Sequence(
                               phHciNfc_sContext_t      *psHciContext,
                               void                 *pHwRef
                           )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
            psHciContext->p_emulation_mgmt_info ;


        if( NULL == psHciContext->p_emulation_mgmt_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {

            switch(p_emulation_mgmt_info->emulation_cur_seq )
            {
                case WI_DISABLE_EMULATION:
                case SWP_DETECTION:
                {
                    if(HCI_SESSION == psHciContext->init_mode)
                    {
                        p_emulation_mgmt_info->emulation_cur_seq =
                                 WI_GET_DEFAULT_MODE;
                    }
                    else
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                        break;
                    }
                }
                /* fall through */
                case WI_GET_DEFAULT_MODE:
                {
                    uint8_t     *p_wi_link_status =
                        &((phHal_sHwReference_t *)pHwRef)->se_if_info.wi_link_status;

                    status = phHciNfc_WI_Get_Default(
                            psHciContext, pHwRef, p_wi_link_status );

                    if (NFCSTATUS_PENDING == status)
                    {
#if ( ES_HW_VER >= 33 )
                        if(NXP_NFC_ROM_INFO(((phHal_sHwReference_t *)pHwRef)->device_info.fw_version)
                                 >= NFC44_ROM33_VER )
                        {
                           p_emulation_mgmt_info->emulation_next_seq =
                                                           WI_SMX_PRESENCE;
                        }
                        else
#endif  /* #if ( ES_HW_VER >= 33 ) */
                        {
                           p_emulation_mgmt_info->emulation_next_seq =
                                                SWP_GET_DEFAULT_MODE;
                        }
                        status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                           UPDATE_SEQ);
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
#if ( ES_HW_VER >= 33 )
                case WI_SMX_PRESENCE:
                {
                    status = phHciNfc_WI_Presence(
                           psHciContext, pHwRef );

                    if (NFCSTATUS_PENDING == status)
                    {
                        p_emulation_mgmt_info->emulation_next_seq =
                                                SWP_GET_DEFAULT_MODE;
                        status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                           UPDATE_SEQ);
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
#endif  /* #if ( ES_HW_VER >= 33 ) */
                case SWP_GET_DEFAULT_MODE:
                {
                    uint8_t     *p_swp_link_status =
                        &((phHal_sHwReference_t *)pHwRef)->se_if_info.swp_link_status;

                    status = phHciNfc_SWP_Get_Default(
                           psHciContext, pHwRef ,
                              p_swp_link_status );

                    if (NFCSTATUS_PENDING == status)
                    {
                        p_emulation_mgmt_info->emulation_next_seq =
                                                      SWP_GET_HIZ_STATE;
                        status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                             UPDATE_SEQ);
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
                case SWP_GET_HIZ_STATE:
                {
                    status = phHciNfc_DevMgmt_Get_Info(psHciContext, pHwRef,
                        NFC_ADDRESS_SWP_HIZ_STATE, &(p_emulation_mgmt_info->swp_hiz_state));
                    if (NFCSTATUS_PENDING == status)
                    {
                    }
                        p_emulation_mgmt_info->emulation_next_seq =
                                                     SWP_GET_HIZ_IO;
                        status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                                UPDATE_SEQ);
                        status = NFCSTATUS_PENDING;
                    break;
                }
                case SWP_GET_HIZ_IO:
                {
                    status = phHciNfc_DevMgmt_Get_Info(psHciContext, pHwRef,
                        NFC_ADDRESS_SWP_HIZ_IO, &(p_emulation_mgmt_info->swp_hiz_io));
                    if (NFCSTATUS_PENDING == status)
                    {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                       END_EMULATION_SEQ;
                            status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                                UPDATE_SEQ);
                    }
                    break;
                }
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                    break;
                }

            }/* End of the Sequence Switch */

        }/* End of the Reader Info Memory Check */

    } /* End of Null Context Check */

    return status;
}

/*!
* \brief Release of RF Emulation Gate Configuration.
*
* This function initialses the RF Emulation Management and
* populates the Reader Management Information Structure
*
*/


NFCSTATUS
phHciNfc_EmuMgmt_Release(
                               phHciNfc_sContext_t      *psHciContext,
                               void                 *pHwRef
                           )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t            *p_pipe_info = NULL;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
            psHciContext->p_emulation_mgmt_info ;

        if( NULL == psHciContext->p_emulation_mgmt_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            switch(p_emulation_mgmt_info->emulation_cur_seq )
            {
#if defined (HOST_EMULATION)
            case HOST_CE_A_RELEASE:
            {
                status = phHciNfc_CE_A_Release( psHciContext, pHwRef);
                break;
            }
            case HOST_CE_B_RELEASE:
            {
                status = phHciNfc_CE_B_Release( psHciContext, pHwRef);
                break;
            }
#endif

#if defined (ENABLE_WI)
                /* Enable/Disable the SmartMx Emulation through WI
                 * After the power down
                 */
                /* Enable the SmartMx Emulation by Default through WI */
                case WI_DEFAULT_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_WI_Info_t *)
                                psHciContext->p_wi_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_WI_Configure_Default( psHciContext,
                            pHwRef, p_emulation_mgmt_info->smx_powerless );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                    WI_DISABLE_EMULATION;
                        }
                    }
                    break;
                }
                /* SmartMx In Default Mode */
                case WI_DISABLE_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_WI_Info_t *)
                                psHciContext->p_wi_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_WI_Configure_Mode( psHciContext,
                            pHwRef, eSmartMx_Default );
                        if(status == NFCSTATUS_SUCCESS )
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                        SWP_DISABLE_EMULATION;
                            status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                                UPDATE_SEQ);
                            /* status = NFCSTATUS_PENDING; */
                        }
                    }
                    break;
                }
                /* Enable/Disable the UICC Emulation through SWP
                 * After the power down
                 */
                /* Enable the UICC Emulation by Default through SWP */
                case SWP_DEFAULT_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_SWP_Configure_Default( psHciContext,
                            pHwRef, p_emulation_mgmt_info->uicc_powerless );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                            SWP_DISABLE_EMULATION;
                            /* status = NFCSTATUS_SUCCESS; */
                        }
                    }
                    break;
                }
#endif /* #if defined (ENABLE_WI) */
                /* Disable the UICC Emulation through SWP */
                case SWP_DISABLE_EMULATION:
                {
                    p_pipe_info = ((phHciNfc_SWP_Info_t *)
                        psHciContext->p_swp_info)->p_pipe_info;
                    if(NULL == p_pipe_info )
                    {
                        status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
                    }
                    else
                    {
                        status = phHciNfc_SWP_Configure_Mode( psHciContext,
                            pHwRef, UICC_SWITCH_MODE_DEFAULT );
                        if(status == NFCSTATUS_PENDING)
                        {
                            p_emulation_mgmt_info->emulation_next_seq =
                                                CONFIG_DEFAULT_EMULATION;
                            status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                                UPDATE_SEQ);
                            status = NFCSTATUS_SUCCESS;
                        }
                    }
                    break;
                }
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                    break;
                }

            }/* End of the Sequence Switch */

        }/* End of the Reader Info Memory Check */

    } /* End of Null Context Check */

    return status;
}

#if 0
NFCSTATUS
phHciNfc_Emulation_Start (
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;

    return status;
}
#endif

NFCSTATUS
phHciNfc_Emulation_Send (
                        phHciNfc_sContext_t             *psHciContext,
                        void                            *pHwRef,
                        phHal_sRemoteDevInformation_t   *p_remote_dev_info,
                        phHciNfc_XchgInfo_t             *p_send_param
                        )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;

    PHNFC_UNUSED_VARIABLE(p_remote_dev_info);

    psHciContext->p_xchg_info = p_send_param;
    switch (psHciContext->host_rf_type)
    {
        case phHal_eISO14443_A_PICC:
        case phHal_eISO14443_4A_PICC:
        {
#ifdef HOST_EMULATION
            status = phHciNfc_CE_A_Send( psHciContext,
                           pHwRef,  p_send_param );
#endif /* #ifdef HOST_EMULATION */
            break;
        }
        case phHal_eISO14443_B_PICC:
        case phHal_eISO14443_4B_PICC:
        {
#ifdef HOST_EMULATION
            status = phHciNfc_CE_B_Send( psHciContext,
                           pHwRef,  p_send_param );
#endif /* #ifdef HOST_EMULATION */
            break;
        }
        default:
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
            break;
        }
    }

    return status;
}


NFCSTATUS
phHciNfc_Emulation_Cfg (
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_eConfigType_t  cfg_type
                       )
{
    NFCSTATUS                       status = NFCSTATUS_SUCCESS;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;
    phHal_sEmulationCfg_t           *p_emulation_cfg = NULL;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if ( ( NULL == psHciContext->p_emulation_mgmt_info )
             || ( NULL == psHciContext->p_config_params ) )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_INFORMATION);
    }
    else
    {
        p_emulation_mgmt_info = (phHciNfc_EmulationMgmt_Info_t *)
            psHciContext->p_emulation_mgmt_info ;
        p_emulation_cfg = psHciContext->p_config_params;
        switch(cfg_type)
        {
            case SMX_WI_CFG:
            {
                phHal_sSmartMX_Cfg_t   *smx_config =
                                        &p_emulation_cfg->config.smartMxCfg;
                p_emulation_mgmt_info->smx_powerless =
                                        (uint8_t)(FALSE != smx_config->lowPowerMode );
                status = phHciNfc_WI_Configure_Default( psHciContext, pHwRef,
                                                smx_config->enableEmulation );
                break;
            }
            case UICC_SWP_CFG:
            {
#ifdef SWP_CFG_SEQ
                phHal_sUiccEmuCfg_t   *uicc_config =
                                    &p_emulation_cfg->config.uiccEmuCfg;
                p_emulation_mgmt_info->uicc_powerless =
                                        (uint8_t)(FALSE != uicc_config->lowPowerMode );
                {
#ifdef SWP_EVENT_USAGE
                    status = phHciNfc_SWP_Configure_Mode( psHciContext, pHwRef,
                        ((TRUE == uicc_config->enableUicc)? /* UICC_SWITCH_MODE_DEFAULT */
                        UICC_SWITCH_MODE_ON :UICC_SWITCH_MODE_OFF));
#else
                    status = phHciNfc_SWP_Configure_Default( psHciContext, pHwRef,
                                                uicc_config->enableUicc );
#endif
                }
#else
                status = phHciNfc_SWP_Config_Sequence( psHciContext,
                                                        pHwRef, p_emulation_cfg);
#endif
                break;
            }
            case SWP_EVT_CFG:
            {
                phHal_sUiccEmuCfg_t   *uicc_config =
                                    &p_emulation_cfg->config.uiccEmuCfg;
                p_emulation_mgmt_info->uicc_powerless =
                                        (uint8_t)(FALSE != uicc_config->lowPowerMode );
                {
                    status = phHciNfc_SWP_Configure_Mode( psHciContext, pHwRef,
                        ((TRUE == uicc_config->enableUicc)? /* UICC_SWITCH_MODE_DEFAULT */
                        UICC_SWITCH_MODE_ON :UICC_SWITCH_MODE_DEFAULT));
                }
                break;
            }
#ifdef HOST_EMULATION
            case NFC_CE_A_CFG:
            {
                phHal_sHostEmuCfg_A_t *host_ce_a_cfg =
                        &p_emulation_cfg->config.hostEmuCfg_A;
                if(host_ce_a_cfg->enableEmulation == TRUE )
                {
                    if((NXP_NFC_ROM_INFO(((phHal_sHwReference_t *)pHwRef)->device_info.fw_version)
                       >= NFC44_ROM33_VER ))
                    {
                       status = phHciNfc_CE_A_Config(psHciContext, pHwRef ,host_ce_a_cfg);
                    }
                    else
                    {
                       /* status = phHciNfc_CE_A_Initialise( psHciContext, pHwRef); */
                       status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED );
                    }
                }
                else
                {
                    status = phHciNfc_CE_A_Mode( psHciContext, pHwRef ,
                                                    HOST_CE_MODE_DISABLE);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
            case NFC_CE_B_CFG:
            {
                phHal_sHostEmuCfg_B_t *host_ce_b_cfg =
                        &p_emulation_cfg->config.hostEmuCfg_B;
                if(host_ce_b_cfg->enableEmulation == TRUE )
                {
                    if((NXP_NFC_ROM_INFO(((phHal_sHwReference_t *)pHwRef)->device_info.fw_version)
                       >= NFC44_ROM33_VER ))
                    {
                       status = phHciNfc_CE_B_Config(psHciContext, pHwRef ,host_ce_b_cfg);
                    }
                    else
                    {
                       /* status = phHciNfc_CE_B_Initialise( psHciContext, pHwRef); */
                       status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED );
                    }
                }
                else
                {
                    status = phHciNfc_CE_B_Mode( psHciContext, pHwRef ,
                                                    HOST_CE_MODE_DISABLE );
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
#endif /* HOST_EMULATION */
            /* case INVALID_CFG:
            case POLL_LOOP_CFG: */
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_INFORMATION);
                break;
            }

        } /* End of the Configuration Switch */
    }

    return status;
}


static
NFCSTATUS
phHciNfc_Recv_Uicc_Cmd (
                        void                *psContext,
                        void                *pHwRef,
                        uint8_t             *pCmd,
                        uint16_t            length
                     )
{
    uint8_t                     pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    uint8_t                     cmd = (uint8_t) HCP_MSG_INSTRUCTION_INVALID;
    uint8_t                     response = (uint8_t) ANY_OK;
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_sContext_t         *psHciContext =
                                    (phHciNfc_sContext_t *)psContext ;
    phHciNfc_HCP_Packet_t       *hcp_packet = NULL;
    phHciNfc_HCP_Message_t      *hcp_message = NULL;
    phHciNfc_Pipe_Info_t            *p_pipe_info = NULL;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info=NULL;

    if( (NULL == psHciContext)
        || (NULL == pHwRef)
        || (HCP_HEADER_LEN > length )
      )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        hcp_packet = (phHciNfc_HCP_Packet_t *)pCmd;
        hcp_message = &hcp_packet->msg.message;
        p_emulation_mgmt_info = psHciContext->p_emulation_mgmt_info;

        /* Get the Command instruction bits from the Message Header */
        cmd = (uint8_t) GET_BITS8( hcp_message->msg_header,
            HCP_MSG_INSTRUCTION_OFFSET, HCP_MSG_INSTRUCTION_LEN);
        pipe_id = p_emulation_mgmt_info->uicc_id;
        p_pipe_info = psHciContext->p_pipe_list[pipe_id];

        switch( cmd )
        {
            /* These are Commands are sent from the UICC Controller */
            case ANY_OPEN_PIPE:
            {
                p_emulation_mgmt_info->uicc_enable = TRUE ;
                break;
            }
            case ANY_CLOSE_PIPE:
            {
                if(TRUE != p_emulation_mgmt_info->uicc_enable)
                {
                    response = ANY_E_PIPE_NOT_OPENED;
                    /* status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FAILED); */
                }
                else
                {
                    p_emulation_mgmt_info->uicc_enable = FALSE;
                }
                break;
            }
            case ANY_SET_PARAMETER:
            case ANY_GET_PARAMETER:
            case PRO_HOST_REQUEST:
            {
                response = ANY_E_CMD_NOT_SUPPORTED;
                /* status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_COMMAND_NOT_SUPPORTED);*/
                break;
            }
            default:
            {
                response = ANY_E_NOK;
                /* status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_COMMAND); */
                break;
            }
        }
        hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
        phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                                pipe_id, HCP_MSG_TYPE_RESPONSE, response );
        psHciContext->tx_total = HCP_HEADER_LEN;
        status = phHciNfc_Send_HCP( (void *)psHciContext, (void *)pHwRef );

        p_pipe_info->recv_msg_type = HCP_MSG_TYPE_COMMAND;
        p_pipe_info->sent_msg_type = HCP_MSG_TYPE_RESPONSE;
        p_pipe_info->prev_msg = response ;
        p_pipe_info->prev_status = status;
        status = NFCSTATUS_SUCCESS;

    }
    return status;
}

static
NFCSTATUS
phHciNfc_Recv_Uicc_Event (
                        void                *psContext,
                        void                *pHwRef,
                        uint8_t             *pEvent,
                        uint16_t            length
                     )
{
    uint8_t                     event = (uint8_t) HCP_MSG_INSTRUCTION_INVALID;
    uint32_t                     i = 0;
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_sContext_t         *psHciContext =
                                    (phHciNfc_sContext_t *)psContext ;
    phHciNfc_HCP_Packet_t           *hcp_packet = NULL;
    phHciNfc_HCP_Message_t          *hcp_message = NULL;
    phHal_sEventInfo_t              event_info;
    phHciNfc_EmulationMgmt_Info_t   *p_emulation_mgmt_info =
                                        psHciContext->p_emulation_mgmt_info ;


    if( (NULL == p_emulation_mgmt_info)
         || ( TRUE !=  p_emulation_mgmt_info->uicc_enable)
       )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        hcp_packet = (phHciNfc_HCP_Packet_t *)pEvent;
        hcp_message = &hcp_packet->msg.message;

        /* Get the Event instruction bits from the Message Header */
        event = (uint8_t) GET_BITS8( hcp_message->msg_header,
            HCP_MSG_INSTRUCTION_OFFSET, HCP_MSG_INSTRUCTION_LEN);
        event_info.eventHost = phHal_eUICCHost ;
        event_info.eventSource = phHal_ePICC_DevType ;

        switch( event )
        {
            case EVT_END_OF_TRANSACTION:
            {
                event_info.eventType = NFC_EVT_END_OF_TRANSACTION;
                break;
            }
            case EVT_TRANSACTION:
            {
               if(length > HCP_HEADER_LEN + TRANSACTION_MIN_LEN)
               {
                    event_info.eventType = NFC_EVT_TRANSACTION;

                    for(;i<(length-HCP_HEADER_LEN);)
                    {
                        switch (hcp_message->payload[i])
                        {
                            case TRANSACTION_AID:
                            {
                                /* AID LENGTH INDEX */
                                i++;
                                /* Fill the event_info.eventInfo.aid
                                * Structure with the Received Transaction AID.
                                */
                                event_info.eventInfo.aid.length =
                                                            hcp_message->payload[i++];
                                (void) memcpy((void *)p_emulation_mgmt_info->uicc_aid,
                                        &(hcp_message->payload[i]),
                                                event_info.eventInfo.aid.length );
                                event_info.eventInfo.aid.buffer = (uint8_t *)
                                                p_emulation_mgmt_info->uicc_aid;
                                i = i + event_info.eventInfo.aid.length;
                                break;
                            }
                            case TRANSACTION_PARAM:
                            {
#define LENGTH_ENCODING_MASK  0x80
                                /* Parameter Length Index */
                                if(hcp_message->payload[++i] & LENGTH_ENCODING_MASK)
                                {
                                  i = i+(hcp_message->payload[i] - LENGTH_ENCODING_MASK);
                                }
                                /* Fill the event_info.eventInfo.param
                                * Structure with the Received Parameter.
                                */
                                p_emulation_mgmt_info->uicc_param_len =
                                                            hcp_message->payload[i++];
                                (void) memcpy((void *)p_emulation_mgmt_info->uicc_param,
                                        &(hcp_message->payload[i]),
                                                p_emulation_mgmt_info->uicc_param_len );
                                event_info.eventInfo.uicc_info.param.length =
                                                p_emulation_mgmt_info->uicc_param_len;
                                event_info.eventInfo.uicc_info.param.buffer = (uint8_t *)
                                                p_emulation_mgmt_info->uicc_param;
                                i = i + event_info.eventInfo.uicc_info.param.length;
                                break;
                            }
                            default:
                            {

                                status = PHNFCSTVAL( CID_NFC_HCI,
                                                    NFCSTATUS_FEATURE_NOT_SUPPORTED );
                                i = length;
                                HCI_DEBUG("%s: Statement Should Not Occur \n",
                                                        "phHciNfc_Recv_Uicc_Event");
                                break;
                            }
                        } /* End of Transaction Switch */
                    }
               }
               break;
            }
            case EVT_CONNECTIVITY:
            {
                event_info.eventType = NFC_EVT_CONNECTIVITY;
                break;
            }
            case EVT_OPERATION_ENDED:
            {
                event_info.eventType = NFC_EVT_OPERATION_ENDED;
                break;
            }
            default:
            {
                status = PHNFCSTVAL( CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED );
                HCI_DEBUG("%s: Statement Should Not Occur \n","phHciNfc_Recv_Uicc_Event");
                break;
            }
        }
        if ( NFCSTATUS_SUCCESS == status )
        {
            phHciNfc_Notify_Event( psHciContext, pHwRef,
                    NFC_NOTIFY_EVENT, (void *)&event_info );
        }
    }
    return status;
}
