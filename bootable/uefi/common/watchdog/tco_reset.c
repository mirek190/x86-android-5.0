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
#include <efilib.h>
#include <uefi_utils.h>
#include <log.h>
#include "tco_reset.h"

EFI_GUID gEfiTcoResetProtocolGuid = EFI_TCO_RESET_PROTOCOL_GUID;

static EFI_STATUS start_tco_watchdog(struct watchdog *wd)
{
	EFI_TCO_RESET_PROTOCOL *tco;
	EFI_STATUS ret;

	debug(L"timeout = %d\n", wd->reg);
	ret = LibLocateProtocol(&gEfiTcoResetProtocolGuid, (void **)&tco);
	if (EFI_ERROR(ret) || !tco) {
		error(L"%x failure\n", __func__);
		goto out;
	}

	ret = uefi_call_wrapper(tco->EnableTcoReset, 1, &(wd->reg));

out:
	return ret;
}

static EFI_STATUS stop_tco_watchdog(struct watchdog *wd)
{
	EFI_TCO_RESET_PROTOCOL *tco;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&gEfiTcoResetProtocolGuid, (void **)&tco);
	if (EFI_ERROR(ret) || !tco) {
		error(L"%x failure\n", __func__);
		goto out;
	}

	ret = uefi_call_wrapper(tco->DisableTcoReset, 1, wd->reg);

out:
	return ret;
}

static void set_tco_timeout(struct watchdog *wd, UINT32 timeout)
{
	wd->reg = timeout;
}

struct watchdog tco_watchdog = {
	.reg = TCO_DEFAULT_TIMEOUT,
	.ops = {
		.start = start_tco_watchdog,
		.stop = stop_tco_watchdog,
		.set_timeout = set_tco_timeout,
	},
};

void tco_start_watchdog(void)
{
	tco_watchdog.ops.start(&tco_watchdog);
}
