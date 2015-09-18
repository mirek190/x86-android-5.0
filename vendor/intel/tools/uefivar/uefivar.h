/*
 * Copyright (c) 2013-2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
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

#ifndef UEFIVAR_H
#define UEFIVAR_H

#include <stdint.h>
#include <stdbool.h>

#define debug(...)				\
	if (verbose) {				\
		ALOGD(__VA_ARGS__);		\
		printf(__VA_ARGS__);		\
	}
#define info(...) {				\
		ALOGI(__VA_ARGS__);		\
		printf(__VA_ARGS__);		\
	}
#define warn(...) {				\
		ALOGW(__VA_ARGS__);		\
		fprintf(stderr, __VA_ARGS__);	\
	}
#define error(...) {				\
		ALOGE(__VA_ARGS__);		\
		fprintf(stderr, __VA_ARGS__);	\
	}

#define EFI_VARNAME_LEN		 511
#define EFI_VARNAME_SIZE	((EFI_VARNAME_LEN + 1) * 2)
#define GUID_FORMAT		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUID_FORMAT_ELT_NB	  11

#define EFI_VAR_BASE_PATH	   "/sys/firmware/efi/vars"
#define EFI_VAR_PATH		EFI_VAR_BASE_PATH"/%s-%s/raw_var"
#define EFI_NEW_VAR_PATH	EFI_VAR_BASE_PATH"/new_var"

#define	EFI_ATTR_NON_VOLATILE		0x1
#define	EFI_ATTR_BOOTSERVICE_ACCESS	0x2
#define	EFI_ATTR_RUNTIME_ACCESS		0x4
#define	EFI_DEFAULT_ATTRIBUTES		EFI_ATTR_NON_VOLATILE | EFI_ATTR_BOOTSERVICE_ACCESS | EFI_ATTR_RUNTIME_ACCESS



struct guid {
	uint32_t part1;
	uint16_t part2;
	uint16_t part3;
	char part4[8];
};

#ifdef KERNEL_X86_64
#define EFIVAR_SIZE_TYPE long long
#else
#define EFIVAR_SIZE_TYPE long
#endif
struct efi_var {
	char name[EFI_VARNAME_SIZE];
	struct guid guid;
	unsigned EFIVAR_SIZE_TYPE size;
	char data[1024];
	unsigned EFIVAR_SIZE_TYPE status;
	uint32_t attributes;
};

struct arguments {
	struct guid guid;
	char *guid_str;
	char *varname;
	char *value;
	char *value_type;
	unsigned long value_size;
	bool keep_size;
};

int uefi_get_value(char *buffer, char *type);
int parse_guid(const char *guidstr, struct guid *guid);

int uefivar_process_entry(const struct arguments args, struct efi_var ** result);
void uefivar_free_buffer(struct efi_var ** buffer);
#endif


