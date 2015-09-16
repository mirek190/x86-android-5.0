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
#include <log.h>
#include "pmic.h"
#include "smbios.h"
#include "efilib.h"

EFI_GUID DeviceSmbiosProtocolGuid = EFI_SMBIOS_PROTOCOL_GUID;

enum pmic_types pmic_get_type_from_smbios()
{
	EFI_STATUS ret;
	EFI_SMBIOS_PROTOCOL *smbios;
	EFI_SMBIOS_HANDLE smbios_handle;
	EFI_SMBIOS_TYPE smbios_type = TYPE_INTEL_SMBIOS;
	SMBIOS_TABLE_TYPE94 *smbios_table;
	char *buffer_str, *buffer_ptr;
	enum pmic_types pmic_type = PMIC_TYPE_UNKNOWN;
	int i;

	ret = LibLocateProtocol(&DeviceSmbiosProtocolGuid, (VOID **)&smbios);
	if (EFI_ERROR(ret) || !smbios) {
		error(L"Failed to get smbios protocol: %r\n", ret);
		return pmic_type;
	}

	ret = uefi_call_wrapper(smbios->GetNext, 5, smbios,
				&smbios_handle, &smbios_type,
				(EFI_SMBIOS_TABLE_HEADER **)&smbios_table, NULL);
	if (EFI_ERROR(ret)) {
		error(L"Failed to call smbios getNext: %r\n", ret);
		return pmic_type;
	}

	buffer_str = buffer_ptr = ((char *)smbios_table) + smbios_table->hdr.Length;
	i = 1;
	while (i < smbios_table->PmicVersion)
	{
		if (*buffer_str == 0)
			break;
		while (*buffer_ptr != 0)
			buffer_ptr++;
		buffer_ptr++;
		buffer_str = buffer_ptr;
		i++;
	}

	if (buffer_str[0] == 0x31 && buffer_str[1] == 0x46) //1F
		pmic_type = CRYSTAL_ROHM;
	else if (buffer_str[0] == 0x34 && buffer_str[1] == 0x31) //41
		pmic_type = DOLLAR_XPOWER;
	else if (buffer_str[0] == 0x30 && buffer_str[1] == 0x33) //03
		pmic_type = DOLLAR_TI;

	return pmic_type;
}
