/*
 * Copyright (c) 2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ESRT_H__
#define __ESRT_H__

#include <efi.h>
#include <efilib.h>

const EFI_GUID SYS_FW_GUID;

struct FW_RES_ENTRY {
	EFI_GUID         FwClass;  /* GUID of FW component */
	UINT32           FwType;
	UINT32           FwVersion;
	UINT32           FwLstCompatVersion;
	UINT32           CapsuleFlags;
	UINT32           LastAttemptVersion;
	UINT32           LastAttemptStatus;
};

struct EFI_SYSTEM_RESOURCE_TABLE {
	UINT32          FwResourceCount;
	UINT32          FwResourceMax;
	UINT64          FwResourceVersion;
	struct FW_RES_ENTRY FwResEntry[];
};

EFI_STATUS get_esrt_table(struct EFI_SYSTEM_RESOURCE_TABLE **esrt_table);
EFI_STATUS get_fw_entry(EFI_GUID* fw_entry_guid, struct FW_RES_ENTRY **esrt);
void print_esrt_table(void);

#endif /* __ESRT_H__ */
