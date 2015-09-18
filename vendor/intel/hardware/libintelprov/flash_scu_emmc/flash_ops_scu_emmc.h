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

#ifndef _FLASH_OPS_SCU_EMMC_H_
#define _FLASH_OPS_SCU_EMMC_H_

#include <flash_ops.h>

#ifdef CONFIG_INTELPROV_SCU_EMMC

bool is_scu_emmc(void);
int flash_ifwi_scu_emmc(void *data, unsigned size);
int flash_dnx_scu_emmc(void *data, unsigned size);
int flash_token_umip_scu_emmc(void *data, size_t size);
int erase_token_umip_scu_emmc(void);
int flash_custom_boot_scu_emmc(void *data, size_t size);
int update_ifwi_file_scu_emmc(void *data, size_t size);
int check_ifwi_file_scu_emmc(void *data, size_t size);
int flash_dnx_timeout_scu_emmc(void *data, size_t size);
int read_dnx_timeout_scu_emmc(void);

#else	/* CONFIG_INTELPROV_SCU_EMMC */

bool is_scu_emmc(void)
{
	return false;
}

#define flash_ifwi_scu_emmc(void *data, unsigned size) {return stub_operation(__func__);}
#define flash_dnx_scu_emmc(void *data, unsigned size) {return stub_operation(__func__);}
#define flash_token_umip_scu_emmc(void *data, size_t size) {return stub_operation(__func__);}
#define erase_token_umip_scu_emmc(void *data, size_t size) {return stub_operation(__func__);}
#define flash_custom_boot_scu_emmc(void) {return stub_operation(__func__);}
#define update_ifwi_file_scu_emmc(void *data, size_t size) {return stub_operation(__func__);}
#define check_ifwi_file_scu_emmc(void *data, size_t size) {return stub_operation(__func__);}
#define flash_dnx_timeout_scu_emmc(void) {return stub_operation(__func__);}
#define read_dnx_timeout_scu_emmc(void) {return stub_operation(__func__);}

#endif	/* CONFIG_INTELPROV_SCU_EMMC */

struct ifwi_operations scu_emmc_ifwi_operations = {
	.flash_ifwi = flash_ifwi_scu_emmc,
	.flash_dnx = flash_dnx_scu_emmc,
	.update_ifwi_file = update_ifwi_file_scu_emmc,
	.check_ifwi_file = check_ifwi_file_scu_emmc,
	.flash_token_umip = flash_token_umip_scu_emmc,
	.erase_token_umip = erase_token_umip_scu_emmc,
	.flash_custom_boot = flash_custom_boot_scu_emmc,
	.flash_dnx_timeout = flash_dnx_timeout_scu_emmc,
	.read_dnx_timeout = read_dnx_timeout_scu_emmc,
};

#endif	/* _FLASH_OPS_SCU_EMMC_H_ */
