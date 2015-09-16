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
#include <fs.h>
#include <uefi_utils.h>
#include <protocol.h>
#include <bootimg.h>
#include <stdlib.h>
#include <string.h>
#include <tco_reset.h>

#include "efilinux.h"
#include "acpi.h"
#include "esrt.h"
#include "intel_partitions.h"
#include "bootlogic.h"
#include "platform/platform.h"
#include "commands.h"
#include "em.h"
#include "config.h"

#define ERROR_STRING_LENGTH	32

#ifndef EFILINUX_BUILD_STRING
#define EFILINUX_BUILD_STRING L"undef"
#endif

#ifndef EFILINUX_VERSION_STRING
#define EFILINUX_VERSION_STRING L"undef"
#endif

#ifndef EFILINUX_VERSION_DATE
#define EFILINUX_VERSION_DATE L"undef"
#endif

static CHAR16 *banner = L"efilinux loader %d.%d %a %a %a\n";
EFI_HANDLE efilinux_image;
void *efilinux_image_base;
EFI_HANDLE main_image_handle;

EFI_SYSTEM_TABLE *sys_table;
EFI_BOOT_SERVICES *boot;
EFI_RUNTIME_SERVICES *runtime;

extern EFI_STATUS start_boot_logic(CHAR8 *cmdline);

struct efilinux_commands {
	CHAR16 *name;
	void (*func)(void);
} commands[] = {
	{L"dump_infos", dump_infos},
	{L"print_pidv", print_pidv},
	{L"print_rsci", print_rsci},
	{L"dump_acpi_tables", dump_acpi_tables},
	{L"load_dsdt", load_dsdt},
	{L"print_esrt", print_esrt_table},
};

#ifdef RUNTIME_SETTINGS
static EFI_STATUS print_memory_map(void)
{
	EFI_MEMORY_DESCRIPTOR *buf;
	UINTN desc_size;
	UINT32 desc_version;
	UINTN size, map_key;
	EFI_MEMORY_DESCRIPTOR *desc;
	EFI_STATUS err;
	int i;

	err = memory_map(&buf, &size, &map_key,
			 &desc_size, &desc_version);
	if (err != EFI_SUCCESS)
		return err;

	info(L"System Memory Map\n");
	info(L"System Memory Map Size: %d\n", size);
	info(L"Descriptor Version: %d\n", desc_version);
	info(L"Descriptor Size: %d\n", desc_size);

	desc = buf;
	i = 0;

	while ((void *)desc < (void *)buf + size) {
		UINTN mapping_size;

		mapping_size = desc->NumberOfPages * PAGE_SIZE;

		info(L"[#%.2d] Type: %s\n", i,
		     memory_type_to_str(desc->Type));

		info(L"      Attr: 0x%016llx\n", desc->Attribute);

		info(L"      Phys: [0x%016llx - 0x%016llx]\n",
		     desc->PhysicalStart,
		     desc->PhysicalStart + mapping_size);

		info(L"      Virt: [0x%016llx - 0x%016llx]",
		     desc->VirtualStart,
		     desc->VirtualStart + mapping_size);

		info(L"\n");
		desc = (void *)desc + desc_size;
		i++;
	}

	FreePool(buf);
	return err;
}

static void (*saved_hook_before_jump)(void);

static void __attribute__((noreturn)) hook_before_jump_forever_loop(void)
{
	if (saved_hook_before_jump)
		saved_hook_before_jump();

	while (1)
		;
}
#endif

static inline BOOLEAN isspace(CHAR16 ch)
{
	return ((unsigned char)ch <= ' ');
}

static CHAR16 *get_argument(CHAR16 *n, CHAR16 **argument)
{
	CHAR16 *word;
	int i;
	CHAR16 *o;

	/* Skip whitespace */
	while (isspace(*n))
		n++;
	word = n;
	i = 0;
	while (*n && !isspace(*n)) {
		i++;
		n++;
	}
	*n++ = '\0';

	o = malloc(sizeof(*o) * (i + 1));
	if (!o) {
		error(L"Unable to alloc argument memory\n");
		goto error;
	}
	o[i--] = '\0';

	StrCpy(o, word);

	*argument = o;
	goto out;
error:
	free(o);
out:
	return n;
}

static EFI_STATUS
parse_args(CHAR16 *options, UINT32 size, CHAR16 *type, CHAR16 **name, CHAR8 **cmdline)
{
	CHAR16 *n;
	EFI_STATUS err;
	int i = 0;

	*cmdline = NULL;
	*name = NULL;

	/* Skip whitespace */
	for (i = 0; i < size && isspace(options[i]); i++)
		     ;

	/* No arguments */
	if (i == size) {
		debug(L"No args\n");
		return EFI_SUCCESS;
	}

	n = &options[i];
	while (n <= &options[size]) {
		if (*n == '-') {
			switch (*++n) {
			case 'h':
				goto usage;
#ifdef RUNTIME_SETTINGS
			case 'l':
				blk_init();
				list_blk_devices();
				blk_exit();
				goto fail;
			case 'm':
				print_memory_map();
				n++;
				goto fail;
			case 'f':
			case 'p':
			case 't':
			case 'c':
#endif	/* RUNTIME_SETTINGS */
			case 'a':
				*type = *n;
				n++;

				n = get_argument(n, name);
				if (!*name)
					goto fail;
				break;
#ifdef RUNTIME_SETTINGS
			case 'e': {
				CHAR16 *em_policy;
				EFI_STATUS status;
				n++;
				n = get_argument(n, &em_policy);
				if (!*em_policy)
					goto usage;
				status = em_set_policy(em_policy);
				if (EFI_ERROR(status))
					goto usage;
				break;
			}
			case 'n': {
				saved_hook_before_jump = loader_ops.hook_before_jump;
				loader_ops.hook_before_jump = hook_before_jump_forever_loop;
				warning(L"-n option set : OSloader will not jump to kernel\n");
				break;
			}
			case 'A':
				list_acpi_tables();
				goto fail;
#endif	/* RUNTIME_SETTINGS */
			default:
				error(L"Unknown command-line switch\n");
				goto usage;
			}
		} else {
			CHAR8 *s1;
			CHAR16 *s2;
			int j;

			j = StrLen(n);
			*cmdline = malloc(j + 1);
			if (!*cmdline) {
				error(L"Unable to alloc cmdline memory\n");
				err = EFI_OUT_OF_RESOURCES;
				goto free_name;
			}

			s1 = *cmdline;
			s2 = n;

			while (j--)
				*s1++ = *s2++;

			*s1 = '\0';

			/* Consume the rest of the args */
			n = &options[size] + 1;
		}
	}

	return EFI_SUCCESS;

usage:
#ifdef RUNTIME_SETTINGS
	Print(L"usage: efilinux [OPTIONS] <kernel-command-line-args>\n\n");
	Print(L"\t-h:             display this help menu\n");
	Print(L"\t-l:             list boot devices\n");
	Print(L"\t-m:             print memory map\n");
#else
	Print(L"usage: efilinux [OPTIONS]\n\n");
#endif	/* RUNTIME_SETTINGS */
	Print(L"\t-a <address>:   boot an already in memory image\n");
#ifdef RUNTIME_SETTINGS
	Print(L"\t-A:             List ACPI tables\n");
	Print(L"\t-e <policy>:    Set the energy management policy ('uefi', 'fake')\n");
	Print(L"\t-f <filename>:  image to load\n");
	Print(L"\t-p <partname>:  partition to load\n");
	Print(L"\t-t <target>:    target to boot\n");
	Print(L"\t-n:             do as usual but wait indefinitely instead of jumping to the loaded image (for test purpose only)\n");
	Print(L"\t-c <command>:   debug commands (dump_infos, print_pidv, print_rsci,\n");
	Print(L"\t                dump_acpi_tables, print_esrt or load_dsdt)\n");
#endif	/* RUNTIME_SETTINGS */

fail:
	err = EFI_INVALID_PARAMETER;

	if (*cmdline)
		free(*cmdline);

free_name:
	if (*name)
		free(*name);
	return err;
}

static inline BOOLEAN
get_path(EFI_LOADED_IMAGE *image, CHAR16 *path, UINTN len)
{
	CHAR16 *buf, *p, *q;
	int i, dev;

	dev = handle_to_dev(image->DeviceHandle);
	if (dev == -1) {
		error(L"Couldn't find boot device handle\n");
		return FALSE;
	}

	/* Find the path of the efilinux executable*/
	p = DevicePathToStr(image->FilePath);
	if (!p) {
		error(L"Failed to get string from device path\n");
		return FALSE;
	}

	q = p + StrLen(p);

	i = StrLen(p);
	while (*q != '\\' && *q != '/') {
		q--;
		i--;
	}

	buf = malloc((i + 1) * sizeof(CHAR16));
	if (!buf) {
		error(L"Failed to allocate buf\n");
		FreePool(p);
		return FALSE;
	}

	memcpy((CHAR8 *)buf, (CHAR8 *)p, i * sizeof(CHAR16));
	FreePool(p);

	buf[i] = '\0';
	SPrint(path, len, L"%d:%s\\%s", dev, buf, EFILINUX_CONFIG);

	return TRUE;
}

#ifdef RUNTIME_SETTINGS
static BOOLEAN
read_config_file(EFI_LOADED_IMAGE *image, CHAR16 **options,
		 UINT32 *options_size)
{
	struct file *file;
	EFI_STATUS err;
	CHAR16 path[4096];
	CHAR16 *u_buf, *q;
	char *a_buf, *p;
	UINT64 size;
	int i;

	err = get_path(image, path, sizeof(path));
	if (err != TRUE)
		return FALSE;

	err = file_open(image, path, &file);
	if (err != EFI_SUCCESS)
		return FALSE;

	err = file_size(file, &size);
	if (err != EFI_SUCCESS)
		goto fail;

	/*
	 * The config file contains ASCII characters, but the command
	 * line parser expects arguments to be UTF-16. Convert them
	 * once we've read them into 'a_buf'.
	 */

	/* Make sure we don't overflow the UINT32 */
	if (size > 0xffffffff || (size * 2) > 0xffffffff ) {
		error(L"Config file size too large. Ignoring.\n");
		goto fail;
	}

	a_buf = malloc((UINTN)size);
	if (!a_buf) {
		error(L"Failed to alloc buffer %d bytes\n", size);
		goto fail;
	}

	u_buf = malloc((UINTN)size * 2);
	if (!u_buf) {
		error(L"Failed to alloc buffer %d bytes\n", size);
		free(a_buf);
		goto fail;
	}

	err = file_read(file, (UINTN *)&size, a_buf);
	if (err != EFI_SUCCESS)
		goto fail;

	info(L"Using efilinux config file\n");

	/*
	 * Read one line. Stamp a NUL-byte into the buffer once we've
	 * read the end of the first line.
	 */
	for (p = a_buf, i = 0; *p && *p != '\n' && i < size; p++, i++)
		;
	if (*p == '\n')
		*p++ = '\0';

	if (i == size && *p) {
		error(L"Missing newline at end of config file?\n");
		goto fail;
	}

	if ((p - a_buf) < size)
		warning(L"config file contains multiple lines?\n");

	p = a_buf;
	q = u_buf;
	for (i = 0; i < size; i++)
		*q++ = *p++;
	free(a_buf);

	*options = u_buf;
	*options_size = (UINT32)size * 2;

	file_close(file);
	return TRUE;
fail:
	file_close(file);
	return FALSE;
}
#else
static BOOLEAN
read_config_file(EFI_LOADED_IMAGE *image, CHAR16 **options,
		 UINT32 *options_size)
{
	return FALSE;
}
#endif

#define EFILINUX_VERSION_VARNAME EFILINUX_VAR_PREFIX "Version"
static EFI_STATUS store_osloader_version(char *version)
{
	EFI_STATUS ret;

	ret = uefi_set_simple_var(EFILINUX_VERSION_VARNAME, &osloader_guid, strlen(version) + 1, version, FALSE);
	if (EFI_ERROR(ret))
		warning(L"Failed to set %a EFI variable\n", EFILINUX_VERSION_VARNAME, ret);

	return ret;
}

static VOID log_init(VOID)
{
	EFI_STATUS err;
	err = log_set_logtag(LOG_TAG);
	if (EFI_ERROR(err)) {
		warning(L"Could not set log tag: %r\n", err);
	}

	log_set_line_len(LOG_LINE_LEN);
	log_set_flush_to_var(LOG_FLUSH_TO_VARIABLE);
	log_set_loglevel(LOG_LEVEL);
	log_set_logtimestamp(LOG_TIMESTAMP);
}

/**
 * efi_main - The entry point for the OS loader image.
 * @image: firmware-allocated handle that identifies the image
 * @sys_table: EFI system table
 */
EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *_table)
{
	WCHAR *error_buf;
	EFI_STATUS err;
	EFI_LOADED_IMAGE *info;
	CHAR16 *name = NULL;
	CHAR16 *options;
	BOOLEAN options_from_conf_file = FALSE;
	UINT32 options_size;
	CHAR8 *cmdline = NULL;
	struct bootimg_hooks hooks;

	main_image_handle = image;
	InitializeLib(image, _table);
	sys_table = _table;
	boot = sys_table->BootServices;
	runtime = sys_table->RuntimeServices;

	if (CheckCrc(ST->Hdr.HeaderSize, &ST->Hdr) != TRUE)
		return EFI_LOAD_ERROR;

	log_init();

	info(banner, EFILINUX_VERSION_MAJOR, EFILINUX_VERSION_MINOR,
		EFILINUX_BUILD_STRING, EFILINUX_VERSION_STRING,
		EFILINUX_VERSION_DATE);

	store_osloader_version(EFILINUX_BUILD_STRING);

	err = fs_init();
	if (err != EFI_SUCCESS)
		error(L"fs_init failed, DnX mode ?\n");

	err = handle_protocol(image, &LoadedImageProtocol, (void **)&info);
	if (err != EFI_SUCCESS)
		goto fs_deinit;

	efilinux_image_base = info->ImageBase;
	efilinux_image = info->DeviceHandle;

	if (!read_config_file(info, &options, &options_size)) {
		int i;

		options = info->LoadOptions;
		options_size = info->LoadOptionsSize;

		/* Skip the first word, that's our name. */
		for (i = 0; i < options_size && options[i] != ' '; i++)
			;
		options = &options[i];
		options_size -= i;
	} else
		options_from_conf_file = TRUE;

	err = init_platform_functions();
	if (EFI_ERROR(err)) {
		error(L"Failed to initialize platform: %r\n", err);
		goto fs_deinit;
	}

	CHAR16 type = '\0';
	if (options && options_size != 0) {
		err = parse_args(options, options_size, &type, &name, &cmdline);

		if (options_from_conf_file)
			free(options);

		/* We print the usage message in case of invalid args */
		if (err == EFI_INVALID_PARAMETER) {
			fs_exit();
			return EFI_SUCCESS;
		}

		if (err != EFI_SUCCESS)
			goto fs_deinit;
	}

	hooks.before_exit = loader_ops.hook_before_exit;
	hooks.before_jump = loader_ops.hook_before_jump;
	hooks.watchdog = tco_start_watchdog;

	debug(L"shell cmdline=%a\n", cmdline);
	switch(type) {
	case 'f':
		if (!name) {
			error(L"No file name specified or name is empty\n");
			goto free_args;
		}
		info(L"Starting file %s\n", name);
		err = android_image_start_file(info->DeviceHandle, name, cmdline, &hooks);
		break;
	case 't': {
		enum targets target;
		if ((err = name_to_target(name, &target)) != EFI_SUCCESS) {
			error(L"Unknown target name %s\n", name);
			goto free_args;
		}
		info(L"Starting target %s\n", name);
		loader_ops.load_target(target, cmdline);
		break;
	}
	case 'p': {
		EFI_GUID part_guid;
		if ((err = name_to_guid(name, &part_guid)) != EFI_SUCCESS) {
			error(L"Unknown target name %s\n", name);
			goto free_args;
		}
		info(L"Starting partition %s\n", name);
		err = android_image_start_partition(&part_guid, cmdline, &hooks);
		break;
	}
	case 'c': {
		int i;
		for (i = 0 ; i < sizeof(commands) / sizeof(*commands); i++)
			if (!StrCmp(commands[i].name, name))
				commands[i].func();
		err = EFI_SUCCESS;
	}
		break;
	case 'a':
	{
		CHAR16 *endptr;
		VOID * addr = (VOID *)strtoul16(name, &endptr, 0);
		if ((name[0] == '\0' || *endptr != '\0')) {
			error(L"Failed to convert %s into address\n", name);
			goto free_args;
		}
		debug(L"Loading android image at 0x%x\n", addr);
		err = android_image_start_buffer(addr, cmdline, &hooks);
		break;
	}
	default:
		debug(L"type=0x%x, starting bootlogic\n", type);
		err = start_boot_logic(cmdline);
		if (EFI_ERROR(err)) {
			error(L"Boot logic failed: %r\n", err);
			goto free_args;
		}
	}

free_args:
	if (cmdline)
		free(cmdline);
	if (name)
		free(name);
fs_deinit:
	fs_exit();
	/*
	 * We need to be careful not to trash 'err' here. If we fail
	 * to allocate enough memory to hold the error string fallback
	 * to returning 'err'.
	 */

	error_buf = AllocatePool(ERROR_STRING_LENGTH);
	if (!error_buf) {
		error(L"Couldn't allocate pages for error string\n");
		return EFI_OUT_OF_RESOURCES;
	}

	StatusToString(error_buf, err);
	error(L": %s\n", error_buf);

	loader_ops.hook_before_exit();

	return exit(image, err, ERROR_STRING_LENGTH, error_buf);
}
