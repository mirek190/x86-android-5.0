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
#include "acpi.h"
#include "efilinux.h"

static struct RSCI_TABLE *RSCI_table = NULL;
static struct OEM1_TABLE *OEM1_table = NULL;

#define RSDT_SIG "RSDT"
#define RSDP_SIG "RSD PTR "

/* This macro is defined to get a specified field from an acpi table
 * which will be loader if necessary.
 * <table> parameter is the name of the requested table passed as-is.
 *
 * Example: get_acpi_field(RSCI, wake_source)
 *
 * In this example, the macro requires that :
 *
 *  - RSCI_SIG is a define of the RSCI table signature,
 *  - RSCI_table is a global variable which will contains the table data,
 *  - struct RSCI_TABLE is the type of the requested table.
 */
#define get_acpi_field(table, field)				\
	(typeof(table##_table->field))				\
	_get_acpi_field((CHAR8 *)#table, (CHAR8 *)#field,	\
			(VOID **)&table##_table,		\
			offsetof(struct table##_TABLE, field), sizeof(table##_table->field))

#define set_acpi_field(table, field, value)			\
	(typeof(table##_table->field))				\
	_set_acpi_field((CHAR8 *)#table, (CHAR8 *)#field,		\
			(VOID **)&table##_table,		\
			offsetof(struct table##_TABLE, field),	\
			sizeof(table##_table->field), value)

static UINT64 _get_acpi_field(CHAR8 *name, CHAR8 *fieldname, VOID **var, UINTN offset, UINTN size)
{
	if (size > sizeof(UINT64)) {
		error(L"Can't get field %a of ACPI table %a : sizeof of field is %d > %d bytes\n", fieldname, name, size, sizeof(UINT64));
		return -1;
	}

	if (!*var) {
		EFI_STATUS ret = get_acpi_table((CHAR8 *)name, (VOID **)var);
		if (EFI_ERROR(ret)) {
			error(L"Failed to retrieve %a table: %r\n", name, ret);
			return -1;
		}
	}

	UINT64 ret = 0;
	CopyMem((CHAR8 *)&ret, (CHAR8 *)*var + offset, size);
	return ret;
}

static EFI_STATUS _set_acpi_field(CHAR8 *name, CHAR8 *fieldname, VOID **var, UINTN offset, UINTN size, UINT64 value)
{
	if (size > sizeof(value)) {
		error(L"Can't set field %a of ACPI table %a : sizeof of field is %d > %d bytes\n", fieldname, name, size, sizeof(UINT64));
		return -1;
	}

	if (!*var) {
		EFI_STATUS ret = get_acpi_table((CHAR8 *)name, (VOID **)var);
		if (EFI_ERROR(ret)) {
			error(L"Failed to retrieve %a table: %r\n", name, ret);
			return ret;
		}
	}

	CopyMem((CHAR8 *)*var + offset, (CHAR8 *)&value, size);
	return EFI_SUCCESS;
}

EFI_STATUS get_rsdt_table(struct RSDT_TABLE **rsdt)
{
	EFI_GUID acpi2_guid = ACPI_20_TABLE_GUID;
	struct RSDP_TABLE *rsdp;
	EFI_STATUS ret;

	ret = LibGetSystemConfigurationTable(&acpi2_guid, (VOID **)&rsdp);
	if (EFI_ERROR(ret)) {
		error(L"Failed to retrieve ACPI 2.0 table", ret);
		goto out;
	}

	if (strncmpa((CHAR8 *)rsdp->signature, (CHAR8 *)RSDP_SIG, sizeof(RSDP_SIG) - 1)) {
		CHAR8 *s = rsdp->signature;
		error(L"RSDP table has wrong signature (%c%c%c%c%c%c%c%c)\n",
		      s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
		ret = EFI_COMPROMISED_DATA;
		goto out;
	}

	*rsdt = (struct RSDT_TABLE *)(UINTN)rsdp->rsdt_address;
	if (strncmpa((CHAR8 *)(*rsdt)->header.signature, (CHAR8 *)RSDT_SIG, sizeof(RSDT_SIG) - 1)) {
		CHAR8 *s = (*rsdt)->header.signature;
		error(L"RSDT table has wrong signature (%c%c%c%c)\n", s[0], s[1], s[2], s[3]);
		ret = EFI_COMPROMISED_DATA;
		goto out;
	}
out:
	return ret;
}

void dump_acpi_tables(void)
{
	struct RSDT_TABLE *rsdt;
	EFI_STATUS ret;
	EFI_FILE_IO_INTERFACE *io;

	ret = get_rsdt_table(&rsdt);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get rsdt table: %r\n", ret);
		goto out;
	}

	int nb_acpi_tables = (rsdt->header.length - sizeof(rsdt->header)) / sizeof(rsdt->entry[1]);
	info(L"Listing %d tables\n", nb_acpi_tables);

	ret = uefi_call_wrapper(BS->HandleProtocol, 3, efilinux_image,
				&FileSystemProtocol, (void **)&io);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get FS_prot:%r\n", ret);
		goto out;
	}

	int i;
	for (i = 0 ; i < nb_acpi_tables; i++) {
		CHAR8 *s = ((struct ACPI_DESC_HEADER *)(UINTN)rsdt->entry[i])->signature;
		CHAR8 signature[5];
		CHAR16 *tmp;
		CHAR16 filename[11 * sizeof(CHAR16)];
		UINTN size = ((struct ACPI_DESC_HEADER *)s)->length;
		UINTN written_size = size;
		info(L"RSDT[%d] = %c%c%c%c\n", i, s[0], s[1], s[2], s[3]);

		CopyMem(signature, s, 4);
		signature[4] = 0;

		tmp = stra_to_str(signature);
		SPrint(filename, 0, L"ACPI\\%s", tmp);
		FreePool(tmp);

		ret = uefi_write_file(io, filename, s, &written_size);
		if (size != written_size)
			error(L"Written %d/%d bytes\n", written_size, size);
		if (EFI_ERROR(ret)) {
			error(L"Failed to write file %s: %r\n", filename, ret);
			goto out;
		}

		if (!strncmpa(signature, (CHAR8 *)"FACP", 4)) {
			struct FACP_TABLE *facp = (VOID *)s;
			struct ACPI_DESC_HEADER *dsdt = (VOID *)(UINTN)facp->dsdt;
			written_size = dsdt->length;;
			size = written_size;

			CHAR16 *dsdt_filename = L"ACPI\\DSDT";
			ret = uefi_write_file(io, dsdt_filename, dsdt, &written_size);
			if (size != written_size)
				error(L"Written %d/%d bytes\n", written_size, size);
			if (EFI_ERROR(ret)) {
				error(L"Failed to write file %s: %r\n", dsdt_filename, ret);
				goto out;
			}
		}
	}
out:
	return;
}

EFI_STATUS list_acpi_tables(void)
{
	struct RSDT_TABLE *rsdt;
	EFI_STATUS ret;

	ret = get_rsdt_table(&rsdt);
	if (EFI_ERROR(ret))
		return ret;

	int nb_acpi_tables = (rsdt->header.length - sizeof(rsdt->header)) / sizeof(rsdt->entry[1]);
	info(L"Listing %d tables\n", nb_acpi_tables);

	int i;
	for (i = 0 ; i < nb_acpi_tables; i++) {
		CHAR8 *s = ((struct ACPI_DESC_HEADER *)(UINTN)rsdt->entry[i])->signature;
		info(L"RSDT[%d] = %c%c%c%c\n", i, s[0], s[1], s[2], s[3]);
	}

	return EFI_SUCCESS;
}

EFI_STATUS get_acpi_table(CHAR8 *signature, VOID **table)
{
	struct RSDT_TABLE *rsdt;
	EFI_STATUS ret;
	int nb_acpi_tables;
	int i;

	ret = get_rsdt_table(&rsdt);
	if (EFI_ERROR(ret))
		goto out;

	nb_acpi_tables = (rsdt->header.length - sizeof(rsdt->header)) / sizeof(rsdt->entry[1]);
	ret = EFI_NOT_FOUND;
	for (i = 0 ; i < nb_acpi_tables; i++) {
		struct ACPI_DESC_HEADER *header = (VOID *)(UINTN)rsdt->entry[i];
		if (!strncmpa(header->signature, signature, strlena(signature))) {
			debug(L"Found %c%c%c%c table\n", signature[0], signature[1], signature[2], signature[3]);
			*table = header;
			ret = EFI_SUCCESS;
			break;
		}
	}
out:
	return ret;
}

enum flow_types acpi_read_flow_type(void)
{
        /* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return FLOW_NORMAL;
}

EFI_STATUS rsci_populate_indicators(void)
{
        /* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return EFI_SUCCESS;
}

enum wake_sources rsci_get_wake_source(void)
{
        return get_acpi_field(RSCI, wake_source);
}

EFI_STATUS rsci_set_reset_source(enum reset_sources value)
{
        return set_acpi_field(RSCI, reset_source, value);
}

enum reset_sources rsci_get_reset_source(void)
{
        return get_acpi_field(RSCI, reset_source);
}

enum reset_types rsci_get_reset_type(void)
{
        return get_acpi_field(RSCI, reset_type);
}

enum shutdown_sources rsci_get_shutdown_source(void)
{
        return get_acpi_field(RSCI, shutdown_source);
}

UINT16 oem1_get_ia_apps_run(void)
{
	return get_acpi_field(OEM1, ia_apps_run);
}

UINT8 oem1_get_ia_apps_cap(void)
{
	return get_acpi_field(OEM1, ia_apps_cap);
}

UINT8 oem1_get_capfreqidx(void)
{
	return get_acpi_field(OEM1, capfreqidx);
}

UINT16 oem1_get_ia_vbattfreqlmt(void)
{
	return get_acpi_field(OEM1, vbattfreqlmt);
}

UINT8 oem1_get_ia_apps_to_use(void)
{
	return get_acpi_field(OEM1, ia_apps_to_use);
}

void print_pidv(void)
{
	struct PIDV_TABLE *pidv;
	EFI_STATUS ret;

	ret = get_acpi_table((CHAR8 *)"PIDV", (VOID **)&pidv);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get PIDV table\n");
		return;
	}

	info(L"table revision	= %d\n", pidv->header.revision);
	info(L"part_number      	= %a\n", pidv->part_number);
	info(L"ext_id_1 x_id1:\n");
	info(L"customer		= 0x%x\n",pidv->x_id1.customer);
	info(L"vendor		= 0x%x\n",pidv->x_id1.vendor);
	info(L"device_manufacturer	= 0x%x\n",pidv->x_id1.device_manufacturer);
	info(L"platform_family	= 0x%x\n",pidv->x_id1.platform_family);
	info(L"product_line		= 0x%x\n",pidv->x_id1.product_line);
	info(L"hardware		= 0x%x\n",pidv->x_id1.hardware);

	int length = 5;
	CHAR16 buf[sizeof(pidv->x_id1.fru) * length];
	CHAR16 *ptr = buf;
	int i;
	for (i = 0; i < sizeof(pidv->x_id1.fru); i++, ptr += length)
		SPrint(ptr, 14, L"0x%02x ", pidv->x_id1.fru[i]);
	ptr[-1] = L'\0';

	info(L"fru			= %s\n", buf);
	info(L"ext_id_2 x_id2:\n");
	info(L"data1			= 0x%x\n", pidv->x_id2.data1);
	info(L"data2			= 0x%x\n", pidv->x_id2.data2);
	info(L"data3			= 0x%x\n", pidv->x_id2.data3);
	info(L"data4			= 0x%x\n", pidv->x_id2.data4);
	info(L"system_uuid		= 0x%x\n", pidv->system_uuid);
}

void print_rsci(void)
{
	struct RSCI_TABLE *rsci;
	EFI_STATUS ret = get_acpi_table((CHAR8 *)"RSCI", (VOID **)&rsci);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get RSCI table\n");
		return;
	}

	info(L"wake_source	=0x%x\n", rsci->wake_source);
	info(L"reset_source	=0x%x\n", rsci->reset_source);
	info(L"reset_type	=0x%x\n", rsci->reset_type);
	info(L"shutdown_source	=0x%x\n", rsci->shutdown_source);
	info(L"indicators	=0x%x\n", rsci->indicators);
}

void load_dsdt(void)
{
	struct RSDT_TABLE *rsdt;
	EFI_STATUS ret;
	EFI_FILE_IO_INTERFACE *io;

	ret = get_rsdt_table(&rsdt);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get rsdt table: %r\n", ret);
		goto out;
	}

	int nb_acpi_tables = (rsdt->header.length - sizeof(rsdt->header)) / sizeof(rsdt->entry[1]);
	info(L"Listing %d tables\n", nb_acpi_tables);

	ret = uefi_call_wrapper(BS->HandleProtocol, 3, efilinux_image,
				&FileSystemProtocol, (void **)&io);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get FS_prot:%r\n", ret);
		goto out;
	}

	int i;
	for (i = 0 ; i < nb_acpi_tables; i++) {
		CHAR8 *s = ((struct ACPI_DESC_HEADER *)(UINTN)rsdt->entry[i])->signature;
		CHAR8 signature[5];
		UINTN size = ((struct ACPI_DESC_HEADER *)s)->length;
		info(L"RSDT[%d] = %c%c%c%c\n", i, s[0], s[1], s[2], s[3]);
		CopyMem(signature, s, 4);
		signature[4] = 0;

		if (!strncmpa(signature, (CHAR8 *)"FACP", 4)) {
			struct FACP_TABLE *facp = (VOID *)s;
			struct ACPI_DESC_HEADER *dsdt = (VOID *)(UINTN)facp->dsdt;
			FreePool(dsdt);
			ret = uefi_read_file(io, L"DSDT", (void **)&dsdt, &size);
			if (EFI_ERROR(ret) || !dsdt) {
				error(L"Failed to read file DSDT:%r\n", ret);
				goto out;
			}
			debug(L"Read %d bytes\n", size);
			facp->dsdt = (UINT32)(UINTN)dsdt;
			info(L"DSDT = %c%c%c%c\n", dsdt->signature[0], dsdt->signature[1], dsdt->signature[2], dsdt->signature[3]);
		}
	}
out:
	return;
}
