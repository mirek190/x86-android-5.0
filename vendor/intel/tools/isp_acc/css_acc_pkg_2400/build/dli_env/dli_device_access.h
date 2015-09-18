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

#ifndef __DLI_DEVICE_ACCESS_H_INCLUDED__
#define __DLI_DEVICE_ACCESS_H_INCLUDED__

#include "device_access.h"
/*
 * User provided file that defines the system address types:
 *	- hrt_address	a type that can hold the (sub)system address range
 */

/*! Set the (sub)system base address

 \param	base_addr[in]		The offset on which the (sub)system is located
							in the global address map

 \return none,
 */
extern void dli_device_set_base_address(
	const sys_address		base_addr);


/*! Get the (sub)system base address

 \return base_address,
 */
extern sys_address dli_device_get_base_address(void);

/*! Read an 8-bit value from a device register or memory in the device

 \param	addr[in]			Local address

 \return device[addr]
 */
extern uint8_t dli_device_load_uint8(
	const hrt_address		addr);

/*! Read a 16-bit value from a device register or memory in the device

 \param	addr[in]			Local address

 \return device[addr]
 */
extern uint16_t dli_device_load_uint16(
	const hrt_address		addr);

/*! Read a 32-bit value from a device register or memory in the device

 \param	addr[in]			Local address

 \return device[addr]
 */
extern uint32_t dli_device_load_uint32(
	const hrt_address		addr);

/*! Read a 64-bit value from a device register or memory in the device

 \param	addr[in]			Local address

 \return device[addr]
 */
extern uint64_t dli_device_load_uint64(
	const hrt_address		addr);

/*! Write an 8-bit value to a device register or memory in the device

 \param	addr[in]			Local address
 \param	data[in]			value

 \return none, device[addr] = value
 */
extern void dli_device_store_uint8(
	const hrt_address		addr,
	const uint8_t			data);

/*! Write a 16-bit value to a device register or memory in the device

 \param	addr[in]			Local address
 \param	data[in]			value

 \return none, device[addr] = value
 */
extern void dli_device_store_uint16(
	const hrt_address		addr,
	const uint16_t			data);

/*! Write a 32-bit value to a device register or memory in the device

 \param	addr[in]			Local address
 \param	data[in]			value

 \return none, device[addr] = value
 */
extern void dli_device_store_uint32(
	const hrt_address		addr,
	const uint32_t			data);

/*! Write a 64-bit value to a device register or memory in the device

 \param	addr[in]			Local address
 \param	data[in]			value

 \return none, device[addr] = value
 */
extern void dli_device_store_uint64(
	const hrt_address		addr,
	const uint64_t			data);

/*! Read an array of bytes from device registers or memory in the device

 \param	addr[in]			Local address
 \param	data[out]			pointer to the destination array
 \param	size[in]			number of bytes to read

 \return none
 */
extern void dli_device_load(
	const hrt_address		addr,
	void					*data,
	const size_t			size);

/*! Write an array of bytes to device registers or memory in the device

 \param	addr[in]			Local address
 \param	data[in]			pointer to the source array
 \param	size[in]			number of bytes to write

 \return none
 */
extern void dli_device_store(
	const hrt_address		addr,
	const void				*data,
	const size_t			size);

#endif /* __DLI_DEVICE_ACCESS_H_INCLUDED__ */
