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

#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <minzip/Zip.h>

#include "util.h"
#include "flash.h"

#define ESP_LABEL		"ESP"
#define ESP_MOUNT_POINT		"/" ESP_LABEL
#define CAPSULE_BIN_FILE	ESP_MOUNT_POINT"/BiosUpdate.fv"
#define ESP_FS_TYPE		"vfat"

bool is_edk2(void)
{
	char *path = NULL;

	if (!get_device_path(&path, ESP_LABEL))
		free(path);

	return !!path;
}

/* This function is workaround replacement of ensure_path_mounted we
 * cannot use due to link issues.  Ideally, this we shoud get rid of
 * this new function as soon as we find a better solution.  */
static int ensure_esp_mounted()
{
	int ret;
	char *path = NULL;

	ret = get_device_path(&path, ESP_LABEL);
	if (ret) {
		error("%s: Unable to get the ESP block device path\n", __func__);
		goto out;
	}

	ret = mkdir(ESP_MOUNT_POINT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (ret == -1 && errno != EEXIST) {
		error("%s: mkdir %s failed\n", __func__, ESP_MOUNT_POINT);
		goto out;
	}

	ret = mount(path, ESP_MOUNT_POINT, ESP_FS_TYPE, MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
	/* EBUSY means that the filesystem is already mounted. */
	if (ret == -1 && errno != EBUSY) {
		error("%s: mount %s failed\n", __func__, ESP_MOUNT_POINT);
		goto out;
	} else if (errno == EBUSY)
		ret = 0;

out:
	free(path);
	return ret;
}

int flash_capsule_esp(void *data, unsigned sz)
{
	int ret;

	ret = ensure_esp_mounted();
	if (ret != 0) {
		error("%s: failed to ensure mount %s\n", __func__, ESP_MOUNT_POINT);
		return ret;
	}

	ret = file_write(CAPSULE_BIN_FILE, data, sz);
	if (ret != 0) {
		error("%s : write data to file %s failed\n", __func__, CAPSULE_BIN_FILE);
		return ret;
	}

	return 0;
}

int flash_esp_update(void *data, unsigned sz)
{
	int ret;
	bool success;
	ZipArchive za;

	ret = ensure_esp_mounted();
	if (ret != 0) {
		error("%s: failed to ensure mount %s\n", __func__, ESP_MOUNT_POINT);
		return ret;
	}

	ret = mzOpenZipArchive(data, sz, &za);
	if (ret != 0) {
		error("%s: failed to Open zip archive\n", __func__);
		return ret;
	}

	success = mzExtractRecursive(&za, "", ESP_MOUNT_POINT, 0, NULL, NULL, NULL, NULL);
	if (success != true)  {
		error("%s: failed to Extract zip archive to %s\n", __func__, ESP_MOUNT_POINT);
		return EXIT_FAILURE;
	}

	return 0;
}
