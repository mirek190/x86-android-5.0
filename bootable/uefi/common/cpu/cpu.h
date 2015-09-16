/*
 * Copyright (c) 2014, Intel Corporation
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

#ifndef _CPU_H_
#define _CPU_H_

#include <efi.h>

enum cpu_id {
	CPU_SILVERMONT = 0x30670,
	CPU_AIRMONT    = 0x406C0,
	CPU_UNKNOWN    = 0x0
};

enum cpu_id x86_identify_cpu();

#if !defined(CONFIG_X86) && !defined(CONFIG_X86_64)
#error "Only architecure x86 and x86_64 are supported"
#endif

static inline uint64_t rdtsc(void)
{
	uint64_t x;

#ifdef CONFIG_X86
	asm volatile ("rdtsc" : "=A" (x));
#elif CONFIG_X86_64
	uint32_t high, low;

	asm volatile ("rdtsc" : "=a" (low), "=d" (high));
	x = ((uint64_t)low) | ((uint64_t)high) << 32;
#endif

	return x;
}

static inline uint64_t rdmsr(unsigned int msr)
{
	uint64_t x;
	asm volatile ("rdmsr" : "=A" (x) : "c" (msr));
	return x;
}

#endif	/* _CPU_H_ */
