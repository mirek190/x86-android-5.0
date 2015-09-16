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
#include "log.h"
#include "fake_em.h"

static enum batt_levels fake_get_battery_level(void)
{
	return BATT_BOOT_OS;
}

static BOOLEAN fake_is_battery_ok(void)
{
	return TRUE;
}

static BOOLEAN fake_is_charger_present(void)
{
	return TRUE;
}

static BOOLEAN fake_is_battery_below_vbattfreqlmt(void)
{
	return FALSE;
}

static void fake_print_battery_infos(void)
{
	info(L"Fake Battery, no info\n");
}

struct energy_mgmt_ops fake_em_ops = {
	.get_battery_level = fake_get_battery_level,
	.is_battery_ok = fake_is_battery_ok,
	.is_charger_present = fake_is_charger_present,
	.is_battery_below_vbattfreqlmt = fake_is_battery_below_vbattfreqlmt,
	.print_battery_infos = fake_print_battery_infos
};
