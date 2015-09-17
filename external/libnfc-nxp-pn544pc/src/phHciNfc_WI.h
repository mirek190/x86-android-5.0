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
* \file  phHciNfc_WI .h                                                       *
* \brief HCI wired interface gate Management Routines.                        *
*                                                                             *
*                                                                             *
* Project: NFC-FRI-1.1                                                        *
*                                                                             *
* $Date: Fri Apr  8 19:47:51 2011 $                                           *
* $Author: ravindrau $                                                         *
* $Revision: 1.12 $                                                            *
* $Aliases:  $                                                                *
*                                                                             *
* =========================================================================== *
*/
#ifndef PHHCINFC_WI_H
#define PHHCINFC_WI_H
/*@}*/
/**
 *  \name HCI
 *
 * File: \ref phHciNfc_WI.h
 *
 */
/*@{*/
#define PHHCINFC_WIRED_FILEREVISION "$Revision: 1.12 $" /**< \ingroup grp_file_attributes */
#define PHHCINFC_WIREDINTERFACE_FILEALIASES  "$Aliases:  $"   /**< \ingroup grp_file_attributes */
/*@}*/

/****************************** Header File Inclusion *****************************/
#include <phHciNfc_Generic.h>
#include <phHciNfc_Emulation.h>

/******************************* Macro Definitions ********************************/

/******************** Enumeration and Structure Definition ***********************/


/* enable /disable notifications */
typedef enum phHciNfc_WI_Events{
    eDisableEvents,
    eEnableEvents
} phHciNfc_WI_Events_t;

typedef enum phHciNfc_WI_Seq{
    eWI_PipeOpen        = 0x00U,
    eWI_SetDefaultMode,
    eWI_PipeClose
} phHciNfc_WI_Seq_t;

/* Information structure for  WI  Gate */
typedef struct phHciNfc_WI_Info{

    /* Pointer to WI gate pipe information */
    phHciNfc_Pipe_Info_t            *p_pipe_info;
    /*  WI gate pipe Identifier */
    uint8_t                         pipe_id;
    /*  Application ID of the Transaction performed */
    uint8_t                         aid[MAX_AID_LEN];
    /*  Default info */
    uint8_t                         default_type;

    /*  SmartMX Connection Information */
    uint8_t                         smx_connected;

    /*  Default info Pointer if needed */
    uint8_t                         *p_default_mode;

    /* Current WI gate Internal Sequence type   */
    phHciNfc_WI_Seq_t               current_seq;
    /*Current WI gate next Sequence ID          */
    phHciNfc_WI_Seq_t               next_seq;

} phHciNfc_WI_Info_t;

/************************ Function Prototype Declaration *************************/
/*!
 * \brief Allocates the resources required for  WI gate management.
 *
 * This function Allocates necessary resources as requiered by WI gate management
 *
 * \param[in]  psHciContext            psHciContext is the pointer to HCI Layer
 *
 * \retval NFCSTATUS_SUCCESS           Function execution is successful
 *
 * \retval NFCSTATUS_INVALID_PARAMETER One or more of the given inputs are not valid
 */
extern
NFCSTATUS
phHciNfc_WI_Init_Resources(phHciNfc_sContext_t   *psHciContext);

/**
* \ingroup grp_hci_nfc
*
* \brief Allocates the resources required for  WI gate management.
*
* This function Allocates necessary resources as requiered by WI gate management
*
* \param[in]  psHciContext            psHciContext is the pointer to HCI Layer
*
* \retval NFCSTATUS_SUCCESS           Function execution is successful
*
* \retval NFCSTATUS_INVALID_PARAMETER One or more of the given inputs are not valid
*/

extern
NFCSTATUS
phHciNfc_WIMgmt_Initialise(
                                phHciNfc_sContext_t     *psHciContext,
                                void                    *pHwRef
                         );
/**
* \ingroup grp_hci_nfc
*
* \brief Allocates the resources required for  WI gate management.
*
* This function Allocates necessary resources as requiered by WI gate management
*
* \param[in]  psHciContext            psHciContext is the pointer to HCI Layer
*
* \retval NFCSTATUS_SUCCESS           Function execution is successful
*
* \retval NFCSTATUS_INVALID_PARAMETER One or more of the given inputs are not valid
*/
extern
NFCSTATUS
phHciNfc_WI_Update_PipeInfo(
                                  phHciNfc_sContext_t     *psHciContext,
                                  uint8_t                 pipeID,
                                  phHciNfc_Pipe_Info_t    *pPipeInfo
                                  );

/**
* \ingroup grp_hci_nfc
*
* \brief Allocates the resources required for  WI gate management.
*
* This function Allocates necessary resources as requiered by WI gate management
*
* \param[in]  psHciContext            psHciContext is the pointer to HCI Layer
*
* \retval NFCSTATUS_SUCCESS           Function execution is successful
*
* \retval NFCSTATUS_INVALID_PARAMETER One or more of the given inputs are not valid
*/
extern
NFCSTATUS
phHciNfc_WI_Configure_Mode(
                                  void                *psHciHandle,
                                  void                *pHwRef,
                                  phHal_eSmartMX_Mode_t   cfg_Mode
                          );

extern
NFCSTATUS
phHciNfc_WI_Configure_Notifications(
                                    void        *psHciHandle,
                                    void        *pHwRef,
                                    phHciNfc_WI_Events_t eNotification
                                );

extern
NFCSTATUS
phHciNfc_WI_Get_PipeID(
                       phHciNfc_sContext_t        *psHciContext,
                       uint8_t                    *ppipe_id
                   );

extern
NFCSTATUS
phHciNfc_WI_Configure_Default(
                              void                  *psHciHandle,
                              void                  *pHwRef,
                              uint8_t               enable_type
                          );

extern
NFCSTATUS
phHciNfc_WI_Get_Default(
                        void                  *psHciHandle,
                        void                  *pHwRef,
                        uint8_t               *p_mode
                        );

extern
NFCSTATUS
phHciNfc_WI_Get_Session(
                        void                  *psHciHandle,
                        void                  *pHwRef
                        );



extern
NFCSTATUS
phHciNfc_WI_Presence(
                        void                  *psHciHandle,
                        void                  *pHwRef
                        );


#endif /* #ifndef PHHCINFC_WI_H */
