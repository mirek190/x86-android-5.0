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

#ifndef __DLI_MEMORY_ACCESS_H_INCLUDED__
#define __DLI_MEMORY_ACCESS_H_INCLUDED__

/*!
 * \brief
 * Define the public interface for virtual memory
 * access functions. Access types are limited to
 * those defined in <stdint.h>
 *
 * The address representation is private to the system
 * and represented as "ia_css_ptr" rather than a
 * pointer, as the memory allocation cannot be accessed
 * by dereferencing but reaquires load and store access
 * functions
 *
 * "store" is a transfer to the system
 * "load" is a transfer from the system
 *
 * Allocation properties can be specified by setting
 * attributes (see below) in case of multiple physical
 * memories the memory ID is encoded on the attribute
 *
 * Allocations in the same physical memory, but in a
 * different (set of) page tables can be shared through
 * a page table information mapping function
 */

#include <stdint.h>
#include <stddef.h>

#include "memory_access.h"
#include <ia_css_types.h>


/*! Initialize the memory manager. Some backends use this to initialize
 *  memory guarding.
 \return none,
 */
extern void dli_mmgr_init(void);

extern uint32_t dli_mmgr_get_base_index(void);

/*! Return the address of an allocation in memory

 \param	size[in]			Size in bytes of the allocation

 \return vaddress
 */
extern ia_css_ptr dli_mmgr_malloc(
	const size_t			size);

/*! Return the address of a zero initialised allocation in memory

 \param	size[in]			Size in bytes of the allocation

 \return vaddress
 */
extern ia_css_ptr dli_mmgr_calloc(
	const size_t			N,
	const size_t			size);

/*! Return the address of a reallocated allocation in memory

 \param	vaddr[in]			Address of an allocation
 \param	size[in]			Size in bytes of the allocation

 \Note
	All limitations and particularities of the C stdlib
	realloc function apply

 \return vaddress
 */
/* unused */
extern ia_css_ptr dli_mmgr_realloc(
	ia_css_ptr			vaddr,
	const size_t			size);

/*! Free the memory allocation identified by the address

 \param	vaddr[in]			Address of the allocation

 \return vaddress
 */
extern void dli_mmgr_free(
	ia_css_ptr			vaddr);

/*! Return the address of an allocation in memory

 \param	size[in]			Size in bytes of the allocation
 \param	attribute[in]		Bit vector specifying the properties
							of the allocation including zero
							initialisation

 \return vaddress
 */
extern ia_css_ptr dli_mmgr_alloc_attr(
	const size_t			size,
	const uint16_t			attribute);

/*! Return the address of a reallocated allocation in memory

 \param	vaddr[in]			Address of an allocation
 \param	size[in]			Size in bytes of the allocation
 \param	attribute[in]		Bit vector specifying the properties
							of the allocation

 \Note
	All limitations and particularities of the C stdlib
	realloc function apply

 \return vaddress
 */
/* unused */
extern ia_css_ptr dli_mmgr_realloc_attr(
	ia_css_ptr			vaddr,
	const size_t			size,
	const uint16_t			attribute);

/*! Return the address of a mapped existing allocation in memory

 \param	ptr[in]				Pointer to an allocation in a different
							virtual memory page table, but the same
							physical memory
 \param size[in]
 \param attributes[in]
 \param context

 \Note
	This interface is tentative, limited to the desired function
	the actual interface may require furhter parameters

 \return vaddress
 */
extern ia_css_ptr dli_mmgr_mmap(const void *ptr, const size_t size,
	  uint16_t attributes, void *context);

/*! Zero initialise an allocation in memory

 \param	vaddr[in]			Address of an allocation
 \param	size[in]			Size in bytes of the area to be cleared

 \return none
 */
extern void dli_mmgr_clear(
	ia_css_ptr			vaddr,
	const size_t			size);

/*! Set an allocation in memory to a value

 \param	vaddr[in]			Address of an allocation
 \param	data[in]			Value to set
 \param	size[in]			Size in bytes of the area to be set

 \return none
 */
/* unused */
extern void dli_mmgr_set(
	ia_css_ptr			vaddr,
	const uint8_t			data,
	const size_t			size);

/*! Read an array of bytes from a virtual memory address

 \param	vaddr[in]			Address of an allocation
 \param	data[out]			pointer to the destination array
 \param	size[in]			number of bytes to read

 \return none
 */
extern void dli_mmgr_load(
	const ia_css_ptr		vaddr,
	void					*data,
	const size_t			size);

/*! Write an array of bytes to device registers or memory in the device

 \param	vaddr[in]			Address of an allocation
 \param	data[in]			pointer to the source array
 \param	size[in]			number of bytes to write

 \return none
 */
extern void dli_mmgr_store(
	const ia_css_ptr		vaddr,
	const void				*data,
	const size_t			size);

#endif /* __DLI_MEMORY_ACCESS_H_INCLUDED__ */
