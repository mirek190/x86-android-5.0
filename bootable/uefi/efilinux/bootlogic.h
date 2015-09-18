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
 * This file defines bootlogic data structures, try to keep it without
 * any external definitions in order to ease export of it.
 */

#ifndef _BOOTLOGIC_H_
#define _BOOTLOGIC_H_

/** RSCI Definitions **/

/* Wake sources */
enum wake_sources {
	WAKE_NOT_APPLICABLE,
	WAKE_BATTERY_INSERTED,
	WAKE_USB_CHARGER_INSERTED,
	WAKE_ACDC_CHARGER_INSERTED,
	WAKE_POWER_BUTTON_PRESSED,
	WAKE_RTC_TIMER,
	WAKE_BATTERY_REACHED_IA_THRESHOLD,
	WAKE_ERROR = -1,
};

/* Reset sources */
enum reset_sources {
	RESET_NOT_APPLICABLE,
	RESET_OS_INITIATED,
	RESET_FORCED,
	RESET_FW_UPDATE,
	RESET_KERNEL_WATCHDOG,
	RESET_SECURITY_WATCHDOG,
	RESET_SECURITY_INITIATED,
	RESET_PMC_WATCHDOG,
	RESET_EC_WATCHDOG,
	RESET_PLATFORM_WATCHDOG,
	RESET_ERROR = -1,
};

/* Reset type */
enum reset_types {
	NOT_APPLICABLE,
	WARM_RESET,
	COLD_RESET,
	GLOBAL_RESET = 0x07,
};

/* Shutdown sources */
enum shutdown_sources {
	SHTDWN_NOT_APPLICABLE,
	SHTDWN_POWER_BUTTON_OVERRIDE,
	SHTDWN_BATTERY_REMOVAL,
	SHTDWN_VCRIT,
	SHTDWN_THERMTRIP,
	SHTDWN_PMICTEMP,
	SHTDWN_SYSTEMP,
	SHTDWN_BATTEMP,
	SHTDWN_SYSUVP,
	SHTDWN_SYSOVP,
	SHTDWN_SECURITY_WATCHDOG,
	SHTDWN_SECURITY_INITIATED,
	SHTDWN_PMC_WATCHDOG,
	SHTDWN_ERROR = -1,
};

/* Indicators */
#define FBR_NONE			0x00
#define FBR_WATCHDOG_COUNTER_EXCEEDED	0x01
#define FBR_NO_MATCHING_OS		0x02
#define FASTBOOT_BUTTONS_COMBO

enum targets {
	TARGET_BOOT	= 0x00,
	TARGET_CHARGING = 0x0A,
	TARGET_RECOVERY = 0x0C,
	TARGET_FASTBOOT = 0x0E,
	TARGET_TEST,
	TARGET_COLD_OFF,
	TARGET_FACTORY	= 0x12,
	TARGET_FACTORY2	= 0x13,
	TARGET_DNX,
	TARGET_UNKNOWN,
	TARGET_ERROR	= -1,
};

enum flow_types {
	FLOW_NORMAL,
	FLOW_3GPP,
	FLOW_UNKNOWN,
};

enum batt_levels {
	BATT_BOOT_CHARGING,
	BATT_BOOT_OS,
	BATT_ERROR = -1,
};

enum combo_keys {
	COMBO_FASTBOOT_MODE,
	COMBO_ERROR = -1,
};

/*
 * Be carefull using this enumeration, values 0 and 1 are
 * reserved for SUCCESS and FAIL.
 */
enum capsule_update_status {
	CAPSULE_UPDATE_SUCCESS,
	CAPSULE_UPDATE_FAIL
};

#endif /* _BOOTLOGIC_H_ */
