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

#include <assert.h>
#include <string.h>
#include "dli_device_access.h"
#include <hrt/master_port.h>	/* hrt_master_port_load() */

/*
 * This is an HRT backend implementation for CSIM
 */

static sys_address		base_address = (sys_address)-1;

void dli_device_set_base_address(
	const sys_address		base_addr)
{
	base_address = base_addr;
return;
}


sys_address dli_device_get_base_address(void)
{
return base_address;
}

uint8_t dli_device_load_uint8(
	const hrt_address		addr)
{
assert(base_address != (sys_address)-1);
return hrt_master_port_uload_8(base_address + addr);
}

uint16_t dli_device_load_uint16(
	const hrt_address		addr)
{
assert(base_address != (sys_address)-1);
assert((addr & 0x01) == 0);
return hrt_master_port_uload_16(base_address + addr);
}

uint32_t dli_device_load_uint32(
	const hrt_address		addr)
{
assert(base_address != (sys_address)-1);
assert((addr & 0x03) == 0);
return hrt_master_port_uload_32(base_address + addr);
}

uint64_t dli_device_load_uint64(
	const hrt_address		addr)
{
assert(base_address != (sys_address)-1);
assert((addr & 0x07) == 0);
assert(0);
return 0;
}

void dli_device_store_uint8(
	const hrt_address		addr,
	const uint8_t			data)
{
assert(base_address != (sys_address)-1);
	hrt_master_port_store_8(base_address + addr, data);
return;
}

void dli_device_store_uint16(
	const hrt_address		addr,
	const uint16_t			data)
{
assert(base_address != (sys_address)-1);
assert((addr & 0x01) == 0);
	hrt_master_port_store_16(base_address + addr, data);
return;
}

void dli_device_store_uint32(
	const hrt_address		addr,
	const uint32_t			data)
{
assert(base_address != (sys_address)-1);
assert((addr & 0x03) == 0);
	hrt_master_port_store_32(base_address + addr, data);
return;
}

void dli_device_store_uint64(
	const hrt_address		addr,
	const uint64_t			data)
{
assert(base_address != (sys_address)-1);
assert((addr & 0x07) == 0);
assert(0);
(void)data;
return;
}

void dli_device_load(
	const hrt_address		addr,
	void					*data,
	const size_t			size)
{
assert(base_address != (sys_address)-1);
#ifndef C_RUN
	hrt_master_port_load(base_address + addr, data, size);
#else
	memcpy(data, (void *)(int)addr, size);
#endif
}

void dli_device_store(
	const hrt_address		addr,
	const void				*data,
	const size_t			size)
{
assert(base_address != (sys_address)-1);
#ifndef C_RUN
	hrt_master_port_store(base_address + addr, data, size);
#else
	memcpy((void *)(int)addr, data, size);
#endif
return;
}
