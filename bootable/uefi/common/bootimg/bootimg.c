/*
 * Copyright (c) 2013, Intel Corporation
 * All rights reserved.
 *
 * Author: Andrew Boie <andrew.p.boie@intel.com>
 * Some Linux bootstrapping code adapted from efilinux by
 * Matt Fleming <matt.fleming@intel.com>
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
 */
#include <efi.h>
#include <efilib.h>
#include <uefi_utils.h>
#include <log.h>
#include <asm/bootparam.h>
#include <string.h>

#include "bootimg.h"
#include "check_signature.h"

#ifdef CONFIG_X86_64
#include "x86_64.h"
#else
#include "i386.h"
#endif

typedef struct boot_img_hdr boot_img_hdr;

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024
#define SETUP_HDR		0x53726448	/* 0x53726448 == "HdrS" */

typedef struct {
	UINT16 limit;
	UINT64 *base;
} __attribute__((packed)) dt_addr_t;

dt_addr_t gdt = { 0x800, (UINT64 *)0 };
dt_addr_t idt = { 0, 0 };

struct boot_img_hdr {
	unsigned char magic[BOOT_MAGIC_SIZE];

	unsigned kernel_size;  /* size in bytes */
	unsigned kernel_addr;  /* physical load addr */

	unsigned ramdisk_size; /* size in bytes */
	unsigned ramdisk_addr; /* physical load addr */

	unsigned second_size;  /* size in bytes */
	unsigned second_addr;  /* physical load addr */

	unsigned tags_addr;    /* physical addr for kernel tags */
	unsigned page_size;    /* flash page size we assume */
	unsigned sig_size;     /* Size of signature block */
	unsigned unused;       /* future expansion: should be 0 */

	unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[BOOT_ARGS_SIZE];

	unsigned id[8]; /* timestamp / checksum / sha1 / etc */

	/* Supplemental command line data; kept here to maintain
	 * binary compatibility with older versions of mkbootimg */
	unsigned char extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
};

static void display_boot_img_hdr(struct boot_img_hdr *h)
{
	debug(L"magic: 0x%02X\n", h->magic);

	debug(L"kernel_size: %d\n", h->kernel_size);
	debug(L"kernel_addr: 0x%02X\n", h->kernel_addr);

	debug(L"ramdisk_size: %d\n", h->ramdisk_size);
	debug(L"ramdisk_addr: 0x%02X\n", h->ramdisk_addr);

	debug(L"second_size: %d\n", h->second_size);
	debug(L"second_addr: 0x%02X\n", h->second_addr);

	debug(L"tags_addr: 0x%02X\n", h->tags_addr);
	debug(L"page_size: %d\n", h->page_size);
	debug(L"sig_size: %d\n", h->sig_size);
	debug(L"name: %x\n", h->name);
	debug(L"cmdline: %a\n", h->cmdline);
	debug(L"id: 0x%02X\n", h->id);
	debug(L"extra_cmdline: %a\n", h->extra_cmdline);
}

/*
** +-----------------+
** | boot header     | 1 page
** +-----------------+
** | kernel          | n pages
** +-----------------+
** | ramdisk         | m pages
** +-----------------+
** | second stage    | o pages
** +-----------------+
** | signature       | p pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
** p = (sig_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
** 7. signature is optional; size should be 0 if not
**    present. signature type specified by bootloader
*/
static UINT32 pages(struct boot_img_hdr *hdr, UINT32 blob_size)
{
	return (blob_size + hdr->page_size - 1) / hdr->page_size;
}

/*
 * FIXME:
 * decide whether or not we use sig_size and patch to avoid these ugly if
 * statement
 */
static UINTN bootimage_size(struct boot_img_hdr *aosp_header,
			    BOOLEAN include_sig)
{
	UINTN size;

	size = (1 + pages(aosp_header, aosp_header->kernel_size) +
		pages(aosp_header, aosp_header->ramdisk_size) +
		pages(aosp_header, aosp_header->second_size)) *
		aosp_header->page_size;

	if (include_sig)
		size += (pages(aosp_header, aosp_header->sig_size) *
			 aosp_header->page_size);

	return size;
}

static EFI_STATUS verify_boot_image(UINT8 *bootimage)
{
	struct boot_img_hdr *aosp_header = (struct boot_img_hdr *)bootimage;
	UINTN sig_offset = bootimage_size(aosp_header, FALSE);
	UINTN sigsize = aosp_header->sig_size;

	debug(L"bootimage addr: 0x%02X\n", bootimage);
	debug(L"bootimage size: %d\n", sig_offset);

	debug(L"manifest addr: 0x%02X\n", bootimage + sig_offset);
	debug(L"manifest size: %d\n", sigsize);

	return check_signature(bootimage, sig_offset,
			       bootimage + sig_offset, sigsize);
}

static EFI_STATUS setup_ramdisk(CHAR8 *bootimage)
{
	struct boot_img_hdr *aosp_header;
	struct boot_params *buf;
	UINT32 roffset, rsize;
	EFI_PHYSICAL_ADDRESS ramdisk_addr;
	EFI_STATUS ret;

	aosp_header = (struct boot_img_hdr *)bootimage;
	buf = (struct boot_params *)(bootimage + aosp_header->page_size);

	roffset = (1 + pages(aosp_header, aosp_header->kernel_size))
		  * aosp_header->page_size;
	rsize = aosp_header->ramdisk_size;
	buf->hdr.ramdisk_size = rsize;
	ret = emalloc(rsize, 0x1000, &ramdisk_addr);
	if (EFI_ERROR(ret))
		return ret;

	if ((UINTN)ramdisk_addr > buf->hdr.initrd_addr_max) {
		error(L"Ramdisk address is too high!\n");
		ret = EFI_OUT_OF_RESOURCES;
		goto out_error;
	}
	memcpy((VOID *)(UINTN)ramdisk_addr, bootimage + roffset, rsize);
	buf->hdr.ramdisk_image = (UINT32)(UINTN)ramdisk_addr;
	debug(L"Ramdisk copied into address 0x%x\n", ramdisk_addr);
	return EFI_SUCCESS;
out_error:
	efree(ramdisk_addr, rsize);
	return ret;
}

extern EFI_GUID GraphicsOutputProtocol;
#define VIDEO_TYPE_EFI	0x70

static void setup_screen_info_from_gop(struct screen_info *pinfo)
{
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&GraphicsOutputProtocol, (void **)&gop);
	if (EFI_ERROR(ret) || gop == NULL || gop->Mode == NULL) {
		warning(L"Failed to locate GOP\n");
		return;
	}

	pinfo->orig_video_isVGA = VIDEO_TYPE_EFI;
	pinfo->lfb_base = (UINT32)gop->Mode->FrameBufferBase;
	pinfo->lfb_size = gop->Mode->FrameBufferSize;
	pinfo->lfb_width = gop->Mode->Info->HorizontalResolution;
	pinfo->lfb_height = gop->Mode->Info->VerticalResolution;
	pinfo->lfb_linelength = gop->Mode->Info->PixelsPerScanLine * 4;
}

static EFI_STATUS setup_command_line(UINT8 *bootimage,
				     CHAR8 *append)
{
	EFI_PHYSICAL_ADDRESS cmdline_addr;
	CHAR8 *full_cmdline;
	UINTN cmdlen;
	EFI_STATUS ret;
	struct boot_img_hdr *aosp_header;
	struct boot_params *buf;
	UINTN cmdline_pool_size = BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE;

	aosp_header = (struct boot_img_hdr *)bootimage;
	buf = (struct boot_params *)(bootimage + aosp_header->page_size);

	full_cmdline = AllocatePool(cmdline_pool_size);
	if (!full_cmdline) {
		ret = EFI_OUT_OF_RESOURCES;
		goto out;
	}
	memcpy(full_cmdline, aosp_header->cmdline, (BOOT_ARGS_SIZE - 1));
	if (aosp_header->cmdline[BOOT_ARGS_SIZE - 2]) {
		memcpy(full_cmdline + (BOOT_ARGS_SIZE - 1),
		       aosp_header->extra_cmdline,
		       BOOT_EXTRA_ARGS_SIZE);
	}
	cmdlen = strlena(full_cmdline);

	if (append) {
		UINTN append_len = strlena(append);
		UINTN new_size = cmdlen + append_len + 2;
		if (new_size >= cmdline_pool_size) {
			full_cmdline = ReallocatePool(full_cmdline, cmdline_pool_size, new_size);
			if (!full_cmdline) {
				error(L"Failed to reallocate cmdline\n");
				ret = EFI_OUT_OF_RESOURCES;
				goto out;
			}
			cmdline_pool_size = new_size;
		}

		full_cmdline[cmdlen] = ' ';
		memcpy(full_cmdline + cmdlen + 1, append, append_len + 1);
		cmdlen = strlena(full_cmdline);
		FreePool(append);
	}
	/* Documentation/x86/boot.txt: "The kernel command line can be located
	 * anywhere between the end of the setup heap and 0xA0000" */
	cmdline_addr = 0xA0000;
	ret = allocate_pages(AllocateMaxAddress, EfiLoaderData,
			     EFI_SIZE_TO_PAGES(cmdlen + 1),
			     &cmdline_addr);
	if (!cmdline_addr || EFI_ERROR(ret))
		goto out;

	memcpy((CHAR8 *)(UINTN)cmdline_addr, full_cmdline, cmdlen + 1);

	buf->hdr.cmd_line_ptr = (UINT32) cmdline_addr;
	buf->hdr.cmdline_size = cmdlen + 1;
	ret = EFI_SUCCESS;
out:
	FreePool(full_cmdline);
	return ret;
}

static EFI_STATUS setup_idt_gdt(void)
{
	EFI_STATUS err;

	err = emalloc(gdt.limit, 8, (EFI_PHYSICAL_ADDRESS *)&gdt.base);
	if (err != EFI_SUCCESS)
		return err;

	memset((CHAR8 *)gdt.base, 0x0, gdt.limit);
	/*
	 * 4Gb - (0x100000*0x1000 = 4Gb)
	 * base address=0
	 * code read/exec
	 * granularity=4096, 386 (+5th nibble of limit)
	 */
	gdt.base[2] = 0x00cf9a000000ffff;

	/*
	 * 4Gb - (0x100000*0x1000 = 4Gb)
	 * base address=0
	 * data read/write
	 * granularity=4096, 386 (+5th nibble of limit)
	 */
	gdt.base[3] = 0x00cf92000000ffff;

	/* Task segment value */
	gdt.base[4] = 0x0080890000000000;

	return EFI_SUCCESS;
}

static void setup_e820_map(struct boot_params *boot_params,
			   EFI_MEMORY_DESCRIPTOR *map_buf,
			   UINTN map_size,
			   UINTN desc_size)
{
	struct e820entry *e820_map = &boot_params->e820_map[0];
	/*
	 * Convert the EFI memory map to E820.
	 */
	UINTN i;
	UINTN n_page = 0;

	for (i = 0; i < map_size / desc_size; i++) {
		EFI_MEMORY_DESCRIPTOR *d;
		unsigned int e820_type = 0;

		d = (EFI_MEMORY_DESCRIPTOR *)((unsigned long)map_buf + (i * desc_size));
		switch (d->Type) {
		case EfiReservedMemoryType:
		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
		case EfiMemoryMappedIO:
		case EfiMemoryMappedIOPortSpace:
		case EfiPalCode:
			e820_type = E820_RESERVED;
			break;

		case EfiUnusableMemory:
			e820_type = E820_UNUSABLE;
			break;

		case EfiACPIReclaimMemory:
			e820_type = E820_ACPI;
			break;

		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			e820_type = E820_RAM;
			break;

		case EfiACPIMemoryNVS:
			e820_type = E820_NVS;
			break;

		default:
			continue;
		}

		if (n_page && e820_map[n_page - 1].type == e820_type &&
				(e820_map[n_page - 1].addr + e820_map[n_page - 1].size) == d->PhysicalStart) {
			e820_map[n_page - 1].size += d->NumberOfPages << EFI_PAGE_SHIFT;
		} else {
			e820_map[n_page].addr = d->PhysicalStart;
			e820_map[n_page].size = d->NumberOfPages << EFI_PAGE_SHIFT;
			e820_map[n_page].type = e820_type;
			n_page++;
		}
	}

	boot_params->e820_entries = n_page;
}

static EFI_STATUS setup_efi_memory_map(struct boot_params *boot_params, UINTN *map_key)
{
	UINTN map_size = 0;
	EFI_STATUS ret = EFI_SUCCESS;
	EFI_MEMORY_DESCRIPTOR *map_buf;
	UINTN descr_size;
	UINT32 descr_version;
	struct efi_info *efi = &boot_params->efi_info;

	ret = memory_map(&map_buf, &map_size, map_key, &descr_size, &descr_version);
	if (ret != EFI_SUCCESS)
		goto out;

	efi->efi_systab = (UINT32)(UINTN)ST;
	efi->efi_memdesc_size = descr_size;
	efi->efi_memdesc_version = descr_version;
	efi->efi_memmap = (UINT32)(UINTN)map_buf;
	efi->efi_memmap_size = map_size;
#ifdef CONFIG_X86_64
	efi->efi_systab_hi = (unsigned long)ST >> 32;
	efi->efi_memmap_hi = (unsigned long)map_buf >> 32;
#endif

	memcpy((CHAR8 *)&efi->efi_loader_signature,
	       (CHAR8 *)EFI_LOADER_SIGNATURE, sizeof(UINT32));

	setup_e820_map(boot_params, map_buf, map_size, descr_size);

out:
	return ret;
}

static EFI_STATUS handover_kernel(CHAR8 *bootimage, BOOLEAN watchdog_en, struct bootimg_hooks *hooks)
{
	EFI_PHYSICAL_ADDRESS kernel_start;
	EFI_PHYSICAL_ADDRESS boot_addr;
	struct boot_params *boot_params;
	UINT64 init_size;
	EFI_STATUS ret;
	struct boot_img_hdr *aosp_header;
	struct boot_params *buf;
	UINT8 setup_sectors;
	UINT32 setup_size;
	UINT32 ksize;
	UINT32 koffset;

	aosp_header = (struct boot_img_hdr *)bootimage;
	buf = (struct boot_params *)(bootimage + aosp_header->page_size);

	koffset = aosp_header->page_size;
	setup_sectors = buf->hdr.setup_sects;
	setup_sectors++; /* Add boot sector */
	setup_size = (UINT32)setup_sectors * 512;
	ksize = aosp_header->kernel_size - setup_size;
	kernel_start = buf->hdr.pref_address;
	init_size = buf->hdr.init_size;
	buf->hdr.type_of_loader = 0xff;

	memset((CHAR8 *)&buf->screen_info, 0x0, sizeof(buf->screen_info));

	setup_screen_info_from_gop(&buf->screen_info);

	ret = allocate_pages(AllocateAddress, EfiLoaderData,
			     EFI_SIZE_TO_PAGES(init_size), &kernel_start);
	if (EFI_ERROR(ret)) {
		/*
		 * We failed to allocate the preferred address, so
		 * just allocate some memory and hope for the best.
		 */
		ret = emalloc(init_size, buf->hdr.kernel_alignment, &kernel_start);
		if (EFI_ERROR(ret))
			return ret;
	}
	debug(L"kernel_start = 0x%x\n", kernel_start);

	if (!buf->hdr.relocatable_kernel && kernel_start != buf->hdr.pref_address) {
		error(L"Failed to store non relocatable kernel to it's preferred address\n");
		ret = EFI_LOAD_ERROR;
		goto out;
	}

	memcpy((CHAR8 *)(UINTN)kernel_start, bootimage + koffset + setup_size, ksize);

	boot_addr = 0x3fffffff;
	ret = allocate_pages(AllocateMaxAddress, EfiLoaderData,
			     EFI_SIZE_TO_PAGES(16384), &boot_addr);
	if (EFI_ERROR(ret))
		goto out;

	boot_params = (struct boot_params *)(UINTN)boot_addr;
	memset((void *)boot_params, 0x0, 16384);

	/* Copy first two sectors to boot_params */
	memcpy((CHAR8 *)boot_params, (CHAR8 *)buf, 2 * 512);
	boot_params->hdr.code32_start = (UINT32)((UINT64)kernel_start);

	ret = EFI_LOAD_ERROR;

	ret = setup_idt_gdt();
	if (EFI_ERROR(ret))
		goto out;

	if (watchdog_en && hooks && hooks->watchdog)
		hooks->watchdog();

	if (hooks && hooks->before_exit)
		hooks->before_exit();

	UINTN map_key;
	ret = setup_efi_memory_map(boot_params, &map_key);
	if (EFI_ERROR(ret))
		goto out;

	/* do not add extra code between this function and
	 * setup_efi__memory_map call, or memory_map key might mismatch with
	 * bios and EBS call will fail
	 **/

	ret = exit_boot_services(bootimage, map_key);
	if (EFI_ERROR(ret))
		goto out;

	if (hooks && hooks->before_jump)
		hooks->before_jump();

	asm volatile ("lidt %0" :: "m" (idt));
	asm volatile ("lgdt %0" :: "m" (gdt));

	kernel_jump(kernel_start, boot_params);
	/* Shouldn't get here */

	free_pages(boot_addr, EFI_SIZE_TO_PAGES(16384));
out:
	debug(L"Can't boot kernel\n");
	efree(kernel_start, ksize);
	return ret;
}

EFI_STATUS android_image_start_partition(
	IN const EFI_GUID *guid,
	IN CHAR8 *cmdline,
	IN struct bootimg_hooks *hooks)
{
	EFI_BLOCK_IO *BlockIo;
	EFI_DISK_IO *DiskIo;
	UINT32 MediaId;
	UINT32 img_size;
	UINT8 *bootimage;
	EFI_STATUS ret;
	struct boot_img_hdr aosp_header;

	debug(L"Locating boot image\n");
	ret = open_partition(guid, &MediaId, &BlockIo, &DiskIo);
	if (EFI_ERROR(ret))
		return ret;

	debug(L"Reading boot image header\n");
	ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
				sizeof(aosp_header), &aosp_header);
	if (EFI_ERROR(ret)) {
		error(L"ReadDisk (header) : %r\n", ret);
		return ret;
	}
	if (strncmpa((CHAR8 *)BOOT_MAGIC, aosp_header.magic, BOOT_MAGIC_SIZE)) {
		error(L"This partition does not appear to contain an Android boot image\n");
		return EFI_INVALID_PARAMETER;
	}

	img_size = bootimage_size(&aosp_header, TRUE);
	bootimage = AllocatePool(img_size);
	if (!bootimage)
		return EFI_OUT_OF_RESOURCES;

	debug(L"Reading full boot image\n");
	ret = uefi_call_wrapper(DiskIo->ReadDisk, 5, DiskIo, MediaId, 0,
				img_size, bootimage);
	if (EFI_ERROR(ret)) {
		error(L"ReadDisk : %r\n", ret);
		goto out;
	}

	ret = android_image_start_buffer(bootimage, cmdline, hooks);
out:
	FreePool(bootimage);
	return ret;
}

EFI_STATUS android_image_start_file(
	IN EFI_HANDLE device,
	IN CHAR16 *loader,
	IN CHAR8 *cmdline,
	IN struct bootimg_hooks *hooks)
{
	EFI_STATUS ret;
	VOID *bootimage;
	EFI_DEVICE_PATH *path;
	EFI_GUID SimpleFileSystemProtocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_GUID EfiFileInfoId = EFI_FILE_INFO_ID;
	EFI_FILE_IO_INTERFACE *drive;
	EFI_FILE_INFO *fileinfo = NULL;
	EFI_FILE *imagefile, *root;
	UINTN buffersize = sizeof(EFI_FILE_INFO);
	struct boot_img_hdr *aosp_header;
	UINTN bsize;

	debug(L"Locating boot image from file %s\n", loader);
	path = FileDevicePath(device, loader);
	if (!path) {
		error(L"Error getting device path\n");
		uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);
		return EFI_INVALID_PARAMETER;
	}

	/* Open the device */
	ret = uefi_call_wrapper(BS->HandleProtocol, 3, device,
				&SimpleFileSystemProtocol, (void **)&drive);
	if (EFI_ERROR(ret)) {
		error(L"HandleProtocol : %r\n", ret);
		return ret;
	}
	ret = uefi_call_wrapper(drive->OpenVolume, 2, drive, &root);
	if (EFI_ERROR(ret)) {
		error(L"OpenVolume : %r\n", ret);
		return ret;
	}

	/* Get information about the boot image file, we need to know
	 * how big it is, and allocate a suitable buffer */
	ret = uefi_call_wrapper(root->Open, 5, root, &imagefile, loader,
				EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(ret)) {
		error(L"Open : %r\n", ret);
		return ret;
	}
	fileinfo = AllocatePool(buffersize);
	if (!fileinfo)
		return EFI_OUT_OF_RESOURCES;

	ret = uefi_call_wrapper(imagefile->GetInfo, 4, imagefile,
				&EfiFileInfoId, &buffersize, fileinfo);
	if (ret == EFI_BUFFER_TOO_SMALL) {
		/* buffersize updated with the required space for
		 * the request */
		FreePool(fileinfo);
		fileinfo = AllocatePool(buffersize);
		if (!fileinfo)
			return EFI_OUT_OF_RESOURCES;
		ret = uefi_call_wrapper(imagefile->GetInfo, 4, imagefile,
					&EfiFileInfoId, &buffersize, fileinfo);
	}
	if (EFI_ERROR(ret)) {
		error(L"GetInfo : %r\n", ret);
		goto free_info;
	}
	buffersize = fileinfo->FileSize;
	bootimage = AllocatePool(buffersize);
	if (!bootimage) {
		ret = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	/* Read the file into the buffer */
	ret = uefi_call_wrapper(imagefile->Read, 3, imagefile,
				&buffersize, bootimage);
	if (ret == EFI_BUFFER_TOO_SMALL) {
		/* buffersize updated with the required space for
		 * the request. By the way it doesn't make any
		 * sense to me why this is needed since we supposedly
		 * got the file size from the GetInfo call but
		 * whatever... */
		FreePool(bootimage);
		bootimage = AllocatePool(buffersize);
		if (!bootimage) {
			ret = EFI_OUT_OF_RESOURCES;
			goto out;
		}
		ret = uefi_call_wrapper(imagefile->Read, 3, imagefile,
					&buffersize, bootimage);
	}
	if (EFI_ERROR(ret)) {
		error(L"Read : %r\n", ret);
		goto out;
	}

	aosp_header = (struct boot_img_hdr *)bootimage;
	bsize = bootimage_size(aosp_header, TRUE);
	display_boot_img_hdr(aosp_header);
	if ((bsize - buffersize) > aosp_header->sig_size) {
		error(L"Boot image size mismatch; got %d expected %d\n",
		      buffersize, bsize);
		ret = EFI_INVALID_PARAMETER;
		goto out;
	}

	ret = android_image_start_buffer(bootimage, cmdline, hooks);

out:
	FreePool(bootimage);
free_info:
	FreePool(fileinfo);

	return ret;
}

EFI_STATUS android_image_start_buffer(
	IN VOID *bootimage,
	IN CHAR8 *cmdline,
	IN struct bootimg_hooks *hooks)
{
	struct boot_img_hdr *aosp_header;
	struct boot_params *buf;
	EFI_STATUS ret;
	aosp_header = (struct boot_img_hdr *)bootimage;
	buf = (struct boot_params *)((CHAR8 *)bootimage + aosp_header->page_size);
	BOOLEAN watchdog_en = TRUE;

	/* Check boot sector signature */
	if (buf->hdr.boot_flag != 0xAA55) {
		error(L"bzImage kernel corrupt\n");
		ret = EFI_INVALID_PARAMETER;
		goto out_bootimage;
	}

	if (buf->hdr.header != SETUP_HDR) {
		error(L"Setup code version is invalid\n");
		ret = EFI_INVALID_PARAMETER;
		goto out_bootimage;
	}

	ret = verify_boot_image(bootimage);
	if (EFI_ERROR(ret)) {
		error(L"boot image digital signature verification failed : %r\n", ret);
		goto out_bootimage;
	}

	debug(L"Creating command line\n");
	ret = setup_command_line(bootimage, cmdline);
	if (EFI_ERROR(ret)) {
		error(L"setup_command_line : %r\n", ret);
		goto out_bootimage;
	}

	debug(L"Loading the ramdisk\n");
	ret = setup_ramdisk(bootimage);
	if (EFI_ERROR(ret)) {
		error(L"setup_ramdisk : %r\n", ret);
		goto out_cmdline;
	}

	if ((buf->hdr.cmd_line_ptr) &&
			strstr((char *)(UINTN)buf->hdr.cmd_line_ptr, "disable_kernel_watchdog=1"))
		watchdog_en = FALSE;

	debug(L"Loading the kernel\n");
	ret = handover_kernel(bootimage, watchdog_en, hooks);
	error(L"handover_kernel : %r\n", ret);

	efree(buf->hdr.ramdisk_image, buf->hdr.ramdisk_size);
out_cmdline:
	if (buf->hdr.cmd_line_ptr)
		free_pages(buf->hdr.cmd_line_ptr,
			   strlena((CHAR8 *)(UINTN)buf->hdr.cmd_line_ptr) + 1);
out_bootimage:
	FreePool(bootimage);
	return ret;
}

/* vim: softtabstop=8:shiftwidth=8:expandtab
 */
