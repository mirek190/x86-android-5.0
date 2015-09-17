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
 * \file phLibNfc_emulation.c

 * Project: NFC FRI
 *
 * $Date: Thu Apr 22 13:59:50 2010 $
 * $Author: Lokeswara B $
 * $Revision: 0.1 $
 * $Aliases: $
 *
 */

/*
************************* Header Files ***************************************
*/

#include <phNfcStatus.h>
#include <phLibNfc.h>
#include <phHal4Nfc.h>
#include <phOsalNfc.h>
#include <phLibNfc_Internal.h>
#include <phLibNfc_SE.h>
#include <phLibNfc_ndef_raw.h>
#include <phLibNfc_initiator.h>
#include <phLibNfc_discovery.h>

#ifdef HOST_EMULATION
/*
*************************** Macro's  ****************************************
*/

#ifndef STATIC_DISABLE
#define STATIC static
#else
#define STATIC
#endif


/*
*************************** Global Variables **********************************
*/


/*
*************************** Static Function Declaration ***********************
*/


/*
*************************** Function Definitions ******************************
*/

/**
* Registers notification handler to handle secure element specific events
*/

NFCSTATUS phLibNfc_CardEmulation_NtfRegister  ( phLibNfc_NtfRegister_RspCb_t  pCEHostNotification,
  void *  pContext
 )
{
     NFCSTATUS         Status = NFCSTATUS_SUCCESS;
     pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)gpphLibContext;

     if((NULL == gpphLibContext) ||
         (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
     {
         Status = NFCSTATUS_NOT_INITIALISED;
     }
     else if((pCEHostNotification == NULL)
         ||(NULL == pContext))
     {
         Status = NFCSTATUS_INVALID_PARAMETER;
     }
     else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
     {
         Status = NFCSTATUS_SHUTDOWN;
     }
     else
     {
         /* Dont register it to lower layer, the event will be received
            either part of SE event notification or default hander*/
            pLibContext->CE_Info.pCeNtfCb = pCEHostNotification;
            pLibContext->CE_Info.pCeNtfCntx = pContext;
     }
     return Status;
}

/**
* SE Notification events are notified with this callback
*/
void phLibNfc_CENotification(void  *context,
                                    phHal_eNotificationType_t    type,
                                    phHal4Nfc_NotificationInfo_t  info,
                                    NFCSTATUS                   status)
{
    phLibNfc_RemoteDevList_t *pCE_dev=NULL;
    NFCSTATUS   RetVal = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibNfcCntx = (pphLibNfc_LibContext_t)context;

    if( (status == NFCSTATUS_SUCCESS) && (pLibNfcCntx == gpphLibContext ) )
    {
        pCE_dev = &pLibNfcCntx->CE_Info.pCE_dev;
        if( pCE_dev->psRemoteDevInfo == NULL)
        {
            pCE_dev->psRemoteDevInfo = (phLibNfc_sRemoteDevInformation_t*)
                      phOsalNfc_GetMemory(sizeof(phLibNfc_sRemoteDevInformation_t));
            if( pCE_dev->psRemoteDevInfo == NULL)
            {
                RetVal= NFCSTATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    else
    {
            RetVal= NFCSTATUS_FAILED;
    }

    if (( RetVal == NFCSTATUS_SUCCESS )
        && (NULL != info.psEventInfo))
    {
        pCE_dev->hTargetDev = (uint32_t) pCE_dev->psRemoteDevInfo;

        switch(info.psEventInfo->eventType)
        {
            case NFC_EVT_ACTIVATED :
            {
                pLibNfcCntx->LibNfcState.cur_state = eLibNfcHalStateConnect;
                if(info.psEventInfo->eventSource == phNfc_eISO14443_A_PICC)
                {
                    pLibNfcCntx->CE_Info.Active = TRUE;
                    pCE_dev->psRemoteDevInfo->RemDevType = phNfc_eISO14443_A_PCD;

                    memcpy(pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AppData ,
                                pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.AppData,
                                pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.AppDataLength );

                    pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AppDataLength =
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.AppDataLength ;

                    memcpy(pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.AtqA ,
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.AtqA, 2 );

                /*    memcpy(pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Uid ,
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.Uid ,
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.UidLength );

                    pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.UidLength =
                           pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.UidLength;
                */
                    pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.UidLength  = 0;

                    pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443A_Info.Sak =
                            gpphLibContext->CE_Info.config_data.config.hostEmuCfg_A.hostEmuCfgInfo.Sak;

                }
                if(info.psEventInfo->eventSource == phNfc_eISO14443_B_PICC)
                {
                    pCE_dev->psRemoteDevInfo->RemDevType = phNfc_eISO14443_B_PCD;
                    pLibNfcCntx->CE_Info.Active = TRUE;
                    memset(&pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info,0,sizeof(phNfc_sIso14443BInfo_t));
                    memcpy(pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.Pupi,
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_B.hostEmuCfgInfo.AtqB.AtqResInfo.Pupi ,
                        PHHAL_PUPI_LENGTH);

                    memcpy(pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.AppData,
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_B.hostEmuCfgInfo.AtqB.AtqResInfo.AppData ,
                        PHHAL_APP_DATA_B_LENGTH);

                    memcpy(pCE_dev->psRemoteDevInfo->RemoteDevInfo.Iso14443B_Info.AtqB.AtqResInfo.ProtInfo,
                        pLibNfcCntx->CE_Info.config_data.config.hostEmuCfg_B.hostEmuCfgInfo.AtqB.AtqResInfo.ProtInfo ,
                        PHHAL_PROT_INFO_B_LENGTH);

                }
                if(pLibNfcCntx->CE_Info.pCeNtfCb != NULL )
                {
                    pLibNfcCntx->CE_Info.pCeNtfCb(pLibNfcCntx->CE_Info.pCeNtfCntx,&pLibNfcCntx->CE_Info.pCE_dev,1,RetVal);
                }
                break;
            }
            case NFC_EVT_DEACTIVATED:
            {
                RetVal = NFCSTATUS_DESELECTED;
                pLibNfcCntx->LibNfcState.cur_state = eLibNfcHalStateRelease;
                pLibNfcCntx->CE_Info.Active = FALSE;
                if(pLibNfcCntx->CE_Info.pCeNtfCb != NULL )
                {
                    pLibNfcCntx->CE_Info.pCeNtfCb(pLibNfcCntx->CE_Info.pCeNtfCntx,&pLibNfcCntx->CE_Info.pCE_dev,1,RetVal);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
  return;
}

/**
 * Unregister the Secured Element Notification.
 */
NFCSTATUS phLibNfc_CardEmulation_NtfUnregister(void)
{
    NFCSTATUS Status = NFCSTATUS_SUCCESS;
    pphLibNfc_LibContext_t pLibContext=(pphLibNfc_LibContext_t)gpphLibContext;

    if((NULL == gpphLibContext) ||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        /*Lib Nfc is not initialized*/
        Status = NFCSTATUS_NOT_INITIALISED;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        Status = NFCSTATUS_SHUTDOWN;
    }
    else
    {
        pLibContext->CE_Info.pCeNtfCb = NULL;
        pLibContext->CE_Info.pCeNtfCntx = NULL;
    }
    return Status;
}


STATIC void phLibNfc_Mgt_SetCE_ConfigParams_Cb(void     *context,
                                        NFCSTATUS status)
{
    pphLibNfc_RspCb_t       pClientCb=NULL;
    void                    *pUpperLayerContext=NULL;
     /* Check for the context returned by below layer */
    if((phLibNfc_LibContext_t *)context != gpphLibContext)
    {   /*wrong context returned*/
        phOsalNfc_RaiseException(phOsalNfc_e_InternalErr,1);
    }
    else
    {
        if(eLibNfcHalStateShutdown == gpphLibContext->LibNfcState.next_state)
        {   /*shutdown called before completion of this api allow
            shutdown to happen */
            phLibNfc_Pending_Shutdown();
            status = NFCSTATUS_SHUTDOWN;
        }
        else
        {
            gpphLibContext->status.GenCb_pending_status = FALSE;
            if(NFCSTATUS_SUCCESS != status)
            {
                status = NFCSTATUS_FAILED;
            }
            else
            {
                status = NFCSTATUS_SUCCESS;
            }
        }
        /*update the current state */
        phLibNfc_UpdateCurState(status,gpphLibContext);

        pClientCb = gpphLibContext->CE_Info.pCfgRspCb;
        pUpperLayerContext = gpphLibContext->CE_Info.pCfgRspCntx ;

        gpphLibContext->CE_Info.pCfgRspCb = NULL;
        gpphLibContext->CE_Info.pCfgRspCntx  = NULL;
        if (NULL != pClientCb)
        {
            /* Notify to upper layer status of configure operation */
            pClientCb(pUpperLayerContext, status);
        }
    }
    return;
}

NFCSTATUS phLibNfc_Mgt_SetCE_ConfigParams  ( phLibNfc_sEmulationCfg_t *  pConfigInfo,
  pphLibNfc_RspCb_t  pConfigRspCb,
  void *  pContext
 )
{

    NFCSTATUS RetVal = NFCSTATUS_SUCCESS;


    if((NULL == gpphLibContext)||
        (gpphLibContext->LibNfcState.cur_state == eLibNfcHalStateShutdown))
    {
        RetVal = NFCSTATUS_NOT_INITIALISED;
    }
    else if((NULL == pConfigInfo) || (NULL == pConfigRspCb)
        || (NULL == pContext))
    {
        RetVal= NFCSTATUS_INVALID_PARAMETER;
    }
    else if(gpphLibContext->LibNfcState.next_state == eLibNfcHalStateShutdown)
    {
        RetVal = NFCSTATUS_SHUTDOWN;
    }
    else if(TRUE == gpphLibContext->status.GenCb_pending_status)
    {
        RetVal = NFCSTATUS_BUSY;
    }
    else
    {

         memcpy(&gpphLibContext->CE_Info.config_data,pConfigInfo,sizeof(phLibNfc_sEmulationCfg_t));



         RetVal = phHal4Nfc_ConfigParameters(
                                        gpphLibContext->psHwReference,
                                        NFC_EMULATION_CONFIG,
                                        (phHal_uConfig_t*)pConfigInfo,
                                        phLibNfc_Mgt_SetCE_ConfigParams_Cb,
                                        (void *)gpphLibContext
                                        );
         RetVal = PHNFCSTATUS(RetVal);
        if( RetVal == NFCSTATUS_PENDING )
        {
            gpphLibContext->CE_Info.pCfgRspCb = pConfigRspCb;
            gpphLibContext->CE_Info.pCfgRspCntx = pContext;
            gpphLibContext->status.GenCb_pending_status=TRUE;
            gpphLibContext->LibNfcState.next_state = eLibNfcHalStateTransaction;
        }


    }
    return RetVal;

}


#endif /*HOST_EMULATION*/
