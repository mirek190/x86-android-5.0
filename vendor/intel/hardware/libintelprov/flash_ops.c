/*
 * Copyright 2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flash_gpt/flash_ops_gpt.h"
#include "flash_osip/flash_ops_osip.h"
#include "flash_scu_emmc/flash_ops_scu_emmc.h"
#include "flash_scu_ipc/flash_ops_scu_ipc.h"
#include "flash_edk2/flash_ops_edk2.h"
#include "flash_fdk/flash_ops_fdk.h"

#include <stdio.h>
struct bootimage_operations *bootimage_ops(void)
{
	if (is_gpt())
		return &gpt_bootimage_operations;

	if (is_osip())
		return &osip_bootimage_operations;

	return NULL;
}

struct ifwi_operations *ifwi_ops(void)
{
	if (is_scu_ipc()) {
		return &scu_ipc_ifwi_operations;
	}

	if (is_scu_emmc()) {
		return &scu_emmc_ifwi_operations;
	}

	return NULL;
}

struct capsule_operations *capsule_ops(void)
{
	if (is_edk2())
		return &edk2_capsule_operations;

	if (is_fdk())
		return &fdk_capsule_operations;

	return NULL;
}
