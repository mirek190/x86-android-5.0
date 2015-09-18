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
#include "bootlogic.h"
#include "acpi.h"
#include "uefi_utils.h"
#include "warmdump.h"
#include "msgbus.h"
#include "log.h"
#include "protocol.h"
#include "config.h"

#define FILE_SEP L"\\"

#ifndef WARMDUMP_BUILD_STRING
#define WARMDUMP_BUILD_STRING L"undef"
#endif

#ifndef WARMDUMP_VERSION_STRING
#define WARMDUMP_VERSION_STRING L"undef"
#endif

#ifndef WARMDUMP_VERSION_DATE
#define WARMDUMP_VERSION_DATE L"undef"
#endif

#define WARMDUMP_BANNER L"version %d.%d %s %s %s\n"

EFI_SYSTEM_TABLE *sys_table;
EFI_BOOT_SERVICES *boot;
EFI_HANDLE efilinux_image;
EFI_HANDLE main_image_handle;

static BOOLEAN need_backup()
{
	enum reset_types rt;
	enum reset_sources rs;

	rt = rsci_get_reset_type();
	debug(L"Reset type = 0x%x\n", rt);

	rs = rsci_get_reset_source();
	debug(L"Reset source = 0x%x\n", rs);

	if (uefi_get_simple_var("WDColdReset", &osloader_guid) == 1) {
		// Workaround for cold reset issue: if set, we need to restore data
		warning(L"WA: WDColdReset variable is set, restore data\n");
		return FALSE;
	}

	return (rt == WARM_RESET) && (rs == RESET_KERNEL_WATCHDOG);
}

/*
 * Punit registers offsets
 */
#define PUNIT_BIOS_CONFIG 0x6
#define PUNIT_BIOS_CONFIG_PDM (1<<17)
#define PUNIT_BIOS_CONFIG_DFX_PDM_MODE (1<<16)
#define PUNIT_BIOS_CONFIG_DDRIO_PWRGATE (1<<8)

/*
 * Lakemore registers offsets
 */
#define LM_OSMC 0x2004
#define LM_OSMC_SWSUSP (1<<13)
#define LM_OSMC_SWSTOP (1<<11)
#define LM_MEMWRPNT 0x2028
#define LM_STORMEMBAR_L 0x201C
#define LM_STORMEMBAR_H 0x2020
#define LM_OSTAT 0x2014
#define LM_OSTAT_ADDROVRFLW (1<<5)
#define LM_MEMDEPTH 0x2024

enum lm_pdm_dfx_setting {
	LM_POWER_SAVE = 0,
	LM_PERF_MODE,
	LM_NOT_VALID,
	LM_PDM_MODE,
};

static enum lm_pdm_dfx_setting lm_get_pdm_dfx_setting(void)
{
	UINT32 BiosConfig;

	BiosConfig = VlvMsgBusReadPunit(PUNIT_BIOS_CONFIG);

	// Description for values of bits [17:16] in Punit BIOSConig register:
	// 0x0 - PDM Off / Dfx Off (PowerSave)
	// 0x1 - PDM Off / Dfx On  (Perf mode) - PDM and Perf modes are mutually exclusive
	// 0x2 - PDM On	 / Dfx Off (Not Valid) - PDM debug will not work with Dfx off
	// 0x3 - PDM On	 / Dfx On  (PDM mode)

	return ((BiosConfig & 0x00030000) >> 16);
}

static void lm_get_backup_data(void **backup_addr, UINT32 *backup_size)
{
	UINT32 MemDepth;
	EFI_PHYSICAL_ADDRESS LmOutputAddr;

	MemDepth = VlvMsgBusReadDfxLM(LM_MEMDEPTH);
	LmOutputAddr = VlvMsgBusReadDfxLM(LM_STORMEMBAR_H);
	LmOutputAddr = LmOutputAddr << 32;
	LmOutputAddr |= VlvMsgBusReadDfxLM(LM_STORMEMBAR_L);

	info(L"MemDepth:%x\n", MemDepth);
	info(L"LmOutputAddr:%x\n", LmOutputAddr);

	*backup_size = MemDepth << 13; // x 8kB
	*backup_addr = (void*)(UINTN)(LmOutputAddr + *backup_size); // start after current buffer
}

static EFI_STATUS data_write_to_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename,
				     void *data, UINTN size)
{
	EFI_STATUS ret = EFI_SUCCESS;
	UINTN written_size = size;

	debug(L"Writing data to ESP in file %s\n", filename);
	ret = uefi_write_file(io, filename, data, &written_size);
	if (size != written_size)
		error(L"Written %d/%d bytes\n", written_size, size);
	if (EFI_ERROR(ret))
		error(L"Failed to write file %s: %r\n", filename, ret);

	return ret;
}

static EFI_STATUS inject_file_ram(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename,
				  void *addr, UINTN size)
{
	EFI_STATUS ret = EFI_SUCCESS;
	void *buf;
	UINTN read_size;

	debug(L"Reading data from ESP file %s\n", filename);
	ret = uefi_read_file(io, filename, &buf, &read_size);
	if (size != read_size)
		error(L"Read %d/%d bytes\n", read_size, size);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read file %s: %r\n", filename, ret);
		return ret;
	}

	// Copy to ram
	uefi_call_wrapper(BS->CopyMem, 3, addr, buf, read_size);

	FreePool(buf);

	return ret;
}

EFI_STATUS pstore_get_buffer(void **addr, UINTN *size)
{
	EFI_GUID global_var_guid = EFI_GLOBAL_VARIABLE;
	void *var;

	var = LibGetVariable(EFIVAR_PSTORE_ADDR, &global_var_guid);
	if (!var) {
		error(L"Var not found : %s\n", EFIVAR_PSTORE_ADDR);
		return EFI_NOT_FOUND;
	}
	*addr = *(void**)var;

	var = LibGetVariable(EFIVAR_PSTORE_SIZE, &global_var_guid);
	if (!var) {
		error(L"Var not found : %s\n", EFIVAR_PSTORE_SIZE);
		return EFI_NOT_FOUND;
	}
	*size = *(UINTN*)var;

	return EFI_SUCCESS;
}

#define PERSISTENT_RAM_SIG (0x43474244) /* DBGC */

static inline BOOLEAN is_pstore_ram_in_ram(void *addr)
{
	return *((UINT32*)addr) == PERSISTENT_RAM_SIG;
}

#define LM_FILE		BACKUP_DIR FILE_SEP L"lm_dump.bin"

void lm_backup(EFI_FILE_IO_INTERFACE *esp_fs)
{
	enum lm_pdm_dfx_setting lm_set = lm_get_pdm_dfx_setting();
	if (lm_set == LM_PDM_MODE) {
		void *lm_addr;
		UINT32 lm_size;

		lm_get_backup_data(&lm_addr, &lm_size);
		debug(L"LM addr:0x%x size:0x%x\n", lm_addr, lm_size);

		if (lm_addr && lm_size)
			data_write_to_file(esp_fs, LM_FILE, lm_addr, lm_size);
		else
			info(L"No Lakemore buffer in RAM\n");
	} else
		info(L"Lakemore not in PDM MODE, %d\n", lm_set);
}

void lm_restore(EFI_FILE_IO_INTERFACE *esp_fs)
{
	EFI_STATUS ret;
	void *lm_addr;
	UINT32 lm_size;

	if (!uefi_exist_file_root(esp_fs, LM_FILE))
		return;

	lm_get_backup_data(&lm_addr, &lm_size);
	ret = inject_file_ram(esp_fs, LM_FILE, lm_addr, lm_size);
	if (EFI_ERROR(ret)) {
		error(L"Failed to inject Lakemore data, file:%s ret:%r\n",
		      LM_FILE, ret);
		return;
	}

	uefi_delete_file(esp_fs, LM_FILE);
}

#define PSTORE_FILE	BACKUP_DIR FILE_SEP L"pstore_ram.bin"

void pstore_backup(EFI_FILE_IO_INTERFACE *esp_fs)
{
	EFI_STATUS ret;
	void *pstore_addr;
	UINTN pstore_size;

	ret = pstore_get_buffer(&pstore_addr, &pstore_size);
	if (EFI_ERROR(ret))
		return;
	debug(L"pstore addr:0x%x size:0x%x\n", pstore_addr, pstore_size);

	if (is_pstore_ram_in_ram(pstore_addr))
		data_write_to_file(esp_fs, PSTORE_FILE, pstore_addr, pstore_size);
	else
		info(L"No pstore buffer in RAM\n");
}

void pstore_restore(EFI_FILE_IO_INTERFACE *esp_fs)
{
	EFI_STATUS ret;
	void *pstore_addr;
	UINTN pstore_size;

	ret = pstore_get_buffer(&pstore_addr, &pstore_size);
	if (EFI_ERROR(ret))
		return;

	if (!uefi_exist_file_root(esp_fs, PSTORE_FILE))
		return;

	ret = inject_file_ram(esp_fs, PSTORE_FILE, pstore_addr, pstore_size);
	if (EFI_ERROR(ret)) {
		error(L"Failed to inject pstore_ram data, file:%s ret:%r\n",
		      PSTORE_FILE, ret);
		return;
	}

	uefi_delete_file(esp_fs, PSTORE_FILE);
}

static VOID log_init(VOID)
{
	EFI_STATUS err;
	err = log_set_logtag(CONFIG_LOG_TAG);
	if (EFI_ERROR(err)) {
		warning(L"Could not set log tag: %r\n", err);
	}

	log_set_loglevel(CONFIG_LOG_LEVEL);
	log_set_logtimestamp(TRUE);
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS ret = EFI_SUCCESS;
	EFI_FILE_IO_INTERFACE *esp_fs;
	EFI_LOADED_IMAGE *info;

	InitializeLib(image, systab);
	sys_table = systab;
	boot = sys_table->BootServices;
	main_image_handle = image;

	log_init();

	ret = handle_protocol(image, &LoadedImageProtocol, (void **)&info);
	if (ret != EFI_SUCCESS)
		return ret;

	efilinux_image = info->DeviceHandle;

	info(WARMDUMP_BANNER, WARMDUMP_VERSION_MAJOR, WARMDUMP_VERSION_MINOR,
	     WARMDUMP_BUILD_STRING, WARMDUMP_VERSION_STRING,
	     WARMDUMP_VERSION_DATE);

	ret = get_esp_fs(&esp_fs);
	if (EFI_ERROR(ret))
		return ret;

	if (!uefi_exist_file_root(esp_fs, BACKUP_DIR)) {
		ret = uefi_create_directory_root(esp_fs, BACKUP_DIR);
		if (EFI_ERROR(ret))
			return ret;
	}

	if (need_backup()) {
		info(L"Backup data\n");
		lm_backup(esp_fs);
		pstore_backup(esp_fs);
	} else {
		info(L"Restore data\n");
		lm_restore(esp_fs);
		pstore_restore(esp_fs);
	}

	return ret;
}
