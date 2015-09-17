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
* \file  phHciNfc_CEB.c                                                       *
* \brief HCI card emulation A management routines.                            *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Fri Aug 21 18:35:05 2009 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.13 $                                                           *
* $Aliases:  $
*                                                                             *
 *
 * $Developed by : Ravindra U   $                                              *
 *
* =========================================================================== *
*/

/*
***************************** Header File Inclusion ****************************
*/
#include <phNfcCompId.h>
#include <phNfcHalTypes.h>
#include <phHciNfc_Pipe.h>
#include <phHciNfc_Emulation.h>
#include <phOsalNfc.h>
/*
****************************** Macro Definitions *******************************
*/
#if defined (HOST_EMULATION)
#include <phHciNfc_CE_B.h>

#define CE_B_EVT_NFC_SEND_DATA               0x10U
#define CE_B_EVT_NFC_FIELD_ON                0x11U
#define CE_B_EVT_NFC_DEACTIVATED             0x12U
#define CE_B_EVT_NFC_ACTIVATED               0x13U
#define CE_B_EVT_NFC_FIELD_OFF               0x14U

/*
*************************** Structure and Enumeration ***************************
*/


/*
*************************** Static Function Declaration **************************
*/

static
NFCSTATUS
phHciNfc_Recv_CE_B_Event(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pEvent,
                             uint16_t           length
                       );

static
NFCSTATUS
phHciNfc_Recv_CE_B_Response(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pResponse,
                             uint16_t           length
                       );

static
NFCSTATUS
phHciNfc_CE_B_ProcessData(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef,
                            uint8_t                 *pData,
                            uint16_t                 length
                       );


static
NFCSTATUS
phHciNfc_CE_B_Send_Data(
                           phHciNfc_sContext_t      *psHciContext,
                           void                     *pHwRef,
                           uint8_t                  pipe_id
                       );


/*
*************************** Function Definitions ***************************
*/
NFCSTATUS
phHciNfc_CE_B_Init_Resources(
                                phHciNfc_sContext_t     *psHciContext
                         )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_CE_B_Info_t        *ps_ce_b_info = NULL;
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if(
            ( NULL == psHciContext->p_ce_b_info ) &&
             (phHciNfc_Allocate_Resource((void **)(&ps_ce_b_info),
            sizeof(phHciNfc_CE_B_Info_t))== NFCSTATUS_SUCCESS)
          )
        {
            psHciContext->p_ce_b_info = ps_ce_b_info;
            ps_ce_b_info->current_seq = HOST_CE_B_INVALID_SEQ;
            ps_ce_b_info->next_seq = HOST_CE_B_INVALID_SEQ;
            ps_ce_b_info->pipe_id = (uint8_t)HCI_UNKNOWN_PIPE_ID;
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
        }

    }
    return status;
}

NFCSTATUS
phHciNfc_CE_B_Initialise(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    NFCSTATUS           status = NFCSTATUS_SUCCESS;
    static uint8_t      pupi[] = {0, 0, 0, 0};
#define ISO1443_4B_ADD_ATQB_INFO_SIZE 0x04
    static uint8_t      atqb_info[ISO1443_4B_ADD_ATQB_INFO_SIZE] = {0x04, 0x00, 0x00, 0x00};

    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t        *ps_ce_b_info = ((phHciNfc_CE_B_Info_t *)
                                                psHciContext->p_ce_b_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_b_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            switch(ps_ce_b_info->current_seq)
            {
                case HOST_CE_B_PIPE_OPEN:
                {
                    status = phHciNfc_Open_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
#if defined (CE_B_CONTINUE_SEQ)
                        ps_ce_b_info->next_seq = HOST_CE_B_ENABLE_SEQ;
#else
                        ps_ce_b_info->next_seq = HOST_CE_B_ATQB_SEQ;
#endif /* #if defined (CE_CONTINUE_SEQ) */
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
                case HOST_CE_B_PUPI_SEQ:
                {
                    /* HOST Card Emulation B PUPI Configuration */
                    ps_pipe_info->reg_index = HOST_CE_B_PUPI_INDEX;

                    ps_pipe_info->param_info =(void*)&pupi ;
                    ps_pipe_info->param_length = sizeof(pupi) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_ATQB_SEQ;
                    }
                    break;
                }
                case HOST_CE_B_ATQB_SEQ:
                {
                    /* HOST Card Emulation B ATQB Configuration */
                    ps_pipe_info->reg_index = HOST_CE_B_ATQB_INDEX;
                    /* Configure the ATQA of Host Card Emulation B */
                    ps_pipe_info->param_info = (void*)atqb_info ;
                    ps_pipe_info->param_length = sizeof(atqb_info) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_ENABLE_SEQ;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
                case HOST_CE_B_ENABLE_SEQ:
                {
                    status = phHciNfc_CE_B_Mode( psHciContext,
                                            pHwRef, HOST_CE_MODE_ENABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_DISABLE_SEQ;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
                default :
                {
                    break;
                }
            }
        }
    }

    return status;
}

NFCSTATUS
phHciNfc_CE_B_Release(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    NFCSTATUS       status = NFCSTATUS_SUCCESS;
    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t        *ps_ce_b_info = ((phHciNfc_CE_B_Info_t *)
                                                psHciContext->p_ce_b_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_b_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            switch(ps_ce_b_info->current_seq)
            {
                case HOST_CE_B_DISABLE_SEQ:
                {
                    status = phHciNfc_CE_B_Mode( psHciContext,
                                            pHwRef,  HOST_CE_MODE_DISABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_PIPE_CLOSE;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
                case HOST_CE_B_PIPE_CLOSE:
                {
                    /* HOST Card Emulation B pipe close sequence */
                    status = phHciNfc_Close_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_PIPE_DELETE;
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
                case HOST_CE_B_PIPE_DELETE:
                {
                    /* HOST Card Emulation A pipe delete sequence */
                    status = phHciNfc_Delete_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_PIPE_OPEN;
                    }
                    break;
                }
                default :
                {
                    break;
                }
            }
        }
    }
    return status;
}


NFCSTATUS
phHciNfc_CE_B_Config(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHal_sHostEmuCfg_B_t   *pCfg
                        )
{
    NFCSTATUS           status = NFCSTATUS_SUCCESS;
    static uint8_t      pupi[] = {0, 0, 0, 0};
#define ISO1443_4B_ADD_ATQB_INFO_SIZE 0x04
    static uint8_t      atqb_add_info[ISO1443_4B_ADD_ATQB_INFO_SIZE] = {0x04, 0x00, 0x00, 0x00};
    uint8_t             i = 0;

    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t        *ps_ce_b_info = ((phHciNfc_CE_B_Info_t *)
                                                psHciContext->p_ce_b_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_b_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            switch(ps_ce_b_info->current_seq)
            {
                case HOST_CE_B_PUPI_SEQ:
                {
                    /* HOST Card Emulation B PUPI Configuration */
                    ps_pipe_info->reg_index = HOST_CE_B_PUPI_INDEX;

                    ps_pipe_info->param_info =(void*)pupi ;
                    ps_pipe_info->param_length = sizeof(pupi) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_AFI_SEQ;
                    }
                    break;
                }
                case HOST_CE_B_AFI_SEQ:
                {
                    /* HOST Card Emulation B PUPI Configuration */
                    ps_pipe_info->reg_index = HOST_CE_B_AFI_INDEX;

                    ps_pipe_info->param_info = (void*) (&pCfg->hostEmuCfgInfo.Afi);
                    ps_pipe_info->param_length = sizeof(pCfg->hostEmuCfgInfo.Afi);

                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_ATQB_SEQ;
                    }
                    break;
                }
                case HOST_CE_B_ATQB_SEQ:
                {
                    i = 0;
                    /* HOST Card Emulation B ATQB Configuration */
                    ps_pipe_info->reg_index = HOST_CE_B_ATQB_INDEX;
                    /* Configure the ATQA of Host Card Emulation B */
                    atqb_add_info[i++]=pCfg->hostEmuCfgInfo.AtqB.AtqResInfo.AppData[1];
                    atqb_add_info[i++]=pCfg->hostEmuCfgInfo.AtqB.AtqResInfo.AppData[2];
                    atqb_add_info[i++]=pCfg->hostEmuCfgInfo.AtqB.AtqResInfo.AppData[3];
                    atqb_add_info[i++]=pCfg->hostEmuCfgInfo.AtqB.AtqResInfo.ProtInfo[2];
                    ps_pipe_info->param_info = (void*)atqb_add_info ;
                    ps_pipe_info->param_length = sizeof(atqb_add_info) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_HLR_SEQ;
                    }
                    break;
                }
                case HOST_CE_B_HLR_SEQ:
                {
                    /* HOST Card Emulation B ATQB Configuration */
                    ps_pipe_info->reg_index = HOST_CE_B_HIGH_LAYER_RESP_INDEX;
                    /* Configure the ATQA of Host Card Emulation B */
                    ps_pipe_info->param_info = (void*)pCfg->hostEmuCfgInfo.HiLayerResp;
                    ps_pipe_info->param_length = pCfg->hostEmuCfgInfo.HiLayerRespLength;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
#if defined (NXP_NFC_CE_B_MAX_DR_SUPPORT)
                        ps_ce_b_info->next_seq = HOST_CE_B_MAX_DR_SEQ;
#else
                        ps_ce_b_info->next_seq = HOST_CE_B_ENABLE_SEQ;
#endif
                    }
                    break;
                }
#if defined (NXP_NFC_CE_B_MAX_DR_SUPPORT)
                case HOST_CE_B_MAX_DR_SEQ:
                {
                      /* HOST Card Emulation B ATQB Configuration */
                      ps_pipe_info->reg_index = HOST_CE_B_DATA_RATE_MAX_INDEX;
                      /* Configure the ATQA of Host Card Emulation B */
                      ps_pipe_info->param_info = (void*)&pCfg->hostEmuCfgInfo.MaxDataRate;
                      ps_pipe_info->param_length = sizeof(pCfg->hostEmuCfgInfo.MaxDataRate);
                      status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                  ps_pipe_info->pipe.pipe_id,
                                                  (uint8_t)ANY_SET_PARAMETER);
                      if(status == NFCSTATUS_PENDING)
                      {
                          ps_ce_b_info->next_seq = HOST_CE_B_ENABLE_SEQ;
                      }
                      break;
                }
#endif
                case HOST_CE_B_ENABLE_SEQ:
                {
                    status = phHciNfc_CE_B_Mode( psHciContext,
                                            pHwRef, HOST_CE_MODE_ENABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_b_info->next_seq = HOST_CE_B_DISABLE_SEQ;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
                default :
                {
                    break;
                }
            }
        }
    }

    return status;
}

NFCSTATUS
phHciNfc_CE_B_Update_PipeInfo(
                                  phHciNfc_sContext_t     *psHciContext,
                                  uint8_t                 pipeID,
                                  phHciNfc_Pipe_Info_t    *pPipeInfo
                                  )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t         *ps_ce_b_info=NULL;
        ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                        psHciContext->p_ce_b_info ;

        ps_ce_b_info->current_seq = HOST_CE_B_PIPE_OPEN;
        ps_ce_b_info->next_seq = HOST_CE_B_PIPE_OPEN;
        /* Update the pipe_id of the card emulation A Gate o
            btained from the HCI Response */
        ps_ce_b_info->pipe_id = pipeID;
        if (HCI_UNKNOWN_PIPE_ID != pipeID)
        {
            ps_ce_b_info->p_pipe_info = pPipeInfo;
            if (NULL != pPipeInfo)
            {
                /* Update the Response Receive routine of the card
                    emulation A Gate */
                pPipeInfo->recv_resp = &phHciNfc_Recv_CE_B_Response;
                /* Update the event Receive routine of the card emulation A Gate */
                pPipeInfo->recv_event = &phHciNfc_Recv_CE_B_Event;
            }
        }
        else
        {
            ps_ce_b_info->p_pipe_info = NULL;
            phOsalNfc_FreeMemory((void *)ps_ce_b_info);
            psHciContext->p_ce_b_info = NULL;
        }
    }

    return status;
}



NFCSTATUS
phHciNfc_CE_B_Get_PipeID(
                             phHciNfc_sContext_t        *psHciContext,
                             uint8_t                    *ppipe_id
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( (NULL != psHciContext)
        && ( NULL != ppipe_id )
        && ( NULL != psHciContext->p_ce_b_info )
        )
    {
        phHciNfc_CE_B_Info_t     *ps_ce_b_info=NULL;
        ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                        psHciContext->p_ce_b_info ;
        *ppipe_id =  ps_ce_b_info->pipe_id  ;
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    return status;
}


NFCSTATUS
phHciNfc_CE_B_Send (
                        phHciNfc_sContext_t             *psHciContext,
                        void                            *pHwRef,
                        phHciNfc_XchgInfo_t             *p_send_param
                        )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    if((NULL == psHciContext)||(NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t     *ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                                                psHciContext->p_ce_b_info ;
        phHciNfc_Pipe_Info_t     *p_pipe_info = ps_ce_b_info->p_pipe_info;

        if (NULL != p_pipe_info)
        {
            p_pipe_info->param_info =(void*)p_send_param->tx_buffer ;
            p_pipe_info->param_length = p_send_param->tx_length  ;
            status = phHciNfc_CE_B_Send_Data(psHciContext,pHwRef,
                                    ps_ce_b_info->pipe_id);
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_HCI_GATE_NOT_SUPPORTED);
        }
    }

    return status;
}

static
NFCSTATUS
phHciNfc_CE_B_Send_Data(
                           phHciNfc_sContext_t      *psHciContext,
                           void                     *pHwRef,
                           uint8_t                  pipe_id
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_HCP_Packet_t   *hcp_packet = NULL;
    phHciNfc_HCP_Message_t  *hcp_message = NULL;
    phHciNfc_Pipe_Info_t    *p_pipe_info = NULL;
    uint8_t                 length = 0;
    uint8_t                 i=0;

    p_pipe_info = (phHciNfc_Pipe_Info_t *)
                    psHciContext->p_pipe_list[pipe_id];
    psHciContext->tx_total = 0 ;
    length = (length + HCP_HEADER_LEN);

    hcp_packet = (phHciNfc_HCP_Packet_t *) psHciContext->send_buffer;
    /* Construct the HCP Frame */
    phHciNfc_Build_HCPFrame(hcp_packet,HCP_CHAINBIT_DEFAULT,
                            (uint8_t) pipe_id,
                            HCP_MSG_TYPE_EVENT, CE_B_EVT_NFC_SEND_DATA);

    hcp_message = &(hcp_packet->msg.message);

    phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                            i,
                            (uint8_t *)p_pipe_info->param_info,
                            p_pipe_info->param_length);
    length = (uint8_t)(length + i + p_pipe_info->param_length);

    p_pipe_info->sent_msg_type = HCP_MSG_TYPE_EVENT ;
    p_pipe_info->prev_msg = CE_B_EVT_NFC_SEND_DATA ;
    psHciContext->tx_total = length;

    /* Send the Constructed HCP packet to the lower layer */
    status = phHciNfc_Send_HCP( psHciContext, pHwRef );
    if(NFCSTATUS_PENDING == status)
    {
        p_pipe_info->prev_status = status;
    }

    return status;
}


static
NFCSTATUS
phHciNfc_Recv_CE_B_Response(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pResponse,
                             uint16_t           length
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_sContext_t         *psHciContext =
                                (phHciNfc_sContext_t *)psContext ;
    if( (NULL == psHciContext) || (NULL == pHwRef) || (NULL == pResponse)
        || (length == 0))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t         *ps_ce_b_info=NULL;
        uint8_t                     prev_cmd = ANY_GET_PARAMETER;
        ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                            psHciContext->p_ce_b_info ;
        if( NULL == ps_ce_b_info->p_pipe_info)
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_INVALID_HCI_SEQUENCE);
        }
        else
        {
            prev_cmd = ps_ce_b_info->p_pipe_info->prev_msg ;
            switch(prev_cmd)
            {
                case ANY_GET_PARAMETER:
                {
#if 0
                    status = phHciNfc_CE_B_InfoUpdate(psHciContext,
                                    ps_ce_b_info->p_pipe_info->reg_index,
                                    &pResponse[HCP_HEADER_LEN],
                                    (length - HCP_HEADER_LEN));
#endif /* #if 0 */
                    break;
                }
                case ANY_SET_PARAMETER:
                {
                    HCI_PRINT("CE B Parameter Set \n");
                    break;
                }
                case ANY_OPEN_PIPE:
                {
                    HCI_PRINT("CE B open pipe complete\n");
                    break;
                }
                case ANY_CLOSE_PIPE:
                {
                    HCI_PRINT("CE B close pipe complete\n");
                    break;
                }
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_RESPONSE);
                    break;
                }
            }
            if( NFCSTATUS_SUCCESS == status )
            {
                status = phHciNfc_CE_B_Update_Seq(psHciContext,
                                                    UPDATE_SEQ);
                ps_ce_b_info->p_pipe_info->prev_status = NFCSTATUS_SUCCESS;
                if (NFCSTATUS_SUCCESS == status)
                {
                    status = phHciNfc_EmuMgmt_Update_Seq(psHciContext,
                                                        UPDATE_SEQ);
                }
            }
        }
    }
    return status;
}

NFCSTATUS
phHciNfc_CE_B_Update_Seq(
                        phHciNfc_sContext_t     *psHciContext,
                        phHciNfc_eSeqType_t     seq_type
                    )
{
    phHciNfc_CE_B_Info_t    *ps_ce_b_info=NULL;
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if( NULL == psHciContext->p_ce_b_info )
    {
        status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                        psHciContext->p_ce_b_info ;
        switch(seq_type)
        {
            case RESET_SEQ:
            case INIT_SEQ:
            {
                ps_ce_b_info->next_seq = HOST_CE_B_PIPE_OPEN;
                ps_ce_b_info->current_seq = HOST_CE_B_PIPE_OPEN;
                break;
            }
            case UPDATE_SEQ:
            {
                ps_ce_b_info->current_seq = ps_ce_b_info->next_seq;
                break;
            }
            case INFO_SEQ:
            {
                break;
            }
            case CONFIG_SEQ:
            {
                ps_ce_b_info->current_seq = HOST_CE_B_AFI_SEQ;
                ps_ce_b_info->next_seq = HOST_CE_B_AFI_SEQ;
                break;
            }
            case REL_SEQ:
            {
                ps_ce_b_info->next_seq = HOST_CE_B_DISABLE_SEQ;
                ps_ce_b_info->current_seq = HOST_CE_B_DISABLE_SEQ;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return status;
}

NFCSTATUS
phHciNfc_CE_B_Mode(
                            void        *psHciHandle,
                            void        *pHwRef,
                            uint8_t     enable_type
                  )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    static uint8_t          param = 0 ;
    phHciNfc_sContext_t     *psHciContext = ((phHciNfc_sContext_t *)psHciHandle);

    if((NULL == psHciContext)||(NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_B_Info_t     *ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                                                psHciContext->p_ce_b_info ;
        phHciNfc_Pipe_Info_t     *p_pipe_info = ps_ce_b_info->p_pipe_info;

        if (NULL != p_pipe_info)
        {
            p_pipe_info->reg_index = HOST_CE_B_MODE_INDEX;
            /* Enable/Disable Host Card Emulation A */
            param = (uint8_t)enable_type;
            p_pipe_info->param_info =(void*)&param ;
            p_pipe_info->param_length = sizeof(param) ;
            status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                    ps_ce_b_info->pipe_id,(uint8_t)ANY_SET_PARAMETER);
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_HCI_GATE_NOT_SUPPORTED);
        }
    }
    return status;
}


static
NFCSTATUS
phHciNfc_Recv_CE_B_Event(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pEvent,
                             uint16_t           length
                       )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_sContext_t         *psHciContext =
                                (phHciNfc_sContext_t *)psContext ;
    if( (NULL == psHciContext) || (NULL == pHwRef) || (NULL == pEvent)
        || (length == 0))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_b_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_HCP_Packet_t       *p_packet = NULL;
        phHciNfc_CE_B_Info_t         *ps_ce_b_info=NULL;
        phHciNfc_HCP_Message_t      *message = NULL;
        static phHal_sEventInfo_t   event_info;
        uint8_t                     instruction=0;

        ps_ce_b_info = (phHciNfc_CE_B_Info_t *)
                        psHciContext->p_ce_b_info ;

        /* Variable was set but never used (ARM warning) */
        PHNFC_UNUSED_VARIABLE(ps_ce_b_info);

        p_packet = (phHciNfc_HCP_Packet_t *)pEvent;
        message = &p_packet->msg.message;
        /* Get the instruction bits from the Message Header */
        instruction = (uint8_t) GET_BITS8( message->msg_header,
                                HCP_MSG_INSTRUCTION_OFFSET,
                                HCP_MSG_INSTRUCTION_LEN);
        psHciContext->host_rf_type = phHal_eISO14443_B_PICC;
        event_info.eventHost = phHal_eHostController;
        event_info.eventSource = phHal_eISO14443_B_PICC;
        switch(instruction)
        {
            case CE_B_EVT_NFC_ACTIVATED:
            {
                event_info.eventType = NFC_EVT_ACTIVATED;
                /* Notify to the HCI Generic layer To Update the FSM */
                phHciNfc_Notify_Event(psHciContext, pHwRef,
                                    NFC_NOTIFY_DEVICE_ACTIVATED,
                                    &event_info);
                status = NFCSTATUS_PENDING;
                break;
            }
            case CE_B_EVT_NFC_DEACTIVATED:
            {
                event_info.eventType = NFC_EVT_DEACTIVATED;
                HCI_PRINT("CE B Target Deactivated\n");
                phHciNfc_Notify_Event(psHciContext, pHwRef,
                                    NFC_NOTIFY_DEVICE_DEACTIVATED,
                                    &event_info);
                status = NFCSTATUS_PENDING;
                break;
            }
            case CE_B_EVT_NFC_SEND_DATA:
            {
                HCI_PRINT("CE B data is received from the PN544\n");
                if(length > HCP_HEADER_LEN)
                {
                    status = phHciNfc_CE_B_ProcessData(
                                            psHciContext, pHwRef,
                                            &pEvent[HCP_HEADER_LEN],
                                            (length - HCP_HEADER_LEN));
                }
                else
                {
                    status = PHNFCSTVAL(CID_NFC_HCI,
                                        NFCSTATUS_INVALID_HCI_RESPONSE);
                }
                break;
            }
            case CE_B_EVT_NFC_FIELD_ON:
            {
                HCI_PRINT("CE B field on\n");
                event_info.eventType = NFC_EVT_FIELD_ON;
                break;
            }
            case CE_B_EVT_NFC_FIELD_OFF:
            {
                HCI_PRINT("CE B field off\n");
                event_info.eventType = NFC_EVT_FIELD_OFF;
                break;
            }
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI,
                                    NFCSTATUS_INVALID_HCI_INSTRUCTION);
                break;
            }
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            phHciNfc_Notify_Event(psHciContext, pHwRef,
                                    NFC_NOTIFY_EVENT,
                                    &event_info);
        }
    }
    return status;
}

static
NFCSTATUS
phHciNfc_CE_B_ProcessData(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef,
                            uint8_t                 *pData,
                            uint16_t                 length
                       )
{
    NFCSTATUS                          status = NFCSTATUS_SUCCESS;
    static phNfc_sTransactionInfo_t    transInfo = {0};
    uint8_t                            index = 0;

    HCI_PRINT(" HCI: Host CE B - Bytes Received :");
    HCI_PRINT_BUFFER(" Buffer: ", &pData[index], (length - index));
    transInfo.status = NFCSTATUS_SUCCESS;
    transInfo.buffer = &pData[index];
    transInfo.length = (length - index);

    status = NFCSTATUS_PENDING;
    /* Event NXP_EVT_NFC_RCV_DATA: so give received data to
       the upper layer */
    phHciNfc_Notify_Event(psHciContext, pHwRef,
                           (uint8_t) NFC_NOTIFY_RECV_EVENT,
                            &transInfo );


    return status;
}


#endif /* #if defined (HOST_EMULATION) */
