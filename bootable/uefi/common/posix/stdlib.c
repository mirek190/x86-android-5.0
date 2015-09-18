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

#include "stdlib.h"
#include "string.h"

static int to_digit(char c, int base)
{
	int value = -1;

	if (c >= '0' && c <= '9')
		value = c - '0';
	else if (c >= 'a' && c <= 'z')
		value = 0xA + c - 'a';
	else if (c >= 'A' && c <= 'Z')
		value = 0xA + c - 'A';

	return value < base ? value : -1;
}

unsigned long int strtoul(const char *nptr, char **endptr, int base)
{
	int value = 0;

	if (!nptr)
		goto out;

	if ((base == 0 || base == 16) &&
	    (strlen(nptr) > 2 && nptr[0] == '0' && nptr[1] == 'x')) {
		nptr += 2;
		base = 16;
	}

	if (base == 0)
		base = 10;

	for (; *nptr != '\0' ; nptr++) {
		INTN t = to_digit(*nptr, base);
		if (t == -1)
			goto out;
		value = (value * base) + t;
	}

out:
	if (endptr)
		*endptr = (char *)nptr;
	return value;
}
