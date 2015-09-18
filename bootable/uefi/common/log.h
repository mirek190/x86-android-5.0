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

#ifndef __LOG_H__
#define __LOG_H__

#include <efi.h>

enum loglevel {
	LEVEL_ERROR,
	LEVEL_WARNING,
	LEVEL_INFO,
	LEVEL_DEBUG,
	LEVEL_PROFILE,
};

#define LOGLEVEL(level)	(log_level >= LEVEL_##level)
extern enum loglevel log_level;

void log(enum loglevel level, const CHAR16 *prefix, const void *func, const INTN line,
	 const CHAR16* fmt, ...);

void log_save_to_variable(CHAR16 *varname, EFI_GUID *guid);

#define profile(...) do { \
		log(LEVEL_PROFILE, L"PROFILE [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	} while (0)

#define debug(...) do { \
		log(LEVEL_DEBUG, L"DEBUG [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	} while (0)

#define info(...) do { \
		log(LEVEL_INFO, L"INFO [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	} while (0)

#define warning(...) do { \
		log(LEVEL_WARNING, L"WARNING [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	} while (0)

#define error(...) do { \
		log(LEVEL_ERROR, L"ERROR [%a:%d] ", \
		    __func__, __LINE__, __VA_ARGS__); \
	} while (0)

/*
 * Returns EFI_INVALID_PARAMETER if tag is too long
 */
EFI_STATUS log_set_logtag(const CHAR16 *tag);

VOID log_set_line_len(UINTN len);

VOID log_set_flush_to_var(BOOLEAN b);

VOID log_set_loglevel(enum loglevel level);

VOID log_set_logtimestamp(BOOLEAN b);

#endif	/* __LOG_H__ */
