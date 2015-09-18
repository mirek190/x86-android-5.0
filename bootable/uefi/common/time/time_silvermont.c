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

#include <efi.h>
#include <cpu.h>
#include "time_silvermont.h"

#define MSR_PLATFORM_INFO	0x000000CE
#define MSR_FSB_FREQ		0xCD

static UINT64 get_tsc_freq(void)
{
	UINT64 platform_info;
	UINT64 clk_info;
	UINT64 bclk_khz;

	platform_info = rdmsr(MSR_PLATFORM_INFO);
	clk_info = rdmsr(MSR_FSB_FREQ);
	switch (clk_info  & 0x3) {
	case 0: bclk_khz =  83333; break;
	case 1: bclk_khz = 100000; break;
	case 2: bclk_khz = 133333; break;
	case 3: bclk_khz = 116666; break;
	}
	return (bclk_khz * ((platform_info >> 8) & 0xff)) / 1000;
}

UINT64 silvermont_get_current_time_us()
{
	static UINT64 tsc_freq, start;
	UINT64 cur = rdtsc();

	if (tsc_freq == 0)
		tsc_freq = get_tsc_freq();

	if (start == 0)
		start = cur;

	return (cur - start) / tsc_freq;
}
