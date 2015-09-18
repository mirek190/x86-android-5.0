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

#ifndef _PARSE_DMI_H_
#define _PARSE_DMI_H_

#include <stdbool.h>
#include <sys/types.h>

static bool verbose = 1;

#define LOG_TAG "libdmi"
#include <utils/Log.h>

#define debug(...)				\
	if (verbose) {				\
		ALOGD(__VA_ARGS__);		\
		printf(__VA_ARGS__);		\
	}
#define info(...) {				\
		ALOGI(__VA_ARGS__);		\
		printf(__VA_ARGS__);		\
	}
#define warn(...) {				\
		ALOGW(__VA_ARGS__);		\
		fprintf(stderr, __VA_ARGS__);	\
	}
#define error(...) {				\
		ALOGE(__VA_ARGS__);		\
		fprintf(stderr, __VA_ARGS__);	\
	}

#define FIELD_DESC(table, field, type_size) {#field, offsetof(table, field), type_size}

struct dmi_header {
	unsigned char type;
	unsigned char length;
	unsigned short handle;
};

struct field_desc {
	char *field;
	unsigned int offset;
	unsigned int type_size;
};

struct field_desc *get_field_desc(struct field_desc *list, int list_size, char *field);
char *parse_dmi_field(struct dmi_header *dmi, unsigned char *data, int type_size);

struct bios_information {
	struct dmi_header hdr;
	unsigned char bios_vendor;
	unsigned char bios_version;
	unsigned short bios_start_addr;
	unsigned char bios_release_date;
	__int64_t bios_characteristics;
	unsigned char bios_major_release;
	unsigned char bios_minor_release;
	unsigned char bios_controller_major_release;
	unsigned char bios_controller_minor_release;
};

static struct field_desc bios_information_desc[] = {
	FIELD_DESC(struct bios_information, bios_vendor, 0),
	FIELD_DESC(struct bios_information, bios_version, 0),
	FIELD_DESC(struct bios_information, bios_start_addr, 2),
	FIELD_DESC(struct bios_information, bios_release_date, 0),
	FIELD_DESC(struct bios_information, bios_characteristics, 8),
	FIELD_DESC(struct bios_information, bios_major_release, 1),
	FIELD_DESC(struct bios_information, bios_minor_release, 1),
	FIELD_DESC(struct bios_information, bios_controller_major_release, 1),
	FIELD_DESC(struct bios_information, bios_controller_minor_release, 1),
};

#define PARSE_FIELD(table, dmi, fieldname) {				\
		struct table *t = (struct table *)dmi;	\
		if (dmi->length != sizeof(*t)) {			\
			error("Table 0x%x has wrong size: is %d bytes, should be %d bytes\n", dmi->type, dmi->length, sizeof(*t)); \
			return NULL;						\
		}							\
									\
		struct field_desc *desc = get_field_desc(table##_desc, sizeof(table##_desc) / sizeof(*table##_desc), fieldname); \
		if (!desc) {						\
			error("Unknown field %s for table 0x%x\n", fieldname, dmi->type); \
			return NULL;					\
		}							\
									\
		return parse_dmi_field(dmi, (unsigned char *)dmi + desc->offset, desc->type_size); \
	}

#endif /* _PARSE_DMI_H_ */
