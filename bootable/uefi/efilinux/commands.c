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
#include "platform/platform.h"

void dump_infos(void)
{
	info(L"Wake source = 0x%x\n", loader_ops.get_wake_source());
	info(L"Reset source = 0x%x\n", loader_ops.get_reset_source());
	info(L"Reset type = 0x%x\n", loader_ops.get_reset_type());
	info(L"Shutdown source = 0x%x\n", loader_ops.get_shutdown_source());
	info(L"Batt level = %d\n", loader_ops.em_ops->get_battery_level());
	info(L"Batt status = 0x%x\n", loader_ops.em_ops->is_battery_ok());
	loader_ops.em_ops->print_battery_infos();
	info(L"COMBO_FASTBOOT_MODE = 0x%x\n", loader_ops.combo_key(COMBO_FASTBOOT_MODE));
	info(L"Target mode = 0x%x\n", loader_ops.get_target_mode());
	info(L"Wdt counter = 0x%x\n", loader_ops.get_wdt_counter());
}
