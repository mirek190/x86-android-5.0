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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <efi.h>
#include <log.h>

#ifdef CONFIG_LOG_TAG
#define LOG_TAG CONFIG_LOG_TAG
#else
#define LOG_TAG L"EFILINUX"
#endif	/* CONFIG_LOG_TAG */

#ifdef CONFIG_LOG_BUF_SIZE
#define LOG_BUF_SIZE CONFIG_LOG_BUF_SIZE
#else
#define LOG_BUF_SIZE 10 * 1024
#endif	/* CONFIG_LOG_BUF_SIZE */

#ifdef CONFIG_LOG_LINE_LEN
#define LOG_LINE_LEN CONFIG_LOG_LINE_LEN
#else
#define LOG_LINE_LEN 100
#endif	/* CONFIG_LOG_LINE_LEN */

#ifdef CONFIG_EFILINUX_VAR_PREFIX
#define EFILINUX_VAR_PREFIX CONFIG_EFILINUX_VAR_PREFIX
#else
#define EFILINUX_VAR_PREFIX "Efilinux"
#endif	/* CONFIG_EFILINUX_VAR_PREFIX */

#ifdef CONFIG_LOG_FLUSH_TO_VARIABLE
#define LOG_FLUSH_TO_VARIABLE TRUE
#else
#define LOG_FLUSH_TO_VARIABLE FALSE
#endif	/* CONFIG_LOG_FLUSH_TO_VARIABLE */

#ifdef CONFIG_LOG_LEVEL
#define LOG_LEVEL CONFIG_LOG_LEVEL
#else
#define LOG_LEVEL LEVEL_WARNING
#endif	/* CONFIG_LOG_LEVEL */

#ifdef CONFIG_LOG_TIMESTAMP
#define LOG_TIMESTAMP TRUE
#else
#define LOG_TIMESTAMP FALSE
#endif	/* CONFIG_LOG_TIMESTAMP */

#define EFILINUX_LOGS_VARNAME EFILINUX_VAR_PREFIX L"Logs"

extern enum loglevel log_level;
extern BOOLEAN log_flush_to_variable;
extern BOOLEAN do_cold_reset_after_wd;
extern BOOLEAN has_warmdump;
extern EFI_GUID osloader_guid;

#endif	/* __CONFIG_H__ */
