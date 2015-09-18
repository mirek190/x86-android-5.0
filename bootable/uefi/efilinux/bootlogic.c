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
#include <uefi_utils.h>
#include "efilinux.h"
#include "acpi.h"
#include "esrt.h"
#include "bootlogic.h"
#include "platform/platform.h"
#include "uefi_osnib.h"
#include "splash.h"
#include "config.h"
#include "fs.h"
#include "pmic.h"

static enum targets boot_bcb(int dummy)
{
	return loader_ops.load_bcb();
}

int batt_boot_os(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return 1;
}

void forced_shutdown(void)
{
	debug(L"Forced shutdown occured\n");
	loader_ops.set_rtc_alarm_charging(0);
	loader_ops.set_wdt_counter(0);
}

enum targets boot_fastboot_combo(enum wake_sources ws)
{
	return loader_ops.combo_key(COMBO_FASTBOOT_MODE) ? TARGET_FASTBOOT : TARGET_UNKNOWN;
}

enum targets boot_power_key(enum wake_sources ws)
{
	return ws == WAKE_POWER_BUTTON_PRESSED ? TARGET_BOOT : TARGET_UNKNOWN;
}

enum targets boot_rtc(enum wake_sources ws)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return TARGET_UNKNOWN;
}

enum targets boot_battery_insertion(enum wake_sources ws)
{
	if (ws == WAKE_BATTERY_INSERTED) {
		/* TI PMIC reports BATTERY INSERTED when charging
		 * from dead battery. Enter COS when charger present
		 */
		if (pmic_get_type_from_smbios() == DOLLAR_TI
		    && loader_ops.em_ops->is_charger_present()) {
			debug(L"Charging from dead battery detected.\n");
			return TARGET_CHARGING;
		}
		debug(L"Battery insertion detected. Shutdown.\n");
		return TARGET_COLD_OFF;
	}
	else
		return TARGET_UNKNOWN;
}

enum targets boot_charger_insertion(enum wake_sources ws)
{
	if (ws == WAKE_USB_CHARGER_INSERTED ||
	    ws == WAKE_ACDC_CHARGER_INSERTED)
		return loader_ops.em_ops->is_charger_present() ?
			TARGET_CHARGING : TARGET_COLD_OFF;
	else
		return TARGET_UNKNOWN;
}

enum targets target_from_off(enum wake_sources ws)
{
	enum targets target = TARGET_UNKNOWN;

	if (loader_ops.get_shutdown_source() == SHTDWN_POWER_BUTTON_OVERRIDE)
		forced_shutdown();

	enum targets (*boot_case[])(enum wake_sources wake_source) = {
		boot_fastboot_combo,
		boot_bcb,
		boot_power_key,
		boot_rtc,
		boot_battery_insertion,
		boot_charger_insertion,
	};

	int i;
	for (i = 0; i < sizeof(boot_case) / sizeof(*boot_case); i++) {
		target = boot_case[i](ws);
		if (target != TARGET_UNKNOWN)
			break;
	}

	return target;
}

enum targets boot_fw_update(enum reset_sources rs)
{
	struct FW_RES_ENTRY *fw_entry;
	EFI_STATUS ret;

	if (rs != RESET_FW_UPDATE)
		return TARGET_UNKNOWN;

	ret = get_fw_entry((EFI_GUID *)&SYS_FW_GUID, &fw_entry);

	if (!EFI_ERROR(ret)) {
		if (fw_entry->FwVersion == fw_entry->LastAttemptVersion) {
			debug(L"capsule udpate success\n");
			uefi_set_capsule_update(CAPSULE_UPDATE_SUCCESS);
		} else {
			debug(L"capsule udpate fail\n");
			uefi_set_capsule_update(CAPSULE_UPDATE_FAIL);
		}
	} else
		error(L"Fail to get fw_entry: %r\n", ret);

	return TARGET_BOOT;
}

enum targets boot_reset(enum reset_sources rs)
{
	if (rs == RESET_OS_INITIATED || rs == RESET_FORCED)
		return loader_ops.get_target_mode();
	else
		return TARGET_UNKNOWN;
}

enum targets em_fallback_target(enum targets target)
{
	enum targets fallback = target;

	if (loader_ops.em_ops->get_battery_level() == BATT_BOOT_CHARGING)
		switch (target) {
		case TARGET_BOOT:
		case TARGET_FACTORY:
		case TARGET_FACTORY2:
			if (loader_ops.em_ops->is_charger_present())
				fallback = TARGET_CHARGING;
			else{
				//fallback = TARGET_COLD_OFF;
				loader_ops.do_cold_off();
			}
			debug(L"EM fallback from 0x%x to 0x%x\n", target, fallback);
			break;
		default:
			fallback = target;
		}

	return fallback;
}

enum targets fallback_target(enum targets target)
{
	enum targets fallback;
	switch (target) {
	case TARGET_BOOT:
	case TARGET_CHARGING:
		fallback = TARGET_RECOVERY;
		break;
	case TARGET_RECOVERY:
		fallback = TARGET_FASTBOOT;
		break;
	case TARGET_FASTBOOT:
		fallback = TARGET_DNX;
		break;
	default:
		fallback = TARGET_UNKNOWN;
	}

	debug(L"Fallback from 0x%x to 0x%x\n", target, fallback);
	return fallback;
}

static inline EFI_STATUS call_warmdump(void)
{
	CHAR16 *path = WARMDUMP_FILE_PATH;

	path_to_dos(path);
	return uefi_call_image(main_image_handle, efilinux_image,
			       path, NULL, NULL);
}

enum targets boot_watchdog(enum reset_sources rs)
{
	if (rs != RESET_KERNEL_WATCHDOG
	    && rs != RESET_SECURITY_WATCHDOG
	    && rs != RESET_SECURITY_INITIATED
	    && rs != RESET_PMC_WATCHDOG
	    && rs != RESET_EC_WATCHDOG
	    && rs != RESET_PLATFORM_WATCHDOG)
		return TARGET_UNKNOWN;

	if (has_warmdump) {
		EFI_STATUS ret = call_warmdump();
		if (EFI_ERROR(ret))
			error(L"Warmdump error (%r)\n", ret);
	}

	enum targets last_target = loader_ops.get_last_target_mode();

	if (do_cold_reset_after_wd) {
		if (uefi_get_wd_cold_reset() == 1) {
			if (EFI_ERROR(uefi_set_wd_cold_reset(0)))
				error(L"Failed to set WDColdReset variable to 0\n");
		} else {
			// else branch for 0 and -1 when WDColdReset var is not yet initialized.
			loader_ops.save_target_mode(last_target);
			loader_ops.save_reset_source(rs);
			if (EFI_ERROR(uefi_set_wd_cold_reset(1)))
				error(L"Failed to set WDColdReset variable to 1\n");
			debug(L"cold reset after watchdog\n");
			uefi_reset_system(EfiResetCold);
			error(L"Reset requested, this code should not be reached\n");
		}
	}

	int wdt_counter = loader_ops.get_wdt_counter();

	wdt_counter++;
	debug(L"watchdog counter = %d\n", wdt_counter);
	debug(L"last target = 0x%x\n", last_target);
	if (wdt_counter >= 3) {
		loader_ops.set_wdt_counter(0);
		return fallback_target(last_target);
	}

	loader_ops.set_wdt_counter(wdt_counter);

	return last_target;
}

enum targets target_from_reset(enum reset_sources rs)
{
	enum targets target = TARGET_UNKNOWN;
	enum targets (*boot_case[])(enum reset_sources reset_source) = {
		boot_watchdog,
		boot_bcb,
		boot_fw_update,
		boot_reset,
	};

	int i = 0;
	for (i = 0; i < sizeof(boot_case) / sizeof(*boot_case); i++) {
		target = boot_case[i](rs);
		if (target != TARGET_UNKNOWN)
			break;
	}

	debug(L"target=0x%x\n", target);
	return target;
}

enum targets target_from_inputs(enum flow_types flow_type)
{
	enum wake_sources ws;
	enum reset_sources rs;

	if (!loader_ops.em_ops->is_battery_ok())
		return TARGET_COLD_OFF;

	ws = loader_ops.get_wake_source();
	rs = loader_ops.get_reset_source();
	debug(L"Wake source = 0x%x\n", ws);
	debug(L"Reset source = 0x%x\n", rs);
	if (ws == WAKE_ERROR) {
		error(L"Wake source couldn't be retrieved. Falling back in TARGET_BOOT\n");
		return TARGET_BOOT;
	}
	/* WA for wrong wake source passed by FW,
	   checking reset source == 6 */
	if (ws != WAKE_NOT_APPLICABLE)
		return target_from_off(ws);

	if (ws == WAKE_NOT_APPLICABLE && rs == RESET_SECURITY_INITIATED)
	{
		debug(L"WA path taken\n");
		ws = WAKE_BATTERY_INSERTED;
		return target_from_off(ws);
	}


	rs = loader_ops.get_reset_source();
	if (rs == RESET_ERROR) {
		error(L"Reset source couldn't be retrieved. Falling back in TARGET_BOOT\n");
		return TARGET_BOOT;
	}
	debug(L"Reset source = 0x%x\n", rs);

	if (do_cold_reset_after_wd && uefi_get_wd_cold_reset() == 1) {
		loader_ops.set_reset_source(rs);
	}

	if (rs != RESET_NOT_APPLICABLE)
		return target_from_reset(rs);

	return target_from_reset(rs);
}

CHAR8 *wake_string[] = {
	"wakeup=no_applicable",
	"wakeup=battery_insertd",
	"wakeup=usb_inserted",
	"wakeup=addc_inserted",
	"wakeup=power_button",
	"wakeup=rtc_timer",
	"wakeup=battery_reach_th",
	"wakeup=Null",
};

CHAR8 *reset_string[] = {
	"reset=no_applicable",
	"reset=os_initialted",
	"reset=forced",
	"reset=fw_updated",
	"reset=kernel_wdt",
	"reset=security_wdt",
	"reset=security_initialed",
	"reset=pmc_wdt",
	"reset=ec_wdt",
	"reset=platform_wdt"
	"reset=Null",
};

CHAR8 *shutdown_string[] = {
	"powerdown=no_applicable",
	"powerdown=button_long",
	"powerdown=battery_remove",
	"powerdown=CRIT",
	"powerdown=thermtrip",
	"powerdown=pmic_temp",
	"powerdown=sys_temp",
	"powerdown=bat_temp",
	"powerdown=sysuvp",
	"powerdown=sysovp",
	"powerdown=security_watchdog",
	"powerdown=security_intiated",
	"powerdown=pmc_watchdog",
	"powerdown=NULL",
};

CHAR8 *get_powerup_reason(CHAR8 *cmdline)
{
	enum wake_sources ws;
	enum shutdown_sources ss;
	enum reset_sources rs;
	ss = loader_ops.get_shutdown_source();
	ws = loader_ops.get_wake_source();
	rs = loader_ops.get_reset_source();

	cmdline = append_strings(shutdown_string[ss], cmdline);
	cmdline = append_strings(wake_string[ws], cmdline);
	cmdline = append_strings(reset_string[rs], cmdline);

	return cmdline;
}

CHAR8 *get_extra_cmdline(CHAR8 *cmdline)
{
	CHAR8 *extra_cmdline;
	CHAR8 *updated_cmdline;

	extra_cmdline = loader_ops.get_extra_cmdline();
	debug(L"Getting extra commandline: %a\n", extra_cmdline ? extra_cmdline : (CHAR8 *)"");

	updated_cmdline = append_strings(extra_cmdline, cmdline);
	updated_cmdline = get_powerup_reason(updated_cmdline);
	if (extra_cmdline)
		FreePool(extra_cmdline);
	if (cmdline)
		FreePool(cmdline);

	return updated_cmdline;
}

CHAR8 *check_vbattfreqlmt(CHAR8 *cmdline)
{
	CHAR8 *updated_cmdline = cmdline;

	if (loader_ops.em_ops->is_battery_below_vbattfreqlmt()) {
		debug(L"Battery voltage below vbattfreqlmt add battlow in cmdline\n");
		updated_cmdline = append_strings((CHAR8 *)"battlow ", cmdline);
		if (cmdline)
			FreePool(cmdline);
	}

	return updated_cmdline;
}

void display_splash(void)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return;
}

EFI_STATUS check_target(enum targets target)
{
	/* TODO */
	debug(L"TO BE IMPLEMENTED\n");
	return EFI_SUCCESS;
}

static EFI_STATUS launch_or_fallback(enum targets target, CHAR8 *cmdline)
{
	EFI_STATUS ret;
	CHAR8 saved_cmdline[(cmdline ? strlena(cmdline) : 0) + 1];

	if (cmdline) {
		CopyMem(saved_cmdline, cmdline, strlena(cmdline) + 1);
		FreePool(cmdline);
	} else
		saved_cmdline[0] = '\0';

	do {
		target = em_fallback_target(target);

		ret = loader_ops.populate_indicators();
		if (EFI_ERROR(ret))
			return ret;

		ret = loader_ops.save_target_mode(target);
		if (EFI_ERROR(ret))
			warning(L"Failed to save the target_mode: %r\n", ret);

		ret = loader_ops.load_target(target, saved_cmdline);

		target = fallback_target(target);
	} while (target != TARGET_UNKNOWN);

	return EFI_INVALID_PARAMETER;
}

EFI_STATUS start_boot_logic(CHAR8 *cmdline)
{
	EFI_STATUS ret;
	enum flow_types flow_type;
	enum targets target;
	CHAR8 *updated_cmdline = NULL;

	loader_ops.hook_bootlogic_begin();

	ret = loader_ops.check_partition_table();
	if (EFI_ERROR(ret))
		goto error;

	loader_ops.save_previous_target_mode(loader_ops.get_last_target_mode());

	flow_type = loader_ops.read_flow_type();

	target = target_from_inputs(flow_type);
	if (target == TARGET_ERROR)
		goto error;
	if (target == TARGET_UNKNOWN) {
		error(L"No valid target found. Fallbacking to MOS\n");
		target = TARGET_BOOT;
	}
	debug(L"target = 0x%x\n", target);

	if (target == TARGET_COLD_OFF) {
		debug(L"TARGET_COLD_OFF shutdown\n");
		loader_ops.do_cold_off();
	}

	loader_ops.display_splash(splash_intel, splash_intel_size);

	updated_cmdline = check_vbattfreqlmt(cmdline);

#ifdef RUNTIME_SETTINGS
	updated_cmdline = get_extra_cmdline(updated_cmdline);
#endif

	loader_ops.hook_bootlogic_end();

	ret = launch_or_fallback(target, updated_cmdline);

error:
	return ret;
}
