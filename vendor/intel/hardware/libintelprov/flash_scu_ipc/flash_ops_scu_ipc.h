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

#ifndef _FLASH_OPS_SCU_IPC_H_
#define _FLASH_OPS_SCU_IPC_H_

#include <flash_ops.h>

#ifdef CONFIG_INTELPROV_SCU_IPC

bool is_scu_ipc(void);
int flash_ifwi_scu_ipc(void *data, unsigned size);
int flash_dnx_scu_ipc(void *data, unsigned size);
int update_ifwi_image_scu_ipc(void *data, size_t size, unsigned reset_flag);
int update_ifwi_file_scu_ipc(const char *dnx, const char *ifwi);

#else	/* CONFIG_INTELPROV_SCU_IPC */

bool is_scu_ipc(void)
{
	return false;
}

int flash_ifwi_scu_ipc(void *data, unsigned size)
{
	return stub_operation(__func__);
}

int flash_dnx_scu_ipc(void *data, unsigned size)
{
	return stub_operation(__func__);
}

int update_ifwi_image_scu_ipc(void *data, size_t size, unsigned reset_flag)
{
	return stub_operation(__func__);
}

int update_ifwi_file_scu_ipc(const char *dnx, const char *ifwi)
{
	return stub_operation(__func__);
}

#endif	/* CONFIG_INTELPROV_SCU_IPC */

struct ifwi_operations scu_ipc_ifwi_operations = {
	.flash_ifwi = flash_ifwi_scu_ipc,
	.flash_dnx = flash_dnx_scu_ipc,
	.update_ifwi_image = update_ifwi_image_scu_ipc,
};

#endif	/* _FLASH_OPS_SCU_IPC_H_ */
