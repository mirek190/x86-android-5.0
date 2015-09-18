/*
 * Copyright (c) 2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "inc/libdmi.h"
#include "parse_dmi.h"
#include "dmi_intel.h"

#define DMISYSFS_PATH "/sys/firmware/dmi/entries/%d-%d/raw"

/*
 * Read sysfs file and return size of the file.
 *
 * If buffer is NULL, then just read using a temporary buffer, else
 * copy the file content in buffer.
 *
 * The caller must allocate the buffer with enough space
 * to contain the whole file (read_sysfs should be called one first
 * time to compute the file size)
 */
static int read_sysfs(const char *path, unsigned char *buffer)
{
	int ret;
	int fd;
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		error("Failed to open %s : %s\n", path, strerror(errno));
		return fd;
	}

	int size = 0;
	unsigned char tmp[BUFSIZ];
	unsigned char *buf = tmp;
	if (buffer)
		buf = buffer;
	do {
		ret = read(fd, buf, BUFSIZ);
		if (ret == -1)
			break;
		size += ret;
		if (buffer)
			buf += ret;
	} while (ret > 0);

close:
	close(fd);

	if (ret < 0) {
		error("Failed to read file %s : %s\n", path, strerror(errno));
		size = ret;
	}
	return size;
}

static struct dmi_header *dmisysfs_gettype(enum smbios_type type)
{
	int fd;
	char *path;
	int ret;
	int size;
	struct dmi_header *dmi = NULL;

	ret = asprintf(&path, DMISYSFS_PATH, type, 0);
	if (ret == -1) {
		error("Memory allocation failure\n");
		goto out;
	}

	size = read_sysfs(path, NULL);
	if (size <= 0)
		goto free_path;

	dmi = malloc(size);
	if (!dmi) {
		error("Memory allocation failure");
		goto free_path;
	}

	read_sysfs(path, (unsigned char *)dmi);

free_path:
	free(path);
out:
	return dmi;
}

struct vendor_parser {
	char *vendor;
	char *(*parser)(struct dmi_header *, char *);
};

static const struct vendor_parser vendor_parsers[] = {
	{"Intel Corp.", intel_dmi_parser},
};

static char *dmi_get_vendor(void);

/* Index begins at 1 */
static char *dmi_string(struct dmi_header *dmi, char index)
{
	if (index <= 0) {
		error("Invalid string index %d\n", index);
		return NULL;
	}

	int i;
	char *string = (char *)dmi + dmi->length;
	for (i = 1; i < index; i++)
		string += strlen(string) + 1;

	return string;
}

static char *oem_dmi_getfield(struct dmi_header *dmi, char *field)
{
	unsigned int i;
	char *vendor = dmi_get_vendor();

	if (!vendor) {
		error("Failed to retrieve vendor information to decode OEM specific table\n");
		return NULL;
	}

	for (i = 0; i < sizeof(vendor_parsers) / sizeof(*vendor_parsers); i++)
		if (!strcmp(vendor, vendor_parsers[i].vendor)) {
			free(vendor);
			return vendor_parsers[i].parser(dmi, field);
		}

	error("Unsupported vendor '%s'\n", vendor);
	free(vendor);
	return NULL;
}

static char *dmi_getfield(struct dmi_header *dmi, char *field)
{
	if (dmi->type >= OEM_SPECIFIC)
		return oem_dmi_getfield(dmi, field);

	switch (dmi->type) {
	case BIOS_INFORMATION:
		PARSE_FIELD(bios_information, dmi, field);
		break;
	default:
		error("Table 0x%x not supported\n", dmi->type);
		return NULL;
	}
}

char *libdmi_getfield(enum smbios_type type, char *field)
{
	struct dmi_header *dmi = dmisysfs_gettype(type);
	if (!dmi)
		return NULL;

	char *value = dmi_getfield(dmi, field);
	free(dmi);
	return value;
}

static char *dmi_get_vendor(void)
{
	return libdmi_getfield(0, "bios_vendor");
}

/*
 * Return stringified value of data.
 *
 * SMBIOS doesn't give information on the type of the data so we
 * encode it using type_size.
 *
 * type_size 0 is special and means data is a string.
 */
char *parse_dmi_field(struct dmi_header *dmi, unsigned char *data, int type_size)
{
	char *value = NULL;
	int ret;
	if (type_size == 0) {
		char *tmp;
		tmp = dmi_string(dmi, *data);
		if (!tmp)
			goto out;
		ret = asprintf(&value, "%s", tmp);
		if (ret == -1) {
			error("Memory allocation failure\n");
			value = NULL;
		}
		goto out;
	}

	__int64_t int_val = 0;
	memcpy(&int_val, data, type_size);
	ret = asprintf(&value, "%lld", int_val);
	if (ret == -1) {
		error("Memory allocation failure\n");
		value = NULL;
	}

out:
	return value;
}

struct field_desc *get_field_desc(struct field_desc *list, int list_size, char *field)
{
	int i;
	for (i = 0; i < list_size ; i++)
		if (!strcmp(list[i].field, field))
		    return &list[i];
	return NULL;
}
