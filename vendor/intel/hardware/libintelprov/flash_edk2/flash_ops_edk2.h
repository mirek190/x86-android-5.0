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

#ifndef _FLASH_OPS_EDK2_H_
#define _FLASH_OPS_EDK2_H_
#include <flash_ops.h>

#ifdef CONFIG_INTELPROV_EDK2

bool is_edk2(void);
int flash_capsule_esp(void *data, unsigned sz);

#else	/* CONFIG_INTELPROV_EDK2 */

bool is_edk2(void)
{
	return false;
}

int flash_capsule_esp(void *data, unsigned sz)
{
	return stub_operation(__func__);
}

#endif	/* CONFIG_INTELPROV_EDK2 */

struct capsule_operations edk2_capsule_operations = {
	.flash_capsule = flash_capsule_esp,
};

#endif	/* _FLASH_OPS_EDK2_H_ */
