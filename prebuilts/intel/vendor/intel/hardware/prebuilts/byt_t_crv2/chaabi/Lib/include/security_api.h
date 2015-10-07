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

#ifndef __SECURITY_API_H__
#define __SECURITY_API_H__

#include "sepdrm-common.h"


//
// Maximum and minimum eFuse DWORD indices, inclusive.
//
#define MAX_EFUSE_DWORD_INDEX 7
#define MIN_EFUSE_DWORD_INDEX 0


//
// eFuse DWORD index lock type values. These are 4-bit values.
//
#define SEP_EFUSE_DWORD_ATTRIBUTE_UNLOCKED    0x0
#define SEP_EFUSE_DWORD_ATTRIBUTE_STICKY_LOCK 0x1
#define SEP_EFUSE_DWORD_ATTRIBUTE_PERM_LOCK   0x2


//
// Maximum eFuse DWORD index attribute value.
//
#define MAX_EFUSE_DWORD_ATTRIBUTE SEP_EFUSE_DWORD_ATTRIBUTE_PERM_LOCK


//
// Size of an OTP key hash in bytes.
//
#define OTP_HASH_SIZE_IN_BYTES 32


//
// sep_get_unique_id() related definitions.
//


#define SALT_STRING_BYTE_LENGTH (16)

// Salt string parameter type.
typedef struct {
	DxUint8_t au8[SALT_STRING_BYTE_LENGTH];
} SaltString;

// SaltString initializer, usage: SaltString saltString = SaltString_INIT;
#define SaltString_INIT { { 0 } }

#define DEVICE_UNIQ_BYTE_LENGTH (16)

// sep_get_unique_id() parameter type.
typedef struct {
	DxUint8_t au8[DEVICE_UNIQ_BYTE_LENGTH];
} DevUniq;

// DevUniq initializer, usage: DevUniq devuniq = DevUniq_INIT;
#define DevUniq_INIT { { 0 } }


/**
 * @brief Write eFuse bit field to the appropriate eFise DWORD
 * @param eFuseDWord The eFuse DWORD index to write the bit field to
 * @param bitfield The eFuse bit field to write
 * @return SEP_SUCCESS Writing the bit field succeeded
 *
 * This API will only set the bits in the eFuse bit field. It will not clear any
 * eFuse bits.
 */
SEP_RETURN sep_efuse_write( uint32_t efuse_value, uint32_t bitfield );

/**
 * @brief Locks the eFuse DWORD with either a sticky lock or a permanent lock
 * @param eFuseDWord eFuse DWORD index to lock
 * @param LockType Type of lock to use
 * @return SEP_SUCCESS The eFuse DWORD index was locked
 */
SEP_RETURN sep_efuse_lock( uint32_t eFuseDWord, uint32_t LockType );

/**
 * @brief Returns the eFuse DWORD index bit field and lock attribute
 * @param peFuseDWord eFuse DWORD index to read; eFuse DWORD bit field return value
 * @param pLockType eFuse DWORD lock type return value
 * @return SEP_SUCCESS The eFuse DWORD index was read successfully
 */
SEP_RETURN sep_efuse_read( uint32_t *peFuseDWord, uint32_t *pLockType );

/**
 * sep_keybox_prov
 * @blobSize:	size of data contained in pEncBlob
 * @pEncBlob:	encrypted provisioning data blob
 *   0 = not engineering override; use boot status
 * @returns: 0 if okay
 * The data is validated and written to Chaabi ACD index 1
 */
SEP_RETURN sep_keybox_prov(uint32_t blobBize, uint8_t *pEncBlob);

/**
 * sep_boot_signedOS
 * @os_status: 1 = signed o/s; 0 = unsigned o/s
 * @eng_override: 1 = engineering override; ignore trust;
 *   0 = not engineering override; use boot status
 * @returns: 0 if okay
 * The boot status set here is locked; cannot be changed until next boot
 */
SEP_RETURN sep_boot_signedOS(uint32_t os_status, uint32_t eng_override);

/**
 * sep_encrypt_data
 * @pplain_data: pointer to plain data
 * @data_size: size of data; maximum is 4k bytes; must be in
   incremental of 16 bytes (aea block size)
 * @pencrypt_data: pointer to encrypted data
 * @returns: 0 if okay
 * Selects the fuse you want to write to
 */
SEP_RETURN sep_encrypt_data(uint8_t *pplain_data, uint32_t data_size,
		uint8_t *pencrypt_data);

/**
 * sep_decrypt data
 * @pencrypt_data: pointer to encrypted data
 * @data_size: size of data; maximum is 4k bytes; must be in
   incremental of 16 bytes (aea block size)
 * @pplain_data: pointer to plain data
 * @returns: 0 if okay
 * Selects the fuse you want to write to
 */
SEP_RETURN sep_decrypt_data(uint8_t *pencrypt_data, uint32_t data_size,
		uint8_t *pplain_data);

/**
 * sep_get_unique_id
 * @pUniqueID: Array of 16 bytes to receive the Unique ID returned by chaabi.
 *   Size of ID is fixed at 128 bits.
 * @pSaltStr: A 16-byte NULL-terminated aApplication-Unique string used to
 *   generate the uniqueID.
 * @returns: 0 if okay
 * Gets the unique device ID based on RKEK
 * Repeating the API with the same string will result in generating
 *   the same uniqueID.
 */
SEP_RETURN sep_get_uniqueID(const uint8_t pSaltStr[16], uint8_t pUniqueID[16]);

/**
 * sep_set_dcu
 * @returns: 0 if okay
 * Lock the device so that debugging hardware cannot be connected
 * to it.
 */
SEP_RETURN sep_set_dcu(uint32_t bits);

/**
 * sep_lock_dcu
 * @returns: 0 if okay
 * Lock the device so that debugging hardware cannot be connected
 * to it.
 */
SEP_RETURN sep_lock_dcu(void);

/**
 * @brief Reads the hash of an OTP fuse
 * @param Index OTP index to read.
 * @param pOTPHash Pointer to an array of OTP_HASH_SIZE_IN_BYTES bytes
 * @return SEP_SUCCESS The OTP hash was successfully read
 */
SEP_RETURN sep_read_otp_hash( uint32_t Index, uint8_t pOTPHash[ static OTP_HASH_SIZE_IN_BYTES ] );


#endif /* __SECURITY_API_H__ */
