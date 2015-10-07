/**********************************************************************
 * Copyright 2009 - 2013 (c) Intel Corporation. All rights reserved.

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

#ifndef __LIBCHAABIDISK_H__
#define __LIBCHAABIDISK_H__

#define SALT_STRING_LENGTH 32
#define SALT_LENGTH_BYTES 16
#define MASTER_KEY_LENGTH_BYTES 16
#define MAX_PASSWD_LEN 256

typedef enum {
	DISK_SEP_OKAY = 0,
	DISK_SEP_CANT_TURN_ON,
	DISK_SEP_CANT_LOCK_MEMORY,
	DISK_SEP_CANT_TURN_OFF,
	DISK_SEP_CANT_ENCRYPT,
	DISK_SEP_CANT_DECRYPT,
	DISK_SEP_BAD_PARAMETER,
} disk_sep_return_t;

/**
 * Encrypt key using secret key based on rkek and o/s sign status
 * @param[in] masterkey :	Pointer to plaintext master key
 * @param[inout] ciphertext_mkey :	Pointer to encrypted master key
 * @param[in] salt :		Pointer to salt; (16 bytes long)
 * @param[in] password :		Pointer to password; (up to 256 bytes long))
 * @param[in] password_length	length of password including null terminator
 * @return DISK_SEP_OKAY if encryption was successfuly; non zero
 * value if there is a failure
 */
disk_sep_return_t sep_encrypt_key( const uint8_t * const masterkey,
				   uint8_t * const ciphertext_mkey,
				   const uint8_t * const salt,
				   const char * const password,
				   const uint32_t password_length);

/**
 * Decrypt key using secret key based on rkek and o/s sign status
 * @param[inout] masterkey :	Pointer to plaintext master key
 * @param[in] ciphertext_mkey :	Pointer to encrypted key
 * @param[in] salt :		Pointer to salt; (16 bytes long)
 * @param[in] password :		Pointer to password; (up to 256 bytes long))
 * @param[in] password_length	length of password including null terminator
 * @return DISK_SEP_OKAY if decryption was successfuly; non zero
 * value if there is a failure
 */
disk_sep_return_t sep_decrypt_key( uint8_t * const masterkey,
				   const uint8_t * const ciphertext_mkey,
				   const uint8_t * const salt,
				   const char * const password,
				   const uint32_t password_length);

#endif /* __LIBCHAABIDISK_H__ */
