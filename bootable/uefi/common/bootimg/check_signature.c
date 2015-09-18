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

#include <efi.h>
#include <efilib.h>
#include <uefi_utils.h>
#include <log.h>


#ifdef USE_INTEL_OS_VERIFICATION
// {DAFB7EEC-B2E9-4ea6-A80A-37FED7909FF3}
EFI_GUID gOsVerificationProtocolGuid =	{
	0xdafb7eec, 0xb2e9, 0x4ea6,
	{ 0xa8, 0xa, 0x37, 0xfe, 0xd7, 0x90, 0x9f, 0xf3 }
};

typedef struct _OS_VERIFICATION_PROTOCOL OS_VERIFICATION_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_OS_VERIFY) (
	IN OS_VERIFICATION_PROTOCOL *This,
	IN VOID                     *OsImagePtr,
	IN UINTN                    OsImageSize,
	IN VOID                     *ManifestPtr,
	IN UINTN                    ManifestSize
);

typedef
EFI_STATUS
(EFIAPI *GET_SECURITY_POLICY) (
	IN     OS_VERIFICATION_PROTOCOL *This,
	IN OUT BOOLEAN                  *AllowUnsignedOS
);

struct _OS_VERIFICATION_PROTOCOL {
	EFI_OS_VERIFY           VerifiyOsImage;
	GET_SECURITY_POLICY     GetSecurityPolicy;
};

static BOOLEAN is_secure_boot_enabled(void)
{
	OS_VERIFICATION_PROTOCOL *ovp;
	EFI_STATUS ret;
	BOOLEAN unsigned_allowed;

	ret = LibLocateProtocol(&gOsVerificationProtocolGuid, (void **)&ovp);
	if (EFI_ERROR(ret) || !ovp) {
		error(L"%x failure\n", __func__);
		goto out;
	}

	ret = uefi_call_wrapper(ovp->GetSecurityPolicy, 2, ovp,
				&unsigned_allowed);
	if (EFI_ERROR(ret))
		goto out;

	debug(L"unsigned_allowed = %x\n", unsigned_allowed);
	return (unsigned_allowed == FALSE);
out:
	return TRUE;
}

EFI_STATUS check_signature(IN VOID *os, IN UINTN os_size,
			   IN VOID *manifest, IN UINTN manifest_size)
{
	OS_VERIFICATION_PROTOCOL *ovp;
	EFI_STATUS ret;

	if (!is_secure_boot_enabled())
		return EFI_SUCCESS;

	if(!manifest_size) {
		error(L"image is not signed\n");
		return EFI_ACCESS_DENIED;
	}
	debug(L"checking boot image signature\n");

	ret = LibLocateProtocol(&gOsVerificationProtocolGuid, (void **)&ovp);
	if (EFI_ERROR(ret) || !ovp) {
		error(L"%x failure\n", __func__);
		goto out;
	}

	ret = uefi_call_wrapper(ovp->VerifiyOsImage, 5, ovp,
				os, os_size,
				manifest, manifest_size);

out:
	return ret;
}

#elif defined(USE_SHIM)
#include <shim.h>

static BOOL is_secure_boot_enabled(void)
{
	UINT8 secure_boot;
	UINTN size;
	EFI_STATUS status;
	EFI_GUID global_var_guid = EFI_GLOBAL_VARIABLE;

	size = sizeof(secure_boot);
	status = uefi_call_wrapper(RT->GetVariable, 5,
				   L"SecureBoot", (EFI_GUID *)&global_var_guid, NULL, &size, (void*)&secure_boot);

	if (EFI_ERROR(status))
		secure_boot = 0;

	debug(L"SecureBoot value = 0x%02X\n", secure_boot);
	return (secure_boot == 1);
}

EFI_GUID gShimLockProtocolGuid = SHIM_LOCK_GUID;

EFI_STATUS check_signature(IN VOID *blob, IN UINTN blob_size,
			   IN VOID *sig, IN UINTN sig_size)
{

	SHIM_LOCK *shim_lock;
	EFI_STATUS ret;

	if (!is_secure_boot_enabled())
		return EFI_SUCCESS;

	if (!sig_size) {
		error(L"Secure boot enabled, "
		      "but Android boot image is unsigned\n");
		return EFI_ACCESS_DENIED;
	}
	debug(L"checking boot image signature\n");

	ret = LibLocateProtocol(&gShimLockProtocolGuid, (VOID **)&shim_lock);
	if (EFI_ERROR(ret)) {
		error(L"Couldn't instantiate shim protocol", ret);
		return ret;
	}

	ret = uefi_call_wrapper(VerifyBlob, 4, shim_lock,
				bootimage, blob_size,
				sig_offset, sig_size);
	if (EFI_ERROR(ret))
		error(L"Boot image verification failed", ret);
	return ret;
}
#endif
