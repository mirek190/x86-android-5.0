/**********************************************************************
 * Copyright (C) 2011 Intel Corporation. All rights reserved.

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


#ifndef __SECURE_TOKEN_H__
#define __SECURE_TOKEN_H__


#include "chaabi_error_codes.h"





//
// Secure token parser sub opcode values.
//
#define SECURE_TOKEN_GENERATE_TOKEN_SUB_OPCODE    0
#define SECURE_TOKEN_PERFORM_OPERATION_SUB_OPCODE 1
#define SECURE_TOKEN_REVOKE_TOKEN_SUB_OPCODE      2


//
// Size of the secure token unique key nonce random number in bytes.
//
#define SECURE_TOKEN_UNIQUE_KEY_NONCE_RANDOM_NUMBER_SIZE_IN_BYTES 12


//
// Size of the nonce in the secure token unique key (in bytes).
//
#define SECURE_TOKEN_UNIQUE_KEY_NONCE_SIZE_IN_BYTES 16


//
// Size of the AES-CMAC in the secure token unique key (in bytes).
//
#define SECURE_TOKEN_UNIQUE_KEY_AES_CMAC_SIZE_IN_BYTES 16


//
// Size of the secure token unique key in bytes.
//
#define SECURE_TOKEN_UNIQUE_KEY_SIZE_IN_BYTES ( SECURE_TOKEN_UNIQUE_KEY_NONCE_SIZE_IN_BYTES + \
                                                SECURE_TOKEN_UNIQUE_KEY_AES_CMAC_SIZE_IN_BYTES )


//
// Size of the Customer ID byte array in bytes.
//
#define SECURE_TOKEN_CUSTOMER_ID_SIZE_IN_BYTES 16


//
// Size of the secure token HMAC in bytes.
//
#define SECURE_TOKEN_HMAC_SIZE_IN_BYTES 32


//
// Minimum size of a secure token in bytes.
//
#define MIN_SECURE_TOKEN_SIZE_IN_BYTES 52


//
// Size of the VRL header for the secure token in bytes. This includes the sizes
// of the CSS header, VRL header, and RSA signature.
//
#define SECURE_TOKEN_BLOB_VRL_HEADER_SIZE_IN_BYTES 480


//
// Minimum size of a secure token blob in bytes.
//
#define MIN_SECURE_TOKEN_BLOB_SIZE_IN_BYTES ( MIN_SECURE_TOKEN_SIZE_IN_BYTES + \
                                              SECURE_TOKEN_BLOB_VRL_HEADER_SIZE_IN_BYTES )


//
// Secure token key index value for being signed with the DPSK.
//
#define SECURE_TOKEN_DPSK_KEY_INDEX 0x10


//
// Macro for adjusting the maximum byte length of a SEP message.
//
#define BYTES_TO_4BYTE_BLOCKS(x) ( ( (x) + 3 ) / 4 )
#define SEP_MSG_PARAM_MAX_LEN(x) ( BYTES_TO_4BYTE_BLOCKS(x) * 4 )


//
// The secure token lifetime value that means the secure token lifetime never
// expires.
//
#define SECURE_TOKEN_LIFETIME_NEVER_EXPIRES 0


//
// Secure token nonce consists of the number of seconds since the secure real
// time clock epoch and a large random number.
//
typedef struct
{
	DxUint32_t secondsSinceEpoch;
	DxUint8_t  aRandomNumber[ SECURE_TOKEN_UNIQUE_KEY_NONCE_RANDOM_NUMBER_SIZE_IN_BYTES ];
} secure_token_nonce_t;


//
// Secure token unique key data type. Contains the nonce and the AES-CMAC of the
// nonce.
//
typedef struct
{
	secure_token_nonce_t nonce;
	DxUint8_t            aAesCmac[ SECURE_TOKEN_UNIQUE_KEY_AES_CMAC_SIZE_IN_BYTES ];
} secure_token_unique_key_t;


//
// Secure token data type, which contains a VRL header, secure token unique key,
// secure token operator, optional operand, and operand size. If the operator
// does not use an operand, then the operand pointer is NULL and the operand
// size is zero.
//
typedef struct
{
	uint32_t                  tokenSignature;
	uint8_t                   keyIndex;
	uint8_t                   numberOfItems;
	uint16_t                  tokenSizeInDwords;
	uint32_t                  intelReserved1;
	uint16_t                  intelreserved2;
	uint16_t                  tokenLifeInMinutes;
	secure_token_unique_key_t uniqueKey;
	uint8_t                   operationType;
	uint8_t                   operationSubType;
	uint16_t                  operationDataSizeInDwords;
} secure_token_t;


//
// Data type for the secure token blob that is sent from the host to Chaabi.
//
typedef struct
{
	uint8_t        aVrlHeader[ SECURE_TOKEN_BLOB_VRL_HEADER_SIZE_IN_BYTES ];
	secure_token_t secureToken;
	uint8_t        aCustomerId[ SECURE_TOKEN_CUSTOMER_ID_SIZE_IN_BYTES ];
} secure_token_blob_t;






/*
 * @brief Gets a secure token unique key from Chaabi.
 * @param[in] uniqueKey A byte array for returning the secure token unique key.
 * @return ST_SUCCESSFUL The secure token request succeeded.
 *
 * The size of the uniqueKey array must be at least
 * SECURE_TOKEN_UNIQUE_KEY_SIZE_IN_BYTES bytes.
 */
ST_RESULT sep_sectoken_request_token( uint8_t uniqueKey[ static SECURE_TOKEN_UNIQUE_KEY_SIZE_IN_BYTES ] );


/*
 * @brief Sends a secure token with an operation to be processed to Chaabi.
 * @param[in] pDataBlob Pointer to the secure token data blob buffer.
 * @param[in] tokenSizeInBytes The total size in bytes of the secure token data
 *            blob.
 * @param[in] pCustomerId An optional pointer to a Customer ID (value may be
 *            NULL).
 * @return ST_SUCCESSFUL The secure token command processing succeeded.
 */
ST_RESULT sep_sectoken_process_command( uint8_t *pDataBlob,
                                        uint32_t tokenSizeInBytes,
                                        uint8_t *pCustomerId );


/*
 * @brief Sends a secure token with an operation to be processed to Chaabi.
 * @param[in] pDataBlob Pointer to the secure token data blob buffer.
 * @param[in] tokenSizeInBytes The total size in bytes of the secure token data
 *            blob.
 * @return ST_SUCCESSFUL The secure token was successfully consumed.
 */
ST_RESULT sep_sectoken_consume_token( uint8_t *pDataBlob, uint32_t tokenSizeInBytes );


/*
 * @brief Revoke a persistent secure token that is stored in Chaabi.
 * @param[in] uniqueKey The secure token unique key, as a byte array, of the
 *            persistent secure token to revoke.
 * @return ST_SUCCESSFUL Revoking the secure token succeeded
 *
 * All persistent secure tokens that contain the secure token unique key will be
 * revoked.
 *
 * The size of the uniqueKey array must be at least
 * SECURE_TOKEN_UNIQUE_KEY_SIZE_IN_BYTES bytes.
 */
ST_RESULT sep_sectoken_revoke_token( uint8_t uniqueKey[ static SECURE_TOKEN_UNIQUE_KEY_SIZE_IN_BYTES ] );




#endif /* __SECURE_TOKEN_H__ */
