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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "dli_env.h"
#include "dli_memory_access.h"
#include "dli_device_access.h"
#include <time.h>

static int print(const char *fmt, va_list args)
{
	int printed;
	char *strp;

	static bool newline = true;

	/**
	 * We only want these time-stamps at a new line.
	 * So we need code (see below) to detect a newline
	 * to handle "partial" prints (ie. prints without a newline)
	 */
	if (newline == true) {
		struct timespec tp;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
		fprintf(stdout, "%01lu:%02lu:%02lu.%03lu ",
			(tp.tv_sec/3600), (tp.tv_sec/60)%60,
			(tp.tv_sec)%60, tp.tv_nsec/1000000);
		newline = false;
	}

	printed = vasprintf(&strp, fmt, args);
	assert(printed >= 0);

	/* For simplicity we only test for newline at the end */
	if ((printed > 0) && (strp[printed-1] == '\n')){
		newline = true;
	}

	printed = fprintf(stdout, "%s", strp);
	fflush(stdout);

	free(strp);

	return printed;
}

static void *
cpu_alloc(size_t bytes, bool zero_mem)
{
	if (zero_mem)
		return calloc(1, bytes);
	else
		return malloc(bytes);
}

static void
cpu_flush(struct ia_css_acc_fw *fw)
{
	/* nothing to do on csim */
	(void)fw;
}

static ia_css_ptr
css_mem_mmap(const void *ptr, const size_t size,
	  uint16_t attrs, void *context)
{
	uint16_t mmgr_attrs = 0;
	/* should all attrs be supported, or only cached and pagealign? */
	if (attrs & IA_CSS_MEM_ATTR_CACHED)
		mmgr_attrs |= MMGR_ATTRIBUTE_CACHED;
	if (attrs & IA_CSS_MEM_ATTR_ZEROED)
		mmgr_attrs |= MMGR_ATTRIBUTE_CLEARED;
	if (attrs & IA_CSS_MEM_ATTR_CONTIGUOUS)
		mmgr_attrs |= MMGR_ATTRIBUTE_CONTIGUOUS;
	if (attrs & IA_CSS_MEM_ATTR_PAGEALIGN)
		mmgr_attrs |= MMGR_ATTRIBUTE_PAGEALIGN;
	return dli_mmgr_mmap(ptr, size, mmgr_attrs, context);
}

static ia_css_ptr
css_mem_alloc(size_t bytes, uint32_t attrs)
{
	uint16_t mmgr_attrs = 0;
	if (attrs & IA_CSS_MEM_ATTR_CACHED)
		mmgr_attrs |= MMGR_ATTRIBUTE_CACHED;
	if (attrs & IA_CSS_MEM_ATTR_ZEROED)
		mmgr_attrs |= MMGR_ATTRIBUTE_CLEARED;
	if (attrs & IA_CSS_MEM_ATTR_CONTIGUOUS)
		mmgr_attrs |= MMGR_ATTRIBUTE_CONTIGUOUS;
	if (attrs & IA_CSS_MEM_ATTR_PAGEALIGN)
		mmgr_attrs |= MMGR_ATTRIBUTE_PAGEALIGN;
	return dli_mmgr_alloc_attr(bytes, mmgr_attrs);
}

static void
css_mem_free(ia_css_ptr ptr)
{
	dli_mmgr_free(ptr);
}

static int
css_mem_load(ia_css_ptr ptr, void *data, size_t bytes)
{
	dli_mmgr_load(ptr, data, bytes);
	return 0;
}

static int
css_mem_store(ia_css_ptr ptr, const void *data, size_t bytes)
{
	dli_mmgr_store(ptr, data, bytes);
	return 0;
}

static int
css_mem_set(ia_css_ptr ptr, int c, size_t bytes)
{
	assert(c == 0);
	dli_mmgr_clear(ptr, bytes);
	return 0;
}

static void
css_store_8(hrt_address addr, uint8_t data)
{
	dli_device_store_uint8(addr, data);
}

static void
css_store_16(hrt_address addr, uint16_t data)
{
	dli_device_store_uint16(addr, data);
}

static void
css_store_32(hrt_address addr, uint32_t data)
{
	dli_device_store_uint32(addr, data);
}

static uint8_t
css_load_8(hrt_address addr)
{
	return dli_device_load_uint8(addr);
}

static uint16_t
css_load_16(hrt_address addr)
{
	return dli_device_load_uint16(addr);
}

static uint32_t
css_load_32(hrt_address addr)
{
	return dli_device_load_uint32(addr);
}

static void
css_store(hrt_address addr, const void *data, uint32_t bytes)
{
	dli_device_store(addr, data, bytes);
}

static void
css_load(hrt_address addr, void *data, uint32_t bytes)
{
	dli_device_load(addr, data, bytes);
}

static struct ia_css_env my_env = {
	.cpu_mem_env = {
		.alloc = cpu_alloc,
		.free  = free,
		.flush = cpu_flush,
	},
	.css_mem_env = {
		.alloc = css_mem_alloc,
		.free  = css_mem_free,
		.load  = css_mem_load,
		.store = css_mem_store,
		.set   = css_mem_set,
		.mmap  = css_mem_mmap,
	},
	.hw_access_env = {
		.store_8  = css_store_8,
		.store_16 = css_store_16,
		.store_32 = css_store_32,
		.load_8   = css_load_8,
		.load_16  = css_load_16,
		.load_32  = css_load_32,
		.store    = css_store,
		.load     = css_load,
	},
	.print_env = {
		.debug_print = print,
		.error_print = print,
	},
};

static bool init_done = false;

static void init(void)
{
	if (!init_done) {
		init_done = true;
		dli_mmgr_init();
		dli_device_set_base_address((sys_address)0);
	}
}

const struct ia_css_env *dli_get_env(void)
{
	init();
	return &my_env;
}

uint32_t dli_get_mmu_base(void)
{
	init();
	/* dummy memory allocation to trigger hmm initialization
	 * and subsequently the writing of the hmmu base index */
	css_mem_alloc(1, 0);
	/* hack, hive_isp_css_mm_hrt.c initializes hmm and writes
	 * the L1 base index to the base_address register in the MMU.
	 * We pick it up from there for now. This needs to be replaced
	 * by a proper mechanism.
	 * */
	return dli_mmgr_get_base_index();
}

