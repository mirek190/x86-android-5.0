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
 */

#include "stdio.h"
#include <efilib.h>
#include <uefi_utils.h>
#include <log.h>

int sprintf(char *str, const char *format, ...)
{
	va_list args;
	UINTN len;
	int ret = -1;
	CHAR16 *str16;
	CHAR16 *format16 = stra_to_str((CHAR8 *)format);

	if (!format16)
		return -1;

	va_start(args, format);
	str16 = VPoolPrint(format16, args);
	va_end(args);

	if (!str16)
		goto free_format16;

	len = StrLen(str16);
	if (str_to_stra((CHAR8 *)str, str16, len) == EFI_SUCCESS) {
		ret = 0;
		str[len] = '\0';
	}

	FreePool(str16);
free_format16:
	FreePool(format16);
	return ret;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
	va_list args;
	int ret;

	va_start(args, format);
	ret = vsnprintf(str, size, format, args);
	va_end(args);
	return ret;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	UINTN len;
	int ret = -1;
	CHAR16 *format16 = stra_to_str((CHAR8 *)format);
	if (!format16)
		return -1;

	CHAR16 *str16 = AllocatePool(size * sizeof(CHAR16));
	if (!str16)
		goto free_format16;

	len = VSPrint(str16, size * sizeof(CHAR16), format16, ap);

	if (str_to_stra((CHAR8 *)str, str16, len) == EFI_SUCCESS) {
		ret = 0;
		str[len] = '\0';
	}

	FreePool(str16);
free_format16:
	FreePool(format16);
	return ret;
}
