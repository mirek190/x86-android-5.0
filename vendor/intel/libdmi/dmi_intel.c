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

#include "parse_dmi.h"
#include "dmi_intel.h"
#include "inc/libdmi.h"

struct intel_0x94 {
	struct dmi_header hdr;
	unsigned char GopVersion;
	unsigned char UCodeVersion;
	unsigned char MRCVersion;
	unsigned char SECVersion;
	unsigned char ULPMCVersion;
	unsigned char PMCVersion;
	unsigned char PUnitVersion;
	unsigned char SoCVersion;
	unsigned char BoardVersion;
	unsigned char FabVersion;
	unsigned char CPUFlavor;
	unsigned char BiosVersion;
	unsigned char PmicVersion;
	unsigned char TouchVersion;
	unsigned char SecureBoot;
	unsigned char BootMode;
	unsigned char SpeedStepMode;
	unsigned char CPUTurboMode;
	unsigned char MaxCState;
	unsigned char GfxTurbo;
	unsigned char S0ix;
	unsigned char RC6;
};

static struct field_desc intel_0x94_desc[] = {
	FIELD_DESC(struct intel_0x94, GopVersion, 0),
	FIELD_DESC(struct intel_0x94, UCodeVersion, 0),
	FIELD_DESC(struct intel_0x94, MRCVersion, 0),
	FIELD_DESC(struct intel_0x94, SECVersion, 0),
	FIELD_DESC(struct intel_0x94, ULPMCVersion, 0),
	FIELD_DESC(struct intel_0x94, PMCVersion, 0),
	FIELD_DESC(struct intel_0x94, PUnitVersion, 0),
	FIELD_DESC(struct intel_0x94, SoCVersion, 0),
	FIELD_DESC(struct intel_0x94, BoardVersion, 0),
	FIELD_DESC(struct intel_0x94, FabVersion, 0),
	FIELD_DESC(struct intel_0x94, CPUFlavor, 0),
	FIELD_DESC(struct intel_0x94, BiosVersion, 0),
	FIELD_DESC(struct intel_0x94, PmicVersion, 0),
	FIELD_DESC(struct intel_0x94, TouchVersion, 0),
	FIELD_DESC(struct intel_0x94, SecureBoot, 0),
	FIELD_DESC(struct intel_0x94, BootMode, 0),
	FIELD_DESC(struct intel_0x94, SpeedStepMode, 0),
	FIELD_DESC(struct intel_0x94, CPUTurboMode, 0),
	FIELD_DESC(struct intel_0x94, MaxCState, 0),
	FIELD_DESC(struct intel_0x94, GfxTurbo, 0),
	FIELD_DESC(struct intel_0x94, S0ix, 0),
	FIELD_DESC(struct intel_0x94, RC6, 0),
};

char *intel_dmi_parser(struct dmi_header *dmi, char *field)
{
	switch (dmi->type) {
	case INTEL_SMBIOS:
		PARSE_FIELD(intel_0x94, dmi, field);
		break;
	default:
		error("Unsupported Intel table: 0x%x\n", dmi->type);
	}
out:
	return NULL;
}
