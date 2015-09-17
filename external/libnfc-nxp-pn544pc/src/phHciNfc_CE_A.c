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
* \file  phHciNfc_CE_A.c                                             *
* \brief HCI card emulation A management routines.                              *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Fri Aug 21 18:35:05 2009 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.14 $                                                           *
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
#include <phHciNfc_CE_A.h>

#define CE_A_EVT_NFC_SEND_DATA               0x10U
#define CE_A_EVT_NFC_FIELD_ON                0x11U
#define CE_A_EVT_NFC_DEACTIVATED             0x12U
#define CE_A_EVT_NFC_ACTIVATED               0x13U
#define CE_A_EVT_NFC_FIELD_OFF               0x14U

/*
*************************** Structure and Enumeration ***************************
*/


/*
*************************** Static Function Declaration **************************
*/

static
NFCSTATUS
phHciNfc_Recv_CE_A_Event(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pEvent,
                             uint16_t           length
                       );

static
NFCSTATUS
phHciNfc_Recv_CE_A_Response(
                             void               *psContext,
                             void               *pHwRef,
                             uint8_t            *pResponse,
                             uint16_t           length
                       );

static
NFCSTATUS
phHciNfc_CE_A_ProcessData(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef,
                            uint8_t                 *pData,
                            uint16_t                 length
                       );

static
NFCSTATUS
phHciNfc_CE_A_Send_Data(
                           phHciNfc_sContext_t      *psHciContext,
                           void                     *pHwRef,
                           uint8_t                  pipe_id
                       );

/*
*************************** Function Definitions ***************************
*/
NFCSTATUS
phHciNfc_CE_A_Init_Resources(
                                phHciNfc_sContext_t     *psHciContext
                         )
{
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    phHciNfc_CE_A_Info_t     *ps_ce_a_info=NULL;
    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        if(
            ( NULL == psHciContext->p_ce_a_info ) &&
             (phHciNfc_Allocate_Resource((void **)(&ps_ce_a_info),
            sizeof(phHciNfc_CE_A_Info_t))== NFCSTATUS_SUCCESS)
          )
        {
            psHciContext->p_ce_a_info = ps_ce_a_info;
            ps_ce_a_info->current_seq = HOST_CE_A_INVALID_SEQ;
            ps_ce_a_info->next_seq = HOST_CE_A_INVALID_SEQ;
            ps_ce_a_info->pipe_id = (uint8_t)HCI_UNKNOWN_PIPE_ID;
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
        }

    }
    return status;
}

NFCSTATUS
phHciNfc_CE_A_Initialise(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    /*
        1. Open Pipe,
        2. Set all parameters
    */
    NFCSTATUS           status = NFCSTATUS_SUCCESS;
    static uint8_t      sak = HOST_CE_A_SAK_DEFAULT;
    static uint8_t      atqa_info[] = { NXP_CE_A_ATQA_HIGH,
                                        NXP_CE_A_ATQA_LOW};

    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t        *ps_ce_a_info = ((phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_a_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            switch(ps_ce_a_info->current_seq)
            {
                case HOST_CE_A_PIPE_OPEN:
                {
                    status = phHciNfc_Open_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_SAK_SEQ;
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
                case HOST_CE_A_SAK_SEQ:
                {
                    /* HOST Card Emulation A SAK Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_SAK_INDEX;
                    /* Configure the SAK of Host Card Emulation A */
                    sak = (uint8_t)HOST_CE_A_SAK_DEFAULT;
                    ps_pipe_info->param_info =(void*)&sak ;
                    ps_pipe_info->param_length = sizeof(sak) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_ATQA_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_ATQA_SEQ:
                {
                    /* HOST Card Emulation A ATQA Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_ATQA_INDEX;
                    /* Configure the ATQA of Host Card Emulation A */
                    ps_pipe_info->param_info = (void*)atqa_info ;
                    ps_pipe_info->param_length = sizeof(atqa_info) ;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_ENABLE_SEQ;
                        status = NFCSTATUS_SUCCESS;
                    }
                    break;
                }
                case HOST_CE_A_ENABLE_SEQ:
                {
                    status = phHciNfc_CE_A_Mode( psHciContext,
                        pHwRef,  HOST_CE_MODE_ENABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_DISABLE_SEQ;
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
phHciNfc_CE_A_Release(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef
                        )
{
    /*
        1. Close pipe
    */
    NFCSTATUS       status = NFCSTATUS_SUCCESS;
    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t        *ps_ce_a_info = ((phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_a_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_NOT_ALLOWED);
        }
        else
        {
            switch(ps_ce_a_info->current_seq)
            {
                case HOST_CE_A_DISABLE_SEQ:
                {
                    status = phHciNfc_CE_A_Mode( psHciContext,
                        pHwRef,  HOST_CE_MODE_DISABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_CLOSE;
                    }
                    break;
                }
                case HOST_CE_A_PIPE_CLOSE:
                {
                    /* HOST Card Emulation A pipe close sequence */
                    status = phHciNfc_Close_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_DELETE;
                        status = NFCSTATUS_PENDING;
                    }
                    break;
                }
                case HOST_CE_A_PIPE_DELETE:
                {
                    /* HOST Card Emulation A pipe delete sequence */
                    status = phHciNfc_Delete_Pipe( psHciContext,
                                                pHwRef, ps_pipe_info );
                    if(status == NFCSTATUS_SUCCESS)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
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
phHciNfc_CE_A_Config(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef ,
                        phHal_sHostEmuCfg_A_t  *pCfg
                        )
{
    /*
        1. Open Pipe,
        2. Set all parameters
    */
    NFCSTATUS           status = NFCSTATUS_SUCCESS;
    static uint8_t      sak = HOST_CE_A_SAK_DEFAULT;

    if ((NULL == psHciContext) || (NULL == pHwRef))
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t        *ps_ce_a_info = ((phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info );
        phHciNfc_Pipe_Info_t        *ps_pipe_info = NULL;

        ps_pipe_info =  ps_ce_a_info->p_pipe_info;
        if(NULL == ps_pipe_info )
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                                NFCSTATUS_INVALID_HCI_INFORMATION);
        }
        else
        {
            switch(ps_ce_a_info->current_seq)
            {
#if 0
                case HOST_CE_A_UID_SEQ:
                {
                    ps_pipe_info->reg_index = HOST_CE_A_UID_REG_INDEX;
                    ps_pipe_info->param_info = pCfg->hostEmuCfgInfo.Uid;
                    ps_pipe_info->param_length = pCfg->hostEmuCfgInfo.UidLength;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_SAK_SEQ;
                    }
                    break;
                }
#endif
                case HOST_CE_A_SAK_SEQ:
                {
                    /* HOST Card Emulation A SAK Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_SAK_INDEX;
                    /* Configure the SAK of Host Card Emulation A */
                    sak = pCfg->hostEmuCfgInfo.Sak;
                    ps_pipe_info->param_info =&sak;
                    ps_pipe_info->param_length = sizeof(pCfg->hostEmuCfgInfo.Sak);

                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_ATQA_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_ATQA_SEQ:
                {
                    /* HOST Card Emulation A ATQA Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_ATQA_INDEX;
                    /* Configure the ATQA of Host Card Emulation A */
                    ps_pipe_info->param_info = (void*)pCfg->hostEmuCfgInfo.AtqA;
                    ps_pipe_info->param_length = sizeof(pCfg->hostEmuCfgInfo.AtqA);
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_APP_DATA_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_APP_DATA_SEQ:
                {
                    /* HOST Card Emulation A ATQA Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_APP_DATA_INDEX;
                    /* Configure the ATQA of Host Card Emulation A */
                    ps_pipe_info->param_info = (void*)pCfg->hostEmuCfgInfo.AppData;
                    ps_pipe_info->param_length = pCfg->hostEmuCfgInfo.AppDataLength;
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_FWI_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_FWI_SEQ:
                {
                    /* HOST Card Emulation A ATQA Configuration */
                    ps_pipe_info->reg_index = HOST_CE_A_FWI_SFGT_INDEX;
                    /* Configure the ATQA of Host Card Emulation A */
                    ps_pipe_info->param_info =
                                    (void*) (& pCfg->hostEmuCfgInfo.Fwi_Sfgt);
                    ps_pipe_info->param_length =
                                        sizeof(pCfg->hostEmuCfgInfo.Fwi_Sfgt);
                    status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                                ps_pipe_info->pipe.pipe_id,
                                                (uint8_t)ANY_SET_PARAMETER);
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_ENABLE_SEQ;
                    }
                    break;
                }
                case HOST_CE_A_ENABLE_SEQ:
                {
                    status = phHciNfc_CE_A_Mode( psHciContext,
                        pHwRef,  HOST_CE_MODE_ENABLE );
                    if(status == NFCSTATUS_PENDING)
                    {
                        ps_ce_a_info->next_seq = HOST_CE_A_DISABLE_SEQ;
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
phHciNfc_CE_A_Update_PipeInfo(
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
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;

        ps_ce_a_info->current_seq = HOST_CE_A_PIPE_OPEN;
        ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
        /* Update the pipe_id of the card emulation A Gate o
            btained from the HCI Response */
        ps_ce_a_info->pipe_id = pipeID;
        if (HCI_UNKNOWN_PIPE_ID != pipeID)
        {
            ps_ce_a_info->p_pipe_info = pPipeInfo;
            if (NULL != pPipeInfo)
            {
                /* Update the Response Receive routine of the card
                    emulation A Gate */
                pPipeInfo->recv_resp = &phHciNfc_Recv_CE_A_Response;
                /* Update the event Receive routine of the card emulation A Gate */
                pPipeInfo->recv_event = &phHciNfc_Recv_CE_A_Event;
            }
        }
        else
        {
            ps_ce_a_info->p_pipe_info = NULL;
            phOsalNfc_FreeMemory((void *)ps_ce_a_info);
            psHciContext->p_ce_a_info = NULL;

        }
    }

    return status;
}

NFCSTATUS
phHciNfc_CE_A_Get_PipeID(
                             phHciNfc_sContext_t        *psHciContext,
                             uint8_t                    *ppipe_id
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if( (NULL != psHciContext)
        && ( NULL != ppipe_id )
        && ( NULL != psHciContext->p_ce_a_info )
        )
    {
        phHciNfc_CE_A_Info_t     *ps_ce_a_info=NULL;
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;
        *ppipe_id =  ps_ce_a_info->pipe_id  ;
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    return status;
}

NFCSTATUS
phHciNfc_CE_A_Send (
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
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t     *ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info ;
        phHciNfc_Pipe_Info_t     *p_pipe_info = ps_ce_a_info->p_pipe_info;

        if (NULL != p_pipe_info)
        {
            p_pipe_info->param_info =(void*)p_send_param->tx_buffer ;
            p_pipe_info->param_length = p_send_param->tx_length  ;
            status = phHciNfc_CE_A_Send_Data(psHciContext,pHwRef,
                                    ps_ce_a_info->pipe_id);
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
phHciNfc_CE_A_Send_Data(
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
                            HCP_MSG_TYPE_EVENT, CE_A_EVT_NFC_SEND_DATA);

    hcp_message = &(hcp_packet->msg.message);

    phHciNfc_Append_HCPFrame((uint8_t *)hcp_message->payload,
                            i,
                            (uint8_t *)p_pipe_info->param_info,
                            p_pipe_info->param_length);
    length = (uint8_t)(length + i + p_pipe_info->param_length);

    p_pipe_info->sent_msg_type = HCP_MSG_TYPE_EVENT ;
    p_pipe_info->prev_msg = CE_A_EVT_NFC_SEND_DATA ;
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
phHciNfc_Recv_CE_A_Response(
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
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
        uint8_t                     prev_cmd = ANY_GET_PARAMETER;
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                            psHciContext->p_ce_a_info ;
        if( NULL == ps_ce_a_info->p_pipe_info)
        {
            status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_INVALID_HCI_SEQUENCE);
        }
        else
        {
            prev_cmd = ps_ce_a_info->p_pipe_info->prev_msg ;
            switch(prev_cmd)
            {
                case ANY_GET_PARAMETER:
                {
#if 0
                    status = phHciNfc_CE_A_InfoUpdate(psHciContext,
                                    ps_ce_a_info->p_pipe_info->reg_index,
                                    &pResponse[HCP_HEADER_LEN],
                                    (length - HCP_HEADER_LEN));
#endif /* #if 0 */
                    break;
                }
                case ANY_SET_PARAMETER:
                {
                    HCI_PRINT("CE A Parameter Set \n");
                    break;
                }
                case ANY_OPEN_PIPE:
                {
                    HCI_PRINT("CE A open pipe complete\n");
                    break;
                }
                case ANY_CLOSE_PIPE:
                {
                    HCI_PRINT("CE A close pipe complete\n");
                    break;
                }
                default:
                {
                    status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_RESPONSE);
                    break;
                }
            }
            if( NFCSTATUS_SUCCESS == status )
            {
                status = phHciNfc_CE_A_Update_Seq(psHciContext,
                                                    UPDATE_SEQ);
                ps_ce_a_info->p_pipe_info->prev_status = NFCSTATUS_SUCCESS;
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
phHciNfc_CE_A_Update_Seq(
                        phHciNfc_sContext_t     *psHciContext,
                        phHciNfc_eSeqType_t     seq_type
                    )
{
    phHciNfc_CE_A_Info_t    *ps_ce_a_info=NULL;
    NFCSTATUS               status = NFCSTATUS_SUCCESS;

    if( NULL == psHciContext )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else if( NULL == psHciContext->p_ce_a_info )
    {
        status = PHNFCSTVAL(CID_NFC_HCI,
                            NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;
        switch(seq_type)
        {
            case RESET_SEQ:
            case INIT_SEQ:
            {
                ps_ce_a_info->next_seq = HOST_CE_A_PIPE_OPEN;
                ps_ce_a_info->current_seq = HOST_CE_A_PIPE_OPEN;
                break;
            }
            case UPDATE_SEQ:
            {
                ps_ce_a_info->current_seq = ps_ce_a_info->next_seq;
                break;
            }
            case INFO_SEQ:
            {
                break;
            }
            case CONFIG_SEQ:
            {
                ps_ce_a_info->current_seq = HOST_CE_A_SAK_SEQ;
                ps_ce_a_info->next_seq = HOST_CE_A_SAK_SEQ;
                break;
            }
            case REL_SEQ:
            {
                ps_ce_a_info->next_seq = HOST_CE_A_DISABLE_SEQ;
                ps_ce_a_info->current_seq = HOST_CE_A_DISABLE_SEQ;
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
phHciNfc_CE_A_Mode(
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
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_CE_A_Info_t     *ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                                                psHciContext->p_ce_a_info ;
        phHciNfc_Pipe_Info_t     *p_pipe_info = ps_ce_a_info->p_pipe_info;

        if (NULL != p_pipe_info)
        {
            p_pipe_info->reg_index = HOST_CE_A_MODE_INDEX;
            /* Enable/Disable Host Card Emulation A */
            param = (uint8_t)enable_type;
            p_pipe_info->param_info =(void*)&param ;
            p_pipe_info->param_length = sizeof(param) ;
            status = phHciNfc_Send_Generic_Cmd(psHciContext,pHwRef,
                                    ps_ce_a_info->pipe_id,(uint8_t)ANY_SET_PARAMETER);
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
phHciNfc_Recv_CE_A_Event(
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
    else if(NULL == psHciContext->p_ce_a_info)
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_FEATURE_NOT_SUPPORTED);
    }
    else
    {
        phHciNfc_HCP_Packet_t       *p_packet = NULL;
        phHciNfc_CE_A_Info_t         *ps_ce_a_info=NULL;
        phHciNfc_HCP_Message_t      *message = NULL;
        static phHal_sEventInfo_t   event_info;
        uint8_t                     instruction=0;

        ps_ce_a_info = (phHciNfc_CE_A_Info_t *)
                        psHciContext->p_ce_a_info ;

        /* Variable was set but never used (ARM warning) */
        PHNFC_UNUSED_VARIABLE(ps_ce_a_info);

        p_packet = (phHciNfc_HCP_Packet_t *)pEvent;
        message = &p_packet->msg.message;
        /* Get the instruction bits from the Message Header */
        instruction = (uint8_t) GET_BITS8( message->msg_header,
                                HCP_MSG_INSTRUCTION_OFFSET,
                                HCP_MSG_INSTRUCTION_LEN);
        psHciContext->host_rf_type = phHal_eISO14443_A_PICC;
        event_info.eventHost = phHal_eHostController;
        event_info.eventSource = phHal_eISO14443_A_PICC;
        switch(instruction)
        {
            case CE_A_EVT_NFC_ACTIVATED:
            {
                event_info.eventType = NFC_EVT_ACTIVATED;
                /* Notify to the HCI Generic layer To Update the FSM */
                phHciNfc_Notify_Event(psHciContext, pHwRef,
                                    NFC_NOTIFY_DEVICE_ACTIVATED,
                                    &event_info);
                status = NFCSTATUS_PENDING;
                break;
            }
            case CE_A_EVT_NFC_DEACTIVATED:
            {
                event_info.eventType = NFC_EVT_DEACTIVATED;
                HCI_PRINT("CE A Target Deactivated\n");
                phHciNfc_Notify_Event(psHciContext, pHwRef,
                                    NFC_NOTIFY_DEVICE_DEACTIVATED,
                                    &event_info);
                status = NFCSTATUS_PENDING;
                break;
            }
            case CE_A_EVT_NFC_SEND_DATA:
            {
                HCI_PRINT("CE A data is received from the PN544\n");
                if(length > HCP_HEADER_LEN)
                {
                    status = phHciNfc_CE_A_ProcessData(
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
            case CE_A_EVT_NFC_FIELD_ON:
            {
                HCI_PRINT("CE A field on\n");
                event_info.eventType = NFC_EVT_FIELD_ON;
                break;
            }
            case CE_A_EVT_NFC_FIELD_OFF:
            {
                HCI_PRINT("CE A field off\n");
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
phHciNfc_CE_A_ProcessData(
                            phHciNfc_sContext_t     *psHciContext,
                            void                    *pHwRef,
                            uint8_t                 *pData,
                            uint16_t                 length
                       )
{
    NFCSTATUS                          status = NFCSTATUS_SUCCESS;
    static phNfc_sTransactionInfo_t    transInfo = {0};
    uint8_t                            index = 0;

    HCI_PRINT(" HCI: Host CE A - Bytes Received :");
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
