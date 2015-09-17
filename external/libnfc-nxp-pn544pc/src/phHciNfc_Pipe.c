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
* \file  hHciNfc_PipeMgmt.c                                                   *
* \brief HCI Pipe Management Routines.                                        *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Wed Dec  8 10:49:49 2010 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.43 $                                                           *
* $Aliases:  $
*
* $Developed by : Ravindra U   $                                              *
*
*                                                                             *
* =========================================================================== *
*/


/*
***************************** Header File Inclusion ****************************
*/

#include <phNfcConfig.h>
#include <phNfcCompId.h>
#include <phHciNfc_Pipe.h>
#include <phOsalNfc.h>
#include <phHciNfc_AdminMgmt.h>
#include <phHciNfc_LinkMgmt.h>
#include <phHciNfc_IDMgmt.h>
#include <phHciNfc_DevMgmt.h>
#include <phHciNfc_PollingLoop.h>
#include <phHciNfc_RFReader.h>
#include <phHciNfc_RFReaderA.h>
#include <phHciNfc_Emulation.h>
#ifdef TYPE_B
#include <phHciNfc_RFReaderB.h>
#endif
#ifdef TYPE_FELICA
#include <phHciNfc_Felica.h>
#endif
#ifdef TYPE_JEWEL
#include <phHciNfc_Jewel.h>
#endif
#ifdef TYPE_ISO15693
#include <phHciNfc_ISO15693.h>
#endif
#ifdef ENABLE_P2P
#include <phHciNfc_NfcIPMgmt.h>
#endif
#ifdef HOST_EMULATION
#include <phHciNfc_CE_A.h>
#include <phHciNfc_CE_B.h>
#endif

#ifdef ENABLE_WI
#include <phHciNfc_WI.h>
#endif /* #ifdef ENABLE_WI */

#ifdef ENABLE_SWP
#include <phHciNfc_SWP.h>
#endif /* #ifdef ENABLE_SWP */

/*
****************************** Macro Definitions *******************************
*/

/*
*************************** Structure and Enumeration ***************************
*/
static phHciNfc_GateID_t host_gate_list[] = {
        phHciNfc_IdentityMgmtGate,
        phHciNfc_PN544MgmtGate,
        phHciNfc_PollingLoopGate,
        phHciNfc_RFReaderAGate,
#ifdef TYPE_B
        phHciNfc_RFReaderBGate,
#endif

#ifdef TYPE_FELICA
        phHciNfc_RFReaderFGate,
#endif

#ifdef TYPE_JEWEL
        phHciNfc_JewelReaderGate,
#endif

#ifdef TYPE_ISO15693
        phHciNfc_ISO15693Gate,
#endif
#ifdef TYPE_HID_READER
        phHciNfc_HIDReaderGate,
#endif
#ifdef ENABLE_P2P
        phHciNfc_NFCIP1InitRFGate,
        phHciNfc_NFCIP1TargetRFGate,
#endif
#if defined(HOST_EMULATION)
        phHciNfc_CETypeAGate,
        phHciNfc_CETypeBGate,
#endif
#ifdef ENABLE_WI
        phHciNfc_NfcWIMgmtGate,
#endif /* #ifdef ENABLE_WI */

#ifdef ENABLE_SWP
        phHciNfc_SwpMgmtGate,
#endif /* #ifdef ENABLE_SWP */

        phHciNfc_UnknownGate
};

/*
*************************** Static Function Declaration **************************
*/

static
NFCSTATUS
phHciNfc_Create_Pipe(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_Gate_Info_t    *destination,
                        phHciNfc_Pipe_Info_t    **ppPipeHandle
                    );


/*
*************************** Function Definitions ***************************
*/


/*!
 * \brief Creation of the Pipe
 *
 * This function creates the pipe between a source host's gate and destination
 * host's gate
 *
 */

static
NFCSTATUS
phHciNfc_Create_Pipe(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_Gate_Info_t    *destination,
                        phHciNfc_Pipe_Info_t    **ppPipeHandle
                    )
{
    NFCSTATUS       status = NFCSTATUS_SUCCESS;

    *ppPipeHandle = (phHciNfc_Pipe_Info_t *)
                        phOsalNfc_GetMemory( sizeof(phHciNfc_Pipe_Info_t) );

    if(NULL != *ppPipeHandle)
    {
        /* The Source Host is the Terminal Host */
        (*ppPipeHandle)->pipe.source.host_id    = (uint8_t) phHciNfc_TerminalHostID;

        /* The Source Gate is same as the Destination Gate */
        (*ppPipeHandle)->pipe.source.gate_id    =
                                ((phHciNfc_Gate_Info_t *)destination)->gate_id;
        (*ppPipeHandle)->pipe.dest.host_id =
                                ((phHciNfc_Gate_Info_t *)destination)->host_id;
        (*ppPipeHandle)->pipe.dest.gate_id  =
                                ((phHciNfc_Gate_Info_t *)destination)->gate_id;

        /* if( hciMode_Override == psHciContext->hci_mode ) */
        {
            /* The Pipe ID is unknown until it is assigned */
            (*ppPipeHandle)->pipe.pipe_id   = (uint8_t) HCI_UNKNOWN_PIPE_ID;

            status = phHciNfc_Send_Admin_Cmd( psHciContext, pHwRef,
                                        ADM_CREATE_PIPE, (PIPEINFO_SIZE-1)
                                                    ,*ppPipeHandle );
        }
    }
    else
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
    }
    return status;
}


NFCSTATUS
phHciNfc_Update_Pipe(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_PipeMgmt_Seq_t *p_pipe_seq
                    )
{
    uint8_t pipe_index = 0;
    uint8_t skip_index = 0;

    phHciNfc_Pipe_Info_t  *p_pipe_info = NULL;
    NFCSTATUS       status = NFCSTATUS_SUCCESS;
    uint16_t        host_gate_list_len =
                     (sizeof(host_gate_list)/sizeof(phHciNfc_GateID_t) );

    for ( pipe_index = HCI_DYNAMIC_PIPE_ID;
            pipe_index <= host_gate_list_len;
                    pipe_index++ )
    {
        uint8_t pipe_id = pipe_index - skip_index;

        if (NULL == psHciContext->p_pipe_list[pipe_id])
        {
           status = phHciNfc_Allocate_Resource((void **)&p_pipe_info,
                                            sizeof(phHciNfc_Pipe_Info_t));
        }

        if( (NFCSTATUS_SUCCESS == status)
            && (NULL != p_pipe_info)
            && ((NULL == psHciContext->p_pipe_list[pipe_id])))
        {
            for( ;(psHciContext->gate_verify == TRUE )&&
                (pipe_index <= host_gate_list_len); pipe_index++,skip_index++)
            {
                if(
                    (NFCSTATUS_SUCCESS == phHciNfc_IDMgmt_Valid_Gate(
                      psHciContext, host_gate_list[pipe_index - HCI_DYNAMIC_PIPE_ID] ))
                  )
                  {
                      break;
                  }
                  else
                  {
                      ;
                  }
            }

            if(pipe_index > host_gate_list_len)
            {
                phOsalNfc_FreeMemory(p_pipe_info);
                p_pipe_info = NULL;
                break;
            }
            /* The Source Host is the Terminal Host */
            p_pipe_info->pipe.source.host_id  = (uint8_t) phHciNfc_TerminalHostID;

            /* The Source Gate is same as the Destination Gate */
            p_pipe_info->pipe.source.gate_id    =
                                    host_gate_list[pipe_index - HCI_DYNAMIC_PIPE_ID];
            p_pipe_info->pipe.dest.host_id =
                                    phHciNfc_HostControllerID;
            p_pipe_info->pipe.dest.gate_id  =
                                    host_gate_list[pipe_index - HCI_DYNAMIC_PIPE_ID];
            /* The Pipe ID is unknown until it is assigned */
            p_pipe_info->pipe.pipe_id   = (uint8_t) HCI_UNKNOWN_PIPE_ID;

            /* Initialise the Resources for the particular Gate */

            status = phHciNfc_Create_All_Pipes(psHciContext,
                                            pHwRef, p_pipe_seq );

            if( NFCSTATUS_SUCCESS == status )
            {
                status = phHciNfc_Update_PipeInfo( psHciContext, p_pipe_seq ,
                                        pipe_id, p_pipe_info );
                if( NFCSTATUS_SUCCESS == status )
                {
                    p_pipe_info->pipe.pipe_id = pipe_id;
                    psHciContext->p_pipe_list[pipe_id] = p_pipe_info;
                }
                else
                {
                    phOsalNfc_FreeMemory(p_pipe_info);
                }

                p_pipe_info = NULL;
            }
            else
            {
                phOsalNfc_FreeMemory(p_pipe_info);
                p_pipe_info = NULL;
            }

            if( phHciNfc_RFReaderAGate == host_gate_list[pipe_index - HCI_DYNAMIC_PIPE_ID] )
            {
                break;
            }
        }
        else if (NULL != psHciContext->p_pipe_list[pipe_id])
        {
          ;
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INSUFFICIENT_RESOURCES);
        }
    }

    if( NFCSTATUS_SUCCESS == status )
    {
        status = phHciNfc_LinkMgmt_Initialise( psHciContext,pHwRef );
        if(NFCSTATUS_SUCCESS == status)
        {
            status = phHciNfc_ReaderMgmt_Initialise( psHciContext,pHwRef );
        }
        if(NFCSTATUS_SUCCESS == status)
        {
            status = phHciNfc_EmuMgmt_Initialise( psHciContext,pHwRef );
        }
    }

    return status;
}


/*!
 * \brief Deletion of the Pipe
 *
 * This function Deletes a pipe created between a terminal host's gate and
 *  destination host's gate
 *
 */

NFCSTATUS
phHciNfc_Delete_Pipe(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_Pipe_Info_t    *pPipeHandle
                    )
{
    NFCSTATUS       status=NFCSTATUS_SUCCESS;
    NFCSTATUS       cmd_status = NFCSTATUS_SUCCESS;

    if( (NULL == psHciContext)
        || (NULL == pHwRef)
        || (NULL == pPipeHandle)
     )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        cmd_status = phHciNfc_Send_Admin_Cmd( psHciContext, pHwRef,
                            ADM_DELETE_PIPE, PIPEID_LEN, pPipeHandle );
        status = ( (cmd_status == NFCSTATUS_PENDING)?
                                        NFCSTATUS_SUCCESS : cmd_status);
    }

    return status;
}

#ifdef HOST_EMULATION

NFCSTATUS
phHciNfc_CE_Pipes_OP(
                                phHciNfc_sContext_t             *psHciContext,
                                void                            *pHwRef,
                                phHciNfc_PipeMgmt_Seq_t         *p_pipe_seq
                             )
{
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t        *p_pipe_info = NULL;
    /* uint8_t                      pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID; */

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        switch( *p_pipe_seq )
        {
            case PIPE_CARD_A_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_CETypeAGate;

                status = phHciNfc_CE_A_Init_Resources ( psHciContext );
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
            case PIPE_CARD_B_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_CETypeBGate;

                status = phHciNfc_CE_B_Init_Resources ( psHciContext );
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
#if 0
            case PIPE_CARD_F_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_CETypeFGate;

                /* status = phHciNfc_Card_Emulation_Init (psHciContext , TYPE_F); */
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                     /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
            case PIPE_CARD_A_DELETE:
            {
                status = phHciNfc_CE_A_Get_PipeID( psHciContext, &pipe_id );
                p_pipe_info = psHciContext->p_pipe_list[pipe_id];
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phHciNfc_Delete_Pipe( psHciContext, pHwRef,
                                p_pipe_info);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
            case PIPE_CARD_B_DELETE:
            {
                status = phHciNfc_CE_B_Get_PipeID( psHciContext, &pipe_id );
                p_pipe_info = psHciContext->p_pipe_list[pipe_id];
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phHciNfc_Delete_Pipe( psHciContext, pHwRef,
                                p_pipe_info);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
#endif
            /* case PIPE_MGMT_END : */
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                break;
            }
        }
    }

    return status;
}
#endif


/*!
 * \brief Creates the Pipes of all the Supported Gates .
 *
 * This function Creates the pipes for all the supported gates
 */

NFCSTATUS
phHciNfc_Create_All_Pipes(
                                phHciNfc_sContext_t             *psHciContext,
                                void                            *pHwRef,
                                phHciNfc_PipeMgmt_Seq_t         *p_pipe_seq
                             )
{
    NFCSTATUS                           status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t                *p_pipe_info = NULL;

    if( (NULL == psHciContext) || (NULL == pHwRef)
        || (NULL == p_pipe_seq) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {

        switch( *p_pipe_seq )
        {
            /* Admin pipe close sequence */
            case PIPE_IDMGMT_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_IdentityMgmtGate;

                status = phHciNfc_IDMgmt_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
            case PIPE_PN544MGMT_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_PN544MgmtGate;

                status = phHciNfc_DevMgmt_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
            case PIPE_POLLINGLOOP_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_PollingLoopGate;

                status = phHciNfc_PollLoop_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
            case PIPE_READER_A_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_RFReaderAGate;

                status = phHciNfc_ReaderA_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#ifdef TYPE_B
            case PIPE_READER_B_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_RFReaderBGate;

                status = phHciNfc_ReaderB_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
/* #ifdef TYPE_B*/
#endif
#ifdef TYPE_FELICA
            case PIPE_READER_F_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_RFReaderFGate;

                status = phHciNfc_Felica_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#endif
#ifdef TYPE_JEWEL
            case PIPE_READER_JWL_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_JewelReaderGate;

                status = phHciNfc_Jewel_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#endif /* #ifdef TYPE_JEWEL */
#ifdef TYPE_ISO15693
            case PIPE_READER_ISO15693_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_ISO15693Gate;

                status = phHciNfc_ISO15693_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#endif /* #ifdef TYPE_ISO15693 */
#ifdef TYPE_HIDREADER
            case PIPE_READER_HIDTAG_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_HIDReaderGate;

                status = phHciNfc_HIDReader_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#endif /* #ifdef TYPE_HIDREADER */
            case PIPE_NFC_INITIATOR_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_NFCIP1InitRFGate;
#ifdef ENABLE_P2P
                status = phHciNfc_Initiator_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
#endif
                break;
            }
            case PIPE_NFC_TARGET_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_NFCIP1TargetRFGate;

#ifdef ENABLE_P2P
                status = phHciNfc_Target_Init_Resources (psHciContext);
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
#endif
                break;
            }
#if defined (HOST_EMULATION)
            case PIPE_CARD_A_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_CETypeAGate;

                status = phHciNfc_CE_A_Init_Resources ( psHciContext );
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
            case PIPE_CARD_B_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_CETypeBGate;

                status = phHciNfc_CE_B_Init_Resources ( psHciContext );
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#if 0
            case PIPE_CARD_F_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_CETypeFGate;

                /* status = phHciNfc_Card_Emulation_Init (psHciContext , TYPE_F); */
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                     /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#endif

#endif /* HOST_EMULATION */

#ifdef ENABLE_WI
            case PIPE_WI_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_NfcWIMgmtGate;

                status = phHciNfc_WI_Init_Resources ( psHciContext );
                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                     /* status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status); */
                }
                break;
            }
#endif /* #ifdef ENABLE_WI */
#if defined( ENABLE_SWP) || !defined (ENABLE_WI)
            case PIPE_SWP_CREATE:
            {
                phHciNfc_Gate_Info_t        id_dest;

                id_dest.host_id = (uint8_t)phHciNfc_HostControllerID;
                id_dest.gate_id = (uint8_t)phHciNfc_SwpMgmtGate;

                status = phHciNfc_SWP_Init_Resources ( psHciContext );

                if((status == NFCSTATUS_SUCCESS)
#ifdef ESTABLISH_SESSION
                    && (hciMode_Session != psHciContext->hci_mode)
#endif
                    )
                {
                    status = phHciNfc_Create_Pipe( psHciContext, pHwRef,
                                &id_dest, &p_pipe_info);
                    status = ((NFCSTATUS_PENDING == status )?
                                        NFCSTATUS_SUCCESS : status);
                }
                break;
            }
#endif /* #if def( ENABLE_SWP ) */
            case PIPE_MGMT_END :
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_SUCCESS);
                break;
            }
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
                break;
            }

        } /* End of Pipe Seq Switch */

    } /* End of Null Check for the Context */

    return status;
}

/*!
 * \brief Deletes the Pipes of all the Supported Gates .
 *
 * This function Deletes the pipes for all the supported gates
 */

NFCSTATUS
phHciNfc_Delete_All_Pipes(
                                phHciNfc_sContext_t             *psHciContext,
                                void                            *pHwRef,
                                phHciNfc_PipeMgmt_Seq_t         pipeSeq
                             )
{
    NFCSTATUS                           status = NFCSTATUS_SUCCESS;
    phHciNfc_Pipe_Info_t                *p_pipe_info = NULL;
    uint8_t                             length = 0;

    if( (NULL == psHciContext) || (NULL == pHwRef) )
    {
      status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* pipeSeq = PIPE_DELETE_ALL; */

        if ( PIPE_DELETE_ALL == pipeSeq )
        {
            /* Admin pipe close sequence */
            p_pipe_info = psHciContext->p_pipe_list[PIPETYPE_STATIC_ADMIN];
            status = phHciNfc_Send_Admin_Cmd( psHciContext,
                                    pHwRef, ADM_CLEAR_ALL_PIPE,
                                        length, p_pipe_info);
            status = ((NFCSTATUS_PENDING == status)?
                                NFCSTATUS_SUCCESS : status);
        }
        else
        {
            status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_HCI_SEQUENCE);
        }


    } /* End of Null Check for the Context */

    return status;
}



NFCSTATUS
phHciNfc_Update_PipeInfo(
                                phHciNfc_sContext_t             *psHciContext,
                                phHciNfc_PipeMgmt_Seq_t         *pPipeSeq,
                                uint8_t                         pipe_id,
                                phHciNfc_Pipe_Info_t            *pPipeInfo
                      )
{
    phHciNfc_GateID_t           gate_id = phHciNfc_UnknownGate;
    NFCSTATUS                   status = NFCSTATUS_SUCCESS;

    if(
        (NULL == psHciContext) || (NULL == pPipeSeq)
        || ( NULL == pPipeInfo )
      )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        gate_id = (phHciNfc_GateID_t )pPipeInfo->pipe.dest.gate_id ;
        switch( gate_id )
        {
            /* Identity Management Pipe Creation */
            case phHciNfc_IdentityMgmtGate:
            {
                status = phHciNfc_IDMgmt_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = PIPE_PN544MGMT_CREATE;
                }
                break;
            }
            case  phHciNfc_PN544MgmtGate:
            {
                status = phHciNfc_DevMgmt_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = ( HCI_SELF_TEST != psHciContext->init_mode )?
                                PIPE_POLLINGLOOP_CREATE:PIPE_DELETE_ALL;
                }
                break;
            }
            case  phHciNfc_PollingLoopGate:
            {
                status = phHciNfc_PollLoop_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = PIPE_READER_A_CREATE;
                }
                break;
            }
            case  phHciNfc_RFReaderAGate:
            {
                status = phHciNfc_ReaderA_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {

#if defined (TYPE_B)
                    *pPipeSeq = PIPE_READER_B_CREATE;
/* #if defined (TYPE_B) */
#elif defined (TYPE_FELICA)
                    *pPipeSeq = PIPE_READER_F_CREATE;
/* #if defined (TYPE_FELICA) */
#elif defined (TYPE_JEWEL)
                    *pPipeSeq = PIPE_READER_JWL_CREATE;
/* #if defined (TYPE_JEWEL) */
#elif defined (TYPE_ISO15693)
                    *pPipeSeq = PIPE_READER_ISO15693_CREATE;
/* #if defined (TYPE_ISO15693) */
#elif  defined(ENABLE_P2P)
                    *pPipeSeq = PIPE_NFC_INITIATOR_CREATE;
/* #if defined(ENABLE_P2P) */
#elif defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/

/*lint -e{91} suppress "Line exceeds"*/
#elif !defined(TYPE_B) && !defined(TYPE_FELICA) && !defined(TYPE_JEWEL) && !defined(TYPE_ISO15693) && !defined(ENABLE_P2P)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif
                }
                break;
            }
#ifdef TYPE_B
            case  phHciNfc_RFReaderBGate:
            {
                status = phHciNfc_ReaderB_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined (TYPE_FELICA)
                    *pPipeSeq = PIPE_READER_F_CREATE;
/* #if defined (TYPE_FELICA) */
#elif defined (TYPE_JEWEL)
                    *pPipeSeq = PIPE_READER_JWL_CREATE;
/* #if defined (TYPE_JEWEL) */
#elif defined (TYPE_ISO15693)
                    *pPipeSeq = PIPE_READER_ISO15693_CREATE;
/* #if defined (TYPE_ISO15693) */
#elif  defined(ENABLE_P2P)
                    *pPipeSeq = PIPE_NFC_INITIATOR_CREATE;
/* #if defined(ENABLE_P2P) */
#elif defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
 /*lint -e{91} suppress "Line exceeds"*/
#elif !defined(TYPE_FELICA) && !defined(TYPE_JEWEL) && !defined(TYPE_ISO15693) && !defined(ENABLE_P2P) && !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif
                }
                break;
            }
#endif
#ifdef TYPE_FELICA
            case  phHciNfc_RFReaderFGate:
            {
                status = phHciNfc_Felica_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined (TYPE_JEWEL)
                    *pPipeSeq = PIPE_READER_JWL_CREATE;
/* #if defined (TYPE_JEWEL) */
#elif defined (TYPE_ISO15693)
                    *pPipeSeq = PIPE_READER_ISO15693_CREATE;
/* #if defined (TYPE_ISO15693) */
#elif  defined(ENABLE_P2P)
                    *pPipeSeq = PIPE_NFC_INITIATOR_CREATE;
/* #if defined(ENABLE_P2P) */
#elif defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
 /*lint -e{91} suppress "Line exceeds"*/
#elif !defined(TYPE_JEWEL) && !defined(TYPE_ISO15693) && !defined(ENABLE_P2P) && !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif
                }
                break;
            }
#endif
#ifdef TYPE_JEWEL
            case  phHciNfc_JewelReaderGate:
            {
                status = phHciNfc_Jewel_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined (TYPE_ISO15693)
                    *pPipeSeq = PIPE_READER_ISO15693_CREATE;
/* #if defined (TYPE_ISO15693) */
#elif  defined(ENABLE_P2P)
                    *pPipeSeq = PIPE_NFC_INITIATOR_CREATE;
/* #if defined(ENABLE_P2P) */
#elif defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
#elif !defined(TYPE_ISO15693) && !defined(ENABLE_P2P) && !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif
                }
                break;
            }
#endif /* #ifdef TYPE_JEWEL */
#if defined (TYPE_ISO15693)
            case  phHciNfc_ISO15693Gate:
            {
                status = phHciNfc_ISO15693_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined( ENABLE_P2P )
                    *pPipeSeq = PIPE_NFC_INITIATOR_CREATE;
/* #if defined (ENABLE_P2P) */
#elif defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
#elif !defined(ENABLE_P2P) && !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif /* #if defined (ENABLE_WI) */
#if defined (TYPE_HIDREADER)
                    if(
                        (NFCSTATUS_SUCCESS == phHciNfc_IDMgmt_Valid_Gate(
                          psHciContext, phHciNfc_HIDReaderGate ))
                      )
                      {
                         *pPipeSeq = PIPE_READER_HIDTAG_CREATE;
                      }
#endif
                }
                break;
            }
#endif /* #if defined (TYPE_ISO15693) */
#if defined (TYPE_HIDREADER)
            case phHciNfc_HIDReaderGate:
            {
                status = phHciNfc_HIDReader_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined( ENABLE_P2P )
                    *pPipeSeq = PIPE_NFC_INITIATOR_CREATE;
#elif defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
#elif !defined(ENABLE_P2P) && !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif /* #if defined (ENABLE_WI) */
/* #if defined (ENABLE_P2P) */
                }
                break;
            }
#endif /* #if defined (TYPE_HIDREADER) */
            case  phHciNfc_NFCIP1InitRFGate:
            {
#ifdef ENABLE_P2P
                status = phHciNfc_Initiator_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = PIPE_NFC_TARGET_CREATE;
                }
#endif
                break;
            }
            case  phHciNfc_NFCIP1TargetRFGate:
            {
#ifdef ENABLE_P2P
                status = phHciNfc_Target_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
#elif !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif

#if defined (HOST_EMULATION)
                    if((NXP_NFC_ROM_INFO((
                        (phHal_sHwReference_t *)psHciContext->p_hw_ref)->device_info.fw_version)
                       >= NFC44_ROM33_VER ))
                     {
                        *pPipeSeq = PIPE_CARD_A_CREATE;
                     }
#endif
                }
#endif
                break;
            }
#ifdef HOST_EMULATION
            case phHciNfc_CETypeAGate:
            {
                status = phHciNfc_CE_A_Update_PipeInfo(psHciContext,
                                pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = PIPE_CARD_B_CREATE;
                }
                break;
            }
            case phHciNfc_CETypeBGate:
            {
                status = phHciNfc_CE_B_Update_PipeInfo(psHciContext,
                                pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
#if defined (ENABLE_WI)
                    *pPipeSeq = PIPE_WI_CREATE;
/* #if defined (ENABLE_WI) */
#elif defined (ENABLE_SWP)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* #if defined (ENABLE_SWP)*/
#elif !defined(ENABLE_WI)
                    *pPipeSeq = PIPE_SWP_CREATE;
/* Should not enter this condition */
#else
                    *pPipeSeq = PIPE_MGMT_END;
#endif
                }
                break;
            }
#endif
#if defined (ENABLE_WI)
            case  phHciNfc_NfcWIMgmtGate:
            {
                status = phHciNfc_WI_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = PIPE_SWP_CREATE;
                }
                break;
            }
#endif /* #if defined (ENABLE_WI) */
            case  phHciNfc_SwpMgmtGate:
            {
                status = phHciNfc_SWP_Update_PipeInfo(psHciContext,
                            pipe_id, pPipeInfo);
                if(NFCSTATUS_SUCCESS == status)
                {
                    *pPipeSeq = PIPE_DELETE_ALL;
                }
                break;
            }
            case phHciNfc_ConnectivityGate:
            {
                status = phHciNfc_Uicc_Update_PipeInfo(psHciContext,
                                pipe_id, pPipeInfo);
                break;
            }
            case phHciNfc_UnknownGate:
            default:
            {
                status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_HCI_GATE_NOT_SUPPORTED );
                break;
            }
            /*End of the default Switch Case */

        } /*End of the Index Switch */
    } /* End of Context and the Identity information validity check */

    return status;
}


/*!
 * \brief Opening the Pipe
 *
 * This function opens the the pipe created between a terminal host's gate and
 *  destination host's gate
 *
 */

NFCSTATUS
phHciNfc_Open_Pipe(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_Pipe_Info_t    *pPipeHandle
                    )
{
    uint8_t                 pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    NFCSTATUS               cmd_status = NFCSTATUS_SUCCESS;

    if( (NULL == psHciContext)
        || ( NULL == pHwRef )
        || ( NULL == pPipeHandle )
      )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Obtain the pipe_id from the pipe_handle */
        pipe_id = pPipeHandle->pipe.pipe_id;

        if ( pipe_id <= PHHCINFC_MAX_PIPE)
        {
            cmd_status = phHciNfc_Send_Generic_Cmd( psHciContext,pHwRef,
                                    pipe_id, ANY_OPEN_PIPE);
            status = ( (cmd_status == NFCSTATUS_PENDING)?
                                        NFCSTATUS_SUCCESS : cmd_status);
        }
    }
    return status;
}


/*!
 * \brief Closing the Pipe
 *
 * This function Closes the the pipe created between a terminal host's gate and
 *  destination host's gate
 *
 */

NFCSTATUS
phHciNfc_Close_Pipe(
                        phHciNfc_sContext_t     *psHciContext,
                        void                    *pHwRef,
                        phHciNfc_Pipe_Info_t    *pPipeHandle
                    )
{
    uint8_t                 pipe_id = (uint8_t) HCI_UNKNOWN_PIPE_ID;
    NFCSTATUS               status = NFCSTATUS_SUCCESS;
    NFCSTATUS               cmd_status = NFCSTATUS_SUCCESS;

    if( (NULL == psHciContext)
        || ( NULL == pHwRef )
        || ( NULL == pPipeHandle )
      )
    {
        status = PHNFCSTVAL(CID_NFC_HCI, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Obtain the pipe_id from the pipe_handle */
        pipe_id = pPipeHandle->pipe.pipe_id;

        if( (uint8_t)HCI_UNKNOWN_PIPE_ID > pipe_id)
        {
            cmd_status = phHciNfc_Send_Generic_Cmd(
                                        psHciContext, pHwRef, pipe_id,
                                        ANY_CLOSE_PIPE );

            status = ((cmd_status == NFCSTATUS_PENDING)?
                                NFCSTATUS_SUCCESS : cmd_status);
        }
    }
    return status;
}
