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

#ifndef _FLASH_OPS_H_
#define _FLASH_OPS_H_

#include <sys/types.h>
#include "util.h"

static inline int stub_operation(const char *func)
{
	error("%s is not implemented\n", func);
	return -1;
}

struct bootimage_operations {
	int (*flash_image) (void *data, unsigned size, const char *name);
	int (*read_image) (const char *name, void **data);
	int (*read_image_signature) (void **buf, char *name);
	int (*is_image_signed) (const char *name);
};

struct ifwi_operations {
	int (*flash_ifwi)(void *data, unsigned size);
	int (*flash_dnx)(void *data, unsigned size);
	int (*update_ifwi_image)(void *data, size_t size, unsigned reset_flag);
	int (*update_ifwi_file)(void *buffer, size_t size);
	int (*check_ifwi_file)(void *buffer, size_t size);
	int (*flash_token_umip)(void *buffer, size_t size);
	int (*erase_token_umip)(void);
	int (*flash_custom_boot)(void *buffer, size_t size);
	int (*flash_dnx_timeout)(void *buffer, size_t size);
	int (*read_dnx_timeout)(void);
};

struct capsule_operations {
	int (*flash_capsule) (void *data, unsigned size);
};

struct bootimage_operations *bootimage_ops(void);
struct ifwi_operations *ifwi_ops(void);
struct capsule_operations *capsule_ops(void);

#endif	/* _FLASH_OPS_H_ */
