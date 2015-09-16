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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <efi.h>
#include "bootlogic.h"
#include "em.h"

struct osloader_ops {
	EFI_STATUS (*check_partition_table)(void);
	enum flow_types (*read_flow_type)(void);
	void (*do_cold_off)(void);
	EFI_STATUS (*populate_indicators)(void);
	EFI_STATUS (*load_target)(enum targets, CHAR8 *cmdline);
	enum wake_sources (*get_wake_source)(void);
	enum reset_sources (*get_reset_source)(void);
	EFI_STATUS (*set_reset_source)(enum reset_sources);
	EFI_STATUS (*save_reset_source)(enum reset_sources);
	enum reset_types (*get_reset_type)(void);
	enum shutdown_sources (*get_shutdown_source)(void);
	int (*is_osnib_corrupted)(void);
	struct energy_mgmt_ops *em_ops;
	int (*combo_key)(enum combo_keys);
	EFI_STATUS (*save_previous_target_mode)(enum targets);
	EFI_STATUS (*save_target_mode)(enum targets);
	EFI_STATUS (*set_rtc_alarm_charging)(int);
	EFI_STATUS (*set_wdt_counter)(int);
	enum targets (*get_last_target_mode)(void);
	enum targets (*get_target_mode)(void);
	int (*get_rtc_alarm_charging)(void);
	int (*get_wdt_counter)(void);
	void (*hook_before_exit)(void);
	void (*hook_before_jump)(void);
	void (*hook_bootlogic_begin)(void);
	void (*hook_bootlogic_end)(void);
	EFI_STATUS (*display_splash)(CHAR8 *bmp, UINTN size);
	EFI_STATUS (*hash_verify)(VOID*, UINTN, VOID*, UINTN);
	CHAR8* (*get_extra_cmdline)(void);
	UINT64 (*get_current_time_us)(void);
	enum targets (*load_bcb)(void);
};

extern struct osloader_ops loader_ops;

EFI_STATUS init_platform_functions(void);

#endif /* _PLATFORM_H_ */
