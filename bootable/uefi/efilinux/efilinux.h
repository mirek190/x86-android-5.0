/*
 * Copyright (c) 2011, Intel Corporation
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
 *
 * This file contains some wrappers around the gnu-efi functions. As
 * we're not going through uefi_call_wrapper() directly, this allows
 * us to get some type-safety for function call arguments and for the
 * compiler to check that the number of function call arguments is
 * correct.
 *
 * It's also a good place to document the EFI interface.
 */

#ifndef __EFILINUX_H__
#define __EFILINUX_H__

#define EFILINUX_VERSION_MAJOR 1
#define EFILINUX_VERSION_MINOR 0

#define EFILINUX_CONFIG	L"efilinux.cfg"

#include <uefi_utils.h>
#include <log.h>

extern EFI_HANDLE efilinux_image;
extern EFI_HANDLE main_image_handle;
extern void *efilinux_image_base;

#define PAGE_SIZE	4096

static const CHAR16 *memory_types[] = {
	L"EfiReservedMemoryType",
	L"EfiLoaderCode",
	L"EfiLoaderData",
	L"EfiBootServicesCode",
	L"EfiBootServicesData",
	L"EfiRuntimeServicesCode",
	L"EfiRuntimeServicesData",
	L"EfiConventionalMemory",
	L"EfiUnusableMemory",
	L"EfiACPIReclaimMemory",
	L"EfiACPIMemoryNVS",
	L"EfiMemoryMappedIO",
	L"EfiMemoryMappedIOPortSpace",
	L"EfiPalCode",
};

static inline const CHAR16 *memory_type_to_str(UINT32 type)
{
	if (type >= sizeof(memory_types)/sizeof(CHAR16 *))
		return L"Unknown";

	return memory_types[type];
}
#endif /* __EFILINUX_H__ */
