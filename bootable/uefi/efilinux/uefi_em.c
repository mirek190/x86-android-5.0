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
#include "efilinux.h"
#include "bootlogic.h"
#include "acpi.h"
#include "em.h"

#define DEVICE_INFO_PROTOCOL {0xE4F3260B, 0xD35F, 0x4AF1, {0xB9, 0x0E, 0x91, 0x0F, 0x5A, 0xD2, 0xE3, 0x26}}
static EFI_GUID DeviceInfoProtocolGuid = DEVICE_INFO_PROTOCOL;

#define USB_CHARGER_SDP (1 << 0);
#define USB_CHARGER_DCP (1 << 1);
#define USB_CHARGER_CDP (1 << 2);
#define USB_CHARGER_ACA (1 << 3);

typedef UINT8 USB_CHARGER_TYPE;
typedef UINT8 BATT_CAPACITY;
typedef UINT16 BATT_VOLTAGE;

typedef EFI_STATUS (EFIAPI *GET_BATTERY_STATUS) (
	OUT BOOLEAN *BatteryPresent,
	OUT BOOLEAN *BatteryValid,
	OUT BOOLEAN *CapacityReadable,
	OUT BATT_VOLTAGE *BatteryVoltageLevel,
	OUT BATT_CAPACITY *BatteryCapacityLevel);

typedef EFI_STATUS (EFIAPI *GET_ACDC_CHARGER_STATUS) (
	OUT BOOLEAN *ACDCChargerPresent);

typedef EFI_STATUS (EFIAPI *GET_USB_CHARGER_STATUS) (
	OUT BOOLEAN *UsbChargerPresent,
	OUT USB_CHARGER_TYPE *UsbChargerType);

struct _DEVICE_INFO_PROTOCOL {
	UINT32 Revision;
	GET_BATTERY_STATUS GetBatteryStatus;
	GET_ACDC_CHARGER_STATUS GetAcDcChargerStatus;
	GET_USB_CHARGER_STATUS GetUsbChargerStatus;
};

struct battery_status {
	BOOLEAN BatteryPresent;
	BOOLEAN BatteryValid;
	BOOLEAN CapacityReadable;
	BATT_VOLTAGE BatteryVoltageLevel;
	BATT_CAPACITY BatteryCapacityLevel;
};

static BOOLEAN uefi_is_charger_present(void)
{
	struct _DEVICE_INFO_PROTOCOL *dev_info;
	BOOLEAN present;
	USB_CHARGER_TYPE type;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&DeviceInfoProtocolGuid, (VOID **)&dev_info);
	if (EFI_ERROR(ret) || !dev_info)
		goto error;

	ret = uefi_call_wrapper(dev_info->GetUsbChargerStatus, 2, &present, &type);
	if (EFI_ERROR(ret))
		goto error;

	return present;
error:
	error(L"Failed to get charger status: %r\n", ret);
	return FALSE;
}

static EFI_STATUS uefi_get_battery_status(struct battery_status *status)
{
	struct _DEVICE_INFO_PROTOCOL *dev_info;
	EFI_STATUS ret;

	ret = LibLocateProtocol(&DeviceInfoProtocolGuid, (VOID **)&dev_info);
	if (EFI_ERROR(ret) || !dev_info)
		goto error;

	ret = uefi_call_wrapper(dev_info->GetBatteryStatus, 5,
		&status->BatteryPresent,
		&status->BatteryValid,
		&status->CapacityReadable,
		&status->BatteryVoltageLevel,
		&status->BatteryCapacityLevel);

	if (EFI_ERROR(ret))
		goto error;
	return ret;

error:
	error(L"Failed to get battery status: %r\n", ret);
	return ret;
}

static enum batt_levels uefi_get_battery_level(void)
{
	struct battery_status status;
	EFI_STATUS ret;
	UINTN value, threshold;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret))
		goto error;

	if (oem1_get_ia_apps_to_use() == OEM1_USE_IA_APPS_CAP) {
		value = status.BatteryCapacityLevel;
		threshold = oem1_get_ia_apps_cap();
		debug(L"Battery: %d%% Threshold: %d%%\n", value, threshold);
	}
	else {
		value = status.BatteryVoltageLevel;
		threshold = oem1_get_ia_apps_run();
		debug(L"Battery: %dmV Threshold: %dmV\n", value, threshold);
	}

	return value > threshold ? BATT_BOOT_OS : BATT_BOOT_CHARGING;

error:
	error(L"Failed to get battery level: %r\n", ret);
	return BATT_ERROR;
}

static BOOLEAN uefi_is_battery_ok(void)
{
	struct battery_status status;
	EFI_STATUS ret;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get battery status: %r\n", ret);
		return FALSE;
	}

	return status.BatteryPresent;
}

static BOOLEAN uefi_is_battery_below_vbattfreqlmt(void)
{
	struct battery_status status;
	EFI_STATUS ret;
	UINT16 vbattfreqlmt, value;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret)) {
		error(L"Failed to get battery status: %r\n", ret);
		return FALSE;
	}

	value = status.BatteryVoltageLevel;
	vbattfreqlmt = oem1_get_ia_vbattfreqlmt();
	debug(L"Battery: %dmV Vbattfreqlmt: %dmV\n", value, vbattfreqlmt);

	return value < vbattfreqlmt;
}

static void uefi_print_battery_infos(void)
{
	struct battery_status status;
	EFI_STATUS ret;

	ret = uefi_get_battery_status(&status);
	if (EFI_ERROR(ret))
		goto error;

	info(L"BatteryPresent = 0x%x\n", status.BatteryPresent);
	info(L"BatteryValid = 0x%x\n", status.BatteryValid);
	info(L"CapacityReadable = 0x%x\n", status.CapacityReadable);
	info(L"BatteryVoltageLevel = %dmV\n", status.BatteryVoltageLevel);
	info(L"BatteryCapacityLevel = %d%%\n", status.BatteryCapacityLevel);

	info(L"IA_APPS_RUN = %dmV\n", oem1_get_ia_apps_run());
	info(L"IA_APPS_CAP = %d%%\n", oem1_get_ia_apps_cap());
	info(L"CAPFREQIDX = %d\n", oem1_get_capfreqidx());
	info(L"VBATTFREQLMT = %dmV\n", oem1_get_ia_vbattfreqlmt());
	info(L"IA_APPS_TO_USE = 0x%x\n", oem1_get_ia_apps_to_use());

	info(L"charger present = %d\n", uefi_is_charger_present());
error:
	if (EFI_ERROR(ret))
		error(L"Failed to get battery status: %r\n", ret);
}

struct energy_mgmt_ops uefi_em_ops = {
	.get_battery_level = uefi_get_battery_level,
	.is_battery_ok = uefi_is_battery_ok,
	.is_charger_present = uefi_is_charger_present,
	.is_battery_below_vbattfreqlmt = uefi_is_battery_below_vbattfreqlmt,
	.print_battery_infos = uefi_print_battery_infos
};
