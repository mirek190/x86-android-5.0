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

#ifndef __SMBIOS_PROTOCOL_H__
#define __SMBIOS_PROTOCOL_H__
#include <efi.h>

#define EFI_SMBIOS_PROTOCOL_GUID \
	{ 0x3583ff6, 0xcb36, 0x4940, { 0x94, 0x7e, 0xb9, 0xb3, 0x9f, 0x4a, 0xfa, 0xf7 }}

//
// SMBIOS type macros which is according to SMBIOS specification.
//
#define EFI_SMBIOS_TYPE_BIOS_INFORMATION                    0
#define EFI_SMBIOS_TYPE_SYSTEM_INFORMATION                  1
#define EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION               2
#define EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE                    3
#define EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION               4
#define EFI_SMBIOS_TYPE_MEMORY_CONTROLLER_INFORMATION       5
#define EFI_SMBIOS_TYPE_MEMORY_MODULE_INFORMATON            6
#define EFI_SMBIOS_TYPE_CACHE_INFORMATION                   7
#define EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION          8
#define EFI_SMBIOS_TYPE_SYSTEM_SLOTS                        9
#define EFI_SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION          10
#define EFI_SMBIOS_TYPE_OEM_STRINGS                         11
#define EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS        12
#define EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION           13
#define EFI_SMBIOS_TYPE_GROUP_ASSOCIATIONS                  14
#define EFI_SMBIOS_TYPE_SYSTEM_EVENT_LOG                    15
#define EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY               16
#define EFI_SMBIOS_TYPE_MEMORY_DEVICE                       17
#define EFI_SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION      18
#define EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS         19
#define EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS        20
#define EFI_SMBIOS_TYPE_BUILT_IN_POINTING_DEVICE            21
#define EFI_SMBIOS_TYPE_PORTABLE_BATTERY                    22
#define EFI_SMBIOS_TYPE_SYSTEM_RESET                        23
#define EFI_SMBIOS_TYPE_HARDWARE_SECURITY                   24
#define EFI_SMBIOS_TYPE_SYSTEM_POWER_CONTROLS               25
#define EFI_SMBIOS_TYPE_VOLTAGE_PROBE                       26
#define EFI_SMBIOS_TYPE_COOLING_DEVICE                      27
#define EFI_SMBIOS_TYPE_TEMPERATURE_PROBE                   28
#define EFI_SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE            29
#define EFI_SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS           30
#define EFI_SMBIOS_TYPE_BOOT_INTEGRITY_SERVICE              31
#define EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION             32
#define EFI_SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION      33
#define EFI_SMBIOS_TYPE_MANAGEMENT_DEVICE                   34
#define EFI_SMBIOS_TYPE_MANAGEMENT_DEVICE_COMPONENT         35
#define EFI_SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA    36
#define EFI_SMBIOS_TYPE_MEMORY_CHANNEL                      37
#define EFI_SMBIOS_TYPE_IPMI_DEVICE_INFORMATION             38
#define EFI_SMBIOS_TYPE_SYSTEM_POWER_SUPPLY                 39
#define EFI_SMBIOS_TYPE_INACTIVE                            126
#define EFI_SMBIOS_TYPE_END_OF_TABLE                        127
#define EFI_SMBIOS_OEM_BEGIN                                128
#define EFI_SMBIOS_OEM_END                                  255

typedef UINT8  EFI_SMBIOS_STRING;

typedef UINT8  EFI_SMBIOS_TYPE;

typedef UINT16 EFI_SMBIOS_HANDLE;

typedef struct {
	EFI_SMBIOS_TYPE   Type;
	UINT8             Length;
	EFI_SMBIOS_HANDLE Handle;
} EFI_SMBIOS_TABLE_HEADER;

typedef struct _EFI_SMBIOS_PROTOCOL EFI_SMBIOS_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_ADD)(
	IN CONST      EFI_SMBIOS_PROTOCOL     *This,
	IN            EFI_HANDLE              ProducerHandle OPTIONAL,
	IN OUT        EFI_SMBIOS_HANDLE       *SmbiosHandle,
	IN            EFI_SMBIOS_TABLE_HEADER *Record
);

typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_UPDATE_STRING)(
	IN CONST EFI_SMBIOS_PROTOCOL *This,
	IN       EFI_SMBIOS_HANDLE   *SmbiosHandle,
	IN       UINTN               *StringNumber,
	IN       CHAR8               *String
);

typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_REMOVE)(
	IN CONST EFI_SMBIOS_PROTOCOL *This,
	IN       EFI_SMBIOS_HANDLE   SmbiosHandle
);

typedef
EFI_STATUS
(EFIAPI *EFI_SMBIOS_GET_NEXT)(
	IN     CONST EFI_SMBIOS_PROTOCOL     *This,
	IN OUT       EFI_SMBIOS_HANDLE       *SmbiosHandle,
	IN           EFI_SMBIOS_TYPE         *Type              OPTIONAL,
	OUT          EFI_SMBIOS_TABLE_HEADER **Record,
	OUT          EFI_HANDLE              *ProducerHandle    OPTIONAL
);

struct _EFI_SMBIOS_PROTOCOL {
	EFI_SMBIOS_ADD           Add;
	EFI_SMBIOS_UPDATE_STRING UpdateString;
	EFI_SMBIOS_REMOVE        Remove;
	EFI_SMBIOS_GET_NEXT      GetNext;
	UINT8                    MajorVersion;
	UINT8                    MinorVersion;
};

extern EFI_GUID gEfiSmbiosProtocolGuid;

#define TYPE_INTEL_SMBIOS 0x94

typedef struct _SMBIOS_TABLE_TYPE94 {
	EFI_SMBIOS_TABLE_HEADER hdr;
	EFI_SMBIOS_STRING GopVersion;
	EFI_SMBIOS_STRING UCodeVersion;
	EFI_SMBIOS_STRING MRCVersion;
	EFI_SMBIOS_STRING SECVersion;
	EFI_SMBIOS_STRING ULPMCVersion;
	EFI_SMBIOS_STRING PMCVersion;
	EFI_SMBIOS_STRING PUnitVersion;
	EFI_SMBIOS_STRING SoCVersion;
	EFI_SMBIOS_STRING BoardVersion;
	EFI_SMBIOS_STRING FabVersion;
	EFI_SMBIOS_STRING CPUFlavor;
	EFI_SMBIOS_STRING BiosVersion;
	EFI_SMBIOS_STRING PmicVersion;
	EFI_SMBIOS_STRING TouchVersion;
	EFI_SMBIOS_STRING SecureBoot;
	EFI_SMBIOS_STRING BootMode;
	EFI_SMBIOS_STRING SpeedStepMode;
	EFI_SMBIOS_STRING CPUTurboMode;
	EFI_SMBIOS_STRING MaxCState;
	EFI_SMBIOS_STRING GfxTurbo;
	EFI_SMBIOS_STRING S0ix;
	EFI_SMBIOS_STRING RC6;
}SMBIOS_TABLE_TYPE94;

#endif // __SMBIOS_PROTOCOL_H__
