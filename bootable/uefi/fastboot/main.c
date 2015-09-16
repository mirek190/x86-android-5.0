/*
 * Copyright (c) 2011-2014, Intel Corporation
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
#include <log.h>
#include <protocol.h>
#include <uefi_utils.h>
#include <gpt.h>
#include <bootimg.h>

#include "flash.h"
#include "fastboot.h"

#ifndef FASTBOOT_BUILD_STRING
#define FASTBOOT_BUILD_STRING L"undef"
#endif

#ifndef FASTBOOT_VERSION_STRING
#define FASTBOOT_VERSION_STRING L"undef"
#endif

#ifndef FASTBOOT_VERSION_DATE
#define FASTBOOT_VERSION_DATE L"undef"
#endif

static CHAR16 *banner = L"fastboot application %a %a %a\n";
EFI_HANDLE main_image_handle;
EFI_HANDLE application_handle;

static EFI_STATUS find_label(UINTN argc, CHAR16 **argv)
{
	struct gpt_partition_interface gparti;
	EFI_STATUS ret;
	CHAR16 *label;

	if (argc != 1) {
		error(L"Usage: find_label <label>\n");
		return EFI_INVALID_PARAMETER;
	}

	label = argv[0];
	ret = gpt_get_partition_by_label(label, &gparti);

	if (EFI_ERROR(ret))
		error(L"Label %s not found, error %r\n", label, ret);

	return ret;
}

static EFI_STATUS command_flash_file(UINTN argc, CHAR16 **argv)
{
	if (argc == 2)
		return flash_file(application_handle, argv[1], argv[0]);

	error(L"Usage: flash_file <partition_name> <file>");
	return EFI_INVALID_PARAMETER;
}

static EFI_STATUS command_erase(UINTN argc, CHAR16 **argv)
{
	if (argc == 1)
		return erase_by_label(argv[0]);

	error(L"Usage: erase <partition_name>");
	return EFI_INVALID_PARAMETER;
}

static EFI_STATUS command_boot(UINTN argc, CHAR16 **argv)
{
	if (argc == 1)
		return android_image_start_file(application_handle, argv[0], NULL, NULL);

	error(L"Usage: boot <path_to_bootimg>");
	return EFI_INVALID_PARAMETER;
}

struct fastboot_commands {
	CHAR16 *name;
	EFI_STATUS (*func)(UINTN argc, CHAR16 **argv);
} commands[] = {
	{L"find_label", find_label},
	{L"flash_file", command_flash_file},
	{L"erase", command_erase},
	{L"boot", command_boot},
};

/**
 * efi_main - The entry point for the OS loader image.
 * @image: firmware-allocated handle that identifies the image
 * @sys_table: EFI system table
 */
EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *_table)
{
	EFI_STATUS ret;
	EFI_LOADED_IMAGE *loaded_img = NULL;
	CHAR16 *options;
	CHAR16 *argv[16];
	CHAR16 *opt;

	UINTN argc;
	INTN i;

	InitializeLib(image, _table);

	main_image_handle = image;

	log_set_loglevel(CONFIG_LOG_LEVEL);

	info(banner, FASTBOOT_BUILD_STRING, FASTBOOT_VERSION_STRING,
	     FASTBOOT_VERSION_DATE);

	ret = handle_protocol(image, &LoadedImageProtocol, (void **)&loaded_img);
	if (ret != EFI_SUCCESS) {
		error(L"LoadedImageProtocol error: %r\n", ret);
		return ret;
	}

	application_handle = loaded_img->DeviceHandle;

	options = loaded_img->LoadOptions;

	argc = split_cmdline(options, ARRAY_SIZE(argv), argv);
	opt = argv[1];

	if (opt[0] == '-' && opt[1]  == 'h') {
		Print(L"usage: fastboot [OPTIONS]\n\n");
		Print(L"\t-h:             display this help menu\n");
		Print(L"\t-c <command>:   execute command:\n");
		for (i = 0 ; i < ARRAY_SIZE(commands); i++)
			Print(L"\t\t%s\n", commands[i].name);
		return ret;
	}

	if (argv[1][0] == '-' && argv[1][1] == 'c') {
		if (argc < 3) {
			error(L"Parameter required\n");
			ret = EFI_INVALID_PARAMETER;
			goto out;
		}

		for (i = 0 ; i < sizeof(commands) / sizeof(*commands); i++) {
			if (!StrCmp(commands[i].name, argv[2])) {
				/* temp to make it build, need to pass remaining args as
				 * a table ( &argv[3] ) */
				ret = commands[i].func(argc - 3, argc == 3 ? NULL : &argv[3]);
				return ret;
			}
		}
		error(L"Unkown command %s\n", argv[2]);
		ret = EFI_INVALID_PARAMETER;
		goto out;
	}
	if (argc != 1)
		error(L"Unkown argument %s\n", argv[1]);

	ret = fastboot_start();
out:
	return ret;
}
