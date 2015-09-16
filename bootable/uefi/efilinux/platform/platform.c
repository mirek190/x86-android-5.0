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
#include <efilinux.h>
#include <cpu.h>
#include "em.h"
#include "fake_em.h"

#include "platform.h"
#include "x86.h"

void init_silvermont(void);
void init_airmont(void);

EFI_STATUS init_platform_functions(void)
{
	enum cpu_id id = x86_identify_cpu();
	switch (id) {
	case CPU_SILVERMONT:
	case CPU_AIRMONT:
		x86_ops(&loader_ops);
		break;
	case CPU_UNKNOWN:
		error(L"Platform not supported!\n");
		return EFI_UNSUPPORTED;
	default:
		warning(L"Unknown CPUID=0x%x, fallback on default X86\n", id);
		x86_ops(&loader_ops);
	}
	return EFI_SUCCESS;
}

/* Stub implementation of loader ops */
static EFI_STATUS stub_check_partition_table(void)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static enum flow_types stub_read_flow_type(void)
{
	warning(L"stubbed!\n");
	return FLOW_UNKNOWN;
}

static void stub_do_cold_off(void)
{
	warning(L"stubbed!\n");
	return;
}

static EFI_STATUS stub_populate_indicators(void)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_load_target(enum targets target, CHAR8 *cmdline)
{
	warning(L"stubbed!\n");
	return EFI_LOAD_ERROR;
}

static enum wake_sources stub_get_wake_source(void)
{
	warning(L"stubbed!\n");
	return WAKE_NOT_APPLICABLE;
}

static enum reset_sources stub_get_reset_source(void)
{
	warning(L"stubbed!\n");
	return RESET_NOT_APPLICABLE;
}

static EFI_STATUS stub_set_reset_source(enum reset_sources value)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_save_reset_source(enum reset_sources value)
{
	debug(L"WARNING: stubbed!\n");
	return EFI_UNSUPPORTED;
}

static enum reset_types stub_get_reset_type(void)
{
	warning(L"stubbed!\n");
	return NOT_APPLICABLE;
}

static enum shutdown_sources stub_get_shutdown_source(void)
{
	warning(L"stubbed!\n");
	return SHTDWN_NOT_APPLICABLE;
}

static int stub_is_osnib_corrupted(void)
{
	warning(L"stubbed!\n");
	return 0;
}

static int stub_combo_key(enum combo_keys combo)
{
	warning(L"stubbed!\n");
	return 0;
}

static EFI_STATUS stub_save_previous_target_mode(enum targets target)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_save_target_mode(enum targets target)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_set_rtc_alarm_charging(int val)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_set_wdt_counter(int val)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static enum targets stub_get_target_mode(void)
{
	warning(L"stubbed!\n");
	return TARGET_BOOT;
}

static enum targets stub_get_last_target_mode(void)
{
	debug(L"WARNING: stubbed!\n");
	return TARGET_BOOT;
}

static int stub_get_rtc_alarm_charging(void)
{
	warning(L"stubbed!\n");
	return 0;
}

static int stub_get_wdt_counter(void)
{
	warning(L"stubbed!\n");
	return 0;
}

static void stub_hook_before_exit(void)
{
	warning(L"stubbed!\n");
}

static void __attribute__((unused)) stub_hook_before_jump(void)
{
}

static void stub_hook_bootlogic_begin(void)
{
	warning(L"stubbed!\n");
}

static void stub_hook_bootlogic_end(void)
{
	warning(L"stubbed!\n");
}

EFI_STATUS stub_update_boot(void)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

EFI_STATUS stub_display_splash(CHAR8 *bmp, UINTN size)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static EFI_STATUS stub_hash_verify(VOID *blob, UINTN blob_size,
		VOID *sig, UINTN sig_size)
{
	warning(L"stubbed!\n");
	return EFI_SUCCESS;
}

static CHAR8* stub_get_extra_cmdline(void)
{
	warning(L"stubbed!\n");
	return NULL;
}

static UINT64 stub_get_current_time_us(void)
{
	/* Don't call the warning function here to avoid a forever
	 * recursive loop. */
	return 0;
}

static enum targets stub_load_bcb(void)
{
	warning(L"stubbed!\n");
	return TARGET_UNKNOWN;
}

struct osloader_ops loader_ops = {
	.check_partition_table = stub_check_partition_table,
	.read_flow_type = stub_read_flow_type,
	.do_cold_off = stub_do_cold_off,
	.populate_indicators = stub_populate_indicators,
	.load_target = stub_load_target,
	.get_wake_source = stub_get_wake_source,
	.get_reset_source = stub_get_reset_source,
	.set_reset_source = stub_set_reset_source,
	.save_reset_source = stub_save_reset_source,
	.get_reset_type = stub_get_reset_type,
	.get_shutdown_source = stub_get_shutdown_source,
	.is_osnib_corrupted = stub_is_osnib_corrupted,
	.em_ops = &fake_em_ops,
	.combo_key = stub_combo_key,
	.save_previous_target_mode = stub_save_previous_target_mode,
	.save_target_mode = stub_save_target_mode,
	.set_rtc_alarm_charging = stub_set_rtc_alarm_charging,
	.set_wdt_counter = stub_set_wdt_counter,
	.get_target_mode = stub_get_target_mode,
	.get_last_target_mode = stub_get_last_target_mode,
	.get_rtc_alarm_charging = stub_get_rtc_alarm_charging,
	.get_wdt_counter = stub_get_wdt_counter,
	.hook_before_exit = stub_hook_before_exit,
	.hook_before_jump = stub_hook_before_jump,
	.hook_bootlogic_begin = stub_hook_bootlogic_begin,
	.hook_bootlogic_end = stub_hook_bootlogic_end,
	.display_splash = stub_display_splash,
	.hash_verify = stub_hash_verify,
	.get_extra_cmdline = stub_get_extra_cmdline,
	.get_current_time_us = stub_get_current_time_us,
	.load_bcb = stub_load_bcb,
};
