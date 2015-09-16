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

#ifndef __UEFI_UTILS_H__
#define __UEFI_UTILS_H__

#include <efi.h>
#include <efilib.h>

#define UINTN_MAX ((UINTN)-1);
#define offsetof(TYPE, MEMBER) ((UINTN) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define max(x,y) (x < y ? y : x)

/**
 * allocate_pages - Allocate memory pages from the system
 * @atype: type of allocation to perform
 * @mtype: type of memory to allocate
 * @num_pages: number of contiguous 4KB pages to allocate
 * @memory: used to return the address of allocated pages
 *
 * Allocate @num_pages physically contiguous pages from the system
 * memory and return a pointer to the base of the allocation in
 * @memory if the allocation succeeds. On success, the firmware memory
 * map is updated accordingly.
 *
 * If @atype is AllocateAddress then, on input, @memory specifies the
 * address at which to attempt to allocate the memory pages.
 */
static inline EFI_STATUS
allocate_pages(EFI_ALLOCATE_TYPE atype, EFI_MEMORY_TYPE mtype,
	       UINTN num_pages, EFI_PHYSICAL_ADDRESS *memory)
{
	return uefi_call_wrapper(BS->AllocatePages, 4, atype,
				 mtype, num_pages, memory);
}

/**
 * free_pages - Return memory allocated by allocate_pages() to the firmware
 * @memory: physical base address of the page range to be freed
 * @num_pages: number of contiguous 4KB pages to free
 *
 * On success, the firmware memory map is updated accordingly.
 */
static inline EFI_STATUS
free_pages(EFI_PHYSICAL_ADDRESS memory, UINTN num_pages)
{
	return uefi_call_wrapper(BS->FreePages, 2, memory, num_pages);
}

/**
 * get_memory_map - Return the current memory map
 * @size: the size in bytes of @map
 * @map: buffer to hold the current memory map
 * @key: used to return the key for the current memory map
 * @descr_size: used to return the size in bytes of EFI_MEMORY_DESCRIPTOR
 * @descr_version: used to return the version of EFI_MEMORY_DESCRIPTOR
 *
 * Get a copy of the current memory map. The memory map is an array of
 * EFI_MEMORY_DESCRIPTORs. An EFI_MEMORY_DESCRIPTOR describes a
 * contiguous block of memory.
 *
 * On success, @key is updated to contain an identifer for the current
 * memory map. The firmware's key is changed every time something in
 * the memory map changes. @size is updated to indicate the size of
 * the memory map pointed to by @map.
 *
 * @descr_size and @descr_version are used to ensure backwards
 * compatibility with future changes made to the EFI_MEMORY_DESCRIPTOR
 * structure. @descr_size MUST be used when the size of an
 * EFI_MEMORY_DESCRIPTOR is used in a calculation, e.g when iterating
 * over an array of EFI_MEMORY_DESCRIPTORs.
 *
 * On failure, and if the buffer pointed to by @map is too small to
 * hold the memory map, EFI_BUFFER_TOO_SMALL is returned and @size is
 * updated to reflect the size of a buffer required to hold the memory
 * map.
 */
static inline EFI_STATUS
get_memory_map(UINTN *size, EFI_MEMORY_DESCRIPTOR *map, UINTN *key,
	       UINTN *descr_size, UINT32 *descr_version)
{
	return uefi_call_wrapper(BS->GetMemoryMap, 5, size, map,
				 key, descr_size, descr_version);
}

/**
 * exit_boot_serivces - Terminate all boot services
 * @image: firmware-allocated handle that identifies the image
 * @key: key to the latest memory map
 *
 * This function is called when efilinux wants to take complete
 * control of the system. efilinux should not make calls to boot time
 * services after this function is called.
 */
static inline EFI_STATUS
exit_boot_services(EFI_HANDLE image, UINTN key)
{
	/* do not add extra code between this function and
	 * setup_efi__memory_map call, or memory_map key might mismatch with
	 * bios and EBS call will fail
         **/

	return uefi_call_wrapper(BS->ExitBootServices, 2, image, key);
}

/**
 * exit - Terminate a loaded EFI image
 * @image: firmware-allocated handle that identifies the image
 * @status: the image's exit code
 * @size: size in bytes of @reason. Ignored if @status is EFI_SUCCESS
 * @reason: a NUL-terminated status string, optionally followed by binary data
 *
 * This function terminates @image and returns control to the boot
 * services. This function MUST NOT be called until all loaded child
 * images have exited. All memory allocated by the image must be freed
 * before calling this function, apart from the buffer @reason, which
 * will be freed by the firmware.
 */
static inline EFI_STATUS
exit(EFI_HANDLE image, EFI_STATUS status, UINTN size, CHAR16 *reason)
{
	return uefi_call_wrapper(BS->Exit, 4, image, status, size, reason);
}

struct EFI_LOAD_OPTION {
	UINT32 Attributes;
	UINT16 FilePathListLength;
} __attribute__((packed));

EFI_STATUS ConvertBmpToGopBlt (VOID *BmpImage, UINTN BmpImageSize,
			       VOID **GopBlt, UINTN *GopBltSize,
			       UINTN *PixelHeight, UINTN *PixelWidth);
EFI_STATUS gop_display_blt(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt, UINTN height, UINTN width);
EFI_STATUS get_esp_handle(EFI_HANDLE **esp);
EFI_STATUS get_esp_fs(EFI_FILE_IO_INTERFACE **esp_fs);
EFI_STATUS uefi_read_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void **data, UINTN *size);
EFI_STATUS uefi_write_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename, void *data, UINTN *size);
EFI_STATUS find_device_partition(const EFI_GUID *guid, EFI_HANDLE **handles, UINTN *no_handles);
void uefi_reset_system(EFI_RESET_TYPE reset_type);
void uefi_shutdown(void);
EFI_STATUS uefi_delete_file(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename);
BOOLEAN uefi_exist_file(EFI_FILE *parent, CHAR16 *filename);
BOOLEAN uefi_exist_file_root(EFI_FILE_IO_INTERFACE *io, CHAR16 *filename);
EFI_STATUS uefi_create_directory(EFI_FILE *parent, CHAR16 *dirname);
EFI_STATUS uefi_create_directory_root(EFI_FILE_IO_INTERFACE *io, CHAR16 *dirname);
EFI_STATUS uefi_set_simple_var(char *name, EFI_GUID *guid, int size, void *data,
			       BOOLEAN persistent);
INT8 uefi_get_simple_var(char *name, EFI_GUID *guid);
EFI_STATUS uefi_usleep(UINTN useconds);
EFI_STATUS uefi_msleep(UINTN mseconds);

EFI_STATUS str_to_stra(CHAR8 *dst, CHAR16 *src, UINTN len);
CHAR16 *stra_to_str(CHAR8 *src);
VOID StrNCpy(OUT CHAR16 *dest, IN const CHAR16 *src, UINT32 n);
UINT8 getdigit(IN CHAR16 *str);
EFI_STATUS string_to_guid(IN CHAR16 *in_guid_str, OUT EFI_GUID *guid);
UINT32 swap_bytes32(UINT32 n);
UINT16 swap_bytes16(UINT16 n);
void copy_and_swap_guid(EFI_GUID *dst, const EFI_GUID *src);
EFI_STATUS open_partition(
                IN const EFI_GUID *guid,
                OUT UINT32 *MediaIdPtr,
                OUT EFI_BLOCK_IO **BlockIoPtr,
                OUT EFI_DISK_IO **DiskIoPtr);
void path_to_dos(CHAR16 *path);
CHAR8 *append_strings(CHAR8 *s1, CHAR8 *s2);
UINTN strtoul16(const CHAR16 *nptr, CHAR16 **endptr, UINTN base);
UINTN split_cmdline(CHAR16 *cmdline, UINTN max_token, CHAR16 *args[]);

void dump_buffer(CHAR8 *b, UINTN size);


EFI_STATUS memory_map(EFI_MEMORY_DESCRIPTOR **map_buf,
			     UINTN *map_size, UINTN *map_key,
			     UINTN *desc_size, UINT32 *desc_version);
EFI_STATUS emalloc(UINTN, UINTN, EFI_PHYSICAL_ADDRESS *);
void efree(EFI_PHYSICAL_ADDRESS, UINTN);

/* Basic port I/O */
static inline void outb(UINT16 port, UINT8 value)
{
	asm volatile("outb %0,%1" : : "a" (value), "dN" (port));
}

static inline UINT8 inb(UINT16 port)
{
	UINT8 value;
	asm volatile("inb %1,%0" : "=a" (value) : "dN" (port));
	return value;
}


#endif /* __UEFI_UTILS_H__ */
