/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BOOTIMG_H__
#define __BOOTIMG_H__

#include <efi.h>
#include <efilib.h>

#define XLF_EFI_HANDOVER_32     (1<<2)
#define XLF_EFI_HANDOVER_64     (1<<3)


struct bootimg_hooks {
	void (*watchdog)(void);
	void (*before_exit)(void);
	void (*before_jump)(void);
};

/* Functions to load an Android boot image.
 * You can do this from a file, a partition GUID, or
 * from a RAM buffer */
EFI_STATUS android_image_start_buffer(
	IN VOID *bootimage,
	IN CHAR8 *cmdline,
	IN struct bootimg_hooks *hooks);

EFI_STATUS android_image_start_file(
	IN EFI_HANDLE device,
	IN CHAR16 *loader,
	IN CHAR8 *cmdline,
	IN struct bootimg_hooks *hooks);

EFI_STATUS android_image_start_partition(
	IN const EFI_GUID *guid,
	IN CHAR8 *cmdline,
	IN struct bootimg_hooks *hooks);

/* Load the next boot target if specified in the BCB partition,
 * which we specify by partition GUID. Place the value in var,
 * which must be freed. Capsule updates are also attempted if
 * specified in the BCB. The success of the capsule update part
 * is not reflected in the return value and instead is placed
 * in the BCB.
 *
 * EFI_SUCCESS - A valid value was found in the BCB and *var is set
 * EFI_NOT_FOUND - BCB didn't specify a target; boot normally
 * EFI_INVALID_PARAMETER - BCB wasn't readble, bad GUID?
 * Potentially other errors related to Disk I/O */
EFI_STATUS android_load_bcb(
	IN EFI_FILE *root_dir,
	IN const EFI_GUID *bcb_guid,
	OUT CHAR16 **var);

/* Convert a string GUID read from a config file into a real GUID
 * that we can do things with */
EFI_STATUS string_to_guid(
	IN CHAR16 *guid_str,
	OUT EFI_GUID *guid);
#endif

/* vim: softtabstop=8:shiftwidth=8:expandtab
 */
