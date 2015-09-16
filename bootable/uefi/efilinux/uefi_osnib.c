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
 */

#include <efi.h>
#include <efilib.h>
#include "efilinux.h"
#include "uefi_osnib.h"
#include "bootlogic.h"
#include "platform/platform.h"
#include "config.h"
#include "uefi_utils.h"

/* Warning: These macros requires that the data is a contained in a BYTE ! */
#define set_osnib_var(var, persistent)				\
	uefi_set_simple_var(#var, &osloader_guid, 1, &var, persistent)

#define get_osnib_var(var)			\
	uefi_get_simple_var(#var, &osloader_guid)

EFI_STATUS uefi_set_rtc_alarm_charging(int RtcAlarmCharging)
{
	return set_osnib_var(RtcAlarmCharging, TRUE);
}

EFI_STATUS uefi_set_wdt_counter(int WdtCounter)
{
	return set_osnib_var(WdtCounter, TRUE);
}

int uefi_get_rtc_alarm_charging(void)
{
	return get_osnib_var(RtcAlarmCharging);
}

int uefi_get_wdt_counter(void)
{
	int counter = get_osnib_var(WdtCounter);
	return counter == -1 ? 0 : counter;
}

CHAR8 *uefi_get_extra_cmdline(void)
{
	return LibGetVariable(L"ExtraKernelCommandLine", &osloader_guid);
}

EFI_STATUS uefi_set_wd_cold_reset(int WDColdReset)
{
	return set_osnib_var(WDColdReset, TRUE);
}

int uefi_get_wd_cold_reset(void)
{
	return get_osnib_var(WDColdReset);
}

EFI_STATUS uefi_set_capsule_update(enum capsule_update_status CapsuleUpdateStatus)
{
	return set_osnib_var(CapsuleUpdateStatus, FALSE);
}

void uefi_populate_osnib_variables(void)
{
	struct int_var {
		int (*get_value)(void);
		char *name;
	} int_vars[] = {
		{ (int (*)(void))loader_ops.get_wake_source, "WakeSource" },
		{ (int (*)(void))loader_ops.get_reset_source, "ResetSource" },
		{ (int (*)(void))loader_ops.get_reset_type, "ResetType" },
		{ (int (*)(void))loader_ops.get_shutdown_source, "ShutdownSource" }
	};

	EFI_STATUS ret;
	int i;
	for (i = 0 ; i < sizeof(int_vars)/sizeof(int_vars[0]) ; i++) {
		struct int_var *var = int_vars + i;
		int value = var->get_value();

		ret = uefi_set_simple_var(var->name, &osloader_guid, 1, &value, FALSE);
		if (EFI_ERROR(ret))
			error(L"Failed to set %a osnib EFI variable", var->name, ret);
	}
}
