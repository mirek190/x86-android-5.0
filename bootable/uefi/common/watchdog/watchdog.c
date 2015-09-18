/*
 * Copyright (c) 2013, Intel Corporation
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
 *
 */

#include <efi.h>
#include <uefi_utils.h>
#include <log.h>
#include "watchdog.h"

static EFI_STATUS stub_watchdog_start(struct watchdog __attribute__((unused)) *wd)
{
	debug(L"boot watchdog disabled on this platform\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_watchdog_stop(struct watchdog __attribute__((unused)) *wd)
{
	debug(L"boot watchdog disabled on this platform\n");
	return EFI_SUCCESS;
}

static void stub_watchdog_set_timeout(struct watchdog __attribute__((unused)) *wd, UINT32 __attribute__((unused)) timeout)
{
	debug(L"boot watchdog disabled on this platform\n");
}

struct watchdog __attribute__((used)) stub_watchdog = {
	.reg = 0,
	.ops = {
		.start = stub_watchdog_start,
		.stop = stub_watchdog_stop,
		.set_timeout = stub_watchdog_set_timeout,
	},
};
