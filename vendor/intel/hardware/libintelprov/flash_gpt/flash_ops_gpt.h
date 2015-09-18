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

#ifndef _FLASH_OPS_GPT_H_
#define _FLASH_OPS_GPT_H_

#include <stdbool.h>
#include <flash_ops.h>

#ifdef CONFIG_INTELPROV_GPT

bool is_gpt(void);
int flash_image_gpt(void *data, unsigned sz, const char *name);
int read_image_gpt(const char *name, void **data);
int read_image_signature_gpt(void **buf, char *name);
int is_image_signed_gpt(const char *name);

#else	/* CONFIG_INTELPROV_GPT */

bool is_gpt(void)
{
	return false;
}

int flash_image_gpt(void *data, unsigned sz, const char *name)
{
	return stub_operation(__func__);
}

int read_image_gpt(const char *name, void **data)
{
	return stub_operation(__func__);
}

int read_image_signature_gpt(void **buf, char *name)
{
	return stub_operation(__func__);
}

int is_image_signed_gpt(const char *name)
{
	return stub_operation(__func__);
}

#endif	/* CONFIG_INTELPROV_GPT */

struct bootimage_operations gpt_bootimage_operations = {
	.flash_image = flash_image_gpt,
	.read_image = read_image_gpt,
	.read_image_signature = read_image_signature_gpt,
	.is_image_signed = is_image_signed_gpt,
};

#endif	/* _FLASH_OPS_GPT_H_ */
