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

#include "type_support.h"
#include "math_support.h"

/* Presently system specific */
#include <hive_isp_css_hrt.h>
#include "dli_memory_access.h"

#include "assert_support.h"

/* Presently system specific */
#include <hmm_64/hmm.h>

/*
 * This is an HRT backend implementation for CSIM
 */

#ifndef SH_CSS_MEMORY_GUARDING
/* Choose default in case not defined */
#ifdef HRT_CSIM
#define SH_CSS_MEMORY_GUARDING (1)
#else
#define SH_CSS_MEMORY_GUARDING (0)
#endif
#endif

#ifdef C_RUN
#define DMA_CHECK_MEMORY_ACCESS 1
#endif

#if SH_CSS_MEMORY_GUARDING
#define DDR_ALIGN(a)	(CEIL_MUL((a), (HIVE_ISP_DDR_WORD_BYTES)))

#define MEM_GUARD_START		0xABBAABBA
#define MEM_GUARD_END		0xBEEFBEEF
#define GUARD_SIZE		sizeof(uint32_t)
#define GUARD_SIZE_ALIGNED	DDR_ALIGN(GUARD_SIZE)

#define MAX_ALLOC_ENTRIES (1024)
#define INVALID_VBASE ((ia_css_ptr)-1)
#define INVALID_SIZE ((uint32_t)-1)

struct alloc_info{
	ia_css_ptr  vbase;
	uint32_t size;
};

static struct alloc_info alloc_admin[MAX_ALLOC_ENTRIES];

static struct alloc_info const alloc_info_invalid
					= { INVALID_VBASE, INVALID_SIZE };

static void alloc_admin_init(void)
{
	int i;

	for (i = 0; i < MAX_ALLOC_ENTRIES; i++)
		alloc_admin[i] = alloc_info_invalid;
}

static struct alloc_info const *alloc_admin_find(ia_css_ptr vaddr)
{
	int i;
	/**
	 * Note that we use <= instead of < because we like to accept
	 * zero-sized operations at the last allocated address
	 * e.g. dli_mmgr_set(vbase+alloc_size, data, 0)
	 */
	for (i = 0; i < MAX_ALLOC_ENTRIES; i++) {
		if (alloc_admin[i].vbase != INVALID_VBASE &&
					vaddr >= alloc_admin[i].vbase &&
					vaddr <= alloc_admin[i].vbase +
							alloc_admin[i].size) {
			return &alloc_admin[i];
		}
	}
	return &alloc_info_invalid;
}

#ifdef  DMA_CHECK_MEMORY_ACCESS
/* HACK: this function is called from within the SP code (dmaproxy.sp.c) in
 * crun only. The prototype below is to avoid compiler warnings about
 * a missing prototype for a non-static function. */
bool check_mem_guard(ia_css_ptr vaddr, ia_css_ptr vaddr_t, uint32_t size_t);

bool check_mem_guard(ia_css_ptr vaddr, ia_css_ptr vaddr_t, uint32_t size_t)
{
	struct alloc_info const *info;

	info = alloc_admin_find(vaddr);
	if (info->vbase != INVALID_VBASE)
	{
		if ((vaddr_t + size_t) > (info->vbase + info->size)) {
			printf("overflow base 0x%X..0x%X size 0x%X address 0x%X size 0x%X overflow size 0x%X\n",
				info->vbase, info->vbase + info->size, info->size, vaddr_t, size_t,
				(vaddr_t + size_t) - (info->vbase + info->size));
			assert(false);
			return false;
		}
		if ((vaddr_t)< (info->vbase)) {
			printf("underflow base 0x%X..0x%X size 0x%X address 0x%X size 0x%X\n",
				info->vbase, info->vbase + info->size, info->size, vaddr_t, size_t);
			assert(false);
			return false;
		}
	}
        return true;
}
#endif

static bool mem_guard_valid(ia_css_ptr vaddr, uint32_t size)
{
	uint32_t mem_guard;
	struct alloc_info const *info;

	info = alloc_admin_find(vaddr);
	if (info->vbase == INVALID_VBASE) {
		assert(false);
		return false;
	}

	/* Check if end is in alloc range*/
	if ((vaddr + size) > (info->vbase + info->size)) {
		assert(false);
		return false;
	}

	hrt_isp_css_mm_load(
			(hmm_ptr)HOST_ADDRESS(info->vbase - sizeof(mem_guard)),
			&mem_guard, sizeof(mem_guard));
	if (mem_guard != MEM_GUARD_START) {
		assert(false);
		return false;
	}

	hrt_isp_css_mm_load((hmm_ptr)HOST_ADDRESS(info->vbase + info->size),
						&mem_guard, sizeof(mem_guard));
	if (mem_guard != MEM_GUARD_END) {
		assert(false);
		return false;
	}

	return true;

}

static void alloc_admin_add(ia_css_ptr vbase, uint32_t size)
{
	int i;
	uint32_t mem_guard;

	assert(alloc_admin_find(vbase)->vbase == INVALID_VBASE);

	mem_guard = MEM_GUARD_START;
	hrt_isp_css_mm_store((hmm_ptr)HOST_ADDRESS(vbase - sizeof(mem_guard)),
						&mem_guard, sizeof(mem_guard));
#ifdef DMA_CHECK_MEMORY_ACCESS
	//printf("mem_guard_create: 0x%X 0x%X\n", HOST_ADDRESS(vbase - sizeof(mem_guard)), size);
#endif

	mem_guard = MEM_GUARD_END;
	hrt_isp_css_mm_store((hmm_ptr)HOST_ADDRESS(vbase + size),
						&mem_guard, sizeof(mem_guard));
#ifdef DMA_CHECK_MEMORY_ACCESS
	//printf("alloc base 0x%X size 0x%X\n",vbase,size);
#endif

	for (i = 0; i < MAX_ALLOC_ENTRIES; i++) {
		if (alloc_admin[i].vbase == INVALID_VBASE) {
			alloc_admin[i].vbase = vbase;
			alloc_admin[i].size = size;
			return;
		}
	}
	assert(false);
}

static void alloc_admin_remove(ia_css_ptr vbase)
{
	int i;
	assert(mem_guard_valid(vbase, 0));
	for (i = 0; i < MAX_ALLOC_ENTRIES; i++) {
		if (alloc_admin[i].vbase == vbase) {
			alloc_admin[i] = alloc_info_invalid;
			return;
		}
	}
	assert(false);
}

#endif

void dli_mmgr_init(void)
{
#if SH_CSS_MEMORY_GUARDING
	alloc_admin_init();
#endif
}

ia_css_ptr dli_mmgr_malloc(
	const size_t			size)
{
return dli_mmgr_alloc_attr(size, MMGR_ATTRIBUTE_DEFAULT);
}

ia_css_ptr dli_mmgr_calloc(
	const size_t			N,
	const size_t			size)
{
return dli_mmgr_alloc_attr(N * size, MMGR_ATTRIBUTE_DEFAULT | MMGR_ATTRIBUTE_CLEARED);
}

ia_css_ptr dli_mmgr_realloc(
	ia_css_ptr			vaddr,
	const size_t			size)
{
return dli_mmgr_realloc_attr(vaddr, size, MMGR_ATTRIBUTE_DEFAULT);
}

void dli_mmgr_free(
	ia_css_ptr			vaddr)
{
/* "free()" should accept NULL, "hrt_isp_css_mm_free()" may not */
	if (vaddr != mmgr_NULL) {
#if SH_CSS_MEMORY_GUARDING
		alloc_admin_remove(vaddr);
		/* Reconstruct the "original" address used with the alloc */
		vaddr -= GUARD_SIZE_ALIGNED;
#endif
		hrt_isp_css_mm_free((hmm_ptr)HOST_ADDRESS(vaddr));
	}
return;
}

ia_css_ptr dli_mmgr_alloc_attr(
	const size_t			size,
	const uint16_t			attribute)
{
	hmm_ptr	ptr;
	size_t	extra_space = 0;
	size_t	aligned_size;

        /*The size of Memory allocated is always made multiple of DDR words.
        The reason for allocating more is, dma_v2 does only full word transfers.
        By allocating a little more we avoid overwrite.*/
        aligned_size = CEIL_MUL(size,HIVE_ISP_DDR_WORD_BYTES);


assert((attribute & MMGR_ATTRIBUTE_UNUSED) == 0);

#if SH_CSS_MEMORY_GUARDING
	/* Add DDR aligned space for a guard at begin and end */
	/* Begin guard must be DDR aligned, "end" guard gets aligned
           because size of allocation is always multiple of DDR words */
        extra_space = GUARD_SIZE_ALIGNED + GUARD_SIZE;
#endif

	if (attribute & MMGR_ATTRIBUTE_CLEARED) {
		if (attribute & MMGR_ATTRIBUTE_CACHED) {
			if (attribute & MMGR_ATTRIBUTE_CONTIGUOUS) /* { */
				ptr = hrt_isp_css_mm_calloc_contiguous(
						aligned_size + extra_space);
			/* } */ else /* { */
				ptr = hrt_isp_css_mm_calloc(
						aligned_size + extra_space);
			/* } */
		} else { /* !MMGR_ATTRIBUTE_CACHED */
			if (attribute & MMGR_ATTRIBUTE_CONTIGUOUS) /* { */
				ptr = hrt_isp_css_mm_calloc_contiguous(
						aligned_size + extra_space);
			/* } */ else /* { */
				ptr = hrt_isp_css_mm_calloc(
						aligned_size + extra_space);
			/* } */
		}
	} else { /* MMGR_ATTRIBUTE_CLEARED */
		if (attribute & MMGR_ATTRIBUTE_CACHED) {
			if (attribute & MMGR_ATTRIBUTE_CONTIGUOUS) /* { */
				ptr = hrt_isp_css_mm_alloc_contiguous(
						aligned_size + extra_space);
			/* } */ else /* { */
				ptr = hrt_isp_css_mm_alloc(
						aligned_size + extra_space);
			/* } */
		} else { /* !MMGR_ATTRIBUTE_CACHED */
			if (attribute & MMGR_ATTRIBUTE_CONTIGUOUS) /* { */
				ptr = hrt_isp_css_mm_alloc_contiguous(
						aligned_size + extra_space);
			/* } */ else /* { */
				ptr = hrt_isp_css_mm_alloc(
						aligned_size + extra_space);
			/* } */
		}
	}

#if SH_CSS_MEMORY_GUARDING
	/* ptr is the user pointer, so we need to skip the "begin" guard */
	ptr += GUARD_SIZE_ALIGNED;
	alloc_admin_add(HOST_ADDRESS(ptr), aligned_size);
#endif

	return HOST_ADDRESS(ptr);
}

ia_css_ptr dli_mmgr_realloc_attr(
	ia_css_ptr			vaddr,
	const size_t			size,
	const uint16_t			attribute)
{
assert((attribute & MMGR_ATTRIBUTE_UNUSED) == 0);
/* assert(attribute == MMGR_ATTRIBUTE_DEFAULT); */
/* Apparently we don't have this one */
assert(0);
(void)vaddr;
(void)size;
(void)attribute;
return mmgr_NULL;
}

ia_css_ptr dli_mmgr_mmap(
	const void *ptr,
	const size_t size,
	uint16_t attribute,
	void *context)
{
assert(ptr != NULL);
/*assert(isPageAligned(ptr)); */
/* We don't have this one for sure */
assert(0);
(void)ptr;
(void)size;
(void)attribute;
(void)context;
return mmgr_NULL;
}

void dli_mmgr_clear(
	ia_css_ptr			vaddr,
	const size_t			size)
{
	dli_mmgr_set(vaddr, (uint8_t)0, size);
}

void dli_mmgr_set(
	ia_css_ptr			vaddr,
	const uint8_t			data,
	const size_t			size)
{
#if SH_CSS_MEMORY_GUARDING
	assert(mem_guard_valid(vaddr, size));
#endif
	hrt_isp_css_mm_set((hmm_ptr)HOST_ADDRESS(vaddr), (int)data, size);
return;
}

void dli_mmgr_load(
	const ia_css_ptr		vaddr,
	void				*data,
	const size_t			size)
{
#if SH_CSS_MEMORY_GUARDING
	assert(mem_guard_valid(vaddr, size));
#endif
	hrt_isp_css_mm_load((hmm_ptr)HOST_ADDRESS(vaddr), data, size);
return;
}

void dli_mmgr_store(
	const ia_css_ptr		vaddr,
	const void				*data,
	const size_t			size)
{
#if SH_CSS_MEMORY_GUARDING
	assert(mem_guard_valid(vaddr, size));
#endif
	hrt_isp_css_mm_store((hmm_ptr)HOST_ADDRESS(vaddr), data, size);
return;
}

uint32_t
dli_mmgr_get_base_index(void)
{
	uint64_t addr = hmm_l1_page_mmu_address();
	return addr / _hrt_page_size_or_one(DATA_MMU);
}
