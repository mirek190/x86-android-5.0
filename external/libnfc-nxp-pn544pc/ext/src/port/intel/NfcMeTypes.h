/*++

   INTEL CONFIDENTIAL
   Copyright 2009 - 2012 Intel Corporation All Rights Reserved.
   The source code contained or described herein and all documents related
   to the source code ("Material") are owned by Intel Corporation or its
   suppliers or licensors. Title to the Material remains with Intel Corporation
   or its suppliers and licensors. The Material contains trade secrets and
   proprietary and confidential information of Intel or its suppliers and
   licensors. The Material is protected by worldwide copyright and trade secret
   laws and treaty provisions. No part of the Material may be used, copied,
   reproduced, modified, published, uploaded, posted, transmitted, distributed,
   or disclosed in any way without Intel’s prior express written permission.
--*/

/*++

@file: NfcMeTypes.h

--*/
#include <stdint.h>

#define HCI_MAX_PAYLOAD_SIZE      300

#pragma pack(1)

typedef enum _eNfcMeCommand
{
    NfcMeCommandMaintenance = 0x00,
    NfcMeCommandSend        = 0x01,
    NfcMeCommandReceive     = 0x02
} eNfcMeCommand;

typedef enum _eNfcMeMaintenanceSubcommand
{
    NfcMeMaintenanceSubcommandConnect            = 0x00,
    NfcMeMaintenanceSubcommandVersionQuery       = 0x01,
    NfcMeMaintenanceSubcommandFwUpdateMode       = 0x02,
    NfcMeMaintenanceSubcommandFwUpdateSend       = 0x03,
    NfcMeMaintenanceSubcommandFwUpdateReceive    = 0x04,
    NfcMeMaintenanceSubcommandRfKillMode         = 0x05,
    NfcMeMaintenanceSubcommandPowerMode          = 0x06,
    NfcMeMaintenanceSubcommandGetRadioReadyState = 0x07
} eNfcMeMaintenanceSubcommand;

typedef enum _eNfcMeConnectStatus
{
    NfcMeStatusNormalMode        = 0x00,
    NfcMeStatusFirwareUpdateMode = 0x01
}eNfcMeConnectStatus;

typedef enum _eNfcMeQueryStatus
{
    NfcMeQueryStatusSuccess        = 0x00,
    NfcMeQueryStatusFailedGetRadio = 0x01
}eNfcMeQueryStatus;

typedef enum _eNfcMePowerStatus
{
    NfcMePowerStatusRequestSuccess      = 0x00,
    NfcMePowerStatusRequestFailed       = 0x01,
    NfcMePowerStatusRequestNotSupported = 0x02

}eNfcMePowerStatus;


typedef enum _eNfcMeSendStatus
{
    NfcMeStatusSendSucceed = 0x00,
    NfcMeStatusSendFailed  = 0x01,
    NfcMeStatusSendFWMode  = 0x11,
} eNfcMeSendStatus;

typedef enum _eNfcMeMaintenanceSubcommandFirwareUpdateCmd
{
    NfcMeMaintenanceSubcommandFirmwareUpdateEnter  = 0x00,
    NfcMeMaintenanceSubcommandFirmwareUpdateExit   = 0x01,
    NfcMeMaintenanceSubcommandFirmwareUpdateQuery  = 0xFF
} eNfcMeMaintenanceSubcommandFirwareUpdateCmd;

typedef struct _tNfcHeciMessageHeader
{
    uint8_t        Command;
    uint8_t        Status;
    uint16_t       RequestId;
    uint32_t       Reserved;
    uint16_t       DataSize;
} tNfcHeciMessageHeader;

#pragma pack()