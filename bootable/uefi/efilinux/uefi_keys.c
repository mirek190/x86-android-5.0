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
#include "bootlogic.h"

#define VOLUME_UP	0x1
#define VOLUME_DOWN	0x2

struct _pressed_scancodes {
	UINT16 scancode;
	CHAR8 pressed;
};

static struct _pressed_scancodes pressed_scancodes[] = {
	{VOLUME_UP, 0},
	{VOLUME_DOWN, 0},
};

EFI_STATUS uefi_get_key(EFI_INPUT_KEY *key)
{
	return uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, key);
}

void get_key_pressed(void)
{
	EFI_INPUT_KEY key;
	UINTN i;
	while (!EFI_ERROR(uefi_get_key(&key)))
		for (i = 0 ; i < sizeof(pressed_scancodes) / sizeof(*pressed_scancodes); i++)
			if (key.ScanCode == pressed_scancodes[i].scancode) {
				pressed_scancodes[i].pressed = 1;
				break;
			}
}

int is_key_pressed(int key)
{
	int i;
	get_key_pressed();

	for (i = 0; i < sizeof(pressed_scancodes) / sizeof(*pressed_scancodes); i++)
		if (pressed_scancodes[i].scancode == key)
			return pressed_scancodes[i].pressed;
	return 0;
}

int uefi_combo_key(enum combo_keys combo)
{
	switch(combo) {
	case COMBO_FASTBOOT_MODE:
		return is_key_pressed(VOLUME_DOWN);
	default:
		error(L"Unknown combo 0x%x\n", combo);
	}
	return 0;
}
