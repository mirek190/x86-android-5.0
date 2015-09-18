/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _IA_ACC_H_
#define _IA_ACC_H_

#include <stdint.h>

/* Buffer shared between CPU and CSS.
   Depending on what the platform supports, this is implemented with
   shared physical memory (SPM) or by a shadow buffer and read/writes
   to sync. */
typedef struct {
    void    *cpu_ptr;
    uint32_t css_ptr;
    int32_t  size;
    int32_t  own_cpu_ptr; /* indicate whether cpu_ptr was allocated by us or not */
} ia_acc_buf;

/* Load specified css_fw_bin and perform CSS Init
   css_fw_bin is only used on CSim. NULL should be passed on hardware */
int
ia_acc_open(const char *css_fw_bin);

void
ia_acc_close(void);

/* Read ISP binary from file */
void*
ia_acc_open_firmware(const char *fw_path, unsigned *size);

/* Load ISP binary */
int
ia_acc_load_firmware(void *fw, unsigned size, unsigned *handle);

/* Unload ISP binary */
int
ia_acc_unload_firmware(unsigned handle);

/* Set a css_ptr as FW argument */
int
ia_acc_set_arg(unsigned handle, unsigned mem, uint32_t css_ptr, size_t size);

/* Start the ISP binary */
int
ia_acc_start_firmware(unsigned handle);

/* Wait for ISP binary to finish */
int
ia_acc_wait_for_firmware(unsigned handle);

/* Map/unmap */
int
ia_acc_map(void *usr_ptr, size_t size, uint32_t *css_ptr);

int
ia_acc_unmap(uint32_t css_ptr, size_t size);

int
ia_acc_set_mapped_arg(unsigned handle, unsigned mem, uint32_t css_ptr, size_t size);

/* Allocate aligned memory, can be mapped into ISP address space */
void*
ia_acc_alloc(size_t size);

void
ia_acc_free(void *ptr);

uint32_t
ia_acc_css_alloc(size_t size);

void
ia_acc_css_free(uint32_t css_ptr);

void
ia_acc_css_load(uint32_t css_ptr, void *dst_ptr, size_t size);

void
ia_acc_css_store(uint32_t css_ptr, void *src_ptr, size_t size);

ia_acc_buf*
ia_acc_buf_alloc(size_t size);

/* Create buf and use cpu_ptr from argument rather than allocating
   one. */
ia_acc_buf*
ia_acc_buf_create(void *cpu_ptr, size_t size);

void
ia_acc_buf_free(ia_acc_buf *buf);

/* Buffer sychronization. These functions make sure buffers are properly
   synced before being passed from CPU to CSS or vice versa.
   The implementation of these functions depends on the platform. Platforms
   that do not support PSM will copy into or from the shadow buffers,
   platforms that do support PSM AND use cached CPU memory will perform
   a cache flush. For platform with PSM support and uncached memory these
   functions may be empty. */
void
ia_acc_buf_sync_to_css(ia_acc_buf *buf);

void
ia_acc_buf_sync_to_cpu(ia_acc_buf *buf);

/* Below is a set of ia_acceleration compatible functions that just ignore the
 * context argument.
 * This is to make it a little easier to connect libraries that use the
 * ia_acceleration struct to this API.
 * Since these functions are only meant to be used as callbacks, we suffix them
 * with _cb. The mapping should be as follows:
 * ia_acceleration field -> ia_acc function
 * .open_firmware        -> ia_acc_open_firmware
 * .load_firmware        -> ia_acc_load_firmware_cb
 * .unload_firmware      -> ia_acc_unload_firmware_cb
 * .map_firmware_arg     -> ia_acc_map_cb
 * .unmap_firmware_arg   -> ia_acc_unmap_cb
 * .set_firmware_arg     -> NULL // obsolete function, no longer used
 * .set_mapped_arg       -> ia_acc_set_arg_cb
 * .start_firmware       -> ia_acc_start_firmware_cb
 * .wait_for_firmware    -> ia_acc_wait_for_firmware_cb
 * .abort_firmware       -> NULL // not implemented yet
 */
static inline int
ia_acc_load_firmware_cb(void *isp, void *fw, unsigned size, unsigned *handle)
{
	(void)isp;
	return ia_acc_load_firmware(fw, size, handle);
}

static inline int
ia_acc_unload_firmware_cb(void *isp, unsigned handle)
{
	(void)isp;
	return ia_acc_unload_firmware(handle);
}

static inline int
ia_acc_map_cb(void *isp, void *usr_ptr, size_t size, uint32_t *css_ptr)
{
	(void)isp;
	return ia_acc_map(usr_ptr, size, css_ptr);
}

static inline int
ia_acc_unmap_cb(void *isp, uint32_t css_ptr, size_t size)
{
	(void)isp;
	return ia_acc_unmap(css_ptr, size);
}

static inline int
ia_acc_set_arg_cb(void *isp, unsigned handle, unsigned mem, uint32_t css_ptr, size_t size)
{
	(void)isp;
	return ia_acc_set_arg(handle, mem, css_ptr, size);
}

static inline int
ia_acc_start_firmware_cb(void *isp, unsigned handle)
{
	(void)isp;
	return ia_acc_start_firmware(handle);
}

static inline int
ia_acc_wait_for_firmware_cb(void *isp, unsigned handle)
{
	(void)isp;
	return ia_acc_wait_for_firmware(handle);
}

#endif /* _IA_ACC_H_ */
