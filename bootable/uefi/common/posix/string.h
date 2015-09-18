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

#ifndef __STRING_H__
#define __STRING_H__

#include <efi.h>
#include <efilib.h>

#define memcpy(dst, src, size) CopyMem(dst, src, size)
#define memset(dst, c, size) ZeroMem(dst, size)
#define memcmp(s1, s2, size) CompareMem(s1, s2, size)
#define strlen(s) strlena((CHAR8 *)s)
#define strcmp(s1,s2) strcmpa((CHAR8 *)s1, (CHAR8 *)s2)

static inline char *strstr(char *haystack, char *needle)
{
	char *p;
	char *word = NULL;
	int len = strlen(needle);

	if (!len)
		return NULL;

	p = haystack;
	while (*p) {
		word = p;
		if (!strncmpa((CHAR8 *)p, (CHAR8 *)needle, len))
			break;
		p++;
		word = NULL;
	}

	return word;
}

#endif	/* __STRING_H__ */
