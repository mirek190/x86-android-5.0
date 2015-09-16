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
 */

#ifndef _TCO_RESET_H
#define _TCO_RESET_H

#include <efi.h>
#include "watchdog.h"

#ifndef TCO_DEFAULT_TIMEOUT
#define TCO_DEFAULT_TIMEOUT 60
#endif

typedef struct _EFI_TCO_RESET_PROTOCOL EFI_TCO_RESET_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_TCO_RESET_PROTOCOL_ENABLE_TCO_RESET) (
	/*
	 *IN EFI_TCO_RESET_PROTOCOL *This,
	 */
	IN OUT      UINT32        *RcrbGcsValue
  );

typedef
EFI_STATUS
(EFIAPI *EFI_TCO_RESET_PROTOCOL_DISABLE_TCO_RESET) (
	/*
	 *IN EFI_TCO_RESET_PROTOCOL *This,
	 */
	IN    UINT32    RcrbGcsValue
  );

struct _EFI_TCO_RESET_PROTOCOL {
  EFI_TCO_RESET_PROTOCOL_ENABLE_TCO_RESET       EnableTcoReset;
  EFI_TCO_RESET_PROTOCOL_DISABLE_TCO_RESET    	DisableTcoReset;
};

//  {A6A79162-E325-4c30-BCC3-59373064EFB3}
#define EFI_TCO_RESET_PROTOCOL_GUID					\
	{								\
		0xa6a79162, 0xe325, 0x4c30,				\
		{ 0xbc, 0xc3, 0x59, 0x37, 0x30, 0x64, 0xef, 0xb3 }	\
	}

extern EFI_GUID gEfiTcoResetProtocolGuid;

void tco_start_watchdog(void);
#endif
