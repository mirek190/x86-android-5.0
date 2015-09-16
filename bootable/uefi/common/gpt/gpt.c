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
#include <uefi_utils.h>
#include <log.h>
#include "gpt.h"

#define PROTECTIVE_MBR 0xEE
#define GPT_SIGNATURE "EFI PART"

struct gpt_header {
	char signature[8];
	UINT32 revision;
	UINT32 size;
	UINT32 header_crc32;
	UINT32 reserved_zero;
	UINT64 my_lba;
	UINT64 alternate_lba;
	UINT64 first_usable_lba;
	UINT64 last_usable_lba;
	EFI_GUID disk_uuid;
	UINT64 entries_lba;
	UINT32 number_of_entries;
	UINT32 size_of_entry;
	UINT32 entries_crc32;
	/* Remainder of sector is reserved and should be 0 */
} __attribute__((packed));

struct legacy_partition {
	UINT8	status;
	UINT8	f_head;
	UINT8	f_sect;
	UINT8	f_cyl;
	UINT8	type;
	UINT8	l_head;
	UINT8	l_sect;
	UINT8	l_cyl;
	UINT32	f_lba;
	UINT32	num_sect;
} __attribute__((packed));

struct pmbr {
	UINT8			bootcode[424];
	EFI_GUID		boot_guid;
	UINT32			disk_id;
	UINT8			magic[2];	// 0x1d, 0x9a
	struct legacy_partition part[4];
	UINT8			sig[2];	// 0x55, 0xaa
} __attribute__((packed));

struct gpt_disk {
	EFI_BLOCK_IO *bio;
	EFI_DISK_IO *dio;
	struct gpt_header gpt_hd;
	struct gpt_partition *partitions;
};

static struct gpt_disk *disks;
static UINTN ndisk;
static UINTN npart;

static EFI_STATUS read_gpt_header(struct gpt_disk *disk)
{
	EFI_STATUS ret;

	ret = uefi_call_wrapper(disk->dio->ReadDisk, 5, disk->dio, disk->bio->Media->MediaId, disk->bio->Media->BlockSize, sizeof(disk->gpt_hd), (VOID *)&disk->gpt_hd);
	if (EFI_ERROR(ret))
		error(L"Failed to read disk for GPT header: %r\n", ret);

	return ret;
}

static BOOLEAN is_gpt_device(struct gpt_header *gpt)
{
	return CompareMem(gpt->signature, GPT_SIGNATURE, sizeof(gpt->signature)) == 0;
}

static EFI_STATUS read_gpt_partitions(struct gpt_disk *disk)
{
	EFI_STATUS ret;
	UINTN offset;
	UINTN size;

	offset = disk->bio->Media->BlockSize * disk->gpt_hd.entries_lba;
	size = disk->gpt_hd.number_of_entries * disk->gpt_hd.size_of_entry;

	disk->partitions = AllocatePool(size);
	if (!disk->partitions) {
		error(L"Failed to allocate %d bytes for partitions\n", size);
		return EFI_OUT_OF_RESOURCES;
	}

	ret = uefi_call_wrapper(disk->dio->ReadDisk, 5, disk->dio, disk->bio->Media->MediaId, offset, size, disk->partitions);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read GPT partitions: %r\n", ret);
		goto free_partitions;
	}
	return ret;

free_partitions:
	FreePool(disk->partitions);
	return ret;
}

static EFI_STATUS gpt_list_partition_on_disk(EFI_HANDLE handle, struct gpt_disk *disk)
{
	EFI_STATUS ret;

	ret = uefi_call_wrapper(BS->HandleProtocol, 3, handle, &BlockIoProtocol, (VOID *)&disk->bio);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get block io protocol: %r\n", ret);
		return ret;
	}

	if (disk->bio->Media->LogicalPartition != 0)
		return EFI_NOT_FOUND;

	ret = uefi_call_wrapper(BS->HandleProtocol, 3, handle, &DiskIoProtocol, (VOID *)&disk->dio);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get disk io protocol: %r\n", ret);
		return ret;
	}

	ret = read_gpt_header(disk);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read GPT header: %r\n", ret);
		return ret;
	}
	if (!is_gpt_device(&disk->gpt_hd))
		return EFI_NOT_FOUND;
	ret = read_gpt_partitions(disk);
	if (EFI_ERROR(ret)) {
		error(L"Failed to read GPT partitions: %r\n", ret);
		return ret;
	}
	return EFI_SUCCESS;
}

static EFI_STATUS gpt_cache_partition(void)
{
	EFI_STATUS ret;
	EFI_HANDLE *handles;
	UINTN buf_size = 0;
	UINTN nb_io;
	UINTN i;

	ret = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &BlockIoProtocol, NULL, &buf_size, NULL);
	if (ret != EFI_BUFFER_TOO_SMALL) {
		error(L"Failed to locate Block IO Protocol: %r\n", ret);
		return ret;
	}

	handles = AllocatePool(buf_size);
	if (!handles) {
		ret = EFI_OUT_OF_RESOURCES;
		error(L"Failed to allocate handles: %r\n", ret);
		return ret;
	}

	ret = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &BlockIoProtocol, NULL, &buf_size, (VOID *)handles);
	if (EFI_ERROR(ret)) {
		error(L"Failed to locate Block IO Protocol: %r\n", ret);
		goto free_handles;
	}

	nb_io = buf_size / sizeof(*handles);
	if (!nb_io) {
		ret = EFI_NOT_FOUND;
		error(L"No block io protocol found\n");
		goto free_handles;
	}
	debug(L"Found %d block io protocols\n", nb_io);

	disks = AllocatePool(nb_io * sizeof(*disks));
	if (!disks) {
		ret = EFI_OUT_OF_RESOURCES;
		error(L"Failed to allocate handles: %r\n", ret);
		goto free_handles;
	}

	for (i = 0; i < nb_io; i++) {
		ret = gpt_list_partition_on_disk(handles[i], &disks[ndisk]);
		if (EFI_ERROR(ret))
			continue;
		debug(L"Disk %d, adding %d gpt partitions\n", ndisk, disks[ndisk].gpt_hd.number_of_entries);
		npart += disks[ndisk].gpt_hd.number_of_entries;
		ndisk++;
	}
	ret = EFI_SUCCESS;
	debug(L"Found %d disk with %d partitions\n", ndisk, npart);

free_handles:
	FreePool(handles);
	return ret;
}

void gpt_free_cache(void)
{
	UINTN i;
	if (!disks)
		return;

	for (i = 0; i < ndisk; i++)
		FreePool(disks[i].partitions);
	FreePool(disks);
	disks = NULL;
	ndisk = 0;
	npart = 0;
}

EFI_STATUS gpt_get_partition_by_label(CHAR16 *label, struct gpt_partition_interface *gpart)
{
	EFI_STATUS ret;
	UINTN i;

	if (!disks) {
		ret = gpt_cache_partition();
		if (EFI_ERROR(ret))
			return ret;
	}

	for (i = 0; i < ndisk; i++) {
		UINTN p;
		for (p = 0; p < disks[i].gpt_hd.number_of_entries; p++) {
			struct gpt_partition *part;

			part = &disks[i].partitions[p];
			if (!CompareGuid(&part->type, &NullGuid) || StrCmp(label, part->name))
				continue;

			debug(L"Found label %s in partition %d\n", label, i + 1);
			CopyMem(&gpart->part, part, sizeof(*part));
			gpart->bio = disks[i].bio;
			gpart->dio = disks[i].dio;
			return EFI_SUCCESS;
		}
	}
	return EFI_NOT_FOUND;
}

EFI_STATUS gpt_list_partition(struct gpt_partition_interface **gpartlist, UINTN *part_count)
{
	EFI_STATUS ret;
	UINTN i;

	if (!disks) {
		ret = gpt_cache_partition();
		if (EFI_ERROR(ret))
			return ret;
	}

	*part_count = 0;
	*gpartlist = AllocatePool(npart * sizeof(struct gpt_partition_interface));
	if (!*gpartlist)
		return EFI_OUT_OF_RESOURCES;

	for (i = 0; i < ndisk; i++) {
		UINTN p;
		for (p = 0; p < disks[i].gpt_hd.number_of_entries; p++) {
			struct gpt_partition *part;
			struct gpt_partition_interface *parti;

			part = &disks[i].partitions[p];
			if (!CompareGuid(&part->type, &NullGuid) || !part->name[0])
				continue;

			parti = &(*gpartlist)[(*part_count)];
			parti->bio = disks[i].bio;
			parti->dio = disks[i].dio;
			CopyMem(&parti->part, part, sizeof(*part));
			(*part_count)++;
		}
	}
	return EFI_SUCCESS;
}
