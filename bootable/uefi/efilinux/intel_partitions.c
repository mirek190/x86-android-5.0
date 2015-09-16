/*
 * Copyright (c) 2013, Intel Corporation
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
#include <uefi_utils.h>
#include <bootimg.h>
#include <stdlib.h>
#include <string.h>
#include <bootloader.h>
#include <tco_reset.h>

#include "efilinux.h"
#include "bootlogic.h"
#include "platform/platform.h"

#define BOOT_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00}}
#define RECOVERY_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01}}
#define FASTBOOT_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}}
#define TEST_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04}}
#define MISC_GUID	{0x80868086, 0x8086, 0x8086, {0x80, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03}}
#define NULL_GUID	{0}

struct target_entry {
	enum targets target;
	CHAR16 *name;
	EFI_GUID guid;
	CHAR8 *cmdline;
};

static struct target_entry android_entries[] = {
	{TARGET_BOOT	 , L"main"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=main"},
	{TARGET_BOOT	 , L"android"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=main"},
	{TARGET_RECOVERY , L"recovery"	, RECOVERY_GUID , (CHAR8 *) "androidboot.mode=fota"},
	{TARGET_FASTBOOT , L"fastboot"	, FASTBOOT_GUID , (CHAR8 *) "androidboot.mode=fastboot"},
	{TARGET_FASTBOOT , L"bootloader", FASTBOOT_GUID , (CHAR8 *) "androidboot.mode=fastboot"},
	{TARGET_TEST	 , L"test"	, TEST_GUID     , (CHAR8 *) "androidboot.mode=test"},
	{TARGET_CHARGING , L"charging"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=charger"},
#ifdef CONFIG_ENABLE_FACTORY_MODES
	{TARGET_FACTORY  , L"factory"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=factory"},
	{TARGET_FACTORY2 , L"factory2"	, BOOT_GUID     , (CHAR8 *) "androidboot.mode=factory2"},
#endif	/* CONFIG_ENABLE_FACTORY_MODES */
	{TARGET_DNX	 , L"dnx"	, NULL_GUID     , NULL},
};

struct target_entry *get_target_entry(enum targets target)
{
	UINTN i;
	for (i = 0; i < sizeof(android_entries) / sizeof(*android_entries); i++)
		if (android_entries[i].target == target)
			return &android_entries[i];
	return NULL;
}

static const UINT64	 EFI_OS_INDICATION_RESCUE_MODE	  = 1 << 5;
static CHAR16		*OS_INDICATIONS_SUPPORTED_VARNAME = L"OsIndicationsSupported";
static CHAR16		*OS_INDICATIONS_VARNAME		  = L"OsIndications";

static EFI_STATUS intel_go_to_rescue_mode(void)
{
	UINTN size;
	UINT64 value = EFI_OS_INDICATION_RESCUE_MODE;
	EFI_STATUS status;

	UINT64 *supported = LibGetVariableAndSize(OS_INDICATIONS_SUPPORTED_VARNAME,
						  &EfiGlobalVariable, &size);
	if (!supported || size != sizeof(*supported)) {
		error(L"Failed to get %s variable\n", OS_INDICATIONS_SUPPORTED_VARNAME);
		return EFI_LOAD_ERROR;
	}

	if (!(*supported & EFI_OS_INDICATION_RESCUE_MODE)) {
		error(L"Rescue mode not supported\n");
		return EFI_UNSUPPORTED;
	}

	UINT64 *os_indications = LibGetVariableAndSize(OS_INDICATIONS_VARNAME,
						       &EfiGlobalVariable, &size);
	if (os_indications && size != sizeof(*os_indications)) {
		error(L"%s has incorrect size\n", OS_INDICATIONS_VARNAME);
		return EFI_LOAD_ERROR;
	}

	if (os_indications)
		value |= *os_indications;

	status = LibSetNVVariable(OS_INDICATIONS_VARNAME, &EfiGlobalVariable,
				  sizeof(*os_indications), &value);
	if (EFI_ERROR(status)) {
		error(L"Failed to set %s variable\n", OS_INDICATIONS_VARNAME);
		return status;
	}

	uefi_reset_system(EfiResetWarm);

	return EFI_SUCCESS;
}

EFI_STATUS intel_load_target(enum targets target, CHAR8 *cmdline)
{
	CHAR8 *updated_cmdline;
	struct bootimg_hooks hooks;

	struct target_entry *entry = get_target_entry(target);
	if (!entry) {
		error(L"Target 0x%x not supported\n", target);
		return EFI_UNSUPPORTED;
	}

	if (target == TARGET_DNX)
		return intel_go_to_rescue_mode();

	updated_cmdline = append_strings(cmdline, entry->cmdline);

	debug(L"target cmdline = %a\n", updated_cmdline ? updated_cmdline : (CHAR8 *)"");
	debug(L"Loading target %s\n", entry->name);

	hooks.before_exit = loader_ops.hook_before_exit;
	hooks.before_jump = loader_ops.hook_before_jump;
	hooks.watchdog = tco_start_watchdog;

	return android_image_start_partition(&entry->guid, updated_cmdline, &hooks);
}

static struct target_entry *name_to_entry(CHAR16 *name)
{
	int i;
	for (i = 0; i < sizeof(android_entries) / sizeof(*android_entries); i++)
		if (!StrCmp(name, android_entries[i].name))
			return &android_entries[i];
	return NULL;
}

EFI_STATUS name_to_guid(CHAR16 *name, EFI_GUID *guid) {
	struct target_entry *entry = name_to_entry(name);

	if (entry) {
		memcpy((CHAR8 *)guid, (CHAR8 *)&entry->guid, sizeof(EFI_GUID));
		return EFI_SUCCESS;
	}

	return EFI_INVALID_PARAMETER;
}

EFI_STATUS name_to_target(CHAR16 *name, enum targets *target)
{
	struct target_entry *entry = name_to_entry(name);

	if (entry) {
		*target = entry->target;
		return EFI_SUCCESS;
	}

	return EFI_INVALID_PARAMETER;
}

EFI_STATUS target_to_name(enum targets target, CHAR16 **name)
{
	struct target_entry *entry = get_target_entry(target);

	if (entry) {
		*name = entry->name;
		return EFI_SUCCESS;
	}

	return EFI_INVALID_PARAMETER;
}

EFI_STATUS check_gpt(void) {
	/* TODO */
	debug(L"NOT IMPLEMENTED\n");
	return EFI_SUCCESS;
}

static EFI_STATUS read_bcb(struct bootloader_message *bcb)
{
	EFI_STATUS ret;
	EFI_BLOCK_IO *BlockIo;
	EFI_DISK_IO *DiskIo;
	UINT32 MediaId;
	EFI_GUID bcb_guid = MISC_GUID;

	ret = open_partition(&bcb_guid, &MediaId, &BlockIo, &DiskIo);
	if (EFI_ERROR(ret)) {
		warning(L"Misc partition not found: %r\n", ret);
		return EFI_NOT_FOUND;
	}

	ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
				sizeof(*bcb), bcb);
	if (EFI_ERROR(ret)) {
		warning(L"Could not read Misc partition: %r\n", ret);
		return ret;
	}

	return EFI_SUCCESS;
}

enum targets load_bcb(void)
{
	EFI_STATUS ret;
	struct bootloader_message bcb;

	ret = read_bcb(&bcb);
	if (EFI_ERROR(ret))
		return TARGET_UNKNOWN;

	if (!strcmpa((CHAR8 *)bcb.command, (CHAR8 *)"boot-recovery")) {
		debug(L"BCB request to boot in recovery\n");
		return TARGET_RECOVERY;
	}

	return TARGET_UNKNOWN;
}
