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
 */

#include <efi.h>
#include <efilib.h>
#include <string.h>
#include <stdlib.h>
#include "efilinux.h"
#include "fs.h"
#include "protocol.h"

struct fs_device {
	EFI_HANDLE handle;
	EFI_FILE_HANDLE fh;
	struct fs_ops *ops;
};

static struct fs_device *fs_devices;
static EFI_FILE_HANDLE *blk_devices;
static UINTN nr_fs_devices;
static UINTN nr_blk_devices;

/**
 * handle_to_dev - Return the device number for a handle
 * @handle: the device handle to search for
 */
int
handle_to_dev(EFI_HANDLE *handle)
{
	int i;

	for (i = 0; i < nr_fs_devices; i++) {
		if (fs_devices[i].handle == handle)
			break;
	}

	if (i == nr_fs_devices)
		return -1;

	return i;
}

/**
 * file_open - Open a file on a volume
 * @name: pathname of the file to open
 * @file: used to return a pointer to the allocated file on success
 */
EFI_STATUS
file_open(EFI_LOADED_IMAGE *image, CHAR16 *name, struct file **file)
{
	EFI_FILE_HANDLE fh;
	struct file *f;
	CHAR16 *filename;
	EFI_STATUS err;
	int dev_len;
	int i;

	f = malloc(sizeof(*f));
	if (!f)
		return EFI_OUT_OF_RESOURCES;

	for (dev_len = 0; name[dev_len]; ++dev_len) {
		if (name[dev_len] == ':')
			break;
	}

	if (!name[dev_len] || !dev_len) {
		dev_len = 0;
		if (!image)
			goto notfound;

		i = handle_to_dev(image->DeviceHandle);
		if (i < 0 || i >= nr_fs_devices)
			goto notfound;

		f->handle = fs_devices[i].fh;
		goto found;
	} else
		name[dev_len++] = 0;

	if (name[0] >= '0' && name[0] <= '9') {
		i = Atoi(name);
		if (i >= nr_fs_devices)
			goto notfound;

		f->handle = fs_devices[i].fh;
		goto found;
	}

	for (i = 0; i < nr_fs_devices; i++) {
		EFI_DEVICE_PATH *path;
		CHAR16 *dev;

		path = DevicePathFromHandle(fs_devices[i].handle);
		if (!path) {
			error(L"No path for device number %d\n", i);
			goto notfound;
		}

		dev = DevicePathToStr(path);

		if (!StriCmp(dev, name)) {
			f->handle = fs_devices[i].fh;
			FreePool(dev);
			break;
		}

		FreePool(dev);
	}

	if (i == nr_fs_devices)
		goto notfound;

found:
	/* Strip the device name */
	filename = name + dev_len;

	/* skip any path separators */
	while (*filename == ':' || *filename == '\\')
		filename++;

	err = uefi_call_wrapper(f->handle->Open, 5, f->handle, &fh,
				filename, EFI_FILE_MODE_READ, (UINT64)0);
	if (err != EFI_SUCCESS)
		goto fail;

	f->fh = fh;
	*file = f;

	return err;

notfound:
	err = EFI_NOT_FOUND;
fail:
	free(f);
	return err;
}

/**
 * file_close - Close a file handle
 * @f: the file to close
 */
EFI_STATUS
file_close(struct file *f)
{
	UINTN err;

	err = uefi_call_wrapper(f->handle->Close, 1, f->fh);

	if (err == EFI_SUCCESS)
		free(f);

	return err;
}

/**
 * list_boot_devices - Print a list of all disks with filesystems
 */
void list_blk_devices(void)
{
	int i;

	info(L"Devices:\n\n");

	for (i = 0; i < nr_blk_devices; i++) {
		EFI_DEVICE_PATH *path;
		EFI_HANDLE dev_handle;
		CHAR16 *dev;
		dev_handle = blk_devices[i];

		path = DevicePathFromHandle(dev_handle);
		if (!path) {
			error(L"No path for blk device %d\n", i);
			return;
		}

		dev = DevicePathToStr(path);

		info(L"\t%d. \"%s\"\n", i, dev);
		FreePool(dev);
	}
}

/*
 * Initialise filesystem protocol.
 */
EFI_STATUS
fs_init(void)
{
	EFI_HANDLE *buf;
	EFI_STATUS err;
	UINTN size = 0;
	int i, j;

	size = 0;
	err = locate_handle(ByProtocol, &FileSystemProtocol,
			    NULL, &size, NULL);

	if (err != EFI_SUCCESS && size == 0) {
		error(L"No devices support filesystems\n");
		return err;
	}

	buf = malloc(size);
	if (!buf)
		return EFI_OUT_OF_RESOURCES;

	nr_fs_devices = size / sizeof(EFI_HANDLE);
	fs_devices = malloc(sizeof(*fs_devices) * nr_fs_devices);
	if (!fs_devices) {
		err = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	err = locate_handle(ByProtocol, &FileSystemProtocol,
			    NULL, &size, (void **)buf);

	for (i = 0; i < nr_fs_devices; i++) {
		EFI_FILE_IO_INTERFACE *io;
		EFI_FILE_HANDLE fh;
		EFI_HANDLE dev_handle;
		dev_handle = buf[i];
		err = handle_protocol(dev_handle, &FileSystemProtocol,
				      (void **)&io);
		if (err != EFI_SUCCESS)
			goto close_handles;

		err = volume_open(io, &fh);
		if (err != EFI_SUCCESS)
			goto close_handles;
		fs_devices[i].handle = dev_handle;
		fs_devices[i].fh = fh;
	}

out:
	free(buf);
	return err;

close_handles:
	for (j = 0; j < i; j++) {
		EFI_FILE_HANDLE fh;

		fh = fs_devices[j].fh;
		uefi_call_wrapper(fh->Close, 1, fh);
	}

	free(fs_devices);
	goto out;
}

/*
 * Initialise blk protocol.
 */
EFI_STATUS
blk_init(void)
{
	EFI_STATUS err;
	UINTN size = 0;

	size = 0;
	err = locate_handle(ByProtocol, &DiskIoProtocol,
			    NULL, &size, NULL);

	if (err != EFI_SUCCESS && size == 0) {
		error(L"No devices support filesystems\n");
		return err;
	}

	blk_devices = malloc(size);
	if (!blk_devices)
		return EFI_OUT_OF_RESOURCES;

	nr_blk_devices = size / sizeof(*blk_devices);

	err = locate_handle(ByProtocol, &DiskIoProtocol,
			    NULL, &size, (void **)blk_devices);
	if (err != EFI_SUCCESS)
		goto free_blk;

	return EFI_SUCCESS;

free_blk:
	free(blk_devices);

	return err;
}

void fs_close(void)
{
	int i;

	for (i = 0; i < nr_fs_devices; i++) {
		EFI_FILE_HANDLE fh;

		fh = fs_devices[i].fh;
		uefi_call_wrapper(fh->Close, 1, fh);
	}
}

void fs_exit(void)
{
	fs_close();
	free(fs_devices);
}

void blk_exit(void)
{
	free(blk_devices);
}

EFI_STATUS uefi_file_get_size(EFI_HANDLE image, CHAR16 *filename, UINT64 *size)
{
	EFI_STATUS ret;
	EFI_LOADED_IMAGE *info;
	struct file *file;
	UINT64 fsize;

	if(!filename)
		return EFI_INVALID_PARAMETER;

	ret = handle_protocol(image, &LoadedImageProtocol, (void **)&info);
	if (EFI_ERROR(ret)) {
		error(L"HandleProtocol %s (%r)\n", filename, ret);
		return ret;
	}
	ret = file_open(info, filename, &file);
	if (EFI_ERROR(ret)) {
		error(L"FileOpen %s (%r)\n", filename, ret);
		return ret;
	}
	ret = file_size(file, &fsize);
	if (EFI_ERROR(ret)) {
		error(L"FileSize %s (%r)\n", filename, ret);
		return ret;
	}
	ret = file_close(file);
	if (EFI_ERROR(ret)) {
		error(L"FileClose %s (%r)\n", filename, ret);
		return ret;
	}

	*size = fsize;

	return ret;
}

EFI_STATUS uefi_call_image(
	IN EFI_HANDLE parent_image,
	IN EFI_HANDLE device,
	IN CHAR16 *filename,
	OUT UINTN *exit_data_size,
	OUT CHAR16 **exit_data)
{
	EFI_STATUS ret;
	EFI_DEVICE_PATH *path;
	UINT64 size;
	EFI_HANDLE image;

	if (!filename)
		return EFI_INVALID_PARAMETER;

	debug(L"Call image file %s\n", filename);
	path = FileDevicePath(device, filename);
	if (!path) {
		error(L"Error getting device path : %s\n", filename);
		return EFI_INVALID_PARAMETER;
	}

	ret = uefi_file_get_size(parent_image, filename, &size);
	if (EFI_ERROR(ret)) {
		error(L"GetSize %s (%r)\n", filename, ret);
		goto out;
	}

	ret = uefi_call_wrapper(BS->LoadImage, 6, FALSE, parent_image, path,
				NULL, size, &image);
	if (EFI_ERROR(ret)) {
		error(L"LoadImage %s (%r)\n", filename, ret);
		goto out;
	}

	ret = uefi_call_wrapper(BS->StartImage, 3, image,
				exit_data_size, exit_data);
	if (EFI_ERROR(ret))
		info(L"StartImage returned error %s (%r)\n", filename, ret);

out:
	if (path)
		FreePool(path);

	return ret;
}
