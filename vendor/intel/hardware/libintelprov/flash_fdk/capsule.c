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

#include <stdbool.h>
#include <stdio.h>
#include <cutils/properties.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"
#include "capsule.h"
#include "flash.h"

#define CAPSULE_PARTITION_LABEL "FWUP"
#define CAPSULE_UPDATE_FLAG_PATH "/sys/firmware/osnib/fw_update"

#define CAPSULE_HEADER "capsule header "
#define MN2_VER_BYTESREORDER(data) data[1]<< 24 | data[0] << 16 | data[3] << 8 | data[2];
#define CAPSULE_FW_VERSION_TAG 0x244d4e32	/* $MN2 */
/* Offset in byte to apply from the last $MN2 TAG byte to found the version value */
#define CAPSULE_FW_VERSION_OFFSET 5

bool is_fdk(void)
{
	char *path = NULL;

	if (!get_device_path(&path, CAPSULE_PARTITION_LABEL))
		free(path);

	return !!path;
}

static bool get_fw_version_tag_offset(u8 ** data_ptr, u8 * end_ptr)
{
	u32 patt = 0;

	for (; *data_ptr < end_ptr; (*data_ptr)++) {
		patt = patt << 8 | **data_ptr;
		if (patt == CAPSULE_FW_VERSION_TAG) {
			*data_ptr += CAPSULE_FW_VERSION_OFFSET;
			return true;
		}
	}

	/* no $MN2 found return 0 */
	return false;
}

static void print_capsule_header(struct capsule *c)
{
	struct capsule_signature *sig;
	struct capsule_version *ver;
	struct capsule_refs *refs;

	if (!c)
		return;

	sig = &(c->header.sig);
	ver = &(c->header.ver);
	refs = &(c->header.refs);

	printf(CAPSULE_HEADER "(0x%02x)\n", sig->signature);
	printf(CAPSULE_HEADER "version (0x%02x) length 0x%02x\n", ver->signature, ver->length);
	printf(CAPSULE_HEADER "iafw_stage_1: version=0x%x size=0x%02x offset=0x%02zx\n",
	       ver->iafw_stage1_version, refs->iafw_stage1_size, refs->iafw_stage1_offset + sizeof(*sig));
	printf(CAPSULE_HEADER "iafw_stage_2: version=0x%x size=0x%02x offset=0x%02zx\n",
	       ver->iafw_stage2_version, refs->iafw_stage2_size, refs->iafw_stage2_offset + sizeof(*sig));
	printf(CAPSULE_HEADER "mcu: version=0x%x\n", ver->mcu_version);
	printf(CAPSULE_HEADER "pdr:  size=0x%02x offset=0x%02zx\n",
	       refs->pdr_size, refs->pdr_offset + sizeof(*sig));
	printf(CAPSULE_HEADER "sec: version=0x%02x (%d) size=0x%02x offset=0x%02zx\n",
	       ver->sec_version,
	       (ver->sec_version & 0xFFFF), refs->sec_size, refs->sec_offset + sizeof(*sig));
}

static bool check_capsule(struct capsule *c, u32 iafw_version, sec_version_t sec_version, u32 pdr_version)
{
	u8 *cdata = (u8 *) c;
	struct capsule_signature *sig = &(c->header.sig);
	struct capsule_refs *refs = &(c->header.refs);
	bool match = false;

	/*  Check PDR region if present request capsule update without condition */
	if (refs->pdr_size) {
		u8 *pdr_data = cdata + refs->pdr_offset + sizeof(*sig);

		if (get_fw_version_tag_offset(&pdr_data, pdr_data + refs->pdr_size)) {
			u32 pdr_mn2_version = MN2_VER_BYTESREORDER(pdr_data);
			printf("PDR $MN2 at offset 0x%02zx version 0x%02x \n", pdr_data - cdata,
			       pdr_mn2_version);

			if (pdr_mn2_version != pdr_version) {
				printf("PDR version mismatch current 0x%02x capsule $MN2 0x%02x \n",
				       pdr_version, pdr_mn2_version);
				printf("Capsule update requested \n");
				return true;
			}
			match = true;
		}
	}

	/* Get IAFW $MN2 capsule version and compare it with current one */
	if (refs->iafw_stage2_size) {
		u8 *iafw_data = cdata + refs->iafw_stage2_offset + sizeof(*sig);

		if (get_fw_version_tag_offset(&iafw_data, iafw_data + refs->iafw_stage2_size)) {
			u32 iafw_mn2_version = MN2_VER_BYTESREORDER(iafw_data);
			printf("IAFW $MN2 at offset 0x%02zx version 0x%02x \n", iafw_data - cdata,
			       iafw_mn2_version);

			if (iafw_mn2_version != iafw_version) {
				printf("IAFW version mismatch current 0x%02x capsule $MN2 0x%02x \n",
				       iafw_version, iafw_mn2_version);
				printf("Capsule update requested \n");
				return true;
			}
			match = true;
		}
	}

	/* Get SECFW $MN2 capsule versions (several $MN2 can be found in SEC image) */
	if (refs->sec_size) {
		u8 *sec_data = cdata + refs->sec_offset + sizeof(*sig);

		/* Check $MN2 version */
		while (get_fw_version_tag_offset(&sec_data, cdata + refs->sec_size)) {
			/* Endianess reordering to get SECFW version needed info */
			sec_version_t sec_mn2_version;
			sec_mn2_version.val_2 = MN2_VER_BYTESREORDER(sec_data);
			sec_mn2_version.val_1 = sec_data[5] << 8 | sec_data[4];
			sec_mn2_version.val_0 = sec_data[7] << 8 | sec_data[6];
			printf("SEC $MN2 at offset 0x%02zx version %d.%d.%d\n", sec_data - cdata,
			       sec_mn2_version.val_2, sec_mn2_version.val_1, sec_mn2_version.val_0);

			if ((sec_mn2_version.val_0 != sec_version.val_0) ||
			    (sec_mn2_version.val_1 != sec_version.val_1) ||
			    (sec_mn2_version.val_2 != sec_version.val_2)) {
				printf("SECFW version mismatch current %d.%d.%d capsule $MN2 %d.%d.%d\n",
				       sec_version.val_2, sec_version.val_1, sec_version.val_0,
				       sec_mn2_version.val_2, sec_mn2_version.val_1, sec_mn2_version.val_0);
				printf("Capsule update requested\n");
				return true;
			}
			match = true;
		}
	}

	/* If no version data have been found or versions are all 0, allow the capsule */
	return !match;
}

int flash_capsule_fdk(void *data, unsigned sz)
{
	int ret_status = -1;
	char capsule_trigger = '1';
	bool capsule_update;
	struct capsule *c;

	char iafw_stage1_version_str[PROPERTY_VALUE_MAX];
	u32 iafw_version_msb = 0;
	u32 iafw_version = 0;

	char sec_version_str[PROPERTY_VALUE_MAX];
	sec_version_t sec_version;

	char pdr_version_str[PROPERTY_VALUE_MAX];
	u32 pdr_version_msb = 0;
	u32 pdr_version = 0;

	char *dev_path = NULL;

	/* Get current FW version */
	property_get("sys.ia32.version", iafw_stage1_version_str, "00.00");
	property_get("sys.chaabi.version", sec_version_str, "00.00");
	property_get("sys.pdr.version", pdr_version_str, "00.00");

	sscanf(iafw_stage1_version_str, "%x.%x", &iafw_version_msb, &iafw_version);
	iafw_version |= iafw_version_msb << 16;
	sscanf(sec_version_str, "%*d.%d.%d.%d", &sec_version.val_2, &sec_version.val_1, &sec_version.val_0);
	sscanf(pdr_version_str, "%x.%x", &pdr_version_msb, &pdr_version);
	pdr_version |= pdr_version_msb << 16;

	printf("current iafw version: %s (0x%02x)\n", iafw_stage1_version_str, iafw_version);

	printf("current sec version: %s (%d.%d.%d)\n", sec_version_str,
	       sec_version.val_2, sec_version.val_1, sec_version.val_0);

	printf("current pdr version: %s (0x%2x)\n", pdr_version_str, pdr_version);

	if (!data || sz <= sizeof(struct capsule_header)) {
		error("Capsule file is not valid: %s\n", strerror(errno));
		goto exit;
	}

	c = (struct capsule *)data;
	print_capsule_header(c);

	/* Check capsule vs current FW version */
	capsule_update = check_capsule(c, iafw_version, sec_version, pdr_version);

	/* Flash capsule file only if versions missmatch */
	if (!capsule_update) {
		printf("Capsule contains same FW versions as current ones , do not request update\n");
		ret_status = 0;
		goto exit;
	}

	if (get_device_path(&dev_path, CAPSULE_PARTITION_LABEL)) {
		error("Unable to get the capsule partition device path\n");
		goto exit;
	}

	if ((ret_status = file_write(dev_path, data, sz))) {
		error("Capsule flashing failed: %s\n", strerror(errno));
		goto exit;
	}

	if ((ret_status = file_write(CAPSULE_UPDATE_FLAG_PATH, &capsule_trigger, sizeof(capsule_trigger)))) {
		error("Can't set fw_update bit: %s\n", strerror(errno));
		goto exit;
	}

exit:
	free(dev_path);
	return ret_status;
}
