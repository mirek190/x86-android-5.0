/*
 * Copyright (c) 2014, Intel Corporation
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

#include <time.h>
#include <uefi_utils.h>

#include "log.h"

#define LOG_TAG_LEN 20
#define LOG_BUF_SIZE ((10 * 1024))

static CHAR16 log_tag[LOG_TAG_LEN];
static UINTN log_line_len = 200;
static BOOLEAN log_flush_to_variable = FALSE;
static BOOLEAN log_timestamp = TRUE;
enum loglevel log_level = LEVEL_DEBUG;

static CHAR16 buffer[LOG_BUF_SIZE / sizeof(CHAR16)];
static CHAR16 *cur = buffer;

EFI_STATUS log_set_logtag(const CHAR16 *tag)
{
	if (!tag || StrLen(tag) >= LOG_TAG_LEN)
		return EFI_INVALID_PARAMETER;
	StrCpy(log_tag, tag);
	return EFI_SUCCESS;
}

VOID log_set_line_len(UINTN len)
{
	log_line_len = len;
}

VOID log_set_flush_to_var(BOOLEAN b)
{
	log_flush_to_variable = b;
}

VOID log_set_loglevel(enum loglevel level)
{
	log_level = level;
}

VOID log_set_logtimestamp(BOOLEAN b)
{
	log_timestamp = b;
}

void log(enum loglevel level, const CHAR16 *prefix, const void *func, const INTN line,
	 const CHAR16* fmt, ...)
{
	CHAR16 *start = cur;
	va_list args;

	va_start(args, fmt);

	if (log_timestamp) {
		UINT64 time = get_current_time_us();
		UINT64 sec = time / 1000000;
		UINT64 usec = time - (sec * 1000000);

		cur += SPrint(cur, sizeof(buffer) - sizeof(CHAR16) * (cur - buffer), L"[%5ld.%06ld] ",
			      sec, usec);
	}
	cur += SPrint(cur, sizeof(buffer) - sizeof(CHAR16) * (cur - buffer), (CHAR16 *)prefix,
		      func, line);
	cur += VSPrint(cur, sizeof(buffer) - sizeof(CHAR16) * (cur - buffer), (CHAR16 *)fmt,
		       args);
	if (((cur - buffer) + log_line_len) * sizeof(CHAR16) >= LOG_BUF_SIZE)
		cur = buffer;


	if (log_level >= level)
		Print(L"%s %s", log_tag, start);

	va_end (args);
}

void log_save_to_variable(CHAR16 *varname, EFI_GUID *guid)
{
	EFI_STATUS ret;
	if (log_flush_to_variable && varname && guid) {
		ret = LibSetVariable(varname, guid, (cur - buffer) * sizeof(CHAR16), buffer);
		if (EFI_ERROR(ret))
			warning(L"Save log into EFI variable failed\n");
	}
}
