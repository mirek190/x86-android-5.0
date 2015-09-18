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

#include "flash.h"
#include "flash_ops.h"
#include "util.h"

static void *o;

#define ops_call(op, func, ...) \
	(o = op##_ops()) && ((struct op##_operations *)o)->func ? ((struct op##_operations *)o)->func(__VA_ARGS__) : stub_operation(#func)

int flash_dnx_timeout(void *data, size_t size)
{
	return ops_call(ifwi, flash_dnx_timeout, data, size);
}

int read_dnx_timeout(void)
{
	return ops_call(ifwi, read_dnx_timeout);
}

int erase_token_umip(void)
{
	return ops_call(ifwi, erase_token_umip);
}

int flash_custom_boot(void *data, size_t size)
{
	return ops_call(ifwi, flash_custom_boot, data, size);
}

int flash_token_umip(void *data, unsigned size)
{
	return ops_call(ifwi, flash_token_umip, data, size);
}

int update_ifwi_image(void *data, size_t size, unsigned reset_flag)
{
	return ops_call(ifwi, update_ifwi_image, data, size, reset_flag);
}

int update_ifwi_file(void *data, size_t size)
{
	return ops_call(ifwi, update_ifwi_file, data, size);
}

int check_ifwi_file(void *data, size_t size)
{
	return ops_call(ifwi, check_ifwi_file, data, size);
}

int flash_ifwi(void *data, unsigned sz)
{
	return ops_call(ifwi, flash_ifwi, data, sz);
}

int flash_dnx(void *data, unsigned sz)
{
	return ops_call(ifwi, flash_dnx, data, sz);
}

int flash_capsule(void *data, unsigned sz)
{
	return ops_call(capsule, flash_capsule, data, sz);
}

int flash_image(void *data, unsigned sz, const char *name)
{
	return ops_call(bootimage, flash_image, data, sz, name);
}

int read_image(const char *name, void **data)
{
	return ops_call(bootimage, read_image, name, data);
}

int read_image_signature(void **buf, char *name)
{
	return ops_call(bootimage, read_image_signature, buf, name);
}

int is_image_signed(const char *name)
{
	return ops_call(bootimage, is_image_signed, name);
}

int flash_android_kernel(void *data, unsigned sz)
{
	return flash_image(data, sz, ANDROID_OS_NAME);
}

int flash_recovery_kernel(void *data, unsigned sz)
{
	return flash_image(data, sz, RECOVERY_OS_NAME);
}

int flash_fastboot_kernel(void *data, unsigned sz)
{
	return flash_image(data, sz, FASTBOOT_OS_NAME);
}

int flash_splashscreen_image1(void *data, unsigned sz)
{
	return flash_image(data, sz, SPLASHSCREEN_NAME);
}

int flash_splashscreen_image2(void *data, unsigned sz)
{
	return flash_image(data, sz, SPLASHSCREEN_NAME2);
}

int flash_splashscreen_image3(void *data, unsigned sz)
{
	return flash_image(data, sz, SPLASHSCREEN_NAME3);
}

int flash_splashscreen_image4(void *data, unsigned sz)
{
	return flash_image(data, sz, SPLASHSCREEN_NAME4);
}

int flash_silent_binary(void *data, unsigned sz)
{
	return flash_image(data, sz, SILENT_BINARY_NAME);
}

int flash_esp(void *data, unsigned sz)
{
	return flash_image(data, sz, ESP_PART_NAME);
}

int flash_testos(void *data, unsigned sz)
{
	return flash_image(data, sz, TEST_OS_NAME);
}

int flash_ramdump(void *data, unsigned sz)
{
	return flash_image(data, sz, RAMDUMP_OS_NAME);
}

