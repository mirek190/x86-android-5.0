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

#include "cpu.h"

#define STR_TO_UINTN(a, b, c, d) ((a) + ((b) << 8) + ((c) << 16) + ((d) << 24))
#define CPUID_MASK	0xffff0

static inline void cpuid(uint32_t op, uint32_t reg[4])
{
#ifdef CONFIG_X86
	asm volatile("pushl %%ebx      \n\t" /* save %ebx */
		     "cpuid            \n\t"
		     "movl %%ebx, %1   \n\t" /* save what cpuid just put in %ebx */
		     "popl %%ebx       \n\t" /* restore the old %ebx */
		     : "=a"(reg[0]), "=r"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
		     : "a"(op)
		     : "cc");
#elif CONFIG_X86_64
	asm volatile("xchg{q}\t{%%}rbx, %q1\n\t"
		     "cpuid\n\t"
		     "xchg{q}\t{%%}rbx, %q1\n\t"
		     : "=a" (reg[0]), "=&r" (reg[1]), "=c" (reg[2]), "=d" (reg[3])
		     : "a" (op));
#endif
}

enum cpu_id x86_identify_cpu()
{
	uint32_t reg[4];

	cpuid(0, reg);
	if (reg[1] != STR_TO_UINTN('G', 'e', 'n', 'u') ||
	    reg[3] != STR_TO_UINTN('i', 'n', 'e', 'I') ||
	    reg[2] != STR_TO_UINTN('n', 't', 'e', 'l')) {
		return CPU_UNKNOWN;
	}

	cpuid(1, reg);
	return reg[0] & CPUID_MASK;
}
