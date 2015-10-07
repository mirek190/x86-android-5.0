/**********************************************************************
 * Copyright 2009 - 2012 (c) Intel Corporation. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **********************************************************************/
#ifndef __FKPMIDWARE_H__
#define __FKPMIDWARE_H__

//
// Chaabi EXTFW sub-opcodes for the secure clock operations.
//
#define SECURE_CLOCK_TID_SUB_OPCODE              1
#define SECURE_CLOCK_PARSE_AND_VERIFY_SUB_OPCODE 2
#define SECURE_CLOCK_GET_CLOCK_SUB_OPCODE        3
#define SECURE_CLOCK_PUSH_DELTA_SUB_OPCODE       4
#define SECURE_CLOCK_POP_DELTA_SUB_OPCODE        5

typedef struct
{
	unsigned int chaabi_ipc_magic_number;
	unsigned int chaapi_ipc_msg_length;
	unsigned int chaapi_ipc_code;
	unsigned int chaabi_ipc_rc[];
} iasystem_field;

typedef enum
{
	FKP_COMMAND_PROVISION_PRSEED=0,
	FKP_COMMAND_LOAD_MODEL_KEY,
	FKP_COMMAND_LOAD_KEY,
	FKP_COMMAND_UNLOAD_KEY,
	FKP_COMMAND_UNLOAD_MODEL_KEY,
	LA_SIGN_CHALLENGE,
	LA_LICENSE_RESPONSE,
	DEVCERT_GENKEYPAIR,
	DEVCERT_SIGNCERT,
	PROCESS_SCURE_CLOCK,
	DRM_GET_SCURECLOCK,
	DRM_GET_SECCLK_TID
} FKP_COMMAND;

typedef enum
{
	WV_DRM_COMMAND_GET_KBOX_INFO=0,
	WV_DRM_COMMAND_SET_ENTITLEMENT_KEY,
	WV_DRM_COMMAND_DERIVE_CONTROL_WORD,
} WIDEVINE_COMMAND;


typedef struct
{
    uint32_t 	wKeyDataSize; // default 72 bytes
    uint32_t 	wDeviceIDSize; 	// default 32 bytes
} drm_wv_keybox_info_length_t;

typedef struct
{
	iasystem_field head;
	unsigned int rc;
	unsigned int resultlen;			//it is byte alligned;
	unsigned int debugmsgstartoffset;	//It is dword alligned.
	uint8_t resultbegin[];
} iasystem_ipcresult_t;

typedef struct
{
	iasystem_field  header;
	unsigned int command;
	unsigned int msglen;
	uint8_t         keycontainer[];
} iafkp_ipc_t;

typedef uint8_t ChaabiipcMaxbuff[8*1024];

#define SSM_FLAG_SENDLEN	0x1
#define SSM_FLAG_RCV_DATAONLY	0x2

int sep_ext_loadkey(uint8_t * keycontainer,int len);
int sep_ext_unloadkey(uint8_t * keycontainer,int len);
int sep_ext_key_provision (uint8_t * keycontainer,int len);
int sendSepMessage(int command, int subcommand, int flags,
		uint8_t *data, size_t len, ChaabiipcMaxbuff rcvf, size_t *rcvLen);

#endif /* __FKPMIDWARE_H__ */
