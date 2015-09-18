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

#include <errno.h>
#include "util.h"

#define ULPMC_PATH "/dev/ulpmc-fwupdate"
int flash_ulpmc(void *data, unsigned sz)
{
	/*
	 * TODO: check version after flashing
	 */
	if (file_write(ULPMC_PATH, data, sz)) {
		error("ULPMC flashing failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}
