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

#include <efi.h>
#include <efilib.h>
#include "esrt.h"
#include <uefi_utils.h>
#include "efilinux.h"
#include "uefi_utils.h"

static const EFI_GUID ESRT_TABLE_GUID =
	{ 0xb122a263, 0x3661, 0x4f68, { 0x99, 0x29, 0x78, 0xf8, 0xb0, 0xd6, 0x21, 0x80 }};

const EFI_GUID SYS_FW_GUID =
	{ 0xb122a262, 0x3551, 0x4f48, { 0x88, 0x92, 0x55, 0xf6, 0xc0, 0x61, 0x42, 0x90 }};

EFI_STATUS get_esrt_table(struct EFI_SYSTEM_RESOURCE_TABLE **esrt_table)
{
	EFI_STATUS ret;

	ret = LibGetSystemConfigurationTable((EFI_GUID *)&ESRT_TABLE_GUID, (VOID **)esrt_table);

	if (EFI_ERROR(ret))
		error(L"Failed to retrieve ESRT System table %r\n", ret);

	if (*esrt_table == NULL) {
		error(L"Pointer to ESRT table is NULL\n");
		ret = EFI_NOT_FOUND;
	}

	return ret;
}

EFI_STATUS get_fw_entry(EFI_GUID* fw_entry_guid, struct FW_RES_ENTRY **fw_entry)
{
	struct EFI_SYSTEM_RESOURCE_TABLE* esrt_table;
	EFI_STATUS ret;

	ret = get_esrt_table(&esrt_table);
	if (EFI_ERROR(ret))
		return ret;

	int fw_entry_idx;
	for (fw_entry_idx = 0; fw_entry_idx < esrt_table->FwResourceCount; fw_entry_idx++) {
		EFI_GUID guid = esrt_table->FwResEntry[fw_entry_idx].FwClass;
		if (CompareGuid(&guid, fw_entry_guid) == 0) {
			*fw_entry = &esrt_table->FwResEntry[fw_entry_idx];
			return EFI_SUCCESS;
		}
	}

	return EFI_NOT_FOUND;
}

void print_esrt_table(void)
{
	struct EFI_SYSTEM_RESOURCE_TABLE *esrt_table;
	EFI_STATUS ret = EFI_SUCCESS;

	ret = get_esrt_table(&esrt_table);
	if (EFI_ERROR(ret))
		return;

	info(L"ESRT table info ResCount=%d ResMax=%d ResVer=%d\n",
	     esrt_table->FwResourceCount, esrt_table->FwResourceMax, esrt_table->FwResourceVersion);

	int fw_entry_idx;
	for (fw_entry_idx = 0; fw_entry_idx < esrt_table->FwResourceCount; fw_entry_idx++) {
		info(L"FW ENTRY FT=%d FV=%d CF=%d LCV=%d LAV=%d LAS=%d\n",
		     esrt_table->FwResEntry[fw_entry_idx].FwType,
		     esrt_table->FwResEntry[fw_entry_idx].FwVersion,
		     esrt_table->FwResEntry[fw_entry_idx].CapsuleFlags,
		     esrt_table->FwResEntry[fw_entry_idx].FwLstCompatVersion,
		     esrt_table->FwResEntry[fw_entry_idx].LastAttemptVersion,
		     esrt_table->FwResEntry[fw_entry_idx].LastAttemptStatus);
	}
}

